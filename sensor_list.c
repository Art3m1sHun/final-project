#include <stdlib.h>
#include "sensor_list.h"
#include <stdio.h>



void sensor_list_init(sensor_list_t *list)
{
    list->head = NULL;

    pthread_mutex_init(&list->mutex, NULL);
}

void sensor_list_insert(sensor_list_t *list,
                        int id,
                        double value)
{
    pthread_mutex_lock(&list->mutex);

    sensor_node_t *node =
        malloc(sizeof(sensor_node_t));

    node->sensor_id = id;
    node->value = value;

    node->next = list->head;

    list->head = node;

    pthread_mutex_unlock(&list->mutex);
}

void sensor_list_print(sensor_list_t *list)
{
    pthread_mutex_lock(&list->mutex);

    printf("\n===== SHARED LIST =====\n");

    sensor_node_t *curr = list->head;

    while(curr)
    {
        printf("Sensor ID: %d | Value: %.2f\n",
               curr->sensor_id,
               curr->value);

        curr = curr->next;
    }

    printf("=======================\n");

    pthread_mutex_unlock(&list->mutex);
}

int sensor_list_pop(sensor_list_t *list,
                    int *sensor_id,
                    double *value)
{
    pthread_mutex_lock(&list->mutex);

    if(list->head == NULL)
    {
        pthread_mutex_unlock(&list->mutex);

        return 0;
    }

    sensor_node_t *temp =
        list->head;

    *sensor_id = temp->sensor_id;

    *value = temp->value;

    list->head = temp->next;

    free(temp);

    pthread_mutex_unlock(&list->mutex);

    return 1;
}