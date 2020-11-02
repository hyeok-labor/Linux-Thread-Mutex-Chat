#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_CLNT 100
#define BUF_SIZE 1024

void *handle_clnt(void *arg);
void send_msg(int sock, char *msg, int len);

int clnt_socks[MAX_CLNT]; // 100
int clnt_cnt = 0;         // count

pthread_mutex_t mtex; // thread protect

int main(int argc, char *argv[])
{
    int serv_sock;
    int clnt_sock;
    struct sockaddr_in serv_addr = {
        0,
    };
    struct sockaddr_in clnt_addr = {
        0,
    };
    socklen_t clnt_size = sizeof(clnt_addr);

    pthread_t t_id; // multi client, thread id
    if (argc != 2)
    {
        printf("%s <port>\n", argv[0]);
        exit(1);
    }

    // init mutex
    pthread_mutex_init(&mtex, NULL);

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    listen(serv_sock, 5);

    while (1)
    {
        // accept
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_size);

        // lock before use clnt_sock and unlock after use *
        pthread_mutex_lock(&mtex);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mtex);
        // execute handle_clnt
        pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
        //thread end and memory free
        pthread_detach(t_id);
        printf("clnt:%s\n", inet_ntoa(clnt_addr.sin_addr));
    }
    // mutex memory free
    pthread_mutex_destroy(&mtex);
    return 0;
}

void *handle_clnt(void *arg)
{
    int clnt_sock = *((int *)arg);
    int str_len = 0;
    char msg[BUF_SIZE];

    while (0 != (str_len = read(clnt_sock, msg, sizeof(msg))))
    {
        send_msg(clnt_sock, msg, str_len); // semd msg function
    }

    // read == 0 ==> clnt_sock close
    // 1.clnt_socks[] delete
    pthread_mutex_lock(&mtex);
    for (int i = 0; i < clnt_cnt; ++i)
    {
        if (clnt_sock == clnt_socks[i])
        {
            while (i < clnt_cnt - 1)
            {
                clnt_socks[i] = clnt_socks[i + 1];

                ++i;
            }
            break;
        }
    }
    --clnt_cnt;
    pthread_mutex_lock(&mtex);
    close(clnt_sock);
    return NULL;
}

void send_msg(int sock, char *msg, int len)
{
    pthread_mutex_lock(&mtex);
    for (int i = 0; i < clnt_cnt; ++i)
    {
        write(clnt_socks[i], msg, len);
    }
    pthread_mutex_unlock(&mtex);
}