#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#include "log.h"

extern pthread_mutex_t log_mutex;

void write_log(const char *msg)
{
    //--------------------------------
    // LOCK FIFO
    //--------------------------------
    

    pthread_mutex_lock(&log_mutex);

    int fd =
        open(FIFO_NAME, O_WRONLY);

    if(fd < 0)
    {
        perror("open fifo");

        pthread_mutex_unlock(&log_mutex);

        return;
    }

    write(fd,
          msg,
          strlen(msg)+1);

    close(fd);

    //--------------------------------
    // UNLOCK FIFO
    //--------------------------------

    pthread_mutex_unlock(&log_mutex);
}

void write_log_n(const char *msg, int num) 
{

    int fd = open(FIFO_NAME, O_WRONLY);
    
    if (fd != -1) {
        char buffer[256];

        int len = snprintf(buffer, sizeof(buffer), "%s: %d\n", msg, num);
        
        // Ghi vào FIFO
        write(fd, buffer, len);
        
        close(fd);
    } else {
        perror("Không thể mở FIFO");
    }
}