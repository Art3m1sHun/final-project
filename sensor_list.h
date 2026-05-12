#ifndef SENSOR_LIST_H
#define SENSOR_LIST_H

#include <pthread.h>

// 1. Define the structures FIRST
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

// 2. Now define the function prototypes that use those types
// Note: I added the parameters to match how you used them in main.c
void sensor_list_init(sensor_list_t *list);
void sensor_list_insert(sensor_list_t *list, int sensor_id, double value);
void sensor_list_print(sensor_list_t *list);
int sensor_list_pop(sensor_list_t *list, int *sensor_id, double *value);

#endif