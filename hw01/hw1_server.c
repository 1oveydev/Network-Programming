// 2020115532 컴퓨터학부 정서현 
// 네트워크 프로그래밍 과제 #01
// 소켓 통신을 이용한 주소 변환 프로그램

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define ERROR 0
#define SUCCESS 1

#define BUF_SIZE 20

#define REQUEST 0
#define RESPONSE 1
#define QUIT 2

void error_handling(char *);

typedef struct {
    int cmd;                // 0: request, 1: response, 2: quit
    char addr[BUF_SIZE];    // dotted-decimal address
    struct in_addr iaddr;   // inet_aton() result
    int result;             // 0: Error, 1: Success
} PACKET;


int main(int argc, char* argv[]){
    // 접속용 소켓 디스크립터
    int serv_sock;
    // 데이터 통신용 소켓 디스크립터
	int clnt_sock;

	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;

    // 명령행 인자의 수가 다른 경우, 에러 메시지 출력 후 종료
    // 포트 번호 입력 여부를 확인
    if (argc != 2){
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

    // Step 1 : socket creation 
    // 성공시 소켓 디스크립터 반환, 실패 시 -1 반환
    // 도메인 : IPv4, 연결 방법 : TCP , IP 사용 (0)
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1)
		error_handling("socket() error");
	
    // sockaddr_in 구조체 초기화
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 서버에 IP 주소 자동 할당
	serv_addr.sin_port = htons(atoi(argv[1])); // 명령행 인자로 포트 번호 입력
	

    // Step 2 : bind
	if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
		error_handling("bind() error"); 
	
    // Step 3 : listen
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");
	
	clnt_addr_size = sizeof(clnt_addr);  

    // Step 4 : accept
    // 데이터 통신용 clnt_sock
	clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
	if (clnt_sock == -1)
		error_handling("accept() error");  

    PACKET recv_packet;
    int rx_len;

    printf("--------------------------\n");
    printf("Address Conversion Server\n");
    printf("--------------------------\n");

    // Send a message to a client
    while (1) {
        rx_len = read(clnt_sock, &recv_packet, sizeof(PACKET));
        if (rx_len == 0)
            break;

        // 서버가 요청 상태 일 때
        if (recv_packet.cmd == REQUEST) {
            printf("[Rx] Received Dotted-Decimal Address: %s\n", recv_packet.addr);

            // struct in_addr iaddr; 
            // 굳이 하나 더 선언 해야 되는가? PACKET 구조체 안에 이미 원하는 구조체가 있는데..
            // 문자열 형태를 in_addr 구조체로 변환 (성공 시 1, 실패 시 0 반환)
            // 실패 시
            if (inet_aton(recv_packet.addr, &recv_packet.iaddr) == 0) {
                recv_packet.cmd = RESPONSE;
                recv_packet.result = ERROR;
                printf("[Tx] Address conversion fail : (%s)\n", recv_packet.addr);

                // 클라이언트로 데이터 전송
                write(clnt_sock, &recv_packet, sizeof(recv_packet));
            } 
            // 성공 시
            else {
                recv_packet.cmd = RESPONSE;
                recv_packet.result = SUCCESS;
                // recv_packet.iaddr = iaddr;
                printf("inet_aton(%s) -> %#x\n", recv_packet.addr, recv_packet.iaddr.s_addr);
                printf("[Tx] cmd : %d, iaddr : %#x, result : %d\n", recv_packet.cmd, recv_packet.iaddr.s_addr, recv_packet.result);

                // 클라이언트로 데이터 전송
                write(clnt_sock, &recv_packet, sizeof(recv_packet));
            }
        } 
        
        // 클라이언트에서 quit 을 받았을 때,
        else if (recv_packet.cmd == QUIT) {
            printf("[Rx] Quit command received\nServer socket close and exit.\n");
            break;
        } 
        
        else {
            printf("[Rx] Invalid command: %d\n", recv_packet.cmd);
            break;
        }
        printf("\n");
    }

    return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
