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
                        const char *type,
                        int sensor_id,
                        double value)
{
    pthread_mutex_lock(&list->mutex);

    sensor_node_t *node =
        malloc(sizeof(sensor_node_t));

    if(node == NULL)
    {
        pthread_mutex_unlock(&list->mutex);

        return;
    }

    //--------------------------------
    // COPY DATA
    //--------------------------------

    strcpy(node->sensor_type,
           type);

    node->sensor_id = sensor_id;

    node->value = value;

    //--------------------------------
    // INSERT HEAD
    //--------------------------------

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
        printf("Type: %s | ID: %d | Value: %.2f\n",
               curr->sensor_type,
               curr->sensor_id,
               curr->value);

        curr = curr->next;
    }

    printf("=======================\n");

    pthread_mutex_unlock(&list->mutex);
}

int sensor_list_pop(sensor_list_t *list,
                    char *type,
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

    //--------------------------------
    // COPY DATA OUT
    //--------------------------------

    strcpy(type,
           temp->sensor_type);

    *sensor_id =
        temp->sensor_id;

    *value =
        temp->value;

    //--------------------------------
    // REMOVE HEAD
    //--------------------------------

    list->head =
        temp->next;

    free(temp);

    pthread_mutex_unlock(&list->mutex);

    return 1;
}