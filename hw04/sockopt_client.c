// 2020115532 컴퓨터학부 정서현
// 네트워크 프로그래밍 과제 #04
// UDP 통신을 이용한 소켓 옵션 조회 프로그램

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

typedef struct
{
    int level;
    int option;
    int optval;
    int result;
} SO_PACKET;

char *sock_option[] = {
    "SO_SNDBUF", "SO_RCVBUF", "SO_REUSEADDR", "SO_KEEPALIVE", "SO_BROADCAST",
    "IP_TOS", "IP_TTL", "TCP_NODELAY", "TCP_MAXSEG", "QUIT"};

void error_handling(char *message);
void printmenu(void);

int main(int argc, char *argv[])
{
    int sock;
    socklen_t adr_sz;

    struct sockaddr_in serv_adr, from_adr;
    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_DGRAM, 0); // UDP
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    SO_PACKET send_packet, recv_packet;
    memset(&send_packet, 0, sizeof(SO_PACKET));
    memset(&recv_packet, 0, sizeof(SO_PACKET));

    int wt, rd, input;
    printmenu();
    while (1)
    {
        printf("Input option number: ");
        scanf("%d", &input);
        if (input >= 1 && input <= 5)
        {
            send_packet.level = 1;
            send_packet.option = input;
        }
        else if (input == 6 || input == 7)
        {
            send_packet.level = 2;
            send_packet.option = input;
        }
        else if (input == 8 || input == 9)
        {
            send_packet.level = 3;
            send_packet.option = input;
        }
        else if (input == 10)
        {
            printf("Client quit.\n");
            break;
        }
        else
        {
            printf("Wrong number. type again!\n");
            continue;
        }
        wt = sendto(sock, &send_packet, sizeof(send_packet), 0, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
        if (wt == -1)
            error_handling("sendto() error");

        adr_sz = sizeof(from_adr);
        rd = recvfrom(sock, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr *)&from_adr, &adr_sz);
        if (rd == -1)
            error_handling("recvfrom() error");
        printf(">>> Server Result: %s: value: %d, result: %d\n\n", sock_option[input - 1], recv_packet.optval, recv_packet.result);
        printmenu();
    }
    close(sock);
    return 0;
}

void printmenu(void)
{
    printf("---------------------------\n");
    printf("1: SO_SNDBUF\n");
    printf("2: SO_RCVBUF\n");
    printf("3: SO_REUSEADDR\n");
    printf("4: SO_KEEPALIVE\n");
    printf("5: SO_BROADCAST\n");
    printf("6: IP_TOS\n");
    printf("7: IP_TTL\n");
    printf("8: TCP_NODELAY\n");
    printf("9: TCP_MAXSEG\n");
    printf("10: Quit\n");
    printf("---------------------------\n");
    return;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
