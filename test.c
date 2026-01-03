/**
 * @file test.c
 * @brief Comprehensive test suite for truerandom.h library
 */

#define TRUERANDOM_IMPLEMENTATION
#include "truerandom.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define TEST_ITERATIONS 1000000
#define BUFFER_SIZE 256

#define ANSI_RESET    "\033[0m"
#define ANSI_BOLD     "\033[1m"
#define ANSI_DIM      "\033[2m"
#define ANSI_RED      "\033[31m"
#define ANSI_GREEN    "\033[32m"
#define ANSI_YELLOW   "\033[33m"
#define ANSI_BLUE     "\033[34m"
#define ANSI_MAGENTA  "\033[35m"
#define ANSI_CYAN     "\033[36m"

#define SYMBOL_PASS     "✓"
#define SYMBOL_FAIL     "✗"
#define SYMBOL_WARNING  "⚠"

static void print_separator(void) {
    printf(ANSI_DIM "════════════════════════════════════════════════════════\n" ANSI_RESET);
}

static void print_thick_separator(void) {
    printf(ANSI_BOLD "════════════════════════════════════════════════════════\n" ANSI_RESET);
}

static void print_header(const char *title) {
    printf("\n");
    print_separator();
    printf(ANSI_BOLD ANSI_CYAN "%s\n" ANSI_RESET, title);
    print_separator();
}

static void print_pass(const char *msg) {
    printf(ANSI_GREEN ANSI_BOLD SYMBOL_PASS " PASS: " ANSI_RESET ANSI_GREEN "%s\n" ANSI_RESET, msg);
}

static void print_fail(const char *msg) {
    printf(ANSI_RED ANSI_BOLD SYMBOL_FAIL " FAIL: " ANSI_RESET ANSI_RED "%s\n" ANSI_RESET, msg);
}

static void print_warning(const char *msg) {
    printf(ANSI_YELLOW ANSI_BOLD SYMBOL_WARNING " WARNING: " ANSI_RESET ANSI_YELLOW "%s\n" ANSI_RESET, msg);
}

/**
 * @brief Test hardware support detection
 */
static int test_support(void) {
    print_header("TEST 1: Hardware Support Detection");

    int supported = truernd_is_supported();
    printf("Hardware RNG supported: %s\n", 
           supported ? ANSI_GREEN "YES" ANSI_RESET : ANSI_RED "NO" ANSI_RESET);

    if (!supported) {
        print_fail("Hardware RNG not supported on this system");
        printf(ANSI_DIM "  This system does not have:\n");
        #if defined(truernd_ARCH_X86)
        printf("  - RDRAND instruction (x86/x64)\n" ANSI_RESET);
        #elif defined(truernd_ARCH_ARM)
        printf("  - RNDR instruction (ARMv8.5-A+)\n" ANSI_RESET);
        #else
        printf("  - Unknown architecture\n" ANSI_RESET);
        #endif
        return 0;
    }
    
    print_pass("Hardware RNG is available");
    return 1;
}

/**
 * @brief Test 32-bit random number generation
 */
static int test_get32(void) {
    print_header("TEST 2: 32-bit Random Generation");
    
    uint32_t values[10];
    int success_count = 0;
    
    printf("Generating 10 random 32-bit numbers:\n");
    for (int i = 0; i < 10; i++) {
        if (truernd_get32(&values[i]) == 0) {  /* 0 = success */
            printf(ANSI_DIM "  [%d]" ANSI_RESET " " ANSI_MAGENTA "0x%08X" ANSI_RESET 
                   " " ANSI_DIM "(%u)\n" ANSI_RESET, i, values[i], values[i]);
            success_count++;
        } else {
            printf(ANSI_RED "  [%d] FAILED to generate\n" ANSI_RESET, i);
        }
    }
    
    if (success_count == 10) {
        print_pass("All 32-bit generations successful");
        return 1;
    } else {
        char msg[100];
        snprintf(msg, sizeof(msg), "Only %d/10 generations successful", success_count);
        print_fail(msg);
        return 0;
    }
}

/**
 * @brief Test 64-bit random number generation
 */
