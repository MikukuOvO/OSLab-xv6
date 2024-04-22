#include "kernel/types.h"
#include "user/user.h"
#include <stdint.h> // Include this header for intptr_t

void test_memory(char *ptr, int size, char pattern) {
    // Write and check memory as before
    for (int i = 0; i < size; i++) {
        ptr[i] = pattern;
    }
    for (int i = 0; i < size; i++) {
        if (ptr[i] != pattern) {
            printf("Memory check failed at position %d\n", i);
            exit(1);
        }
    }
}

int main(int argc, char *argv[]) {
    int size = 4096;  // Typically the size of a page
    char pattern = 0xAA;  // Test pattern

    // Allocate memory with sbrk()
    char *ptr = sbrk(size);
    if ((intptr_t)ptr == -1) {  // Use intptr_t for casting
        printf("sbrk failed to allocate memory\n");
        exit(1);
    }

    // Test this memory
    test_memory(ptr, size, pattern);

    // Attempt to deallocate memory: demonstration only, unsafe in real use
    char *rollback = sbrk(-size);
    if ((intptr_t)rollback == -1) {  // Use intptr_t for casting
        printf("sbrk failed to deallocate memory\n");
        exit(1);
    }

    printf("All tests passed\n");
    exit(0);
}
