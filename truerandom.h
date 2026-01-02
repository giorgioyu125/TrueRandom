/*
 * @file truerandom.h
 * @brief True random number generator using hardware instructions (RDRAND for x86, RNDR for ARM)
 * @version 0.2
 * 
 * This is a single-header library that provides true random number generation
 * using CPU hardware random number generators.
 * 
 * The caller is responsible for checking return values and handling retries.
 * 
 * Usage:
 *   In ONE C file, define TRUERANDOM_IMPLEMENTATION before including:
 *   
 *   #define TRUERANDOM_IMPLEMENTATION
 *   #include "truerandom.h"
 *   
 *   In other files, just include normally:
 *   #include "truerandom.h"
 *   
 *   Then use the API:
 *   uint64_t random_val;
 *   if (truerng_get64(&random_val)) {
 *       // Use random_val
 *   } else {
 *       // Handle failure or retry
 *   }
 */

#ifndef TRUERANDOM_H
#define TRUERANDOM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Naked function attribute */
#if defined(_MSC_VER)
    #define NAKED __declspec(naked)
#else
    #define NAKED __attribute__((naked))
#endif

/* Architecture detection */
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define TRUERNG_ARCH_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)
    #define TRUERNG_ARCH_ARM 1
#else
    #define TRUERNG_ARCH_UNKNOWN 1
    #error "Unsupported architecture"
#endif

/*
 * PUBLIC API - DECLARATIONS
 */

/**
 * @brief Check if hardware random number generation is supported
 * @return true if supported, false otherwise
 */
bool truerng_is_supported(void);

/**
 * @brief Generate a 32-bit true random number (single attempt)
 * @param out Pointer to store the random number
 * @return true on success, false on failure (caller should retry)
 */
bool truerng_get32(uint32_t *out);

/**
 * @brief Generate a 64-bit true random number (single attempt)
 * @param out Pointer to store the random number
 * @return true on success, false on failure (caller should retry)
 */
bool truerng_get64(uint64_t *out);

/**
 * @brief Fill a buffer with random bytes
 * @param buf Buffer to fill
 * @param len Length of buffer in bytes
 * @return true on success, false on failure (caller should retry)
 * @note This function makes multiple calls to truerng_get64. If any call fails,
 *       the function returns false immediately. The buffer may be partially filled.
 */
bool truerng_fill(void *buf, size_t len);

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
#if defined(TRUERNG_ARCH_X86)

#include <cpuid.h>

static bool truerng_has_rdrand(void) {
    uint32_t eax, ebx, ecx, edx;
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        return false;
    }
    return (ecx & (1 << 30)) != 0; /* RDRAND is bit 30 of ECX */
}

bool truerng_is_supported(void) {
    return truerng_has_rdrand();
}

bool truerng_get32(uint32_t *out) {
    if (!out) return false;
    
    unsigned char ok;
    __asm__ volatile(
        "rdrand %0; setc %1"
        : "=r" (*out), "=qm" (ok)
        :
        : "cc"
    );
    return ok != 0;
}

bool truerng_get64(uint64_t *out) {
    if (!out) return false;
    
#if defined(__x86_64__) || defined(_M_X64)
    unsigned char ok;
    __asm__ volatile(
        "rdrand %0; setc %1"
        : "=r" (*out), "=qm" (ok)
        :
        : "cc"
    );
    return ok != 0;
#else
    /* 32-bit x86: generate two 32-bit values */
    uint32_t low, high;
    if (!truerng_get32(&low)) return false;
    if (!truerng_get32(&high)) return false;
    *out = ((uint64_t)high << 32) | low;
    return true;
#endif
}

/* ARM implementation using RNDR */
#elif defined(TRUERNG_ARCH_ARM)

bool truerng_is_supported(void) {
#if defined(__aarch64__) || defined(_M_ARM64)
    /* Check if RNDR is available by reading ID_AA64ISAR0_EL1
     * On ARMv8.5-A+, RNDR is available if ID_AA64ISAR0_EL1.RNDR != 0 */
    uint64_t val;
    __asm__ volatile(
        "mrs %0, ID_AA64ISAR0_EL1"
        : "=r" (val)
    );
    return ((val >> 60) & 0xF) != 0; /* RNDR field is bits 63:60 */
#else
    return false; /* 32-bit ARM typically doesn't have RNDR */
#endif
}

bool truerng_get32(uint32_t *out) {
    if (!out) return false;
    
#if defined(__aarch64__) || defined(_M_ARM64)
    uint64_t val;
    uint64_t ok;
    __asm__ volatile(
        "mrs %0, RNDR\n\t"
        "cset %1, ne"
        : "=r" (val), "=r" (ok)
        :
        : "cc"
    );
    if (ok) {
        *out = (uint32_t)val;
        return true;
    }
#endif
    return false;
}

bool truerng_get64(uint64_t *out) {
    if (!out) return false;
    
#if defined(__aarch64__) || defined(_M_ARM64)
    uint64_t val;
    uint64_t ok;
    __asm__ volatile(
        "mrs %0, RNDR\n\t"
        "cset %1, ne"
        : "=r" (val), "=r" (ok)
        :
        : "cc"
    );
    if (ok) {
        *out = val;
        return true;
    }
    return false;
#else
    /* 32-bit ARM: generate two 32-bit values */
    uint32_t low, high;
    if (!truerng_get32(&low)) return false;
    if (!truerng_get32(&high)) return false;
    *out = ((uint64_t)high << 32) | low;
    return true;
#endif
}

/* Unsupported architecture */
#else

bool truerng_is_supported(void) {
    return false;
}

bool truerng_get32(uint32_t *out) {
    (void)out;
    return false;
}

bool truerng_get64(uint64_t *out) {
    (void)out;
    return false;
}

#endif /* Architecture selection */

/* Common implementation for truerng_fill */
bool truerng_fill(void *buf, size_t len) {
    if (!buf || len == 0) return false;
    
    uint8_t *ptr = (uint8_t *)buf;
    
    /* Fill in 64-bit chunks */
    while (len >= 8) {
        uint64_t val;
        if (!truerng_get64(&val)) return false;
        for (int i = 0; i < 8; i++) {
            *ptr++ = (uint8_t)(val >> (i * 8));
        }
        len -= 8;
    }
    
    /* Fill remaining bytes */
    if (len > 0) {
        uint64_t val;
        if (!truerng_get64(&val)) return false;
        for (size_t i = 0; i < len; i++) {
            *ptr++ = (uint8_t)(val >> (i * 8));
        }
    }
    
    return true;
}

#ifdef __cplusplus
}
#endif

#endif /* TRUERANDOM_IMPLEMENTATION */
