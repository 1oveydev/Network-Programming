// 2020115532 컴퓨터학부 정서현
// 네트워크 프로그래밍 과제 #07
// 멀티 캐스트를 이용한 채팅 프로그램

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/wait.h>

#define TTL 64
#define BUF_SIZE 120

void error_handling(char *message);

int main(int argc, char *argv[]){

    if (argc != 4){
        printf("Usage: %s <GroupIP> <Port> <Name>\n", argv[0]);
        exit(1);
    }
    
    int status;
    pid_t pid = fork();

    if (pid == -1){
        error_handling("fork() error!");
    }

    // 자식 프로세스 : Multicast Receiver
    else if (pid == 0){
        int recv_sock;
        struct sockaddr_in adr;
        struct ip_mreq join_adr;

        recv_sock = socket(PF_INET, SOCK_DGRAM, 0); // UDP
        memset(&adr, 0, sizeof(adr));
        adr.sin_family = AF_INET;
        adr.sin_addr.s_addr = htonl(INADDR_ANY);
        adr.sin_port = htons(atoi(argv[2]));

        // true 로 설정
        int multi_ok = 1;

        // 바인드 전, 여러 프로세스가 동일한 멀티캐스트 주소와 포트를 사용할 수 있게?
        // SO_REUSEADDR 적용 (이미 사용중인 포트에 대해 새로운 소켓을 바인딩 가능하게)
        setsockopt(recv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&multi_ok, sizeof(multi_ok));

        // 맥 커널 환경에서 bind error 가 자꾸나서.. 추가한 코드
        setsockopt(recv_sock, SOL_SOCKET, SO_REUSEPORT, (void*)&multi_ok, sizeof(multi_ok));
        
        if (bind(recv_sock, (struct sockaddr*)&adr, sizeof(adr)) == -1)
            error_handling("bind() error");
        
        join_adr.imr_multiaddr.s_addr = inet_addr(argv[1]); // 가입할 멀티캐스트 그룹 주소
        join_adr.imr_interface.s_addr = htonl(INADDR_ANY); // 멀티캐스트 그룹에 가입할 자신의 IP 주소

        // 멀티캐스트 그룹 가입
        setsockopt(recv_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&join_adr, sizeof(join_adr));


        char buf[BUF_SIZE];
        int str_len;

        while (1){
            str_len = recvfrom(recv_sock, buf, BUF_SIZE - 1, 0, NULL, 0);
            if (str_len < 0) break;
            buf[str_len] = 0;
            fputs(buf, stdout);
        }
    }

    // 부모 프로세스 : Multicast Sender 
    else {
        int send_sock;
        struct sockaddr_in mul_adr;
        int time_live = TTL;

        send_sock = socket(PF_INET, SOCK_DGRAM, 0);
        memset(&mul_adr, 0, sizeof(mul_adr));
        mul_adr.sin_family = AF_INET;
        mul_adr.sin_addr.s_addr = inet_addr(argv[1]); // 멀티캐스트 IP
        mul_adr.sin_port = htons(atoi(argv[2])); // 멀티캐스트 Port

        setsockopt(send_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&time_live, sizeof(time_live)); // 멀티캐스트 TTL 설정

        char name[15];
        char message[101];

        // buf 에 이름과 메시지 모두 담아서 전송
        char buf[BUF_SIZE];

        // name 배열에 [이름] + 띄어쓰기 1개 형태로 저장
        strcpy(name, "[");
        strcat(name, argv[3]);
        strcat(name, "] ");

        while (1){
            strcpy(buf, name);
            fgets(message, BUF_SIZE, stdin);

            if (!strcmp(message, "Q\n") || !strcmp(message, "q\n")){
                printf("SIGKILL: Multicast Receiver terminate!\n");
                kill(pid, SIGKILL);
                break;
            }
            strcat(buf, message);
            sendto(send_sock, buf, strlen(buf), 0, (struct sockaddr*)&mul_adr, sizeof(mul_adr));
        }
        printf("Multicast Sender(Parent process) exit!\n");
    }
    return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}