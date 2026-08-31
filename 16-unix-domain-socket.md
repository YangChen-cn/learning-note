# Unix Domain Socket 本机进程间通信（下一课预习）

## 本课目标

练习 11 使用两个 pipe 实现了两个方向的数据流：

~~~
request_pipe：  父进程 → 子进程
response_pipe： 子进程 → 父进程
~~~

本课使用 `socketpair()`，把两个 pipe 改成一个全双工通信连接：

~~~
父进程 write ─────┐
父进程 read  ─────┤ socketpair
                  ├───── 子进程 read
                  └───── 子进程 write
~~~

最终仍然保留原来的实用功能：

~~~
父进程发送 meminfo 请求
子进程读取 /proc/meminfo
子进程返回内存信息
父进程显示结果
~~~

区别是：父子进程不再需要两个 pipe，每个进程只需要保留一个 socket 文件描述符，而且这个描述符可以同时读写。

## 一、Unix Domain Socket 是什么

socket 常被用于网络通信，但 Unix Domain Socket 专门用于同一台机器上的进程通信。

它的特点是：

- 不经过网卡
- 不需要 IP 地址和端口号
- 可以在本机进程之间传递字节流
- 可以使用 `read()`、`write()` 进行读写
- `SOCK_STREAM` 模式下，行为和可靠字节流类似

本课先学习 `socketpair()`。它直接创建一对已经连接好的 socket，非常适合父子进程通信。

以后学习真正的本地 socket 服务时，再学习：

~~~
socket → bind → listen → accept
                         ↑
                       connect
~~~

## 二、socketpair() 函数

函数原型：

~~~c
#include <sys/socket.h>

int socketpair(int domain, int type, int protocol, int socket_vector[2]);
~~~

参数含义：

- `domain`：地址族，本课使用 `AF_UNIX`
- `type`：通信类型，本课使用 `SOCK_STREAM`
- `protocol`：协议，通常填写 `0`，让系统选择默认协议
- `socket_vector`：长度为 2 的数组，用来接收两个 socket 文件描述符

调用成功后：

~~~c
int sockets[2];

if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
    perror("socketpair");
    return EXIT_FAILURE;
}
~~~

返回值：

- `0`：成功
- `-1`：失败，并设置 `errno`

和 pipe 不同，`sockets[0]`、`sockets[1]` 没有固定的读端和写端。两个 socket 都可以读，也都可以写。

## 三、socketpair() 和 pipe() 的区别

pipe：

~~~c
int pipefd[2];
pipe(pipefd);
~~~

通常约定：

~~~
pipefd[0]：读
pipefd[1]：写
~~~

如果要双向通信，就需要两个 pipe。

socketpair：

~~~c
int sockets[2];
socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
~~~

两个描述符都支持读写：

~~~
sockets[0]：可读、可写
sockets[1]：可读、可写
~~~

因此父子进程可以各保留一个：

~~~
父进程使用 sockets[0]
子进程使用 sockets[1]
~~~

## 四、fork() 后应该关闭什么

`socketpair()` 必须在 `fork()` 之前调用，这样子进程才能继承 socket 描述符。

调用 `fork()` 后：

~~~
父进程：sockets[0]、sockets[1]
子进程：sockets[0]、sockets[1]
~~~

父进程应该关闭子进程使用的那一个：

~~~c
close(sockets[1]);
~~~

子进程应该关闭父进程使用的那一个：

~~~c
close(sockets[0]);
~~~

最终：

~~~
父进程：只使用 sockets[0]，读写都通过它
子进程：只使用 sockets[1]，读写都通过它
~~~

这里关闭的原因和 pipe 类似：避免同一个进程意外保留对方的描述符，也方便 EOF 和资源回收行为符合预期。

## 五、最小请求和响应示例

下面是一个可以编译运行的最小请求—响应程序。它只演示 socketpair，不读取 `/proc`：

