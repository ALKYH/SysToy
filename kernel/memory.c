#include "kernel.h"

struct memory_block {
    u32 start;
    u32 size;
    u32 used;
};

static struct memory_block kernel_heap;

void memory_init(u32 start, u32 size) {
    kernel_heap.start = start;
    kernel_heap.size = size;
    kernel_heap.used = 0;
}

void* memory_alloc(u32 size) {
    u32 aligned = (size + 0x0FFF) & ~0x0FFF;
    void* result;

    if (kernel_heap.used + aligned > kernel_heap.size) {
        return (void*)0;
    }

    result = (void*)(kernel_heap.start + kernel_heap.used);
    kernel_heap.used += aligned;
    return result;
}

u32 memory_remaining(void) {
    return kernel_heap.size - kernel_heap.used;
}