static int test_get64(void) {
    print_header("TEST 3: 64-bit Random Generation");

    uint64_t values[10];
    int success_count = 0;

    printf("Generating 10 random 64-bit numbers:\n");
    for (int i = 0; i < 10; i++) {
        if (truernd_get64(&values[i]) == 0) {  /* 0 = success */
            printf(ANSI_DIM "  [%d]" ANSI_RESET " " ANSI_MAGENTA "0x%016llX" ANSI_RESET 
                   " " ANSI_DIM "(%llu)\n" ANSI_RESET,
                   i, (unsigned long long)values[i], (unsigned long long)values[i]);
            success_count++;
        } else {
            printf(ANSI_RED "  [%d] FAILED to generate\n" ANSI_RESET, i);
        }
    }

    if (success_count == 10) {
        print_pass("All 64-bit generations successful");
        return 1;
    } else {
        char msg[100];
        snprintf(msg, sizeof(msg), "Only %d/10 generations successful", success_count);
        print_fail(msg);
        return 0;
    }
}

/**
 * @brief Test buffer filling functionality
 */
static int test_fill(void) {
    print_header("TEST 4: Buffer Fill");

    uint8_t buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    printf("Filling %d byte buffer with random data...\n", BUFFER_SIZE);
    if (truernd_fill(buffer, sizeof(buffer)) != 0) {  /* 0 = success, -1 = error */
        print_fail("Failed to fill buffer");
        return 0;
    }

    printf(ANSI_DIM "First 64 bytes (hex):\n" ANSI_RESET);
    for (int i = 0; i < 64; i++) {
        if (i > 0 && i % 16 == 0) printf("\n");
        printf(ANSI_MAGENTA "%02X " ANSI_RESET, buffer[i]);
    }
    printf("\n");

    int non_zero = 0;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (buffer[i] != 0) non_zero++;
    }

    if (non_zero > BUFFER_SIZE / 2) {
        char msg[100];
        snprintf(msg, sizeof(msg), "Buffer filled with random data (%d/%d non-zero bytes)", 
                 non_zero, BUFFER_SIZE);
        print_pass(msg);
        return 1;
    } else {
        char msg[100];
        snprintf(msg, sizeof(msg), "Buffer appears not random (%d/%d non-zero bytes)", 
                 non_zero, BUFFER_SIZE);
        print_fail(msg);
        return 0;
    }
}

/**
 * @brief Test for basic randomness (uniqueness check)
 */
static int test_uniqueness(void) {
    print_header("TEST 5: Uniqueness Check");

    uint64_t values[100];
    int duplicates = 0;

    printf("Generating 100 random 64-bit numbers and checking for duplicates...\n");

    for (int i = 0; i < 100; i++) {
        if (truernd_get64(&values[i]) != 0) {  /* 0 = success */
            char msg[100];
            snprintf(msg, sizeof(msg), "Failed to generate value at index %d", i);
            print_fail(msg);
            return 0;
        }
    }

    for (int i = 0; i < 100; i++) {
        for (int j = i + 1; j < 100; j++) {
            if (values[i] == values[j]) {
                duplicates++;
                printf(ANSI_YELLOW "  Duplicate found: values[%d] == values[%d] = 0x%016llX\n" ANSI_RESET,
                       i, j, (unsigned long long)values[i]);
            }
        }
    }

    if (duplicates == 0) {
        print_pass("All 100 values are unique");
        return 1;
    } else {
        char msg[100];
        snprintf(msg, sizeof(msg), "Found %d duplicate(s) (may occur randomly)", duplicates);
        print_warning(msg);
        return duplicates <= 1 ? 1 : 0;
    }
}

/**
 * @brief Test performance/throughput
 */
