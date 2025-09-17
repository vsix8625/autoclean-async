#include "autoclean.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// Custom cleanup function for mmap
void mmap_cleanup(void *ptr)
{
    if (ptr)
    {
        printf("Cleaning up mmap memory at %p\n", ptr);
        munmap(ptr, 4096);
    }
}

int main(void)
{
    printf("=== Autoclean Demo with RingLog ===\n");

    // 1. Allocate regular memory
    char *buf = AC_Alloc(128);
    if (!buf)
        return 1;
    snprintf(buf, 128, "Hello AC_Alloc!");
    printf("Allocated buffer at %p: '%s'\n", (void *) buf, buf);

    // 2. Allocate strdup memory
    char *str = AC_Strdup("Hello AC_Strdup!");
    if (!str)
        return 1;
    printf("Allocated strdup string at %p: '%s'\n", (void *) str, str);

    // 3. Allocate mmap memory
    void *mem = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED)
    {
        perror("mmap failed");
        return 1;
    }
    AC_Register(mem, mmap_cleanup);
    printf("Allocated mmap memory at %p\n", mem);
    AC_Register(mem, mmap_cleanup);

    // 4. Modify memory
    buf[0] = 'A';
    str[0] = 'S';
    ((char *) mem)[0] = 'M';
    printf("Modified buffer, strdup, and mmap memory\n");

    // 5. Remove strdup entry early
    if (AC_Remove(str))
        printf("AC_Remove: Freed strdup string at %p\n", (void *) str);

    // 6. Run async cleanup
    printf("Running AC_CleanupAllAsync()...\n");
    AC_CleanupAllAsync();

    // Give cleanup thread some time to finish
    sleep(1);

    // 7. Flush any async cleanup messages from ring log
    AC_FlushLogs();

    printf("Demo finished\n");
    return 0;
}
