#ifndef LOG_H
#define LOG_H

#define FIFO_NAME "./FIFO-log"

void write_log_n(const char *msg, int num);
void write_log(const char *msg);

#endif