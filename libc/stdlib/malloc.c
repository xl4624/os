#include <stddef.h>
#include <stdlib.h>

#if defined(__is_libk)

/*
 * Kernel build: delegate to the kernel heap allocator.
 * These symbols are defined in kernel/mm/heap.cpp and resolved by the linker
 * when libk.a is linked with the kernel object files.
 */
void *kmalloc(size_t size);
void kfree(void *ptr);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t size);

void *malloc(size_t size) {
    return kmalloc(size);
}

void free(void *ptr) {
    kfree(ptr);
}

void *calloc(size_t nmemb, size_t size) {
    return kcalloc(nmemb, size);
}

void *realloc(void *ptr, size_t size) {
    return krealloc(ptr, size);
}

#elif defined(__is_libc)

/*
 * Userspace build: not yet implemented.
 * Will delegate to brk(2) / mmap(2) system calls once userspace is supported.
 */
void *malloc(size_t size) {
    (void)size;
    return (void *)0;
}

void free(void *ptr) {
    (void)ptr;
}

void *calloc(size_t nmemb, size_t size) {
    (void)nmemb;
    (void)size;
    return (void *)0;
}

void *realloc(void *ptr, size_t size) {
    (void)ptr;
    (void)size;
    return (void *)0;
}

#endif
