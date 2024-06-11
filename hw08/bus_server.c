// 2020115532 컴퓨터학부 정서현
// 네트워크 프로그래밍 과제 #08
// 멀티 쓰레드를 이용한 버스 예약 시스템 구현

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <semaphore.h>

#define ROWS 2
#define COLS 10

#define SUCCESS 0 // 성공
#define WRONG -1 // 좌석번호 잘못 입력
#define RESERVE -2 // 이미 예약 된 좌석, 실패
#define NOTRESERVE -3 // 예약된 좌석은 아닌데, 실패
#define NOTOWNER -4 // 예약된 좌석이지만, 예약자가 아니어서 실패

typedef struct {
    int command;
    int seatno;
} REQ_PACKET;

typedef struct {
    int command;
    int seatno;
    int seats[ROWS][COLS];
    int result;
} RES_PACKET;

int connected_clients = 0;
int seats[ROWS][COLS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t sem1, sem2;

void initialize_seats() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            seats[i][j] = 0;
        }
    }
}

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);
    REQ_PACKET req;
    RES_PACKET res;

    while (1) {
        if (recv(client_sock, &req, sizeof(req), 0) <= 0) {
            pthread_mutex_lock(&mutex);
            connected_clients--;
            printf("Client removed: clnt_sock=%d, i=%d\n", client_sock, connected_clients);
            pthread_mutex_unlock(&mutex);
            close(client_sock);
            pthread_exit(NULL);
        }

        pthread_mutex_lock(&mutex);

        res.command = req.command;
        res.seatno = req.seatno;
        memcpy(res.seats, seats, sizeof(seats));

        if (req.command == 1) { // 예약 현황 조회
            res.result = SUCCESS;
        } 
        else if (req.command == 2) { // 예약
            if (req.seatno < 1 || req.seatno > ROWS * COLS) {
                res.result = WRONG;
            } else {
                int row = (req.seatno - 1) / COLS;
                int col = (req.seatno - 1) % COLS;
                if (seats[row][col] == 0) {
                    seats[row][col] = client_sock;
                    res.result = SUCCESS;
                } else {
                    res.result = RESERVE;
                }
                memcpy(res.seats, seats, sizeof(seats)); // 예약 후 갱신된 좌석 정보
            }
        } 
        else if (req.command == 3) { // 예약 취소
            if (req.seatno < 1 || req.seatno > ROWS * COLS) {
                res.result = WRONG;
            } else {
                int row = (req.seatno - 1) / COLS;
                int col = (req.seatno - 1) % COLS;
                if (seats[row][col] == client_sock) {
                    seats[row][col] = 0;
                    res.result = SUCCESS;
                } else if (seats[row][col] == 0) {
                    res.result = NOTRESERVE;
                } else {
                    res.result = NOTOWNER;
                }
                memcpy(res.seats, seats, sizeof(seats)); // 예약 취소 후 갱신된 좌석 정보
            }
        } 
        else if (req.command == 4) { // 프로그램 종료
            res.result = SUCCESS;
            send(client_sock, &res, sizeof(res), 0);
            pthread_mutex_unlock(&mutex);
            pthread_mutex_lock(&mutex);
            connected_clients--;
            printf("Client removed: clnt_sock=%d, i=%d\n", client_sock, connected_clients);
            pthread_mutex_unlock(&mutex);
            close(client_sock);
            pthread_exit(NULL);
        }

        pthread_mutex_unlock(&mutex);

        send(client_sock, &res, sizeof(res), 0);
    }
}

int main(int argc, char *argv[]) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    pthread_t tid;

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    initialize_seats();

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 10);

    printf("-----------------------------------------\n");
    printf("         Bus Reservation System\n");
    printf("-----------------------------------------\n");

    sem_init(&sem1, 0, 0);
    sem_init(&sem2, 0, 1);

    while (1) {
        client_addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_size);

        if (client_sock == -1) {
            printf("accept() error\n");
            break;
        }

        pthread_mutex_lock(&mutex);
        connected_clients++;
        printf("Connected client IP: %s, clnt_sock=%d\n", inet_ntoa(client_addr.sin_addr), client_sock);
        pthread_mutex_unlock(&mutex);

        int *client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_sock;
        pthread_create(&tid, NULL, handle_client, (void *)client_sock_ptr);
        pthread_detach(tid);
    }

    sem_destroy(&sem1);
    sem_destroy(&sem2);
    close(server_sock);

    return 0;
}
