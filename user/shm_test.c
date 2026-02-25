#include <stdio.h>
#include <sys/shm.h>
#include <unistd.h>

/* Virtual address where both processes will map the shared page. */
#define SHM_ADDR ((void*)0x00A00000)
#define SHM_SIZE 4096

int main(void) {
  int id = shmget(SHM_SIZE);
  if (id < 0) {
    printf("shm_test: shmget() failed\n");
    return 1;
  }

  if (shmat(id, SHM_ADDR) < 0) {
    printf("shm_test: shmat() failed\n");
    return 1;
  }

  /* Write a value into the shared page. */
  volatile int* shared = (volatile int*)SHM_ADDR;
  *shared = 42;

  printf("parent: wrote %d to shared memory\n", *shared);

  int pid = fork();
  if (pid < 0) {
    printf("shm_test: fork() failed\n");
    return 1;
  }

  if (pid == 0) {
    /* Child: the shared page is already mapped (inherited via fork). */
    printf("child: read %d from shared memory\n", *shared);

    /* Write a different value. */
    *shared = 99;
    printf("child: wrote %d to shared memory\n", *shared);

    shmdt(SHM_ADDR, SHM_SIZE);
    return 0;
  }

  /* Parent: wait for child. */
  int status = 0;
  int ret;
  while ((ret = waitpid(-1, &status)) == 0) {
  }

  printf("parent: after child, shared memory = %d\n", *shared);

  shmdt(SHM_ADDR, SHM_SIZE);
  printf("shm_test: done\n");
  return 0;
}
