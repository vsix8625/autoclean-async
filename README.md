## Autoclean

`autoclean.h` is a simple, thread-safe C library for automatic resource management. It provides a set of functions to automatically track and clean up dynamically allocated memory and other resources, which helps prevent memory leaks and simplifies resource management in multithreaded applications.

### Features

- **Automatic Memory Cleanup**: Easily allocate memory (`AC_Alloc`, `AC_ArrayAlloc`, `AC_Strdup`) that is automatically registered for cleanup.
- **Generic Resource Management**: Register any resource with a custom cleanup function using `AC_Register`.
- **Thread-Safe**: All core functions are protected by a mutex, making the library safe to use in multithreaded environments.
- **Asynchronous Cleanup**: Perform a full cleanup of all tracked resources in a detached background thread.
- **Built-in Logging**: A simple, thread-safe logging mechanism helps you track cleanup status.

---

### How It Works

The library maintains a global, thread-safe list of pointers and their corresponding cleanup functions.

- When you call an allocation function, the returned pointer is added to this list with a default `free()` function.
- When you use `AC_Register`, you add a custom pointer and a custom cleanup function.
- When `AC_CleanupAll` is called, the library iterates through the list in reverse and executes the cleanup function for each item.
- The `AC_Remove` function lets you manually clean up and remove a specific resource.

---

### Usage

#### 1. Include the Header

```c
#include "autoclean.h"
```

#### 2. Allocating memory 

#### Use the provided functions as drop-in replacements for standard C library functions.

```c
char *data = AC_Alloc(1024);
int *numbers = AC_ArrayAlloc(100, sizeof(int));
char *my_string = AC_Strdup("Hello, Autoclean!");
```

#### 3. Registering Custom Resources

#### Use AC_Register to add a pointer to any resource along with a custom cleanup function.

```c
void close_file(void *ptr) {
    FILE *fp = (FILE *)ptr;
    if (fp) {
        fclose(fp);
    }
}

FILE *file_handle = fopen("log.txt", "w");
if (file_handle) {
    AC_Register(file_handle, close_file);
}
```

#### 4. Cleaning up

#### You have two options for cleaning up all registered resources.

##### Synchronous Cleanup (Blocking)
```c
AC_CleanupAll();
```

##### Asynchronous Cleanup (Non-Blocking)
```c
AC_CleanupAllAsync();
```

#### 5. Manual Removal

#### If you need to free a specific resource before a full cleanup, use AC_Remove.

```c
if (AC_Remove(data)) {
    printf("Successfully removed 'data' from the cleanup list and freed it.\n");
}
```

#### 6. Flushing Logs

#### To see log messages from the asynchronous cleanup thread, you need to flush the log buffer.

```c
AC_FlushLogs();
```

## Compilation

### Since the library uses pthreads, its recommended to link with the pthread library.

```bash
gcc -c autoclean.c -o autoclean.o
gcc your_program.c autoclean.o -o your_program -pthread
```
