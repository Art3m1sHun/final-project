#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sensor_list.h"

void sensor_list_init(sensor_list_t *list)
{
    list->head = NULL;

    pthread_mutex_init(&list->mutex, NULL);

    pthread_cond_init(&list->cond, NULL);
}

void sensor_list_insert(sensor_list_t *list,
                        double ecg,
                        double ppg)
{
    pthread_mutex_lock(&list->mutex);

    sensor_node_t *node =
        malloc(sizeof(sensor_node_t));

    if(node == NULL)
    {
        pthread_mutex_unlock(&list->mutex);
        return;
    }

    node->ecg = ecg;
    node->ppg = ppg;

    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC_RAW,
                  &now);

    long sec =
        now.tv_sec - start_time.tv_sec;

    long nsec =
        now.tv_nsec - start_time.tv_nsec;

    if(nsec < 0)
    {
        sec--;
        nsec += 1000000000L;
    }

    sprintf(node->timestamp,
            "%ld.%09ld",
            sec,
            nsec);

    node->next = list->head;

    list->head = node;

    pthread_cond_signal(&list->cond);

    pthread_mutex_unlock(&list->mutex);
}

void sensor_list_print(sensor_list_t *list)
{
    pthread_mutex_lock(&list->mutex);

    printf("\n===== SHARED LIST =====\n");

    sensor_node_t *curr = list->head;

    while(curr)
    {
        printf("TIME: %s ECG: %.2f PPG: %.2f\n",
            curr->timestamp,
            curr->ecg,
            curr->ppg);

        curr = curr->next;
    }

    printf("=======================\n");

    pthread_mutex_unlock(&list->mutex);
}

int sensor_list_pop(sensor_list_t *list, double *ecg, double *ppg, char *timestamp)
{
    pthread_mutex_lock(&list->mutex);

    if(list->head == NULL)
    {
        pthread_mutex_unlock(&list->mutex);
        return 0;
    }

    sensor_node_t *node = list->head;

    list->head = node->next;

    *ecg = node->ecg;
    *ppg = node->ppg;

    strcpy(timestamp,
           node->timestamp);

    free(node);

    pthread_mutex_unlock(&list->mutex);

    return 1;
}