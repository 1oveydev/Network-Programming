// 2020115532 컴퓨터학부 정서현
// 네트워크 프로그래밍 과제 #03
// UDP 프로토콜을 이용한 Tic-Tac-Toe 프로그램

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define BOARD_SIZE 3

#define INIT_VALUE 0
#define S_VALUE 1
#define C_VALUE 2

typedef struct
{
    int board[BOARD_SIZE][BOARD_SIZE];
} GAMEBOARD;

void draw_board(GAMEBOARD *);
void error_handling(char *);
int available_space(GAMEBOARD *);

int main(int argc, char *argv[])
{
    int sock;
    int int_len;
    socklen_t adr_sz;
    struct sockaddr_in serv_adr, clnt_adr; // 서버와 클라이언트의 소켓 정보를 저장할 구조체

    // GAMEBOARD 구조체 0으로 초기화
    GAMEBOARD clnt_game;
    memset(&clnt_game, 0, sizeof(clnt_game));

    int input_x, input_y;

    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // UDP 소켓 생성
    sock = socket(PF_INET, SOCK_DGRAM, 0);

    if (sock == -1)
        error_handling("socket() error");

    // 서버 소켓 구조체 초기화
    memset(&serv_adr, 0, sizeof(serv_adr));

    // 클라이언트 소켓 구조체 초기화
    // 이건 왜 안하는거지?
    // memset(&clnt_adr, 0, sizeof(clnt_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    printf("Tic-Tac-Toe Client\n");

    // for (int i = 0; i < BOARD_SIZE; i++)
    // {
    //     for (int j = 0; j < BOARD_SIZE; j++)
    //         printf("%d ", clnt_game.board[i][j]);
    //     printf("\n");
    // }

    draw_board(&clnt_game);
    printf("\n");

    while (available_space(&clnt_game))
    {
        printf("Input row, column : ");
        scanf("%d %d", &input_x, &input_y);

        // 보드의 범위를 벗어난 입력을 받으면
        if (input_x < 0 || input_x >= BOARD_SIZE || input_y < 0 || input_y >= BOARD_SIZE)
            continue;

        // 입력 받은 칸이 비어있지 않은 경우
        else if (clnt_game.board[input_x][input_y] != INIT_VALUE)
            continue;

        // 유효한 입력 값을 받았다!
        else
        {
            clnt_game.board[input_x][input_y] = C_VALUE; // 클라이언트가 선택한 칸
            draw_board(&clnt_game);
            sendto(sock, &clnt_game, sizeof(clnt_game), 0, (struct sockaddr *)&serv_adr, sizeof(serv_adr));

            // 클라이언트가 칸을 선택한 이후 선택할 칸이 없으면
            if (!available_space(&clnt_game))
                break;
        }

        // 빈 공간이 아직 있으므로 서버가 랜덤으로 선택한 칸을 전달 받음
        adr_sz = sizeof(clnt_adr);
        recvfrom(sock, &clnt_game, sizeof(clnt_game), 0, (struct sockaddr *)&clnt_adr, &adr_sz);
        draw_board(&clnt_game);
        printf("\n");
    }
    printf("No available space. Exit Client\n");
    close(sock);
    printf("Tic Tac Toe Client Close\n");

    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void draw_board(GAMEBOARD *gboard)
{
    char value = ' ';
    int i, j;

    printf("+-----------+\n");

    for (i = 0; i < BOARD_SIZE; i++)
    {
        for (j = 0; j < BOARD_SIZE; j++)
        {
            if (gboard->board[i][j] == INIT_VALUE) // 초기 값 0
                value = ' ';

            else if (gboard->board[i][j] == S_VALUE) // Server 표시 (1)
                value = 'O';

            else if (gboard->board[i][j] == C_VALUE) // Client 표시 (2)
                value = 'X';

            else
                value = ' ';

            printf("| %c ", value);
        }
        printf("|");
        printf("\n+-----------+\n");
    }
}

int available_space(GAMEBOARD *gboard)
{
    int i, j;
    int check = 0; // 0 = 빈 공간이 없는 경우, 1 = 빈 공간이 존재하는 경우

    for (i = 0; i < BOARD_SIZE; i++)
    {
        for (j = 0; j < BOARD_SIZE; j++)
        {
            // 빈 공간 발견!
            if (gboard->board[i][j] == 0)
            {
                check = 1;
                break;
            }
        }
        if (check)
            break;
    }
    return check;
}