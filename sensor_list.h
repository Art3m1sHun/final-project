#ifndef SENSOR_LIST_H
#define SENSOR_LIST_H

#include <pthread.h>

typedef struct sensor_node
{
    char sensor_type[16];

    int sensor_id;

    double value;

    struct sensor_node *next;

} sensor_node_t;

typedef struct
{
    sensor_node_t *head;

    pthread_mutex_t mutex;

    pthread_cond_t cond;

} sensor_list_t;

void sensor_list_init(sensor_list_t *list);

void sensor_list_insert(sensor_list_t *list,
                        const char *type,
                        int sensor_id,
                        double value);

void sensor_list_print(sensor_list_t *list);

int sensor_list_pop(sensor_list_t *list,
                    char *type,
                    int *sensor_id,
                    double *value);

#endif