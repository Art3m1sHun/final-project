#ifndef CONNECTION_H
#define CONNECTION_H

#include "sensor_list.h"
#include "log.h"

typedef struct
{
    int server_fd;

    sensor_list_t *shared_list;

} connection_arg_t;

typedef struct
{
    int client_fd;

    sensor_list_t *list;

} sensor_client_arg_t;

void* connection(void *args);

#endif