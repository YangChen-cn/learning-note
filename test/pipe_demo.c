/*
 * 练习 10：父子进程通过 pipe 单向通信
 *
 * 目标：
 *   父进程发送一条文本消息
 *   子进程读取消息并打印
 *
 * 方向：父进程 write → pipe → 子进程 read
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int pipefd[2];
    pid_t child_pid;
    int status;

    /* TODO 1：在 fork() 之前创建 pipe。 */
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (child_pid == 0) {
        /* TODO 2：子进程关闭不使用的端点，读取并打印父进程消息。 */
        close(pipefd[1]);
        while (1) {
            char buffer[128];
            ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);
            if (bytes_read < 0) {
                perror("read");
                close(pipefd[0]);
                _exit(EXIT_FAILURE);
            } else if (bytes_read == 0) {
                /* EOF */
                break;
            }
            buffer[bytes_read] = '\0';
            printf("子进程收到消息: %s\n", buffer);
        }
        close(pipefd[0]);
        return EXIT_SUCCESS;
    }

    //父进程关闭不使用的端点，写入一条消息并关闭写端
    close(pipefd[0]);
    const char *message = "子进程我草拟马！";
   size_t total_written = 0;
   size_t len = strlen(message);
   while (total_written < len) {
    ssize_t ret = write(pipefd[1], message + total_written, len - total_written);
    if (ret < 0) {
        perror("write");
        close(pipefd[1]);
        return EXIT_FAILURE;
    }
    total_written += ret;
    }
    close(pipefd[1]);
    // 父进程等待子进程退出并打印退出状态码。 
    do {
        child_pid = waitpid(child_pid, &status, 0);
    } while (child_pid < 0 && errno == EINTR);

    if (child_pid < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
        printf("子进程退出，状态码: %d\n", WEXITSTATUS(status));
    }

    return EXIT_SUCCESS;
}
