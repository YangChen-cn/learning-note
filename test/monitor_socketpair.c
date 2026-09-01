/*
 * 练习 12：Unix Domain Socket（socketpair）全双工通信 + /proc 内存信息
 *
 * 功能目标：
 *   1. 使用 socketpair() 替代练习 11 中的两个 pipe，建立一对全双工连接；
 *   2. 父进程通过 sockets[0] 发送 "meminfo\n" 请求；
 *   3. 子进程通过 sockets[1] 接收请求，解析 /proc/meminfo；
 *   4. 子进程通过同一个 sockets[1] 把 MemTotal 和 MemAvailable 返回给父进程；
 *   5. 父进程通过同一个 sockets[0] 循环读取响应并打印，直到对端关闭（EOF）；
 *   6. 父进程使用 waitpid() 回收子进程。
 *
 * 全双工数据流：
 *   sockets[0] (父进程) ⇄ sockets[1] (子进程)
 *     请求数据流：父进程 write(sockets[0]) -> 子进程 read(sockets[1])
 *     响应数据流：子进程 write(sockets[1]) -> 父进程 read(sockets[0])
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int write_all(int fd, const void *buffer, size_t length);

int main(void)
{
    int sockets[2];
    pid_t child_pid;
    int status;

    /*
     * TODO 1: 创建全双工套接字对 (socketpair)
     *
     * 提示：
     *   - domain: AF_UNIX（本地通信）
     *   - type: SOCK_STREAM（流式套接字）
     *   - protocol: 0（系统默认协议）
     *   - sv: sockets
     *   - 检查返回值，如果失败打印 perror("socketpair") 并返回 EXIT_FAILURE。
     */
    /* >>> 请在此处实现 TODO 1 <<< */
   if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
    perror("socketpair");
    return EXIT_FAILURE;
    }
    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        close(sockets[0]);
        close(sockets[1]);
        return EXIT_FAILURE;
    }

    if (child_pid == 0) {
        /*
         * 子进程逻辑：
         *   - 子进程只使用 sockets[1] 进行全双工通信（既读请求又写响应）。
         *   - 必须立即关闭不属于子进程的 sockets[0]，避免引用计数残留导致无法触发 EOF。
         */
        close(sockets[0]);

        char buff[64];
        ssize_t bytes_read;

        /*
         * TODO 2.1: 从 sockets[1] 读取父进程发来的请求
         *
         * 提示：
         *   - 使用 read(sockets[1], buff, sizeof(buff) - 1)
         *   - 检查返回值是否 < 0；若出错 perror("child read request") 并关闭 sockets[1] 退出
         *   - 读取成功后在结尾补 '\0'，以便当做字符串处理
         */
        /* >>> 请在此处实现 TODO 2.1 <<< */
        if ((bytes_read = read(sockets[1], buff, sizeof(buff) - 1)) < 0) {
            perror("child read request");
            close(sockets[1]);
            _exit(EXIT_FAILURE);
        }
        buff[bytes_read] = '\0';

        /*
         * TODO 2.2: 判断请求是否为 "meminfo\n"
         *   - 若匹配，打开 "/proc/meminfo"（使用 fopen）
         *   - 逐行读取（fgets），使用 sscanf 解析 MemTotal 与 MemAvailable
         *   - 将匹配到的行通过 write(sockets[1], line, strlen(line)) 发送给父进程
         *   - 记得 fclose 文件
         */
        /* >>> 请在此处实现 TODO 2.2 <<< */
        if(strcmp(buff, "meminfo\n") == 0) {
            FILE *fp = fopen("/proc/meminfo", "r");
            if (fp == NULL) {
                perror("fopen /proc/meminfo");
                close(sockets[1]);
                _exit(EXIT_FAILURE);   
            }
            char line[128];
            char name[32];
            unsigned long value_kb;
            while (fgets(line, sizeof(line), fp) != NULL) {
                if((sscanf(line, "%31[^:] : %lu kB", name, &value_kb) == 2)&& (strcmp(name, "MemTotal") == 0 || strcmp(name, "MemAvailable") == 0)) {
                    if (write_all(sockets[1], line, strlen(line)) < 0) {
                        perror("child write response");
                        fclose(fp);
                        close(sockets[1]);
                        _exit(EXIT_FAILURE);
                    }
                }
            }
            fclose(fp);
        }

        /*
         * 子进程处理完成：
         *   - 关闭 sockets[1]，使父进程的 read() 能够收到 EOF（返回 0）。
         *   - 调用 _exit(EXIT_SUCCESS) 退出。
         */
        close(sockets[1]);
        _exit(EXIT_SUCCESS);
    }

    /*
     * 父进程逻辑：
     *   - 父进程只使用 sockets[0] 进行全双工通信（既写请求又读响应）。
     *   - 必须立即关闭不属于父进程的 sockets[1]。
     */
    close(sockets[1]);

    /*
     * TODO 3.1: 向 sockets[0] 发送请求 "meminfo\n"
     *
     * 提示：
     *   - 使用 write(sockets[0], "meminfo\n", strlen("meminfo\n"))
     *   - 检查返回值，处理错误与短写
     */
    /* >>> 请在此处实现 TODO 3.1 <<< */
    char request[] = "meminfo\n";
    if (write_all(sockets[0], request, strlen(request)) < 0) {
        close(sockets[0]);
        return EXIT_FAILURE;
    }

    /*
     * TODO 3.2: 循环从 sockets[0] 读取子进程的响应，直到 EOF (read 返回 0)
     *
     * 提示：
     *   - char response[256];
     *   - ssize_t n;
     *   - while ((n = read(sockets[0], response, sizeof(response) - 1)) > 0) { ... }
     *   - 每次读到数据后补 '\0'，并打印输出
     *   - 循环结束后检查 n < 0 是否出错
     */
    /* >>> 请在此处实现 TODO 3.2 <<< */
    char response[128];
    ssize_t n;
    while ((n = read(sockets[0], response, sizeof(response) - 1)) > 0) {
        response[n] = '\0';
        printf("%s", response);
    }
    if (n < 0) {
        perror("parent read response");
        close(sockets[0]);
        return EXIT_FAILURE;
    }
    /* 等待并回收子进程状态 */
    do {
        child_pid = waitpid(child_pid, &status, 0);
    } while (child_pid < 0 && errno == EINTR);

    if (child_pid < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
        printf("子进程正常退出，退出码：%d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("子进程被信号终止，信号：%d\n", WTERMSIG(status));
    }

    return EXIT_SUCCESS;
}

static int write_all(int fd, const void *buffer, size_t length)
{
    const char *current = buffer;

    while (length > 0) {
        ssize_t count = write(fd, current, length);

        if (count < 0) {
            if (errno == EINTR) {
                /* 被信号打断，没有写入数据，重新尝试。 */
                continue;
            }

            perror("write");
            return -1;
        }

        if (count == 0) {
            fprintf(stderr, "write returned 0\n");
            return -1;
        }

        current += count;
        length -= (size_t)count;
    }

    return 0;
}