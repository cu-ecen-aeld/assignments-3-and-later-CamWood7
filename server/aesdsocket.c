#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include "queue.h"

//Declare global variables

#define BUFFER_SIZE 4096
#define PORT "9000"
#define MAX_CONNECTIONS 10
#define FILE_PATH "/var/tmp/aesdsocketdata"
static bool sigint_received = false;
static bool sigterm_received = false;
static pthread_mutex_t file_mutex;
static pthread_t timer_thread;

typedef struct threadParams_t {
    int conn_fd;
    bool thread_complete;
} threadParams_t;

SLIST_HEAD(slisthead, slist_data_s);
typedef struct slist_data_s slist_data_t;
struct slist_data_s {
    struct threadParams_t *thread_info;
    pthread_t thread_id;
    SLIST_ENTRY(slist_data_s) entries;
};

// Signal handler
static void signal_handler(int signal_number) {
    int errno_saved = errno;
    if (signal_number == SIGINT) {
        sigint_received = true;
        syslog(LOG_DEBUG, "Caught SIGINT, exiting");
    } else if (signal_number == SIGTERM) {
        sigterm_received = true;
        syslog(LOG_DEBUG, "Caught SIGTERM, exiting");
    }
    errno = errno_saved;
}

void *thread_proc(void *thread_params) {
    struct threadParams_t *thread_data = (struct threadParams_t *) thread_params;
    int conn_fd = thread_data->conn_fd;
    thread_data->thread_complete = false;

    ssize_t recv_ret;
    char buff[BUFFER_SIZE] = {0};

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    getpeername(conn_fd, (struct sockaddr *)&addr, &addr_len);
    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, addr_str, INET_ADDRSTRLEN);
    syslog(LOG_DEBUG, "Accepted connection from %s", addr_str);

    bool newline_found = false;

    pthread_mutex_lock(&file_mutex);
    syslog(LOG_DEBUG, "Mutex lock from %d", conn_fd);

    FILE *fp = fopen(FILE_PATH, "a+");

    while (!newline_found) {
        recv_ret = recv(conn_fd, buff, BUFFER_SIZE - 1, 0);

        if (recv_ret < 0) {
            fprintf(stderr, "Error in recv()\n");
            break;
        }

        if (recv_ret == 0) {
            break;
        }

        buff[recv_ret] = '\0';
        fprintf(fp, "%s", buff);

        if (strstr(buff, "\n")) {
            newline_found = true;
        }

        memset(buff, 0, sizeof(buff));
    }

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    rewind(fp);
    char file_text[filesize];
    fread(file_text, 1, filesize, fp);

    send(conn_fd, file_text, filesize, 0);
    fclose(fp);

    pthread_mutex_unlock(&file_mutex);
    syslog(LOG_DEBUG, "Mutex unlock from %d", conn_fd);
    syslog(LOG_DEBUG, "Closed connection from %s", addr_str);

    close(conn_fd);
    thread_data->thread_complete = true;
    return NULL;
}

void *run_timer() {
    time_t prev_time = time(NULL);
    time_t curr_time = prev_time;
    bool stop_timer = false;

    while (!stop_timer) {
        if (sigint_received || sigterm_received) {
            stop_timer = true;
            break;
        }

        curr_time = time(NULL);

        if (difftime(curr_time, prev_time) < 10.0) {
            continue;
        }

        prev_time = curr_time;

        char outstr[200];
        time_t t = time(NULL);
        struct tm *tmp = localtime(&t);
        if (tmp == NULL) {
            perror("localtime");
            exit(EXIT_FAILURE);
        }
        int num_chars = strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T %z", tmp);
        if (num_chars == 0) {
            fprintf(stderr, "strftime returned 0");
            exit(EXIT_FAILURE);
        }

        outstr[num_chars] = '\n';
        char prefix[] = "timestamp:";

        pthread_mutex_lock(&file_mutex);
        int fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0666);
        write(fd, prefix, sizeof(prefix) - 1);
        write(fd, outstr, num_chars + 1);
        close(fd);
        pthread_mutex_unlock(&file_mutex);
    }
    return NULL;
}

int main(int argc, char **argv) {
    // Initialization
    openlog(NULL, 0, LOG_USER);
    pthread_mutex_init(&file_mutex, NULL);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *server_info;

    if (getaddrinfo(NULL, PORT, &hints, &server_info) != 0) {
        syslog(LOG_ERR, "getaddrinfo error");
        return -1;
    }

    int sockfd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (sockfd == -1) {
        syslog(LOG_ERR, "socket error: %s", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }

    int option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));

    if (bind(sockfd, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        syslog(LOG_ERR, "bind error: %s", strerror(errno));
        close(sockfd);
        freeaddrinfo(server_info);
        return -1;
    }

    freeaddrinfo(server_info);

    if (listen(sockfd, MAX_CONNECTIONS) == -1) {
        syslog(LOG_ERR, "listen error: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        pid_t pid = fork();
        if (pid != 0) {
            exit(0);
        }
        setsid();
        chdir("/");
        fclose(stdin);
        fclose(stdout);
        fclose(stderr);
    }

    remove(FILE_PATH);

    pthread_create(&timer_thread, NULL, run_timer, NULL);

    slist_data_t *thread_entry = NULL;
    slist_data_t *thread_entry_temp = NULL;

    struct slisthead head;
    SLIST_INIT(&head);

    socklen_t addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in client_addr;

    while (!sigint_received && !sigterm_received) {
        int conn_fd = accept(sockfd, (struct sockaddr*)&client_addr, &addr_len);
        if (conn_fd < 0) {
            if (errno == EINTR) {
                break;
            }
            syslog(LOG_ERR, "accept error: %s", strerror(errno));
            continue;
        }

        threadParams_t *thread_data = (threadParams_t*)malloc(sizeof(threadParams_t));
        thread_data->conn_fd = conn_fd;
        thread_data->thread_complete = false;

        pthread_t new_thread;
        pthread_create(&new_thread, NULL, thread_proc, thread_data);

        thread_entry = malloc(sizeof(slist_data_t));
        thread_entry->thread_id = new_thread;
        thread_entry->thread_info = thread_data;
        SLIST_INSERT_HEAD(&head, thread_entry, entries);

        SLIST_FOREACH_SAFE(thread_entry, &head, entries, thread_entry_temp) {
            if (thread_entry->thread_info->thread_complete) {
                pthread_join(thread_entry->thread_id, NULL);
                SLIST_REMOVE(&head, thread_entry, slist_data_s, entries);
                free(thread_entry->thread_info);
                free(thread_entry);
            }
        }
    }

    close(sockfd);

    while (!SLIST_EMPTY(&head)) {
        thread_entry = SLIST_FIRST(&head);
        pthread_join(thread_entry->thread_id, NULL);
        SLIST_REMOVE_HEAD(&head, entries);
        free(thread_entry->thread_info);
        free(thread_entry);
    }

    pthread_join(timer_thread, NULL);
    pthread_mutex_destroy(&file_mutex);
    remove(FILE_PATH);
    closelog();

    return 0;
}

