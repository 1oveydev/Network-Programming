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
#include <netinet/tcp.h>

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

int main(int argc, char *argv[])
{
    int serv_sock; // 데이터 통신용 소켓 디스크립터 (UDP)
    socklen_t clnt_adr_sz;

    struct sockaddr_in serv_adr, clnt_adr;
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
        error_handling("UDP socket creation error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    SO_PACKET send_packet, recv_packet;
    memset(&send_packet, 0, sizeof(SO_PACKET)); // 구조체 0으로 초기화!
    memset(&recv_packet, 0, sizeof(SO_PACKET)); // 구조체 0으로 초기화!

    int tcp_sock = socket(PF_INET, SOCK_STREAM, 0); // 소켓 옵션 값 확인 용도 소켓 디스크립터 (TCP)
    int sock_type;
    socklen_t optlen;

    printf("Socket Option Server Start\n");
    int rd, wt; // 몇 바이트 수신하고 발신했는지?
    int check, lv, opt;
    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        rd = recvfrom(serv_sock, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
        if (rd == -1)
            error_handling("recvfrom() error");

        printf("\n\n\n\n\n\n\n\n\n\n");
        printf(">>> Received Socket option: %s\n", sock_option[recv_packet.option - 1]);

        // level 1 : SOL_SOCKET
        if (recv_packet.level == 1)
        {
            if (strcmp(sock_option[recv_packet.option - 1], "SO_SNDBUF") == 0)
            {
                opt = SO_SNDBUF;
            }
            else if (strcmp(sock_option[recv_packet.option - 1], "SO_RCVBUF") == 0)
            {
                opt = SO_RCVBUF;
            }
            else if (strcmp(sock_option[recv_packet.option - 1], "SO_REUSEADDR") == 0)
            {
                opt = SO_REUSEADDR;
            }
            else if (strcmp(sock_option[recv_packet.option - 1], "SO_KEEPALIVE") == 0)
            {
                opt = SO_KEEPALIVE;
            }
            else if (strcmp(sock_option[recv_packet.option - 1], "SO_BROADCAST") == 0)
            {
                opt = SO_BROADCAST;
            }
            check = getsockopt(tcp_sock, SOL_SOCKET, opt, &sock_type, &optlen);
        }

        // level 2 : IPPROTO_IP
        else if (recv_packet.level == 2)
        {
            if (strcmp(sock_option[recv_packet.option - 1], "IP_TOS") == 0)
            {
                opt = IP_TOS;
            }
            else if (strcmp(sock_option[recv_packet.option - 1], "IP_TTL") == 0)
            {
                opt = IP_TTL;
            }
            check = getsockopt(tcp_sock, IPPROTO_IP, opt, &sock_type, &optlen);
        }

        // level 3 : IPPROTO_TCP
        else if (recv_packet.level == 3)
        {
            if (!strcmp(sock_option[recv_packet.option - 1], "TCP_NODELAY"))
            {
                opt = TCP_NODELAY;
            }
            else if (!strcmp(sock_option[recv_packet.option - 1], "TCP_MAXSEG"))
            {
                opt = TCP_MAXSEG;
            }
            check = getsockopt(tcp_sock, IPPROTO_TCP, opt, &sock_type, &optlen);
        }

        // 그 이외의 level 들은 예외처리
        else
            error_handling("Wrong level!");

        // 옵션 확인 실패 : result -> -1
        if (check == -1)
            send_packet.result = -1;

        // 옵션 확인 성공 : result -> 0
        else
        {
            send_packet.optval = sock_type;
            send_packet.result = 0;
        }

        wt = sendto(serv_sock, &send_packet, sizeof(send_packet), 0, (struct sockaddr *)&clnt_adr, clnt_adr_sz);
        if (wt == -1)
            error_handling("sendto() error");

        printf("<<< Send option: %s: %d, result: %d\n", sock_option[recv_packet.option - 1], send_packet.optval, send_packet.result);
    }
    close(tcp_sock);
    close(serv_sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