static int test_performance(void) {
    print_header("TEST 6: Performance Test");

    clock_t start, end;
    double cpu_time;

    printf("Generating %s%d%s random 32-bit numbers...\n", 
           ANSI_CYAN, TEST_ITERATIONS, ANSI_RESET);
    start = clock();
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        uint32_t val;
        if (truernd_get32(&val) != 0) {  /* 0 = success */
            char msg[100];
            snprintf(msg, sizeof(msg), "Generation failed at iteration %d", i);
            print_fail(msg);
            return 0;
        }
    }
    end = clock();
    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf(ANSI_DIM "  Time: " ANSI_RESET ANSI_GREEN "%.4f seconds\n" ANSI_RESET, cpu_time);
    printf(ANSI_DIM "  Rate: " ANSI_RESET ANSI_GREEN "%.0f numbers/second\n" ANSI_RESET, 
           TEST_ITERATIONS / cpu_time);

    printf("\nGenerating %s%d%s random 64-bit numbers...\n", 
           ANSI_CYAN, TEST_ITERATIONS, ANSI_RESET);
    start = clock();
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        uint64_t val;
        if (truernd_get64(&val) != 0) {  /* 0 = success */
            char msg[100];
            snprintf(msg, sizeof(msg), "Generation failed at iteration %d", i);
            print_fail(msg);
            return 0;
        }
    }
    end = clock();
    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf(ANSI_DIM "  Time: " ANSI_RESET ANSI_GREEN "%.4f seconds\n" ANSI_RESET, cpu_time);
    printf(ANSI_DIM "  Rate: " ANSI_RESET ANSI_GREEN "%.0f numbers/second\n" ANSI_RESET, 
           TEST_ITERATIONS / cpu_time);
    
    print_pass("Performance test completed");
    return 1;
}

/**
 * @brief Test error handling (NULL pointers and invalid parameters)
 */
static int test_error_handling(void) {
    print_header("TEST 7: Error Handling");

    int all_passed = 1;

    printf("Testing NULL pointer handling...\n");

    /* Test get32 with NULL - should return -1 */
    int result32 = truernd_get32(NULL);
    printf("  truernd_get32(NULL):     %s", 
           result32 == -1 ? ANSI_GREEN "PASS" ANSI_RESET : ANSI_RED "FAIL" ANSI_RESET);
    printf(ANSI_DIM " (returned %d, expected -1)\n" ANSI_RESET, result32);
    if (result32 != -1) all_passed = 0;

    /* Test get64 with NULL - should return -1 */
    int result64 = truernd_get64(NULL);
    printf("  truernd_get64(NULL):     %s", 
           result64 == -1 ? ANSI_GREEN "PASS" ANSI_RESET : ANSI_RED "FAIL" ANSI_RESET);
    printf(ANSI_DIM " (returned %d, expected -1)\n" ANSI_RESET, result64);
    if (result64 != -1) all_passed = 0;

    /* Test fill with NULL buffer - should return -1 */
    int result_fill = truernd_fill(NULL, 100);
    printf("  truernd_fill(NULL, 100): %s", 
           result_fill == -1 ? ANSI_GREEN "PASS" ANSI_RESET : ANSI_RED "FAIL" ANSI_RESET);
    printf(ANSI_DIM " (returned %d, expected -1)\n" ANSI_RESET, result_fill);
    if (result_fill != -1) all_passed = 0;

    /* Test fill with zero length - should return -1 */
    uint8_t buf[10];
    int result_fill_zero = truernd_fill(buf, 0);
    printf("  truernd_fill(buf, 0):    %s", 
           result_fill_zero == -1 ? ANSI_GREEN "PASS" ANSI_RESET : ANSI_RED "FAIL" ANSI_RESET);
    printf(ANSI_DIM " (returned %d, expected -1)\n" ANSI_RESET, result_fill_zero);
    if (result_fill_zero != -1) all_passed = 0;

    if (all_passed) {
        print_pass("All error cases handled correctly");
        return 1;
    } else {
        print_fail("Error handling not working correctly");
        return 0;
    }
}

/**
 * @brief Test gen32 and gen64 functions
 */
