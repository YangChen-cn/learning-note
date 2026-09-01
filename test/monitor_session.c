/*
 * 练习 8：setsid 与后台会话
 *
 * 目标：
 *   父进程创建子进程并等待
 *   子进程创建新会话，再启动已有的 monitor_args
 *
 * 这次先使用 -n 5，让测试程序自动结束，暂时不引入 kill()。
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    pid_t child_pid;
    int status;

    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (child_pid == 0) {
        /* TODO：子进程完成 setsid、标准输入输出处理和 exec。 */
        /* 目标程序：./build/monitor_args -i 1 -n 5 */
        if (setsid() < 0) {
            perror("setsid");
            _exit(EXIT_FAILURE);
        }
        int fd = open("/dev/null", O_RDWR);
        if (fd < 0) {
            perror("open");
            _exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
        execl("./build/monitor_args", "monitor_args", "-i", "1", "-n", "5", (char *)NULL);
        perror("execl");
        _exit(EXIT_FAILURE);
    }

    printf("父进程 %ld 等待会话子进程 %ld\n",
           (long)getpid(), (long)child_pid);

    do {
        child_pid = waitpid(child_pid, &status, 0);
    } while (child_pid < 0 && errno == EINTR);

    if (child_pid < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
        printf("会话子进程退出，状态码: %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("会话子进程被信号 %d 终止\n", WTERMSIG(status));
    }

    return EXIT_SUCCESS;
}
