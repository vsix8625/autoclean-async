#ifndef AUTOCLEAN_CX_H_
#define AUTOCLEAN_CX_H_

#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (*AC_CleanupFn)(void *);

typedef struct AC_Entry
{
    void *ptr;
    AC_CleanupFn fn;
} AC_Entry;

/* Allocates memory of size bytes, tracks it for cleanup with free().
 * Returns NULL on failure or if cleanup list is full.
 */
void *AC_Alloc(size_t size);

/* Registers a pointer with a custom cleanup function. Warns if cleanup list is full. */
void AC_Register(void *ptr, AC_CleanupFn fn);

void *AC_ArrayAlloc(size_t count, size_t size);

char *AC_Strdup(const char *s);

/* Remove a specific pointer from the list.
 * @note: AC_Remove modifies the cleanup order and should be used with caution.
 */
int AC_Remove(void *ptr);

/* Cleans up all registered resources in reverse order and resets the cleanup list. */
void AC_CleanupAll(void);

/* Cleans up all registered resources in reverse order and resets the cleanup list.
 * This functions runs on a detached thread*/
void AC_CleanupAllAsync(void);

void AC_FlushLogs(void);

//----------------------------------------------------------------------------------------------------
//------------------- Async_Log ----------------------------------------------------------------------

#define ASYNC_LOG_CAPACITY 64
#define ASYNC_LOG_MSG_SIZE 128

typedef struct AsyncLog
{
    char buffer[ASYNC_LOG_CAPACITY][ASYNC_LOG_MSG_SIZE];
    atomic_size_t head;
    atomic_size_t tail;
    pthread_mutex_t lock;
} AsyncLog;

void AsyncLogInit(AsyncLog *al);
void AsyncLogPush(AsyncLog *al, const char *msg);
void AsyncLogFlush(AsyncLog *al);

#endif  // AUTOCLEAN_CX_H_