static int test_gen_functions(void) {
    print_header("TEST 8: Gen32/Gen64 Functions");

    printf("Testing truernd_gen32() and truernd_gen64()...\n");

    printf("\nGenerating 5 random 32-bit values with truernd_gen32():\n");
    for (int i = 0; i < 5; i++) {
        uint32_t val = truernd_gen32();
        printf(ANSI_DIM "  [%d]" ANSI_RESET " " ANSI_MAGENTA "0x%08X" ANSI_RESET 
               " " ANSI_DIM "(%u)\n" ANSI_RESET, i, val, val);
    }

    printf("\nGenerating 5 random 64-bit values with truernd_gen64():\n");
    for (int i = 0; i < 5; i++) {
        uint64_t val = truernd_gen64();
        printf(ANSI_DIM "  [%d]" ANSI_RESET " " ANSI_MAGENTA "0x%016llX" ANSI_RESET 
               " " ANSI_DIM "(%llu)\n" ANSI_RESET, 
               i, (unsigned long long)val, (unsigned long long)val);
    }

    print_pass("Gen functions completed successfully");
    return 1;
}

/**
 * @brief Main test runner
 */
int main(void) {
    int total_tests = 0;
    int passed_tests = 0;

    printf("\n");
    print_thick_separator();
    printf(ANSI_BOLD ANSI_CYAN "       TRUERANDOM.H - TEST SUITE       \n" ANSI_RESET);
    print_thick_separator();
    printf("\n");

    printf(ANSI_BOLD "Architecture: " ANSI_RESET);
    #if defined(truernd_ARCH_X86)
        #if defined(__x86_64__) || defined(_M_X64)
        printf(ANSI_CYAN "x86_64 (64-bit)\n" ANSI_RESET);
        #else
        printf(ANSI_CYAN "x86 (32-bit)\n" ANSI_RESET);
        #endif
    #elif defined(truernd_ARCH_ARM)
        #if defined(__aarch64__) || defined(_M_ARM64)
        printf(ANSI_CYAN "ARM64 (64-bit)\n" ANSI_RESET);
        #else
        printf(ANSI_CYAN "ARM (32-bit)\n" ANSI_RESET);
        #endif
    #else
        printf(ANSI_RED "Unknown/Unsupported\n" ANSI_RESET);
    #endif

    total_tests++; 
    passed_tests += test_support();

    if (!truernd_is_supported()) {
        printf("\n");
        print_thick_separator();
        printf(ANSI_RED ANSI_BOLD "Cannot continue testing without hardware support\n" ANSI_RESET);
        print_thick_separator();
        printf("\n");
        return 1;
    }

    total_tests++; passed_tests += test_get32();
    total_tests++; passed_tests += test_get64();
    total_tests++; passed_tests += test_fill();
    total_tests++; passed_tests += test_uniqueness();
    total_tests++; passed_tests += test_performance();
    total_tests++; passed_tests += test_error_handling();
    total_tests++; passed_tests += test_gen_functions();

    printf("\n");
    print_thick_separator();
    printf(ANSI_BOLD ANSI_CYAN "TEST SUMMARY\n" ANSI_RESET);
    print_thick_separator();
    printf(ANSI_BOLD "Total tests:  " ANSI_RESET "%s%d\n" ANSI_RESET, 
           ANSI_CYAN, total_tests);
    printf(ANSI_BOLD "Passed:       " ANSI_RESET "%s%d\n" ANSI_RESET, 
           ANSI_GREEN, passed_tests);
    printf(ANSI_BOLD "Failed:       " ANSI_RESET "%s%d\n" ANSI_RESET, 
           (total_tests - passed_tests) > 0 ? ANSI_RED : ANSI_GREEN, 
           total_tests - passed_tests);
    printf(ANSI_BOLD "Success rate: " ANSI_RESET "%s%.1f%%\n" ANSI_RESET,
           passed_tests == total_tests ? ANSI_GREEN : ANSI_YELLOW,
           (100.0 * passed_tests) / total_tests);
    print_thick_separator();

    if (passed_tests == total_tests) {
        printf(ANSI_GREEN ANSI_BOLD SYMBOL_PASS " ALL TESTS PASSED!\n" ANSI_RESET);
        print_thick_separator();
        printf("\n");
        return 0;
    } else {
        printf(ANSI_RED ANSI_BOLD SYMBOL_FAIL " SOME TESTS FAILED\n" ANSI_RESET);
        print_thick_separator();
        printf("\n");
        return 1;
    }
}
