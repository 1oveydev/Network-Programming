// 2020115532 컴퓨터학부 정서현
// 네트워크 프로그래밍 과제 #08
// 멀티 쓰레드를 이용한 버스 예약 시스템 구현

#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>

#define	ROWS 2
#define COLS 10

#define SUCCESS 0 // 성공
#define WRONG -1 // 좌석번호 잘못 입력
#define RESERVE -2 // 이미 예약 된 좌석, 실패
#define NOTRESERVE -3 // 예약된 좌석은 아닌데, 실패
#define NOTOWNER -4 // 예약된 좌석이지만, 예약자가 아니어서 실패

void error_handling(char *message);
void *send_thread(void *arg);
void *recv_thread(void *arg);
void print_seats(int seats[ROWS][COLS]);

typedef	struct	{
	int	command;
	int	seatno;
	int	seats[ROWS][COLS];
	int	result;
}RES_PACKET;

typedef struct	{
	int	command;
	int	seatno;
}REQ_PACKET;

int sock;
static sem_t sem1, sem2;

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr;

    if (argc != 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    sem_init(&sem1, 0, 0);
    sem_init(&sem2, 0, 1);

    pthread_t snd_thread, rcv_thread;
    pthread_create(&snd_thread, NULL, send_thread, NULL);
    pthread_create(&rcv_thread, NULL, recv_thread, NULL);
    pthread_join(snd_thread, NULL);
    pthread_join(rcv_thread, NULL);

    sem_destroy(&sem1);
    sem_destroy(&sem2);
    close(sock);

    return 0;
}

void *send_thread(void *arg) {
    REQ_PACKET req_packet;
    int menu;

    while (1) {
        sem_wait(&sem2);

        printf("1: Inquiry, 2: Reservation, 3: Cancellation, 4: Quit \n");
        scanf("%d", &menu);

        req_packet.command = menu;

        switch (menu) {
            case 1: // 예약 현황 조회
                req_packet.seatno = 0;
                break;
            case 2: // 예약
                printf("Input seat number: ");
                scanf("%d", &req_packet.seatno);
                break;
            case 3: // 예약 취소
                printf("Input seat number for cancellation: ");
                scanf("%d", &req_packet.seatno);
                break;
            case 4: // 프로그램 종료
                printf("Quit.\n");
                req_packet.seatno = 0;
                break;
            default:
                printf("Invalid menu.\n");
                sem_post(&sem2);
                continue;
        }
        write(sock, &req_packet, sizeof(REQ_PACKET));

        sem_post(&sem1);
        if (menu == 4) break;
    }
    return NULL;
}

void *recv_thread(void *arg) {
    RES_PACKET res_packet;

    while (1) {
        sem_wait(&sem1);
        if (read(sock, &res_packet, sizeof(RES_PACKET)) <= 0) {
            error_handling("read() error\n");
        }
        switch (res_packet.command) {
            case 1: // Inquiry
                print_seats(res_packet.seats);
                printf("Operation success.\n");
                break;
            case 2: // Reservation
                print_seats(res_packet.seats); // 예약 결과 출력 이후에 좌석 정보 출력
                if (res_packet.result == SUCCESS) {
                    printf("Operation success.\n");
                } else if (res_packet.result == WRONG) {
                    printf("Wrong seat number\n");
                } else if (res_packet.result == RESERVE) {
                    printf("Reservation Failed. (The seat was already reserved.)\n");
                }
                break;
            case 3: // Cancellation
                print_seats(res_packet.seats);
                if (res_packet.result == 0) {
                    printf("Operation success.\n");
                } else if (res_packet.result == NOTRESERVE) {
                    printf("Cancellation failed. (The seat was not reserved.)\n");
                } else if (res_packet.result == NOTOWNER) {
                    printf("Cancellation Failed. (The seat was reserved by another person.)\n");
                } else {
                    printf("Cancellation Failed: Invalid seat number.\n");
                }
                break;
            case 4: // Quit
                sem_post(&sem2);
                return NULL;
            default:
                printf("Unknown command.\n");
                break;
        }
        sem_post(&sem2);
    }
    return NULL;
}


void print_seats(int seats[ROWS][COLS]) {
    printf("-----------------------------------------\n");
    printf("         Bus Reservation Status\n");
    printf("-----------------------------------------\n|");

    for (int j = 1; j < COLS + 1; j++) {
        printf(" %2d|", j);
    }
    printf("\n-----------------------------------------\n|");
    for (int j = 0; j < COLS; j++) {
        printf(" %2d|", seats[0][j]);
    }
    printf("\n-----------------------------------------\n|");
    for (int j = 11; j < 2 * COLS + 1; j++) {
        printf(" %2d|", j);
    }
    printf("\n-----------------------------------------\n|");
    for (int j = 0; j < COLS; j++) {
        printf(" %2d|", seats[1][j]);
    }    
    printf("\n-----------------------------------------\n");
}


void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}