~~~c
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int sockets[2];
    pid_t child_pid;
    int status;

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
        char request[64];
        ssize_t bytes_read;
        const char response[] = "running\n";

        close(sockets[0]);

        bytes_read = read(sockets[1], request, sizeof(request) - 1);
        if (bytes_read < 0) {
            perror("child read");
            close(sockets[1]);
            _exit(EXIT_FAILURE);
        }
        request[bytes_read] = '\0';

        printf("子进程收到：%s", request);
        fflush(stdout);

        {
            ssize_t bytes_written = write(sockets[1], response,
                                          strlen(response));

            if (bytes_written < 0) {
                perror("child write");
                close(sockets[1]);
                _exit(EXIT_FAILURE);
            }

            if ((size_t)bytes_written != strlen(response)) {
                fprintf(stderr, "child short write\n");
                close(sockets[1]);
                _exit(EXIT_FAILURE);
            }
        }

        close(sockets[1]);
        _exit(EXIT_SUCCESS);
    }

    close(sockets[1]);

    {
        const char request[] = "status\n";
        char response[64];
        ssize_t bytes_written;
        ssize_t bytes_read;

        bytes_written = write(sockets[0], request, strlen(request));
        if (bytes_written < 0) {
            perror("parent write");
            close(sockets[0]);
            return EXIT_FAILURE;
        }

        if ((size_t)bytes_written != strlen(request)) {
            fprintf(stderr, "parent short write\n");
            close(sockets[0]);
            return EXIT_FAILURE;
        }

        bytes_read = read(sockets[0], response, sizeof(response) - 1);
        if (bytes_read < 0) {
            perror("parent read");
            close(sockets[0]);
            return EXIT_FAILURE;
        }
        response[bytes_read] = '\0';
        printf("父进程收到：%s", response);
    }

    close(sockets[0]);

    if (waitpid(child_pid, &status, 0) < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
        printf("子进程退出，状态码：%d\n", WEXITSTATUS(status));
    }

    return EXIT_SUCCESS;
}
~~~

数据流变成：

~~~
父进程 write(sockets[0], "status")
子进程 read(sockets[1])

子进程 write(sockets[1], "running")
父进程 read(sockets[0])
~~~

注意：这里的示例代码已经检查了短写；真实练习中还要继续考虑 `EINTR`，可以直接复用上一课的 `write_all()`。

运行结果类似：

~~~text
子进程收到：status
父进程收到：running
子进程退出，状态码：0
~~~

`fflush(stdout)` 是因为子进程最后使用 `_exit()`。`_exit()` 不会执行 C 标准库的输出缓冲刷新，所以如果不手动刷新，`printf()` 的内容可能看不到。

## 六、socketpair() 仍然是字节流

使用 `SOCK_STREAM` 时，socket 不会自动保存消息边界。

如果发送两次：

~~~c
write(fd, "hello", 5);
write(fd, "world", 5);
~~~

接收方可能一次读到：

~~~
helloworld
~~~

也可能分两次读到。因此仍然需要设计协议：

- 使用 `\n` 作为一条消息的结束
- 先发送固定长度
- 先发送长度，再发送正文
- 或者使用结构明确的二进制数据

本课继续使用短文本加换行的方式，先不引入复杂协议。

## 七、阻塞和 EOF

socket 的 `read()`、`write()` 默认也可能阻塞：

- 没有数据时，`read()` 等待
- 对方没有关闭连接时，读完现有数据后可能继续等待
- 缓冲区没有空间时，`write()` 可能等待

如果子进程写完响应后关闭自己的 socket：

~~~c
close(sockets[1]);
~~~

父进程继续读取时，读完剩余数据后会收到 `0`。

如果双方都先执行 `read()`，也可能像双 pipe 一样相互等待，形成死锁。

## 八、本课练习的具体目标

本课不会直接创建完整代码。理解教材后，下一步会创建一个少提示的框架，要求你：

1. 用 `socketpair(AF_UNIX, SOCK_STREAM, 0, sockets)` 替换两个 pipe。
2. 父进程通过 `sockets[0]` 发送 `meminfo\n`。
3. 子进程通过 `sockets[1]` 接收请求。
4. 子进程读取 `/proc/meminfo`。
5. 子进程通过同一个 `sockets[1]` 返回 `MemTotal` 和 `MemAvailable`。
6. 父进程通过同一个 `sockets[0]` 读取并显示响应。
7. 使用 `waitpid()` 回收子进程。

预期结构：

~~~
父进程 socket[0] ⇄ socket[1] 子进程
       请求  ───────────────→
       响应  ←───────────────
~~~

## 九、常见错误

- 把 `socketpair()` 当成 pipe，误以为 `[0]` 只能读、`[1]` 只能写
- 忘记在 `fork()` 前创建 socketpair
- 父子进程没有各自关闭不用的 socket 描述符
- 子进程写完响应后没有关闭 socket，父进程一直等不到 EOF
- 忘记处理 `read()` 返回 `0`
- 把一次 `write()` 当成一条完整消息
- 忘记处理短写和 `EINTR`
- 双方同时先 `read()`，导致死锁

## 十、和真正 Unix Domain Socket 服务的区别

本课使用的是 `socketpair()`，它只适合已经存在的、通常是父子关系的两个进程。

如果两个互不相关的进程也要通信，就需要使用路径名创建本地 socket：

~~~
服务端：socket → bind → listen → accept
客户端：socket → connect
~~~

那会是后续的小课。本课先掌握“一个 socket 同时读写”的核心概念。

阅读完本教材后，回复“开始练习 12”，再创建代码框架。
