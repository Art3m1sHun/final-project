#ifndef LOG_H
#define LOG_H

#define FIFO_NAME "./FIFO-log"

void write_log(const char *msg);

void write_log_format(const char *fmt, ...);

#endif