/*
 * 练习 13：Unix Domain Socket 命名客户端 (monitor_client.c)
 *
 * 功能目标：
 *   1. 使用 socket(AF_UNIX, SOCK_STREAM, 0) 创建客户端套接字 sock_fd；
 *   2. 准备 struct sockaddr_un，设置 sun_family = AF_UNIX 和目标路径 "/tmp/monitor.sock"；
 *   3. 调用 connect() 连接到服务端；
 *   4. 向服务端发送请求 "meminfo\n"；
 *   5. 循环读取服务端返回的内存信息并打印到控制台，直到服务端关闭连接（EOF，返回 0）；
 *   6. 关闭 sock_fd 并退出。
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/monitor.sock"

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
    int sock_fd;
    struct sockaddr_un addr;

    /* 1. 创建套接字 */
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    /*
     * TODO 1: 连接到服务端 (connect)
     *
     * 提示：
     *   1. 用 memset(&addr, 0, sizeof(addr)) 清空结构体；
     *   2. 设置 addr.sun_family = AF_UNIX；
     *   3. 将 SOCKET_PATH 复制到 addr.sun_path（注意长度防溢出）；
     *   4. 调用 connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr))；
     *   5. 检查返回值，如果失败：
     *      - perror("connect")
     *      - close(sock_fd)
     *      - return EXIT_FAILURE。
     */
    /* >>> 请在此处实现 TODO 1 <<< */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock_fd);
        return EXIT_FAILURE;
    }

    printf("[Client] 成功连接到监控服务端！\n");

    /*
     * TODO 2: 发送 "meminfo\n" 请求
     *
     * 提示：
     *   - 调用 write_all(sock_fd, "meminfo\n", strlen("meminfo\n"))；
     *   - 检查返回值，失败时 close(sock_fd) 并退出。
     */
    /* >>> 请在此处实现 TODO 2 <<< */
    if (write_all(sock_fd, "meminfo\n", strlen("meminfo\n")) < 0) {
        close(sock_fd);
        return EXIT_FAILURE;
    }
    printf("[Client] 已发送 meminfo 请求，正在接收响应：\n");
    printf("----------------------------------------\n");

    /*
     * TODO 3: 循环读取服务端响应并打印，直到 EOF (read 返回 0)
     *
     * 提示：
     *   - char buf[128];
     *   - ssize_t n;
     *   - while ((n = read(sock_fd, buf, sizeof(buf) - 1)) > 0) {
     *         buf[n] = '\0';
     *         printf("%s", buf);
     *     }
     *   - 循环结束后检查 n < 0 是否出错，若出错 perror("read")；
     *   - 最后 close(sock_fd)。
     */
    /* >>> 请在此处实现 TODO 3 <<< */
    char buf[128];
    ssize_t n;
    while ((n = read(sock_fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
    if (n < 0) {
        perror("read");
    }
    close(sock_fd);

    printf("----------------------------------------\n");
    printf("[Client] 接收完毕，正常退出。\n");

    return EXIT_SUCCESS;
}

