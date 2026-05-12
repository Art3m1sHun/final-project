
#ifndef SENSOR_LIST_H
#define SENSOR_LIST_H

void sensor_list_init();
void sensor_list_insert();
void sensor_list_print();

#include <pthread.h>

typedef struct sensor_node
{
    int sensor_id;
    double value;

    struct sensor_node *next;

} sensor_node_t;

typedef struct
{
    sensor_node_t *head;

    pthread_mutex_t mutex;

} sensor_list_t;

#endif