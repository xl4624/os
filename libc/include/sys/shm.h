#ifndef _SYS_SHM_H
#define _SYS_SHM_H

#include <sys/cdefs.h>

__BEGIN_DECLS

// Allocate a shared memory region of the given size (rounded up to page
// boundaries). Returns a shmid (>= 0) on success, -1 on failure.
int shmget(unsigned int size);

// Map the shared memory region identified by shmid into the caller's
// address space at the given virtual address. Returns 0 on success, -1
// on failure.
int shmat(int shmid, void* vaddr);

// Unmap the shared memory region starting at vaddr (size bytes) from
// the caller's address space. Returns 0 on success, -1 on failure.
int shmdt(void* vaddr, unsigned int size);

__END_DECLS

#endif
