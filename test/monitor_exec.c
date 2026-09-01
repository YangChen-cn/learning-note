/*
 * 练习 7：fork + exec 启动已有的监控程序
 *
 * 目标：
 *   父进程创建子进程并等待
 *   子进程使用 exec 启动 ./build/monitor_log
 *
 * 注意：exec 不会创建新进程，而是替换当前子进程正在运行的程序。
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

static volatile sig_atomic_t g_parent_stop = 0;

static void on_parent_sigint(int signum)
{
    (void)signum;
    g_parent_stop = 1;
}

int main(void)
{
    pid_t child_pid;
    int status;

    if (signal(SIGINT, on_parent_sigint) == SIG_ERR) {
    perror("signal");
    return EXIT_FAILURE;
 }
    /* TODO 1：创建子进程，并处理 fork() 的三个返回结果。 */
    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (child_pid == 0) {
        /* TODO 2：子进程用 execl() 启动 ./build/monitor_log。 */
        /* exec 成功不会返回；只有失败时才执行下面的错误处理。 */
        execl("./build/monitor_log", "monitor_log", (char *)NULL);
        perror("execl");
        _exit(EXIT_FAILURE);
    }

    printf("父进程 %ld 等待子进程 %ld\n",
           (long)getpid(), (long)child_pid);

    /* TODO 3：父进程使用 waitpid() 等待，并检查子进程退出状态。 */
    pid_t result;
    do {
        result = waitpid(child_pid, &status, 0);
    } while (result < 0 && errno == EINTR );
    if (result < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }
    if (WIFEXITED(status)) {
            printf("监控子进程退出，状态码: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("监控子进程被信号 %d 终止\n", WTERMSIG(status));
        } else {
            printf("监控子进程异常退出\n");
        }
    if (g_parent_stop) {
        printf("父进程收到 SIGINT，子进程已经结束\n");
    }
    return EXIT_SUCCESS;
}
