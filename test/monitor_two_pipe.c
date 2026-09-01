/*
 * 练习 11：父子进程双向通信 + /proc 内存信息
 *
 * 功能目标：
 *   父进程发送 "meminfo" 请求；
 *   子进程读取 /proc/meminfo；
 *   子进程把 MemTotal 和 MemAvailable 返回给父进程；
 *   父进程显示返回结果并等待子进程退出。
 *
 * 通信方向：
 *   request_pipe：  父进程 write -> 子进程 read
 *   response_pipe： 子进程 write -> 父进程 read
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
    int request_pipe[2];
    int response_pipe[2];
    pid_t child_pid;
    int status;

    /* 两个 pipe 都必须在 fork() 之前创建，子进程才能继承它们。 */
    if (pipe(request_pipe) < 0) {
        perror("pipe request_pipe");
        return EXIT_FAILURE;
    }

    if (pipe(response_pipe) < 0) {
        perror("pipe response_pipe");
        close(request_pipe[0]);
        close(request_pipe[1]);
        return EXIT_FAILURE;
    }

    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        close(request_pipe[0]);
        close(request_pipe[1]);
        close(response_pipe[0]);
        close(response_pipe[1]);
        return EXIT_FAILURE;
    }

    if (child_pid == 0) {
        char request[64];
        ssize_t bytes_read;

        /* 子进程只读请求、只写响应。 */
        close(request_pipe[1]);
        close(response_pipe[0]);

        /* 读取父进程的请求，并补上字符串结尾。 */
        bytes_read = read(request_pipe[0], request, sizeof(request) - 1);
        if (bytes_read < 0) {
            perror("child read request");
            close(request_pipe[0]);
            close(response_pipe[1]);
            _exit(EXIT_FAILURE);
        }
        request[bytes_read] = '\0';

        /* TODO 1：判断请求是否为 "meminfo"。 */
        if (strcmp(request, "meminfo\n") == 0) {
            FILE *proc_file;
            char line[256];
            char name[32];
            unsigned long value;

            proc_file = fopen("/proc/meminfo", "r");
            if (proc_file == NULL) {
                perror("child fopen /proc/meminfo");
                close(request_pipe[0]);
                close(response_pipe[1]);
                _exit(EXIT_FAILURE);
            }

            while (fgets(line, sizeof(line), proc_file) != NULL) {
                /*
                 * TODO 2：只把 MemTotal 和 MemAvailable 两行写入响应 pipe。
                 * 提示：可以复用 system_monitor_learn.c 中的 sscanf/strcmp。
                 * 写入后检查 write() 返回值，并思考短写问题。
                 */
                if (sscanf(line, "%31[^:] : %lu kB", name, &value) == 2 &&(( strcmp(name, "MemTotal") == 0) || (strcmp(name, "MemAvailable") == 0))) {
                    if (write(response_pipe[1], line, strlen(line)) < 0) {
                        perror("child write response");
                        fclose(proc_file);
                        close(request_pipe[0]);
                        close(response_pipe[1]);
                        _exit(EXIT_FAILURE);
                    }
                }
            }
            if (ferror(proc_file)) {
                perror("child fgets /proc/meminfo");
                fclose(proc_file);
                close(request_pipe[0]);
                close(response_pipe[1]);
                _exit(EXIT_FAILURE);
            }

            fclose(proc_file);
        }

        /* 关闭响应写端，父进程才能通过 read() 收到 EOF。 */
        close(request_pipe[0]);
        close(response_pipe[1]);
        _exit(EXIT_SUCCESS);
    }

    /* 父进程只写请求、只读响应。 */
    close(request_pipe[0]);
    close(response_pipe[1]);

    /* 发送 "meminfo\n"，然后关闭请求写端。 */
    {
        const char *request = "meminfo\n";
        size_t request_length = strlen(request);
        ssize_t bytes_written = write(request_pipe[1], request, request_length);

        if (bytes_written < 0) {
            perror("parent write request");
            close(request_pipe[1]);
            close(response_pipe[0]);
            return EXIT_FAILURE;
        }

        if ((size_t)bytes_written != request_length) {
            fprintf(stderr, "short write for request\n");
            close(request_pipe[1]);
            close(response_pipe[0]);
            return EXIT_FAILURE;
        }
    }
    close(request_pipe[1]);

    /* TODO 3：循环读取响应，直到 read() 返回 0，并打印每次读到的内容。 */
    {
        char response[256];
        ssize_t bytes_read;

        while ((bytes_read = read(response_pipe[0], response,
                                  sizeof(response) - 1)) > 0) {
            response[bytes_read] = '\0';
            printf("父进程收到：%s", response);
        }

        if (bytes_read < 0) {
            perror("parent read response");
            close(response_pipe[0]);
            return EXIT_FAILURE;
        }
    }
    close(response_pipe[0]);

    /* 用 waitpid() 等待子进程，并检查它的退出状态。 */
    do {
        child_pid = waitpid(child_pid, &status, 0);
    } while (child_pid < 0 && errno == EINTR);

    if (child_pid < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
        printf("子进程退出，状态码：%d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("子进程被信号终止：%d\n", WTERMSIG(status));
    }

    return EXIT_SUCCESS;
}
