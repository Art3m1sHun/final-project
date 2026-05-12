#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "connection.h"


void* connection(void *args)
{
    connection_arg_t *conn =
        (connection_arg_t*) args;

    int server_fd =
        conn->server_fd;

    sensor_list_t *list =
        conn->shared_list;

    while(1)
    {
        printf("Waiting for sensor...\n");

        struct sockaddr_in client_addr;

        socklen_t client_len =
            sizeof(client_addr);

        int client_fd =
            accept(server_fd,
                   (struct sockaddr*)&client_addr,
                   &client_len);

        if(client_fd < 0)
        {
            perror("accept");
            continue;
        }

        printf("New sensor connected!\n");

        write_log("New sensor connected");

        char buffer[128];

        int n = recv(client_fd,
                     buffer,
                     sizeof(buffer)-1,
                     0);

        if(n > 0)
        {
            buffer[n] = '\0';

            printf("Received: %s\n", buffer);

            //--------------------------------
            // PARSE SENSOR PACKET
            //--------------------------------

            int sensor_id;

            double value;

            sscanf(buffer,
                   "%d %lf",
                   &sensor_id,
                   &value);

            //--------------------------------
            // INSERT SHARED LIST
            //--------------------------------

            sensor_list_insert(list,
                               sensor_id,
                               value);

            write_log("Sensor data inserted");
        }

        close(client_fd);
    }

    return NULL;
}