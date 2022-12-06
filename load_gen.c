/* run using: ./load_gen localhost <server port> <number of concurrent users>
   <think time (in s)> <test duration (in s)> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <limits.h>
#include <sys/time.h>

#define HTTP_REQUEST_TEMPLATE "GET / HTTP/1.0"

void *user_function(void *);
double time_diff(struct timeval *, struct timeval *);
void error(char *);

int time_up, portno;
FILE *log_file;
char *urls[64], *hostname;
double think_time;

// user info struct
struct user_info
{
  // user id
  int id;

  // user metrics
  int total_count;
  double total_rtt;
};

int main(int argc, char *argv[])
{
  int user_count, test_duration;
  char *urls[64];

  if (argc != 6)
  {
    fprintf(stderr,
            "Usage: %s <hostname> <server port> <number of concurrent users> "
            "<think time (in s)> <test duration (in s)>\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  hostname = argv[1];
  portno = atoi(argv[2]);
  user_count = atoi(argv[3]);
  think_time = atof(argv[4]);
  test_duration = atoi(argv[5]);

  printf("Hostname: %s\n", hostname);
  printf("Port: %d\n", portno);
  printf("User Count: %d\n", user_count);
  printf("Think Time: %f s\n", think_time);
  printf("Test Duration: %d s\n\n", test_duration);

  /* open log file */
  log_file = fopen("load_gen.log", "w");

  pthread_t threads[user_count];
  struct user_info info[user_count];
  struct timeval start, end;

  /* start timer */
  gettimeofday(&start, NULL);
  time_up = 0;

  for (int i = 0; i < user_count; ++i)
  {
    /* initialize user info */
    info[i].id = i;
    info[i].total_count = 0;
    info[i].total_rtt = 0;

    /* create user thread */
    if (pthread_create(&threads[i], NULL, user_function, (void *)&info[i]) != 0)
    {
      perror("Thread: ");
    }

    fprintf(log_file, "Created thread %d\n", i);
  }

  /* wait for test duration */
  sleep(test_duration);

  fprintf(log_file, "Woke up\n");

  /* end timer */
  time_up = 1;
  gettimeofday(&end, NULL);

  int total_requests = 0;
  double total_rtt = 0;
  /* wait for all threads to finish */
  for (int i = 0; i < user_count; i++)
  {
    pthread_join(threads[i], NULL);
    total_requests += info[i].total_count;
    total_rtt += info[i].total_rtt;
  }

  /* print results */
  double avg_rt = total_rtt * 1000 / total_requests; // ms/req
  int throughput = total_requests / test_duration;

  printf("Total Requests: %d\nThroughput: %d reqs/s\nAvg Response Time: %f ms/req\n\n", total_requests, throughput, avg_rt);

  FILE *data = fopen("data", "a");
  char temp[32];
  bzero(temp, 32);
  sprintf(temp, "%d %d %f\n", user_count, throughput, avg_rt);
  fprintf(data, temp, strlen(temp));

  /* close log file */
  fclose(log_file);
  fclose(data);

  return 0;
}

// user thread function
void *user_function(void *arg)
{
  /* get user info */
  struct user_info *info = (struct user_info *)arg;

  int sockfd, n;
  char buffer[256], http_request[256];
  struct timeval start, end;

  srand((unsigned)time(0));

  struct sockaddr_in serv_addr;
  struct hostent *server = gethostbyname(hostname);
  if (server == NULL)
  {
    fprintf(stderr, "ERROR, no such host\n");
    exit(0);
  }

  /* keep sending requests till the  */
  while (!time_up)
  {
    /* start timer */
    gettimeofday(&start, NULL);

    /* create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
      error("ERROR, opening socket\n");
      continue;
    }

    /* set server attrs */
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    /* connect to server */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
      perror("ERROR, can't connect");
      close(sockfd);
      pthread_exit(NULL);
    }

    /* send message to server */
    bzero(http_request, 256);
    if (write(sockfd, HTTP_REQUEST_TEMPLATE, strlen(HTTP_REQUEST_TEMPLATE)) < 0)
    {
      perror("Can't write into the socket");
      pthread_exit(NULL);
    }

    /* read reply from server */
    do
    {
      n = read(sockfd, buffer, 256);
    } while (n > 0);

    if (n < 0)
    {
      perror("Can't read from the socket");
      pthread_exit(NULL);
    }

    /* close socket */
    close(sockfd);

    /* end timer */
    gettimeofday(&end, NULL);

    /* update user metrics */
    info->total_count++;
    info->total_rtt += time_diff(&end, &start);

    /* sleep for think time */
    usleep(think_time * 1e6);

    if (time_up)
      break;
  }

  /* exit thread */
  fprintf(log_file, "User #%d finished\n", info->id);
  fflush(log_file);
  pthread_exit(NULL);
}

// time diff in seconds
double time_diff(struct timeval *t2, struct timeval *t1)
{
  return (t2->tv_sec - t1->tv_sec) + (t2->tv_usec - t1->tv_usec) / 1e6;
}

// error handling function
void error(char *msg)
{
  perror(msg);
  exit(0);
}
