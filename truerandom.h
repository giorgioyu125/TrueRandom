/*
 * @file truerandom.h
 * @brief True random number generator using hardware instructions (RDRAND for x86, RNDR for ARM)
 * @version 0.0.3
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
 */
static inline int 
truernd_fill(void *buf, size_t len);

/** @}*/

#ifdef __cplusplus
}
#endif

#endif /* TRUERANDOM_H */

/*
 * IMPLEMENTATION
 */

#ifdef TRUERANDOM_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif

/* Intel/AMD x86/x64 implementation using RDRAND */
#if defined(truernd_ARCH_X86)

#include <cpuid.h>

int 
truernd_is_supported(void) {
    uint32_t __eax, __ebx, __ecx, __edx;
    if (!__get_cpuid(1, &__eax, &__ebx, &__ecx, &__edx)) {
        return 0;
    }
    return (__ecx & (1 << 30)) != 0;
}

/* 
 * Generates a 32-bit random number.
 * Returns: The number on success, or 0 on failure.
 */
NAKED uint32_t 
truernd_gen32(void) {
    __asm__ volatile(
        "rdrand %eax       \n\t" // Generate 32-bit random into EAX
        "jnc    1f         \n\t" // If Carry Flag=0 (Fail), jump to label 1
        "ret               \n\t" // Return (Result is in EAX)

        "1:                \n\t" // Failure path
        "xor    %eax, %eax \n\t" // Zero out EAX
        "ret               \n\t"
    );
}

/* 
 * Generates a 64-bit random number.
 * Note: On x64, we use a single 64-bit rdrand instead of two 32-bit ones.
 * Returns: The number on success, or 0 on failure.
 */
NAKED  uint64_t 
truernd_gen64(void) {
    __asm__ volatile(
        "rdrand %rax       \n\t" // Generate 64-bit random into RAX
        "jnc    1f         \n\t" // If Fail, jump to label 1
        "ret               \n\t" // Return (Result is in RAX)

        "1:                \n\t" // Failure path
        "xor    %rax, %rax \n\t" // Zero out RAX
        "ret               \n\t"
    );
}

/*
 * Fills a pointer with a 32-bit random number.
 * Args: out (RDI)
 * Returns: 0 (Success), -1 (Error)
 */
NAKED int 
truernd_get32(uint32_t *out) {
    __asm__ volatile(
        // Check if pointer (RDI) is NULL
        "test   %rdi, %rdi \n\t"
        "jz     1f         \n\t" // Jump to error handler

        // Generate Random
        "rdrand %ecx       \n\t" // Use ECX as temp
        "jnc    1f         \n\t" // Jump to error if HW fails

        // Success Path
        "movl   %ecx, (%rdi)\n\t" // Store result to memory at [RDI]
        "xor    %eax, %eax \n\t"  // Return 0 (success)
        "ret               \n\t"

        // Error Path (both NULL and HW fail)
        "1:                \n\t"
        "mov    $-1, %eax  \n\t"  // Return -1 (error)
        "ret               \n\t"
    );
}

/*
 * Fills a pointer with a 64-bit random number.
 * Args: out (RDI)
 * Returns: 0 (Success), -1 (Error)
 */
NAKED int 
truernd_get64(uint64_t *out) {
    __asm__ volatile(
        // Check if pointer (RDI) is NULL
        "test   %rdi, %rdi \n\t"
        "jz     1f         \n\t"

        // Generate Random (64-bit)
        "rdrand %rcx       \n\t" // Use RCX as temp
        "jnc    1f         \n\t" 

        // Success Path
        "movq   %rcx, (%rdi)\n\t" // Store 64-bit result to memory
        "xor    %eax, %eax \n\t"  // Return 0 (success)
        "ret               \n\t"

        // Error Path
        "1:                \n\t"
        "mov    $-1, %eax  \n\t"  // Return -1 (error)
        "ret               \n\t"
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
        "cmp x0, #0               \n\t"  // Compare with 0
        "cset w0, ne              \n\t"  // Set w0 to 1 if not equal
        "ret                      \n\t"
    );
}

