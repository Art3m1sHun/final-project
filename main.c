#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>

#include "log.h"
#include "sensor_list.h"
#include "connection.h"
#include "database.h"

#include <errno.h>

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t thread_ecg, thread_ppg;

volatile int running = 1;
int global_server_fd;

void handle_sigint(int sig)
{
    running = 0;

    close(global_server_fd);
}

static void *sensor_client(void *args)
{
    sensor_client_arg_t *client =
        (sensor_client_arg_t*) args;

    int client_fd =
        client->client_fd;

    sensor_list_t *list =
        client->list;

    free(client);

    char buffer[128];
    int sensor_id = -1;

    while(running)
    {
        int n = recv(client_fd,
                     buffer,
                     sizeof(buffer)-1,
                     0);

        if(n <= 0)
        {
            char disconnect_msg[64];
            if (sensor_id != -1) {
                // Nếu đã từng nhận được dữ liệu, ta biết ID là gì
                sprintf(disconnect_msg, "Sensor %d disconnected", sensor_id);
            } else {
                // Nếu chưa kịp gửi dữ liệu đã ngắt kết nối
                sprintf(disconnect_msg, "Unknown sensor disconnected (fd: %d)", client_fd);
            }

            printf("%s\n", disconnect_msg);
            write_log(disconnect_msg); // Ghi log có chứa ID
            break;
        }

        buffer[n] = '\0';

        char sensor_type[16];

        double value;

        if(sscanf(buffer, "%s %d %lf", sensor_type, &sensor_id, &value) == 3){
        sensor_list_insert(list, sensor_type, sensor_id,
                   value);

        sensor_list_print(list);

        printf("Received: %s\n", buffer);

        write_log(buffer);
        }
    }

    close(client_fd);

    return NULL;
}

void* connection(void *args)
{
    connection_arg_t *conn = (connection_arg_t*) args;

    int server_fd = conn->server_fd;

    sensor_list_t *shared_list = conn->shared_list;

    while(running)
    {
        printf("Waiting for sensor...\n");

        struct sockaddr_in client_addr;

        socklen_t client_len =
            sizeof(client_addr);

        int client_fd =
            accept(server_fd,
                   (struct sockaddr*)&client_addr,
                   &client_len);

        if(client_fd < 0){
            if(running)
            {
                perror("accept");
            }
            break;
        }

        printf("New sensor connected!\n");

        write_log("New sensor connected");

        //--------------------------------
        // CREATE CLIENT THREAD
        //--------------------------------

        pthread_t sensor_thread;

        sensor_client_arg_t *client_arg = malloc(sizeof(sensor_client_arg_t));

        client_arg->client_fd = client_fd;

        client_arg->list = shared_list;
        pthread_create(&sensor_thread,
                    NULL,
                    sensor_client,
                    client_arg);

        pthread_detach(sensor_thread);
    }

    return NULL;
}

static void *data (void *args)
{
    //sensor_list_t *list =(sensor_list_t*) args;

    //sensor_list_insert(list, 1, 25.5);

    write_log("data inserted");

    return NULL;
}

static void *storage(void *args)
{
    sensor_list_t *list =
        (sensor_list_t*) args;

    int retry = 0;

    sqlite3 *db = NULL;

    //--------------------------------
    // CONNECT DATABASE
    //--------------------------------

    while(retry < 3)
    {
        db = db_connect();

        if(db != NULL)
        {
            printf("Database connected\n");

            write_log("Database connected");

            break;
        }

        retry++;

        printf("DB connect failed. Retry %d\n",
               retry);

        write_log("DB connect failed");

        sleep(3);
    }

    //--------------------------------
    // FAIL AFTER 3 ATTEMPTS
    //--------------------------------

    if(db == NULL)
    {
        printf("Gateway shutting down\n");

        write_log("Gateway shutting down");

        exit(1);
    }

    //--------------------------------
    // STORAGE LOOP
    //--------------------------------

    while(running)
    {
        char sensor_type[16];

        int sensor_id;

        double value;

        int ok =
        sensor_list_pop(list, sensor_type, &sensor_id, &value);
        if(ok)
        {
            printf("Store DB: %d %.2f\n", sensor_id, value);

            db_insert(db, sensor_type, sensor_id, value);

            write_log("Data inserted to DB");
        }else{
            pthread_mutex_lock(&list->mutex);

            pthread_cond_wait(&list->cond, &list->mutex);

            pthread_mutex_unlock(&list->mutex);
        }
    }

    sqlite3_close(db);

    return NULL;
}


