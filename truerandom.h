/*
 * @file truerandom.h
 * @brief True random number generator using hardware instructions (RDRAND for x86, RNDR for ARM)
 * @version 0.0.4
 */

#ifndef TRUERANDOM_H
#define TRUERANDOM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 * User Configurations
 */

#ifndef TRUERND_MAX_RETRIES
#define TRUERND_MAX_RETRIES 10
#endif

/*
 * End User Configurations
 */


/* Naked function attribute */
#if defined(_MSC_VER)
    #define NAKED __declspec(naked)
#else
    #define NAKED __attribute__((naked))
#endif

/* Architecture detection */
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define truernd_ARCH_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)
    #define truernd_ARCH_ARM 1
#else
    #define truernd_ARCH_UNKNOWN 1
    #error "Unsupported architecture"
#endif

/** \addtogroup PUBLIC API
 *  @{
 */

/**
 * @brief Check if hardware random number generation is supported
 * @return 1 if supported, 0 if not supported
 */
int 
truernd_is_supported(void);

/**
 * @brief Generate a 32-bit true random number (single attempt)
 * @return The random value
 */
NAKED uint32_t 
truernd_gen32(void);

/**
 * @brief Generate a 64-bit true random number (single attempt)
 * @return The random value
 */
NAKED uint64_t 
truernd_gen64(void);

/**
 * @brief Generate a 32-bit true random number (single attempt)
 * @param[out] out Pointer to store the random number
 * @return 0 on success, -1 on failure
 */
NAKED int 
truernd_get32(uint32_t *out);

/**
 * @brief Generate a 64-bit true random number (single attempt)
 * @param[out] out Pointer to store the random number
 * @return 0 on success, -1 on failure
 */
NAKED int 
truernd_get64(uint64_t *out);

/**
 * @brief Fill a buffer with random bytes
 * @param buf Buffer to fill
 * @param len Length of buffer in bytes
 * @return 0 on success, -1 on failure
 * @note Assumes little-endian byte order for byte extraction from 64-bit values
 */
static inline int 
truernd_fill(void *buf, size_t len);

/** @}*/

#ifdef __cplusplus
}
#endif

#endif /* TRUERANDOM_H */

#ifdef TRUERANDOM_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif

/* Intel/AMD x86/x64 implementation using RDRAND */
#if defined(truernd_ARCH_X86)

#include <cpuid.h>

NAKED int 
truernd_is_supported(void) {
    __asm__ volatile(
            "xor     %esi, %esi     \n\t"
            "xor     %eax, %eax     \n\t"
            "xchg    %rdi, %rbx     \n\t"
            "cpuid                  \n\t"
            "xchg    %rdi, %rbx     \n\t"
            "test    %eax, %eax     \n\t"
            "je      1f             \n\t"
            "mov     $1, %eax       \n\t"
            "xchg    %rsi, %rbx     \n\t"
            "cpuid                  \n\t"
            "xchg    %rsi, %rbx     \n\t"
            "mov     %esi, %ecx     \n\t"
            "shr     $30, %esi      \n\t"
            "and     $1, %esi       \n\t"
            "1:                     \n\t"
            "mov     %eax, %esi     \n\t"
            "ret                    \n\t"
    );
}

NAKED uint32_t 
truernd_gen32(void) {
    __asm__ volatile(
        "rdrand %eax       \n\t"
        "jnc    1f         \n\t"
        "ret               \n\t"

        "1:                \n\t"
        "xor    %eax, %eax \n\t"
        "ret               \n\t"
    );
}

NAKED uint64_t 
truernd_gen64(void) {
    __asm__ volatile(
        "rdrand %rax            \n\t"
        "jnc    1f              \n\t"
        "ret                    \n\t"
    "1:                         \n\t"
        "xor    %rax, %rax      \n\t"
        "ret                    \n\t"
    );
}

NAKED int 
truernd_get32(uint32_t *) {
    __asm__ volatile(
        "test   %rdi, %rdi      \n\t"
        "jz     1f              \n\t"

        "rdrand %ecx            \n\t"
        "jnc    1f              \n\t"

        "movl   %ecx, (%rdi)    \n\t"
        "xor    %eax, %eax      \n\t"
        "ret                    \n\t"
    "1:                         \n\t"
        "mov    $-1, %eax       \n\t"
        "ret                    \n\t"
    );
}

NAKED int 
truernd_get64(uint64_t *) {
    __asm__ volatile(
        "test   %rdi, %rdi         \n\t"
        "jz     1f                 \n\t"

        "rdrand %rcx               \n\t"
        "jnc    1f                 \n\t"

        "movq   %rcx, (%rdi)       \n\t"
        "xor    %eax, %eax         \n\t"
        "ret                       \n\t"
    "1:                            \n\t"
        "mov    $-1, %eax          \n\t"
        "ret                       \n\t"
    );
}

