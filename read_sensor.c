#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <time.h>

#define ADS1115_ADDR 0x48

#define CONFIG_REG 0x01
#define CONVERSION_REG 0x00

//--------------------------------
// ADS1115 CONFIG
//--------------------------------

#define OS_SINGLE       (1 << 15)

#define MUX_AIN0_GND    (4 << 12)
#define MUX_AIN1_GND    (5 << 12)

#define PGA_4_096V      (1 << 9)

#define MODE_SINGLE     (1 << 8)

#define DR_860SPS       (7 << 5)

#define COMP_DISABLE    (3)

static int i2c_fd;

//--------------------------------
// WRITE CONFIG
//--------------------------------

void ads1115_write_config(uint16_t config)
{
    uint8_t buffer[3];

    buffer[0] = CONFIG_REG;

    buffer[1] = (config >> 8) & 0xFF;

    buffer[2] = config & 0xFF;

    write(i2c_fd, buffer, 3);
}

//--------------------------------
// READ CONVERSION
//--------------------------------

int16_t ads1115_read_conversion()
{
    uint8_t reg = CONVERSION_REG;

    write(i2c_fd, &reg, 1);

    uint8_t data[2];

    read(i2c_fd, data, 2);

    return (data[0] << 8) | data[1];
}

//--------------------------------
// READ CHANNEL
//--------------------------------

double ads1115_read_channel(uint16_t mux)
{
    uint16_t config =
        OS_SINGLE |
        mux |
        PGA_4_096V |
        MODE_SINGLE |
        DR_860SPS |
        COMP_DISABLE;

    ads1115_write_config(config);

    //--------------------------------
    // WAIT CONVERSION
    //--------------------------------

    struct timespec ts =
    {
        .tv_sec = 0,
        .tv_nsec = 1500000
    };

    nanosleep(&ts, NULL);

    int16_t raw =
        ads1115_read_conversion();

    //--------------------------------
    // CONVERT TO VOLTAGE
    //--------------------------------

    double voltage =
        raw * 4.096 / 32768.0;

    return voltage;
}

//--------------------------------
// MAIN
//--------------------------------

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        printf("Usage: %s <server_ip> <port>\n",
               argv[0]);

        return 1;
    }

    //--------------------------------
    // OPEN I2C
    //--------------------------------

    i2c_fd =
        open("/dev/i2c-1", O_RDWR);

    if(i2c_fd < 0)
    {
        perror("open i2c");

        return 1;
    }

    //--------------------------------
    // SET SLAVE ADDRESS
    //--------------------------------

    if(ioctl(i2c_fd,
             I2C_SLAVE,
             ADS1115_ADDR) < 0)
    {
        perror("ioctl");

        return 1;
    }

    //--------------------------------
    // TCP SOCKET
    //--------------------------------

    int sock =
        socket(AF_INET,
               SOCK_STREAM,
               0);

    if(sock < 0)
    {
        perror("socket");

        return 1;
    }

    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;

    server_addr.sin_port =
        htons(atoi(argv[2]));

    inet_pton(AF_INET,
              argv[1],
              &server_addr.sin_addr);

    //--------------------------------
    // CONNECT
    //--------------------------------

    if(connect(sock,
               (struct sockaddr*)&server_addr,
               sizeof(server_addr)) < 0)
    {
        perror("connect");

        return 1;
    }

    printf("Connected to gateway\n");

    //--------------------------------
    // ACQUISITION LOOP
    //--------------------------------

    while(1)
    {
        //--------------------------------
        // ECG = AIN0
        //--------------------------------

        double ecg =
            ads1115_read_channel(MUX_AIN0_GND);

        //--------------------------------
        // PPG = AIN1
        //--------------------------------

        double ppg =
            ads1115_read_channel(MUX_AIN1_GND);

        //--------------------------------
        // SEND PACKET
        //--------------------------------

        char buffer[128];

        snprintf(buffer,
                 sizeof(buffer),
                 "%.6f %.6f\n",
                 ecg,
                 ppg);

        send(sock,
             buffer,
             strlen(buffer),
             0);
    }

    close(sock);

    close(i2c_fd);

    return 0;
}