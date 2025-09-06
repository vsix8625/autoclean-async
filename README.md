# AutoClean (AC)

A minimal **resource manager in C** for automatic cleanup of malloc, mmap, file descriptors, and other resources.  

Inspired by C++ destructors / scope-based cleanup patterns, AutoClean allows you to register resources with their cleanup functions, and automatically cleans them in **reverse order** when finished.

---

## Features

- Track `malloc` allocations automatically via `AC_Alloc`.
- Track **any custom resource** using `AC_Register`.
- FILO cleanup ensures the **last registered resource is cleaned first**, preventing leaks and respecting dependencies.
- Minimal and portable C implementation.

Installation

Include autoclean.h and autoclean.c in your project:

```c```
    #include "autoclean.h"
```

## Usage Example

```c```

#include "autoclean.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

void mmap_cleanup(void *ptr) {
    if (ptr) {
        printf("Cleaning mmap memory at %p\n", ptr);
        munmap(ptr, 4096);
    }
}

int main(void) {
    printf("=== AutoClean Demo ===\n");

    // Allocate regular memory
    char *buf = AC_Alloc(128);
    if (!buf) return 1;
    snprintf(buf, 128, "Hello AutoClean!");
    printf("Allocated buffer at %p: '%s'\n", (void*)buf, buf);

    // Allocate mmap memory and register custom cleanup
    void *mem = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }
    AC_Register(mem, mmap_cleanup);
    printf("Allocated mmap memory at %p\n", mem);

    // Modify both resources
    buf[0] = 'A';
    ((char*)mem)[0] = 'M';
    printf("Modified buffer and mmap memory\n");

    // Cleanup all resources
    printf("Running AC_CleanupAll()\n");
    AC_CleanupAll();
    printf("Cleanup finished\n");

    return 0;
}

```


# Expected Output: 

=== AutoClean Demo ===
Allocated buffer at 0x55a1c2b6b260: 'Hello AutoClean!'
Allocated mmap memory at 0x7f9d3c400000
Modified buffer and mmap memory
Running AC_CleanupAll()
Cleaning mmap memory at 0x7f9d3c400000
Cleanup finished

## Limitations
- Maximum of 1024 tracked resources (`MAX_TRACKED_ALLOC`). Exceeding this causes `AC_Alloc` to fail and `AC_Register` to warn.
- Not thread-safe (planned mutex support in future).
- `NULL` pointers or cleanup functions are safely ignored by `AC_CleanupAll`.
