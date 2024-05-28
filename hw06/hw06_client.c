// 2020115532 컴퓨터학부 정서현
// 네트워크 프로그래밍 과제 #06
// Packet Forwarding Client 및 Server 구현

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 2048

typedef struct
{
  int mode;
  char buf[BUF_SIZE];
  int buflen;
} PACKET;

void select_mode();
int init_sock(int sock, struct sockaddr_in sock_addr, char *argv[]);
void error_handling(char *message);

int main(int argc, char *argv[])
{
  int cnt = 0;
  int sock;
  int str_len;
  struct sockaddr_in serv_adr;

  if (argc != 3)
  {
    printf("Usage : %s <IP> <port>\n", argv[0]);
    exit(1);
  }

  // 초기 모드 선택 (Sender / Receiver)
  select_mode();

  PACKET clnt_packet;
  memset(&clnt_packet, 0, sizeof(clnt_packet)); // 패킷 초기화
  struct timeval timeout;
  fd_set reads, cpy_reads;
  int fd_max, fd_num;

  scanf("%d", &clnt_packet.mode);

  // Sender
  if (clnt_packet.mode == 1)
  {
    printf("\nFile Sender Start!\n");

    // 파일 디스크립터 초기화
    int fd = open("rfc1180.txt", O_RDONLY);

    // 파일 열기 실패
    if (fd == -1)
      error_handling("read error!");

    // socket 초기화
    sock = init_sock(sock, serv_adr, argv);

    // fd 초기화
    FD_ZERO(&reads);
    FD_SET(sock, &reads);
    if (fd > 0)
      FD_SET(fd, &reads);

    if (sock > fd)
      fd_max = sock;
    else
      fd_max = fd;

    printf("fd1: %d, fd2: %d\n", fd, sock);
    printf("fd_max : %d\n", fd_max);

    while (1)
    {
      memset(&clnt_packet, 0, sizeof(clnt_packet));
      cpy_reads = reads; // 원본 설정 (reads) 을 cpy_reads 에 복사 후 사용

      // select 의 timeout 은 3초로 고정!
      timeout.tv_sec = 3;
      timeout.tv_usec = 0;

      // sock 을 통해 수신된 데이터가 있는지 관찰 (0 ~ fd_max 까지)
      // 등록된 파일 디스크립터에 변화가 발생하면 0보다 큰 값 (= 변화가 발생한 파일 디스크립터의 수) 이 반환
      fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout);

      if (fd_num == -1)
        error_handling("select() error\n");

      else if (fd_num == 0)
        continue;

      for (int i = 0; i < fd_max + 1; i++)
      {
        if (FD_ISSET(i, &reads))
        {
          if (i == fd)
          {
            clnt_packet.buflen = read(fd, &clnt_packet.buf, sizeof(clnt_packet.buf));

            if (clnt_packet.buflen == -1)
              error_handling("read() error!");

            clnt_packet.mode = 1;

            write(sock, &clnt_packet, sizeof(clnt_packet));
            sleep(1);
          }

          else if (i == sock)
          {
            read(sock, &clnt_packet, sizeof(clnt_packet));

            clnt_packet.mode = 1;
            clnt_packet.buf[clnt_packet.buflen] = 0;

            printf("%s", clnt_packet.buf);
          }
        }
      }

      // 마지막 송신
      if (clnt_packet.buflen < BUF_SIZE)
        break;
    }

    // 파일 디스크립터 삭제
    FD_CLR(sock, &reads);
    FD_CLR(fd, &reads);
    close(fd);
  }

  // Receiver
  else if (clnt_packet.mode == 2)
  {
    int exit_flag = 1;
    printf("\nFile Receiver Start!\n");

    // socket 초기화
    sock = init_sock(sock, serv_adr, argv);

    // fd 초기화
    FD_ZERO(&reads);
    FD_SET(sock, &reads);

    fd_max = sock;

    printf("fd2: %d\n", sock);
    printf("fd_max: %d\n", fd_max);

    while (exit_flag)
    {
      memset(&clnt_packet, 0, sizeof(clnt_packet));
      cpy_reads = reads; // 원본 설정 (reads) 을 cpy_reads 에 복사 후 사용

      // select 의 timeout 은 3초로 고정!
      timeout.tv_sec = 3;
      timeout.tv_usec = 0;

      // sock 을 통해 수신된 데이터가 있는지 관찰 (0 ~ fd_max 까지)
      // 등록된 파일 디스크립터에 변화가 발생하면 0보다 큰 값 (= 변화가 발생한 파일 디스크립터의 수) 이 반환
      fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout);

      if (fd_num == -1)
        error_handling("select() error\n");

      else if (fd_num == 0)
        continue;

      for (int i = 0; i < fd_max + 1; i++)
      {
        if (FD_ISSET(i, &reads))
        {
          if (i == sock)
          {
            read(sock, &clnt_packet, sizeof(clnt_packet));

            clnt_packet.mode = 2;
            clnt_packet.buf[clnt_packet.buflen] = 0;

            printf("%s\n", clnt_packet.buf);

            // 마지막 수신
            if (clnt_packet.buflen < BUF_SIZE)
              exit_flag = 0;

            write(sock, &clnt_packet, sizeof(clnt_packet));
          }
        }
      }
    }
    // 파일 디스크립터 삭제
    FD_CLR(sock, &reads);
  }
  close(sock);
  return 0;
}

void select_mode()
{
  printf("-----------------------------\n");
  printf(" Choose Function\n");
  printf(" 1. Sender  2. Receiver\n");
  printf("-----------------------------\n");
  printf(" => ");
}

int init_sock(int sock, struct sockaddr_in sock_addr, char *argv[])
{
  sock = socket(PF_INET, SOCK_STREAM, 0); // TCP

  if (sock == -1)
    error_handling("socket() error");

  memset(&sock_addr, 0, sizeof(sock_addr));
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_addr.s_addr = inet_addr(argv[1]);
  sock_addr.sin_port = htons(atoi(argv[2]));

  // 연결 실패
  if (connect(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1)
    error_handling("connect() error!");
  else
    puts("Connected………..");

  return sock;
}

void error_handling(char *message)
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}