NAKED int 
truernd_get32(uint32_t *out) {
    __asm__ volatile(
        // x0 = out pointer (first argument)
        "cbz x0, 1f               \n\t"  // if (out == NULL) goto fail

        "mrs x1, RNDR             \n\t"  // Read random number
        "b.eq 1f                  \n\t"  // if (NZCV.Z set) goto fail

        "str w1, [x0]             \n\t"  // *out = (uint32_t)val
        "mov w0, #0               \n\t"  // return 0 (success)
        "ret                      \n\t"

    "1:                           \n\t"  // fail:
        "mov w0, #-1              \n\t"  // return -1 (error)
        "ret                      \n\t"
    );
}


NAKED int 
truernd_get64(uint64_t *out) {
    __asm__ volatile(
        // x0 = out pointer (first argument)
        "cbz x0, 1f               \n\t"  // if (out == NULL) goto fail

        "mrs x1, RNDR             \n\t"  // Read random number
        "b.eq 1f                  \n\t"  // if (NZCV.Z set) goto fail

        "str x1, [x0]             \n\t"  // *out = val
        "mov w0, #0               \n\t"  // return 0 (success)
        "ret                      \n\t"

    "1:                           \n\t"  // fail:
        "mov w0, #-1              \n\t"  // return -1 (error)
        "ret                      \n\t"
    );
}

NAKED uint32_t 
truernd_gen32(void) {
    __asm__ volatile(
        "mrs x0, RNDR             \n\t"  // Read random number
        "b.eq 1f                  \n\t"  // If failed, jump to error path
        "ret                      \n\t"  // Return w0 (lower 32 bits)
        
    "1:                           \n\t"  // fail:
        "mov w0, #0               \n\t"  // Return 0 on failure
        "ret                      \n\t"
    );
}

NAKED uint64_t 
truernd_gen64(void) {
    __asm__ volatile(
        "mrs x0, RNDR             \n\t"  // Read random number
        "b.eq 1f                  \n\t"  // If failed, jump to error path
        "ret                      \n\t"  // Return x0
        
    "1:                           \n\t"  // fail:
        "mov x0, #0               \n\t"  // Return 0 on failure
        "ret                      \n\t"
    );
}

#else /* 32-bit ARM - RNDR not available */

NAKED int 
truernd_is_supported(void) {
    __asm__ volatile(
        "mov r0, #0               \n\t"  // Return 0 (not supported)
        "bx lr                    \n\t"
    );
}

NAKED int 
truernd_get32(uint32_t *out) {
    __asm__ volatile(
        "mov r0, #-1              \n\t"  // Return -1 (error)
        "bx lr                    \n\t"
    );
}

NAKED int 
truernd_get64(uint64_t *out) {
    __asm__ volatile(
        "mov r0, #-1              \n\t"  // Return -1 (error)
        "bx lr                    \n\t"
    );
}

NAKED uint32_t 
truernd_gen32(void) {
    __asm__ volatile(
        "mov r0, #0               \n\t"  // Return 0 (unsupported)
        "bx lr                    \n\t"
    );
}

NAKED  uint64_t 
truernd_gen64(void) {
    __asm__ volatile(
        "mov r0, #0               \n\t"  // Return 0 (unsupported)
        "mov r1, #0               \n\t"  // High word for 64-bit return
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
/**
 * @brief Fill a buffer with random bytes
 * @param buf Buffer to fill
 * @param len Length of buffer in bytes
 * @return 0 on success, -1 on failure
 * @note Assumes little-endian byte order for byte extraction from 64-bit values
 */
static inline int 
truernd_fill(void *buf, size_t len) {
    if (!buf || len == 0) return -1;

    uint8_t *ptr = (uint8_t *)buf;

    /* Fill in 64-bit chunks */
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

    /* Fill remaining bytes */
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