int main(int argc, char *argv[])
{
    if(mkfifo(FIFO_NAME, 0666) < 0)
    {
        if(errno != EEXIST)
        {
            perror("mkfifo");

            exit(1);
        }
    }


    pthread_t connect_thread;
    pthread_t data_thread;
    pthread_t storage_thread;
    
    sensor_list_t shared_list;
    sensor_list_init(&shared_list);


    
    int ret;

    /*thiet lap giao thuc tcp giao tiep*/

    if(argc != 2)
    {
        printf("Usage: %s <port>\n",
               argv[0]);

        return 1;
    }

    int port = atoi(argv[1]);

    //----------------------------------
    // CREATE SOCKET
    //----------------------------------

    int server_fd =
        socket(AF_INET,
               SOCK_STREAM,
               0);

    if(server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    //----------------------------------
    // SERVER ADDRESS
    //----------------------------------

    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;

    server_addr.sin_addr.s_addr =
        INADDR_ANY;

    server_addr.sin_port =
        htons(port);

    //----------------------------------
    // BIND
    //----------------------------------

    if(bind(server_fd,
            (struct sockaddr*)&server_addr,
            sizeof(server_addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    //----------------------------------
    // LISTEN
    //----------------------------------

    if(listen(server_fd, 5) < 0)
    {
        perror("listen");
        return 1;
    }
    
    printf("Server listening on port %d\n", port);

    pid_t pid_log = fork();
    sleep(1);

    if(pid_log >= 0){
        if(pid_log == 0){ // tiến trình ghi log
            signal(SIGINT, handle_sigint);
            FILE *logfile = fopen("gateway.log", "a");
            if(logfile == NULL)
            {
                perror("fopen");
                exit(1);
            }
            int fd = open(FIFO_NAME, O_RDONLY);
            char buffer[256];
            if(fd < 0)
            {
                perror("open fifo");
                exit(1);
            }
            int sequence = 1;
            
            while(running)
            {
                int n = read(fd, buffer, sizeof(buffer));
                
                

                if(n > 0)
                {
                    buffer[n] = '\0';
                    time_t now = time(NULL);

                    struct tm *t = localtime(&now);

                    char time_str[16];

                    strftime(time_str, sizeof(time_str), "%H:%M:%S", t);

                    fprintf(logfile, "%d %s %s\n", sequence, time_str, buffer);

                    fflush(logfile);

                    sequence++;
                }
            }
            fclose(logfile);
        }else{ // tiến trinh cha bao gom connect, data, storage
        
            connection_arg_t conn_arg;
            conn_arg.server_fd = server_fd;

            conn_arg.server_fd = server_fd;

            conn_arg.shared_list = &shared_list;

          if(ret = pthread_create(&connect_thread, NULL, connection, &conn_arg)){
            write_log_n("pthread_create(connect) error: %d\n", ret);
            return -1;
          }
          
          if(ret = pthread_create(&data_thread,NULL,data,&shared_list)){
            write_log_n("pthread_create(data) error: %d\n", ret);
            return -1;          
          }
          
          
          if(ret = pthread_create(&storage_thread,NULL,storage,&shared_list)){
            write_log_n("pthread_create(storage) error: %d\n", ret);
            return -1;          
          }

            pthread_join(connect_thread, NULL);
            pthread_join(data_thread, NULL);
            pthread_join(storage_thread, NULL);
            printf("Graceful shutdown...\n");

            write_log("Gateway shutting down");

            kill(pid_log, SIGTERM);

            unlink(FIFO_NAME);

            close(server_fd);

            sleep(1);
        }
    }else{
        perror("fork() log unsuccessfuly\n");
        exit(1);
    }

    return 0;
}