#pragma once

typedef void (*test_function_t)(void);
typedef struct {
    const char *name;           // Short name for command reference
    test_function_t function;  // Pointer to the test function
    const char *description;    // Full description of the test
} test_t;

const test_t *find_test(const char *name);
void show_test_library(void);

