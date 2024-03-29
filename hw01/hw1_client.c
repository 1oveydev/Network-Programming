#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define ERROR 0
#define SUCCESS 1

#define BUF_SIZE 20

#define REQUEST 0
#define RESPONSE 1
#define QUIT 2

void error_handling(char*);

typedef struct {
    int cmd;                // 0: request, 1: response, 2: quit
    char addr[BUF_SIZE];    // dotted-decimal address
    struct in_addr iaddr;   // inet_aton() result
    int result;             // 0: Error, 1: Success
} PACKET;

int main(int argc, char *argv[]) {
    // 클라이언트는 하나의 소켓 디스크립터만 사용
    int sock;
    struct sockaddr_in serv_addr;

    // 명령행 인자의 수가 다른 경우, 에러 메시지 출력 후 종료
    if (argc != 3){
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

    // Step 1 : 소켓 creation
    sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		error_handling("socket() error");

    // sockaddr_in 구조체 초기화
    memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]); // 명령행 인자로 서버의 주소 설정
	serv_addr.sin_port=htons(atoi(argv[2])); // 포트번호 : 명령행 인자로 받은 문자열을 정수형으로 바꾸고 호스트 바이트 순서를 네트워크 바이트 순서로 변경

    // Step 2: connection request
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");


    // Step 3 : receiving a message from the server
    char input[BUF_SIZE];
    PACKET packet;

    while (1) {
        printf("Input dotted-decimal address : ");
        fgets(input, BUF_SIZE, stdin);

        // 개행문자 제거
        if (strlen(input) > 0 && input[strlen(input) - 1] == '\n') 
            input[strlen(input) - 1] = '\0';
        
        // QUIT 상태로 변경 (2)
        if (strcmp(input, "quit") == 0) {
            packet.cmd = QUIT;
            write(sock, &packet, sizeof(packet));
            printf("[Tx] cmd : %d (QUIT)\nClient socket close and exit.\n", packet.cmd);
            break;
        }

        packet.cmd = REQUEST; // 서버로 주소를 전송 하기 전 요청 상태로 설정 (0)
        strcpy(packet.addr, input); // input 문자열을 packet.addr 로 복사

        printf("[Tx] cmd : %d, addr : %s\n", packet.cmd, packet.addr);

        // 입력된 주소를 서버로 전송하기 위해 패킷을 소켓을 통해 전송
        write(sock, &packet, sizeof(packet));
        // 서버로부터 결과를 수신하기 위해 패킷을 소켓을 통해 수신
        read(sock, &packet, sizeof(packet));

        // 수신 성공
        if (packet.result == SUCCESS) {
            printf("[Rx] cmd : %d, Address conversion : %#x (result : %d) \n", packet.cmd, packet.iaddr.s_addr, packet.result);
        } 
        // 수신 실패
        else {
            printf("[Rx] cmd : %d, Address conversion fail! (result : %d) \n", packet.cmd, packet.result);
        }

        printf("\n");
    }

    // 소켓 닫기
    close(sock);

    return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

