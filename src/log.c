#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>

#include "log.h"

extern pthread_mutex_t log_mutex;

static int log_fd = -1;

static int get_log_fd()
{
    if(log_fd == -1)
    {
        log_fd = open(FIFO_NAME, O_WRONLY);

        if(log_fd < 0)
        {
            perror("open fifo");
        }
    }

    return log_fd;
}

void write_log(const char *msg)
{
    pthread_mutex_lock(&log_mutex);

    int fd = get_log_fd();

    if(fd >= 0)
    {
        write(fd,
              msg,
              strlen(msg));

        write(fd,
              "\n",
              1);
    }

    pthread_mutex_unlock(&log_mutex);
}

void write_log_format(const char *fmt, ...)
{
    pthread_mutex_lock(&log_mutex);

    int fd = get_log_fd();

    if(fd >= 0)
    {
        char buffer[512];

        va_list args;

        va_start(args, fmt);

        vsnprintf(buffer,
                  sizeof(buffer),
                  fmt,
                  args);

        va_end(args);

        write(fd,
              buffer,
              strlen(buffer));

        write(fd,
              "\n",
              1);
    }

    pthread_mutex_unlock(&log_mutex);
}