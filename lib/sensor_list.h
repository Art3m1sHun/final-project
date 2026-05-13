
#ifndef SENSOR_LIST_H
#define SENSOR_LIST_H

#include <pthread.h>
#include <time.h>

extern struct timespec start_time;

extern int acquisition_started;

typedef struct sensor_node
{
    char timestamp[32];

    double ecg;
    double ppg;

    struct sensor_node *next;

} sensor_node_t;
typedef struct
{
    sensor_node_t *head;
    sensor_node_t *tail;

    pthread_mutex_t mutex;

    pthread_cond_t cond;

} sensor_list_t;

void sensor_list_init(sensor_list_t *list);

void sensor_list_insert(sensor_list_t *list,
                        double ecg,
                        double ppg);

void sensor_list_print(sensor_list_t *list);

int sensor_list_pop(sensor_list_t *list,
                    double *ecg,
                    double *ppg,
                    char *timestamp);
#endif