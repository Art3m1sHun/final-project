#define _POSIX_C_SOURCE 200809L
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
#include <sys/wait.h>

#include "log.h"
#include "sensor_list.h"
#include "connection.h"
#include "database.h"

#include <errno.h>


pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t thread_ecg, thread_ppg;

sensor_list_t *global_list;
sensor_list_t shared_list;
sensor_list_t filtered_list;    // data sau khi filter

struct timespec start_time;
volatile int running = 1;
int global_server_fd;
volatile int logger_running = 1;
int acquisition_started = 0;

double filter_ecg(double x)
{
    static double y = 0;

    y = 0.95 * y + 0.05 * x;

    return y;
}

double filter_ppg(double x)
{
    static double y = 0;

    y = 0.90 * y + 0.10 * x;

    return y;
}

void logger_sigterm(int sig)
{
    logger_running = 0;
}

void handle_sigint(int sig)
{
    running = 0;
    shutdown(global_server_fd, SHUT_RDWR);
    close(global_server_fd);

    pthread_mutex_lock(&shared_list.mutex);
    pthread_cond_broadcast(&shared_list.cond);
    pthread_mutex_unlock(&shared_list.mutex);

    pthread_mutex_lock(&filtered_list.mutex);
    pthread_cond_broadcast(&filtered_list.cond);
    pthread_mutex_unlock(&filtered_list.mutex);
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

        double ecg;
        double ppg;

        if(sscanf(buffer, "%lf %lf", &ecg, &ppg) == 2)
        {
            sensor_list_insert(list, ecg, ppg);

            printf("ECG: %.2f PPG: %.2f\n", ecg, ppg);
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
        if(client_arg == NULL)
        {
            perror("malloc");
            close(client_fd);
            continue;
        }

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

static void *data(void *args)
{
    sensor_list_t *raw    = ((sensor_list_t**)args)[0];
    sensor_list_t *filtered = ((sensor_list_t**)args)[1];

    double ecg, ppg;
    char timestamp[32];

    while(running)
    {
        pthread_mutex_lock(&raw->mutex);
        while(raw->head == NULL && running)
            pthread_cond_wait(&raw->cond, &raw->mutex);
        pthread_mutex_unlock(&raw->mutex);

        if(!running) break;

        if(sensor_list_pop(raw, &ecg, &ppg, timestamp))
        {
            double ecg_f = filter_ecg(ecg);
            double ppg_f = filter_ppg(ppg);

            printf("Filtered → ECG: %.2f PPG: %.2f\n", ecg_f, ppg_f);

            sensor_list_insert(filtered, ecg_f, ppg_f);
        }
    }
    return NULL;
}

static void *storage(void *args)
{
    sensor_list_t *list =
        (sensor_list_t*) args;

    int retry = 0;
    double ecg;
    double ppg;
    char timestamp[32];

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
        pthread_mutex_lock(&list->mutex);

        while(list->head == NULL && running)
        {
            pthread_cond_wait(&list->cond, &list->mutex);
        }

        if(!running) {
            pthread_mutex_unlock(&list->mutex);  // ← THÊM DÒNG NÀY
            break;
        }

        pthread_mutex_unlock(&list->mutex);

        if(sensor_list_pop(list, &ecg, &ppg, timestamp))
        {
            db_insert(db, timestamp, ecg, ppg);
        }
    }

    sqlite3_close(db);

    db_export_session();

    remove("current_session.db");
    write_log("Exporting session database");
    write_log("Database reset complete");

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

    global_list = &shared_list;
    sensor_list_init(&shared_list);
    sensor_list_init(&filtered_list);
    clock_gettime(CLOCK_MONOTONIC, &start_time);




    
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

    int opt = 1;

    setsockopt(server_fd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt,
            sizeof(opt));
    global_server_fd = server_fd;

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
        if(pid_log == 0)
            {
                signal(SIGTERM, logger_sigterm);
                signal(SIGINT, SIG_IGN);
                
                FILE *logfile =
                    fopen("gateway.log", "a");

                if(logfile == NULL)
                {
                    perror("fopen");
                    exit(1);
                }

                int fd =
                    open(FIFO_NAME, O_RDONLY);

                if(fd < 0)
                {
                    perror("open fifo");
                    exit(1);
                }

                char buffer[512];

                int sequence = 1;

                while(logger_running)
                {
                    int n =
                        read(fd,
                            buffer,
                            sizeof(buffer)-1);

                    if(n > 0)
                    {
                        buffer[n] = '\0';

                        struct timespec ts;

                        clock_gettime(CLOCK_REALTIME,
                                    &ts);

                        fprintf(logfile,
                                "%d %ld.%09ld %s\n",
                                sequence,
                                ts.tv_sec,
                                ts.tv_nsec,
                                buffer);

                        fflush(logfile);

                        sequence++;
                    }
                    else if(n == 0)
                    {
                        sleep(1);
                    }
                }

                fclose(logfile);

                close(fd);

                exit(0);
            }else{ // tiến trinh cha bao gom connect, data, storage
            signal(SIGINT, handle_sigint);
            signal(SIGTERM, SIG_DFL);

            connection_arg_t conn_arg;
            conn_arg.server_fd = server_fd;

            conn_arg.server_fd = server_fd;

            conn_arg.shared_list = &shared_list;
            ret = pthread_create(&connect_thread, NULL, connection, &conn_arg);
            if(ret != 0){
                write_log_format("pthread_create(connect) error: %d", ret);
                return -1;
            }
            sensor_list_t *data_args[2] = { &shared_list, &filtered_list };
            ret = pthread_create(&data_thread, NULL, data, data_args);
            if(ret != 0){
                write_log_format("pthread_create(connect) error: %d", ret);
                return -1;          
            }
            
            ret = pthread_create(&storage_thread, NULL, storage, &filtered_list);
            if(ret != 0){
                write_log_format("pthread_create(connect) error: %d", ret);
                return -1;          
            }

            pthread_join(connect_thread, NULL);
            pthread_join(data_thread, NULL);
            pthread_join(storage_thread, NULL);

            printf("AFTER JOIN\n");
            fflush(stdout);

            printf("Graceful shutdown...\n");

            write_log("Gateway shutting down");
            sleep(5);
            kill(pid_log, SIGTERM);
            waitpid(pid_log, NULL, 0);

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