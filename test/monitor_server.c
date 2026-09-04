/*
 * 练习 13：Unix Domain Socket 命名服务端 (monitor_server.c)
 *
 * 功能目标：
 *   1. 使用 socket(AF_UNIX, SOCK_STREAM, 0) 创建监听套接字 server_fd；
 *   2. 清除可能残留的旧套接字文件（unlink），并使用 bind() 绑定到 "/tmp/monitor.sock"；
 *   3. 调用 listen() 开启监听队列；
 *   4. 循环调用 accept() 等待客户端连接，得到专属通信描述符 client_fd；
 *   5. 读取客户端发来的请求，若为 "meminfo\n"，解析 /proc/meminfo 并返回 MemTotal 与 MemAvailable；
 *   6. 处理完后 close(client_fd)；
 *   7. 捕获 SIGINT (Ctrl+C)，退出时主动 unlink 清理套接字文件。
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/monitor.sock"

static volatile sig_atomic_t g_running = 1;

static void sigint_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

static int write_all(int fd, const void *buffer, size_t length)
{
    const char *current = buffer;

    while (length > 0) {
        ssize_t count = write(fd, current, length);

        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("write");
            return -1;
        }

        if (count == 0) {
            return -1;
        }

        current += count;
        length -= (size_t)count;
    }

    return 0;
}

int main(void)
{
    int server_fd;
    struct sockaddr_un addr;

    /* 注册 SIGINT 信号，按 Ctrl+C 时能够优雅退出并清理 socket 文件 */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    /* 1. 创建套接字 */
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    /*
     * TODO 1: 绑定套接字到路径 (SOCKET_PATH)
     *
     * 提示：
     *   1. 调用 unlink(SOCKET_PATH) 清理可能存在的旧文件（防止 bind 报 EADDRINUSE）；
     *   2. 用 memset(&addr, 0, sizeof(addr)) 清空结构体；
     *   3. 设置 addr.sun_family = AF_UNIX；
     *   4. 将 SOCKET_PATH 复制到 addr.sun_path（注意长度防溢出，sizeof(addr.sun_path) - 1）；
     *   5. 调用 bind(server_fd, (struct sockaddr *)&addr, sizeof(addr))，失败时 perror 并退出。
     */
    /* >>> 请在此处实现 TODO 1 <<< */
    unlink(SOCKET_PATH);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    /*
     * TODO 2: 开启监听 (listen)
     *
     * 提示：
     *   - 调用 listen(server_fd, 5)；
     *   - 检查返回值，若失败 perror("listen")，清理并退出。
     */
    /* >>> 请在此处实现 TODO 2 <<< */
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        return EXIT_FAILURE;
    }

    printf("[Server] 监控服务端已启动，监听路径：%s\n", SOCKET_PATH);
    printf("[Server] 等待客户端连接（按 Ctrl+C 停止服务端）...\n");

    /* 主服务循环：不断等待并服务客户端 */
    while (g_running) {
        int client_fd;

        /*
         * TODO 3: 接收客户端连接 (accept)
         *
         * 提示：
         *   - 调用 accept(server_fd, NULL, NULL) 获取 client_fd；
         *   - 若返回值 < 0：
         *       - 如果 errno == EINTR 说明是被 Ctrl+C 信号打断，应 break 或 continue；
         *       - 否则 perror("accept") 并 continue 重试。
         */
        /* >>> 请在此处实现 TODO 3 <<< */
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EINTR) {
                break; /* 被 Ctrl+C 打断，退出循环 */
            } else {
                perror("accept");
                continue; /* 其他错误，继续等待下一个连接 */
            }
        }

        printf("[Server] 客户端已连接！\n");

        /*
         * TODO 4: 读取客户端请求并响应
         *
         * 提示：
         *   1. 从 client_fd 读取请求数据到 char request[64]，结尾补 '\0'；
         *   2. 判断如果请求是 "meminfo\n"：
         *      - fopen 打开 "/proc/meminfo"
         *      - 逐行 fgets 读取，并用 sscanf 解析 "MemTotal" 和 "MemAvailable"
         *      - 用 write_all(client_fd, line, strlen(line)) 将这两行发送给客户端
         *      - 记得 fclose 文件
         *   3. 完成后关闭当前客户端连接：close(client_fd)。
         */
        /* >>> 请在此处实现 TODO 4 <<< */
        char request[64] = {0};
        ssize_t bytes_read = read(client_fd, request, sizeof(request) - 1);
        if (bytes_read < 0) {
            perror("read");
            close(client_fd);
            continue;
        }
        request[bytes_read] = '\0'; /* 确保字符串以 '\0' 结尾 */
        if (strcmp(request, "meminfo\n") == 0) {
            FILE *fp = fopen("/proc/meminfo", "r");
            if (!fp) {
                perror("fopen");
                close(client_fd);
                continue;
            }
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "MemTotal:", 9) == 0 || strncmp(line, "MemAvailable:", 13) == 0) {
                    if (write_all(client_fd, line, strlen(line)) < 0) {
                        perror("write_all");
                        break; /* 写入失败，退出循环 */
                    }
                }
            }
            fclose(fp);
        } else {
            const char *msg = "Unknown command\n";
            write_all(client_fd, msg, strlen(msg));
        }
        close(client_fd);

        printf("[Server] 响应完成，已断开该客户端连接，继续等待下一连接...\n");
    }

    printf("\n[Server] 正在停止服务并清理资源...\n");
    close(server_fd);
    unlink(SOCKET_PATH); /* 退出前彻底删除 socket 临时文件 */

    return EXIT_SUCCESS;
}

