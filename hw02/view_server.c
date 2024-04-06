// 2020115532 컴퓨터학부 정서현 
// 네트워크 프로그래밍 과제 #02
// TCP 통신을 이용한 원격 파일 뷰어 프로그램

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
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
     // 접속용 소켓 디스크립터
    int serv_sock;
    // 데이터 통신용 소켓 디스크립터
	int clnt_sock;

	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;

    PACKET send_packet;
    PACKET recv_packet;
    memset(&send_packet, 0, sizeof(PACKET));
    memset(&recv_packet, 0, sizeof(PACKET));

    int read_len;
    int rx_len;
    int total_tx_cnt = 0;
    int total_tx_bytes = 0;

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

    printf("---------------------------\n");
    printf("TCP Remote File View Server\n");
    printf("---------------------------\n");

    // 클라이언트가 보낸 패킷을 통해 파일이름을 읽음
    read(clnt_sock, &recv_packet, sizeof(recv_packet));
    printf("[Rx] cmd : %3d, file_name : %s\n", recv_packet.cmd, recv_packet.buf);

    int fd;

    // 해당 파일이 없는 경우
    if ( (fd = open(recv_packet.buf, O_RDONLY)) == -1 ){
        send_packet.cmd = FILE_NOT_FOUND;
        strcpy(send_packet.buf, recv_packet.buf);
        write(clnt_sock, &send_packet, sizeof(send_packet));
        printf("[Tx] cmd : %3d, %s : File Not Found\n", send_packet.cmd, recv_packet.buf);
    }

    // 파일이 존재
    else {
        while(1){
            if ( (read_len = read(fd, &send_packet.buf, sizeof(send_packet.buf))) < BUF_SIZE )
                send_packet.cmd = FILE_END; 
            else
                send_packet.cmd = FILE_RES;
            
            send_packet.buf_len = read_len;
            total_tx_cnt += 1;
            total_tx_bytes += read_len;

            // 1초마다 클라이언트로 패킷 전송
            // usleep(10000); 테스트용 마이크로초 단위  -> 정상 작동 확인
            sleep(1);
            write(clnt_sock, &send_packet, sizeof(send_packet));
            printf("[Tx] cmd : %3d, len : %3d, total_tx_cnt : %3d, total_tx_bytes : %3d\n", send_packet.cmd, read_len, total_tx_cnt, total_tx_bytes);

            // 클라이언트에서 서버로 보내온 패킷 읽기
            read(clnt_sock, &recv_packet, sizeof(recv_packet));

            // 여전히 요청 상태를 받음
            if (recv_packet.cmd == FILE_REQ)
                continue;
            
            // 파일의 끝을 확인했다는 신호를 받음
            else if (recv_packet.cmd == FILE_END_ACK){
                printf("[Rx] cmd : %3d, FILE_END_ACK\n", recv_packet.cmd);
                close(fd); // 파일 닫기
                break;
            }   
        }
    }

    printf("-----------------------------------\n");
    printf("Total Tx count : %3d, bytes : %3d\n", total_tx_cnt, total_tx_bytes);

    close(clnt_sock);
    close(serv_sock);

    printf("TCP Server Socket Close!\n");
    printf("-----------------------------------\n");
    return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
