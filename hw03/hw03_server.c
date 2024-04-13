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
    int serv_sock;
    int int_len;
    socklen_t clnt_adr_sz;
    struct sockaddr_in serv_adr, clnt_adr;

    // GAMEBOARD 구조체 0으로 초기화
    GAMEBOARD serv_game;
    memset(&serv_game, 0, sizeof(serv_game));
    int x, y;

    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    // UDP 소켓 생성
    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);

    if (serv_sock == -1)
        error_handling("UDP socket creation Error.\n");

    memset(&serv_adr, 0, sizeof(serv_adr));
    memset(&clnt_adr, 0, sizeof(clnt_adr));
    serv_adr.sin_family = AF_INET;
    // 모든 아이피 허용, htonl을 통해 빅 엔디안으로 전환
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    printf("Tic-Tac-Toe Server\n");

    // for (int i = 0; i < BOARD_SIZE; i++)
    // {
    //     for (int j = 0; j < BOARD_SIZE; j++)
    //         printf("%d ", serv_game.board[i][j]);
    //     printf("\n");
    // }

    draw_board(&serv_game);
    printf("\n");

    while (available_space(&serv_game))
    {
        // 클라이언트가 선택한 칸 입력
        clnt_adr_sz = sizeof(clnt_adr);
        recvfrom(serv_sock, &serv_game, sizeof(serv_game), 0, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
        draw_board(&serv_game);

        // 클라이언트가 선택한 뒤에도 선택 가능한 공간이 있는지?
        if (!available_space(&serv_game))
            break;

        // 빈 칸이 아직 있으므로 서버가 랜덤으로 빈 칸을 선택
        while (1)
        {
            srand(time(NULL));
            x = rand() % 3;
            y = rand() % 3;

            // 보드의 범위를 벗어난 입력을 받으면
            if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE)
                continue;

            // 입력 받은 칸이 비어있지 않은 경우
            else if (serv_game.board[x][y] != INIT_VALUE)
                continue;

            // 유효한 입력 값을 받았다!
            else
                break;
        }

        // 서버가 랜덤으로 뽑은 칸 보내기
        printf("Server choose : [%d, %d]\n", x, y);
        serv_game.board[x][y] = S_VALUE; // 서버가 선택한 칸
        draw_board(&serv_game);
        printf("\n");

        sendto(serv_sock, &serv_game, sizeof(serv_game), 0, (struct sockaddr *)&clnt_adr, clnt_adr_sz);
    }

    printf("No available space. Exit this program\n");
    close(serv_sock);
    printf("Tic Tac Toe Server Close\n");

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

// 리턴 값
// 0 = 빈 공간이 없는 경우, 1 = 빈 공간이 존재하는 경우
int available_space(GAMEBOARD *gboard)
{
    int i, j;

    for (i = 0; i < BOARD_SIZE; i++)
    {
        for (j = 0; j < BOARD_SIZE; j++)
        {
            // 빈 공간 발견!
            if (gboard->board[i][j] == 0)
                return 1;
        }
    }
    return 0;
}