/* ARM implementation using RNDR */
#elif defined(truernd_ARCH_ARM)

#if defined(__aarch64__) || defined(_M_ARM64)

NAKED int 
truernd_is_supported(void) {
    __asm__ volatile(
        "mrs x0, ID_AA64ISAR0_EL1 \n\t"
        "lsr x0, x0, #60          \n\t"
        "and x0, x0, #0xF         \n\t"
        "cmp x0, #0               \n\t"
        "cset w0, ne              \n\t"
        "ret                      \n\t"
    );
}

NAKED uint32_t 
truernd_gen32(void) {
    __asm__ volatile(
        "mrs x0, RNDR             \n\t"
        "b.eq 1f                  \n\t"
        "ret                      \n\t"
    "1:                           \n\t"
        "mov w0, #0               \n\t"
        "ret                      \n\t"
    );
}

NAKED uint64_t 
truernd_gen64(void) {
    __asm__ volatile(
        "mrs x0, RNDR             \n\t"
        "b.eq 1f                  \n\t"
        "ret                      \n\t"
    "1:                           \n\t"
        "mov x0, #0               \n\t"
        "ret                      \n\t"
    );
}

NAKED int 
truernd_get32(uint32_t *out) {
    __asm__ volatile(
        "cbz x0, 1f               \n\t"

        "mrs x1, RNDR             \n\t"
        "b.eq 1f                  \n\t"

        "str w1, [x0]             \n\t"
        "mov w0, #0               \n\t"
        "ret                      \n\t"
    "1:                           \n\t"
        "mov w0, #-1              \n\t"
        "ret                      \n\t"
    );
}

NAKED int 
truernd_get64(uint64_t *out) {
    __asm__ volatile(
        "cbz x0, 1f               \n\t"

        "mrs x1, RNDR             \n\t"
        "b.eq 1f                  \n\t"

        "str x1, [x0]             \n\t"
        "mov w0, #0               \n\t"
        "ret                      \n\t"
    "1:                           \n\t"
        "mov w0, #-1              \n\t"
        "ret                      \n\t"
    );
}

#else /* 32-bit ARM - RNDR not available */

NAKED int 
truernd_is_supported(void) {
    __asm__ volatile(
        "mov r0, #0               \n\t"
        "bx lr                    \n\t"
    );
}

NAKED int 
truernd_get32(uint32_t *out) {
    __asm__ volatile(
        "mov r0, #-1              \n\t"
        "bx lr                    \n\t"
    );
}

NAKED int 
truernd_get64(uint64_t *out) {
    __asm__ volatile(
        "mov r0, #-1              \n\t"
        "bx lr                    \n\t"
    );
}

NAKED uint32_t 
truernd_gen32(void) {
    __asm__ volatile(
        "mov r0, #0               \n\t"
        "bx lr                    \n\t"
    );
}

NAKED  uint64_t 
truernd_gen64(void) {
    __asm__ volatile(
        "mov r0, #0               \n\t"
        "mov r1, #0               \n\t"
        "bx lr                    \n\t"
    );
}

#endif /* __aarch64__ */

/* Unsupported architecture */
#else

int 
truernd_is_supported(void) {
    return 0;
}

int 
truernd_get32(uint32_t *out) {
    (void)out;
    return -1;
}

int 
truernd_get64(uint64_t *out) {
    (void)out;
    return -1;
}

uint32_t 
truernd_gen32(void) {
    return 0;
}

uint64_t 
truernd_gen64(void) {
    return 0;
}

#endif /* Architecture selection */

/* Common implementation for truernd_fill */
static inline int 
truernd_fill(void *buf, size_t len) {
    if (!buf || len == 0) return -1;

    uint8_t *ptr = (uint8_t*)buf;

    while (len >= 8) {
        uint64_t val;
        int retries = 0;
        while (truernd_get64(&val) != 0) {
            if (++retries >= TRUERND_MAX_RETRIES) return -1;
        }
        for (int i = 0; i < 8; i++) {
            *ptr++ = (uint8_t)(val >> (i * 8));
        }
        len -= 8;
    }

    if (len > 0) {
        uint64_t val;
        int retries = 0;
        while (truernd_get64(&val) != 0) {
            if (++retries >= TRUERND_MAX_RETRIES) return -1;
        }
        for (size_t i = 0; i < len; i++) {
            *ptr++ = (uint8_t)(val >> (i * 8));
        }
    }

    return 0;
}
#ifdef __cplusplus
}
#endif 

#endif /* TRUERANDOM_IMPLEMENTATION */
