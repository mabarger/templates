// Helper functions for f&r
static __always_inline __attribute__((always_inline)) void maccess(void *p) {
    *(volatile unsigned char *)p;
}

static __always_inline __attribute__((always_inline)) uint64_t rdtscp(void) {
    uint64_t lo, hi;
    asm volatile("rdtscp\n" : "=a" (lo), "=d" (hi) :: "rcx");
    return (hi << 32) | lo;
}

static __always_inline __attribute__((always_inline)) uint64_t load_time(void *p) {
    uint64_t t0 = rdtscp();
    maccess(p);
    return rdtscp() - t0;
}
