// 2020115532 컴퓨터학부 정서현
// 네트워크 프로그래밍 과제 #06
// Packet Forwarding Client 및 Server 구현

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

#define BUF_SIZE 2048 // 버퍼 크기 2048 바이트 고정

typedef struct
{
  int mode;
  char buf[BUF_SIZE];
  int buflen;
} PACKET;

void error_handling(char *buf);

int main(int argc, char *argv[])
{
  // serv_sock : 클라이언트의 연결 요청 처리 용도
  // clnt_sock : 클라이언트와 데이터 송수신 용도
  int serv_sock, clnt_sock;
  struct sockaddr_in serv_adr, clnt_adr;
  struct timeval timeout;
  fd_set reads, cpy_reads;

  socklen_t adr_sz;
  int fd_max, str_len, fd_num, i;

  if (argc != 2)
  {
    printf("Usage: %s <port>\n", argv[0]);
    exit(1);
  }

  serv_sock = socket(PF_INET, SOCK_STREAM, 0); // TCP
  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_adr.sin_port = htons(atoi(argv[1]));

  if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    error_handling("bind() error");

  if (listen(serv_sock, 5) == -1)
    error_handling("listen() error");

  // serv_sock (리스닝 소켓) 을 통한 연결 요청도 데이터 수신으로 구분
  FD_ZERO(&reads);
  FD_SET(serv_sock, &reads);
  fd_max = serv_sock; // 관찰 대상에 추가!

  PACKET serv_packet;
  memset(&serv_packet, 0, sizeof(serv_packet)); // 패킷 초기화
  int check = 1;
  int first_sock = 0;
  int second_sock = 0;

  while (1)
  {
    memset(&serv_packet, 0, sizeof(serv_packet)); // 패킷 초기화
    cpy_reads = reads;                            // 원본 설정 (reads) 을 cpy_reads 에 복사 후 사용

    // select 의 timeout 은 3초로 고정!
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    // serv_sock 을 통해 수신된 데이터가 있는지 관찰 (0 ~ fd_max 까지)
    // 등록된 파일 디스크립터에 변화가 발생하면 0보다 큰 값 (= 변화가 발생한 파일 디스크립터의 수) 이 반환
    if ((fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout)) == -1)
      break;

    // 변화가 발생한 파일 디스크립터가 없음!
    if (fd_num == 0)
      continue;

    // 변화가 발생한 파일 디스크립터가 존재!
    for (i = 0; i < fd_max + 1; i++)
    {
      // 해당 파일 디스크립터에 변화가 있는지 확인
      if (FD_ISSET(i, &cpy_reads))
      {
        // 수신된 데이터가 serv_sock (연결 처리 용도) 에 있으면, 연결 처리
        if (i == serv_sock)
        {
          adr_sz = sizeof(clnt_adr);
          clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz);
          FD_SET(clnt_sock, &reads); // clnt_sock 을 select() 의 감시 대상에 추가!

          if (fd_max < clnt_sock)
            fd_max = clnt_sock;

          // 첫 번째 소켓 디스크립터 연결
          if (check == 1)
          {
            first_sock = clnt_sock;
            check--;
          }

          // 두 번째 소켓 디스크립터 연결
          else if (check == 0)
          {
            second_sock = clnt_sock;
          }

          printf("Connected client: %d (fd_max: %d)\n", clnt_sock, fd_max);
        }

        // 수신된 데이터가 clnt_sock (데이터 송수신 용도) 에 있으면, 에코 처리
        else
        {
          str_len = read(i, &serv_packet, sizeof(serv_packet));

          // 클라이언트가 close() 호출 시 반환 값은 0
          if (str_len == 0)
          {
            FD_CLR(i, &reads); // reads 에서 파일 디스크립터 'i' 를 삭제
            close(i);
            printf("closed client: %d\n", i);
          }

          // 에코
          else
          {
            // mode 1 : Forwarding
            if (serv_packet.mode == 1)
            {
              write(first_sock, &serv_packet, sizeof(serv_packet));
              printf("Forward  [%d] ---> [%d]\n", first_sock, second_sock);
            }

            // mode 2 : Backwarding
            else if (serv_packet.mode == 2)
            {
              write(second_sock, &serv_packet, sizeof(serv_packet));
              printf("Backward [%d] <--- [%d]\n", second_sock, first_sock);
            }

            // 이외의 mode 값이라면 예외처리!
            else
            {
              error_handling("Incorrect mode!");
            }
          }
        }
      }
    }
  }
  close(serv_sock);
  return 0;
}

void error_handling(char *buf)
{
  fputs(buf, stderr);
  fputc('\n', stderr);
  exit(1);
}
