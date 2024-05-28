// 2020115532 컴퓨터학부 정서현
// 네트워크 프로그래밍 과제 #05
// fork() 및 sigaction() 함수를 활용한 다중 타이머 기능 구현

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

void child_handler(int);
void parent_handler(int);
void sigint_handler(int);
void child_exit_handler(int);

int callAlarm = 1;
int callChildCount = 0;
int callParentCount = 0;
pid_t child_pid;

int main()
{
  pid_t pid;

  struct sigaction child_act, parent_act, sigint_act, child_exit_act;
  printf("Parent process created.\n");

  pid = fork();

  // 자식 프로세스
  if (pid == 0)
  {
    printf("Child process created.\n");

    // 자식 프로세스 SIGALRM 등록 후 시그널 처리
    child_act.sa_handler = child_handler;
    sigemptyset(&child_act.sa_mask);
    child_act.sa_flags = 0;
    sigaction(SIGALRM, &child_act, NULL);

    alarm(5);

    while (callAlarm)
    {
      pause(); // 알람 울릴 때 까지 정지
    }
  }
  // 부모 프로세스
  else if (pid > 0)
  {
    child_pid = pid;

    // 부모 프로세스 SIGALRM 등록 후 시그널 처리
    parent_act.sa_handler = parent_handler;
    sigemptyset(&parent_act.sa_mask);
    parent_act.sa_flags = 0;
    sigaction(SIGALRM, &parent_act, NULL);

    sigint_act.sa_handler = sigint_handler;
    sigemptyset(&sigint_act.sa_mask);
    sigint_act.sa_flags = 0;
    sigaction(SIGINT, &sigint_act, NULL);

    child_exit_act.sa_handler = child_exit_handler;
    sigemptyset(&child_exit_act.sa_mask);
    child_exit_act.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &child_exit_act, NULL);

    alarm(2);

    while (callAlarm)
    {
      sleep(1);
    }
  }
  // fork 실패 시
  else
  {
    perror("fork error!");
    return -1;
  }

  return 0;
}

void child_handler(int signum)
{
  if (callAlarm == 1)
  {
    callChildCount++;
    printf("[Child] time out: 5, elapsed time: %d seconds (%d)\n", 5 * callChildCount, callChildCount);
  }

  if (callChildCount == 5)
    exit(5);

  alarm(5);
}

void parent_handler(int signum)
{
  if (signum == SIGALRM && callAlarm == 1)
  {
    callParentCount++;
    printf("<Parent> time out: 2, elapsed time: %d seconds\n", 2 * callParentCount);
    alarm(2);
  }
}

void sigint_handler(int signum)
{
  // SIGINT 발생 동안은 타이머 정지
  callAlarm = 0;

  // 입력 버퍼 비워서 무한 루프 방지
  getchar();

  char input;
  printf(" SIGINT: Do you want to exit (y or Y to exit)? \n");
  scanf(" %c", &input);

  // 부모 프로세스 종료
  if (input == 'y' || input == 'Y')
  {
    // 자식 프로세스에도 SIGINT 전송하여 종료
    kill(child_pid, SIGINT);
    exit(0);
  }

  // 입력 값으로 Y나 y를 입력받지 못하면 타이머 재개
  else
  {
    callAlarm = 1;
    alarm(2);
  }
}

void child_exit_handler(int signum)
{
  pid_t pid;
  int status;

  pid = waitpid(-1, &status, WNOHANG);

  // 자식 프로세스가 종료되면 해당 PID를 얻어서 상태를 출력
  if (WEXITSTATUS(status))
  {
    printf("Child id: %d, sent: %d\n", pid, WEXITSTATUS(status));
  }
}