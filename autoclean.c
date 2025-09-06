#include "autoclean.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define INITIAL_LIST_SIZE 0x400

static AC_Entry *cleanup_list = NULL;
static size_t cleanup_count = 0;
static size_t cleanup_capacity = 0;

static pthread_mutex_t cleanup_lock = PTHREAD_MUTEX_INITIALIZER;

static AsyncLog ac_log;

int ac_ensure_capacity(void)
{
    if (!cleanup_list)
    {
        cleanup_capacity = INITIAL_LIST_SIZE;
        cleanup_list = malloc(sizeof(AC_Entry) * cleanup_capacity);
        if (!cleanup_list)
        {
            return 0;  // failed to allocate
        }
    }
    else if (cleanup_count >= cleanup_capacity)
    {
        size_t new_capacity = cleanup_capacity * 2;
        AC_Entry *tmp = realloc(cleanup_list, sizeof(AC_Entry) * new_capacity);
        if (!tmp)
        {
            return 0;  // failed to realloc
        }
        cleanup_list = tmp;
        cleanup_capacity = new_capacity;
    }
    return 1;
}

void *AC_Alloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr)
    {
        fprintf(stderr, "Failed to allocate %zu bytes\n", size);
        return NULL;
    }

    pthread_mutex_lock(&cleanup_lock);

    if (!ac_ensure_capacity())
    {
        pthread_mutex_unlock(&cleanup_lock);
        free(ptr);
        return NULL;
    }

    cleanup_list[cleanup_count++] = (AC_Entry) {ptr, free};
    pthread_mutex_unlock(&cleanup_lock);
    return ptr;
}

void *AC_ArrayAlloc(size_t count, size_t size)
{
    return AC_Alloc(count * size);
}

char *AC_Strdup(const char *s)
{
    if (!s)
    {
        return NULL;
    }
    size_t len = strlen(s) + 1;
    char *dup = AC_Alloc(len);
    if (!dup)
    {
        return NULL;
    }

    memcpy(dup, s, len);
    return dup;
}

void AC_Register(void *ptr, AC_CleanupFn fn)
{
    pthread_mutex_lock(&cleanup_lock);
    if (!ac_ensure_capacity())
    {
        pthread_mutex_unlock(&cleanup_lock);
        return;
    }
    cleanup_list[cleanup_count++] = (AC_Entry) {ptr, fn};
    pthread_mutex_unlock(&cleanup_lock);
}

int AC_Remove(void *ptr)
{
    int status = 0;
    pthread_mutex_lock(&cleanup_lock);
    for (size_t i = 0; i < cleanup_count; i++)
    {
        if (cleanup_list[i].ptr == ptr)
        {
            if (cleanup_list[i].fn)
            {
                // call the cleanup function of the ptr
                cleanup_list[i].fn(cleanup_list[i].ptr);
            }

            // shift remaining ptrs down
            for (size_t j = i; j + 1 < cleanup_count; j++)
            {
                cleanup_list[j] = cleanup_list[j + 1];
            }

            cleanup_count--;
            status = 1;  // success
            break;
        }
    }
    pthread_mutex_unlock(&cleanup_lock);
    return status;
}

void AC_CleanupAll(void)
{
    pthread_mutex_lock(&cleanup_lock);
    for (ssize_t i = cleanup_count - 1; i >= 0; i--)
    {
        if (cleanup_list[i].ptr && cleanup_list[i].fn)
        {
            cleanup_list[i].fn(cleanup_list[i].ptr);
        }
    }
    cleanup_count = 0;
    pthread_mutex_unlock(&cleanup_lock);
}

void *ac_cleanup_thread(void *arg)
{
    (void) arg;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    AC_CleanupAll();

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    char msg[128];
    snprintf(msg, sizeof(msg), "[AC_CleanupAllAsync] Finished in %.6f sec", elapsed);
    AsyncLogPush(&ac_log, msg);
    return NULL;
}

void AC_CleanupAllAsync(void)
{
    pthread_t tid;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&tid, &attr, ac_cleanup_thread, NULL) != 0)
    {
        fprintf(stderr, "Failed to create cleanup thread, running sync\n");
        AC_CleanupAll();  // fallback
        char msg[128];
        snprintf(msg, sizeof(msg), "[AC_CleanupAll-Fallback] Finished sync cleanup");
        AsyncLogPush(&ac_log, msg);
    }
    pthread_attr_destroy(&attr);
}

// ------------------- Async_Log--------------------

void AsyncLogInit(AsyncLog *al)
{
    atomic_init(&al->head, 0);
    atomic_init(&al->tail, 0);
    pthread_mutex_init(&al->lock, NULL);
}

void AsyncLogPush(AsyncLog *al, const char *msg)
{
    pthread_mutex_lock(&al->lock);

    size_t head = atomic_load(&al->head);
    size_t next = (head + 1) % ASYNC_LOG_CAPACITY;
    if (next == atomic_load(&al->tail))
    {
        atomic_store(&al->tail, (atomic_load(&al->tail) + 1) % ASYNC_LOG_CAPACITY);
    }
    strncpy(al->buffer[head], msg, ASYNC_LOG_MSG_SIZE - 1);
    al->buffer[head][ASYNC_LOG_MSG_SIZE - 1] = '\0';

    atomic_store(&al->head, next);
    pthread_mutex_unlock(&al->lock);
}

void AsyncLogFlush(AsyncLog *al)
{
    pthread_mutex_lock(&al->lock);
    while (atomic_load(&al->tail) != atomic_load(&al->head))
    {
        size_t tail = atomic_load(&al->tail);
        printf("%s\n", al->buffer[tail]);
        atomic_store(&al->tail, (tail + 1) % ASYNC_LOG_CAPACITY);
    }
    pthread_mutex_unlock(&al->lock);
}

void AC_FlushLogs(void)
{
    AsyncLogFlush(&ac_log);
}
