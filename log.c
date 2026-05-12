#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "log.h"

void write_log(const char *msg)
{
    int fd = open(FIFO_NAME, O_WRONLY);

    write(fd, msg, strlen(msg) + 1);

    close(fd);
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