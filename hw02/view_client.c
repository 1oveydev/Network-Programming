// 2020115532 컴퓨터학부 정서현 
// 네트워크 프로그래밍 과제 #02
// TCP 통신을 이용한 원격 파일 뷰어 프로그램

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUF_SIZE 100

// cmd type
#define FILE_REQ 1
#define FILE_RES 2
#define FILE_END 3
#define FILE_END_ACK 4
#define FILE_NOT_FOUND 5

typedef struct{
    int cmd;
    int buf_len; // 실제 전송되는 파일의 크기 저장
    char buf[BUF_SIZE];
} PACKET;

void error_handling(char *message);

int main(int argc, char *argv[]){
    // 클라이언트는 하나의 소켓 디스크립터만 사용
    int sock;
    struct sockaddr_in serv_addr;

    int total_rx_cnt = 0;
    int total_rx_bytes = 0;

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
    PACKET recv_packet;
    PACKET send_packet;
    memset(&send_packet, 0, sizeof(PACKET));
    memset(&recv_packet, 0, sizeof(PACKET));

    printf("Input file name : ");
    scanf("%s", send_packet.buf);

    // printf("%s", send_packet.buf);

    // 서버에 파일 이름 전달
    send_packet.cmd = FILE_REQ;
    write(sock, &send_packet, sizeof(send_packet));
    printf("[Tx] cmd : %3d, file_name : %s\n", send_packet.cmd, send_packet.buf);

    int tx_len;

    while (1){
        tx_len = read(sock, &recv_packet, sizeof(recv_packet));

        if (tx_len == 0)
            break;

        else{
            if (recv_packet.cmd == FILE_RES) {
                total_rx_cnt += 1;
                total_rx_bytes += recv_packet.buf_len;
                printf("%s", recv_packet.buf);
                write(sock, &send_packet, sizeof(send_packet));
            }

            else if (recv_packet.cmd == FILE_END){
                total_rx_cnt += 1;
                total_rx_bytes += recv_packet.buf_len;

                // 마지막으로 받아온 문자열을 출력하는 과정
                // char last_str[BUF_SIZE];
                // strncpy(last_str, recv_packet.buf, recv_packet.buf_len);
                recv_packet.buf[recv_packet.buf_len] = '\0';
                printf("%s\n", recv_packet.buf);
                
                printf("-----------------------------------\n");
                printf("[Rx] cmd : %3d, FILE_END\n", recv_packet.cmd);

                send_packet.cmd = FILE_END_ACK;
                write(sock, &send_packet, sizeof(send_packet));

                printf("[Tx] cmd : %3d, FILE_END_ACK\n", send_packet.cmd);
                break;
            }

            else if (recv_packet.cmd == FILE_NOT_FOUND){
                printf("[Rx] cmd : %3d, %s : File Not Found\n", recv_packet.cmd, recv_packet.buf);
            }   

            else{
                error_handling("Something wrong...");
            }
        }    
    }

    printf("-----------------------------------\n");
    printf("Total Rx count : %3d, bytes : %d\n", total_rx_cnt, total_rx_bytes);

    // 소켓 닫기
    close(sock);

    printf("TCP Client Socket Close!\n");
    printf("-----------------------------------\n");


    return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
