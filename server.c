#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <netinet/in.h>
#include "hserver.h"

// to use gettid
#ifndef SYS_gettid
#error "SYS_gettid unavailable on this system"
#endif
#define gettid() ((pid_t)syscall(SYS_gettid))

#define MAX_TASKS 2
#define MAX_THREADS 2

int front = 0, rear = 0, connection_queue[MAX_TASKS];
pthread_mutex_t mutex_queue;
pthread_cond_t cond_master, cond_worker;

void *worker_start(void *);
void io_handler(int);

int main(int argc, char *argv[])
{
    int n, sockfd, newsockfd, portno;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    pthread_t thread_pool[MAX_THREADS];
    pthread_mutex_init(&mutex_queue, NULL);
    pthread_cond_init(&cond_master, NULL);
    pthread_cond_init(&cond_worker, NULL);

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided");
        exit(EXIT_FAILURE);
    }

    /* create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "ERROR, opening socket");
        exit(EXIT_FAILURE);
    }

    /* fill in port number to listen on. IP address can be anything (INADDR_ANY) */
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* bind socket to this port number on this machine */
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "ERROR, on binding");
        exit(EXIT_FAILURE);
    }

    /* create the thread pool */
    for (int i = 0; i < MAX_THREADS; i++)
    {
        pthread_create(&thread_pool[i], NULL, worker_start, NULL);
    }

    /* listen for incoming connection requests */
    listen(sockfd, 200);

    while (1)
    {
        bzero((char *)&cli_addr, sizeof(cli_addr));

        /* accept a new request, create a newsockfd */

        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            perror("ERROR, accepting new connection");
            continue;
        }

        pthread_mutex_lock(&mutex_queue);

        /* store the socketfd in the queue */
        if ((rear + 1) % MAX_TASKS == front)
        {
            // TODO: Bug - Master is not sleeping
            /* Queue Full Master Sleeps*/
            printf("\n\nDrop Connection\n\n");
            close(newsockfd);
            // pthread_cond_wait(&cond_master, &mutex_queue);
        }
        else
        {
            rear = (rear + 1) % MAX_TASKS;
            connection_queue[rear] = newsockfd;
            newsockfd = 0;
        }

        pthread_mutex_unlock(&mutex_queue);
        pthread_cond_signal(&cond_worker);
    }

    return 0;
}

void *worker_start(void *args)
{
    int socket;

    while (1)
    {
        /* lock before accessing connection_count & connection_queue */
        pthread_mutex_lock(&mutex_queue);
        while (front == rear)
        {
            /* worker sleeps if no job available */
            printf("Worker (%d) Sleeps\n", gettid());
            pthread_cond_wait(&cond_worker, &mutex_queue);
        }

        /* get a socketfd from the queue */
        front = (front + 1) % MAX_TASKS;
        socket = connection_queue[front];
        connection_queue[front] = 0;

        pthread_mutex_unlock(&mutex_queue);
        // pthread_cond_signal(&cond_master);

        io_handler(socket);
    }
}

void io_handler(int socket)
{
    printf("Thread: %d\n", gettid());
    char request_str[1024];
    read(socket, request_str, 1023);

    HTTP_Response *response = handle_request(request_str);

    char *response_str = get_string(response);

    write(socket, response_str, strlen(response_str));

    if (response_str[9] == '2' && response_str[10] == '0' && response_str[11] == '0')
        sleep(5); // blocking for valid requests

    cleanup(response);
    free(response_str);
}
