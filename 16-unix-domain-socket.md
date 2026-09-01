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

## 二、socketpair() 函数详解

函数原型：

~~~c
#include <sys/types.h>
#include <sys/socket.h>

int socketpair(int domain, int type, int protocol, int sv[2]);
~~~

### 1. 参数深度剖析

#### ① `domain`（地址族 / 通信域）
告诉内核**“通信发生的范围与环境”**：
- **`AF_UNIX`**（或 `AF_LOCAL`）：**Unix 域协议族**。专门用于**同一台主机上的本地进程间通信**。数据直接在内核内存中流转，不经过物理网卡、不需要 IP 地址和端口号，效率极高。`socketpair()` 绝大多数情况下只支持 `AF_UNIX`。
- *对比参考*：`AF_INET` 代表 IPv4 网络通信（跨机器），`AF_INET6` 代表 IPv6 网络通信。
  > **注**：`AF_` 是 **A**ddress **F**amily 的缩写；你有时会看到 `PF_`（Protocol Family），在现代 Linux 中两者数值完全等价。

#### ② `type`（套接字类型 / 数据传输模式）
告诉内核**“数据以何种规则在两者之间流动”**：
- **`SOCK_STREAM`**（流式传输）：
  - 提供**面向连接、双向、可靠、按顺序到达的字节流**。
  - **无消息边界**（像水管一样流水，多次 `write` 可能会被一次 `read` 读走，或者一次 `write` 被分多次 `read` 读走）。本课我们采用这种最常用的流模式。
- **`SOCK_DGRAM`**（数据报传输）：
  - 提供**保留消息边界**的独立数据包通信（一次 `write` 发一个包，一次 `read` 收一个包）。
- **`SOCK_CLOEXEC` / `SOCK_NONBLOCK`**（可选标志位）：
  - 可以通过位或（`|`）添加，例如 `SOCK_STREAM | SOCK_CLOEXEC`，直接在创建时开启执行 `exec` 时自动关闭或非阻塞 I/O。

| 类型 | 是否有消息边界 | 是否可靠顺序 | 典型类比 |
|---|---|---|---|
| **`SOCK_STREAM`** | ❌ 无边界（连续字节流） | ✅ 绝对可靠、严格有序 | 像自来水管，字节连续流动 |
| **`SOCK_DGRAM`** | ✅ 有边界（独立数据包） | ✅ 本地通信下可靠（网络下不可靠） | 像寄信件，一封就是一封 |

#### ③ `protocol`（具体协议）
告诉内核在给定的 `domain` 和 `type` 下使用哪种具体底层协议：
- 通常直接填 **`0`**：表示由内核**自动选择该组合下的默认协议**（在 `AF_UNIX` + `SOCK_STREAM` 组合下，`0` 就会自动选择 Unix 域流式协议）。

#### ④ `sv[2]`（输出参数：文件描述符数组）
长度为 2 的整型数组，用来**接收内核创建好的两个 socket 文件描述符**：
- `sv[0]` 和 `sv[1]`：两个完全对称、已经相互连接好的 socket 句柄。

---

### 2. 返回值与错误处理

- **成功**：返回 **`0`**，且内核将两个连通的 socket 文件描述符填入 `sv[0]` 和 `sv[1]`。
- **失败**：返回 **`-1`**，并设置全局错误码 `errno`。

标准检查代码：
~~~c
int sv[2];

if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
    perror("socketpair");
    return EXIT_FAILURE;
}
~~~

---

## 三、socketpair() 与 pipe() 的本质区别

很多初学者容易把 `socketpair()` 当成管道，两者的核心差异在于**“单向 vs 全双工”**和**“端点对称性”**：

### 1. 结构与对称性对比

| 特性 | `pipe(pipefd)` | `socketpair(..., sv)` |
| :--- | :--- | :--- |
| **方向性** | **半双工（单向）** | **全双工（双向同时可读写）** |
| **端点职责** | 严格固定：<br>`pipefd[0]` **只能读**<br>`pipefd[1]` **只能写** | **完全对称**：<br>`sv[0]` **可读也可写**<br>`sv[1]` **可读也可写** |
| **双向通信所需数量** | 需要 **2 个 pipe**（共 4 个 fd） | 只需 **1 个 socketpair**（共 2 个 fd） |
| **底层实现** | 匿名管道缓冲区 | 本地双向 Socket 套接字对 |

### 2. 全双工数据流图解

```text
               ┌────────── 写入 sv[0] ──────────┐
               │                                │
               │                                ▼
       ┌───────────────┐                ┌───────────────┐
       │     sv[0]     │                │     sv[1]     │
       │ (父进程持有)   │                │ (子进程持有)   │
       └───────────────┘                └───────────────┘
               ▲                                │
               │                                │
               └────────── 写入 sv[1] ──────────┘
```

- 向 `sv[0]` 执行 `write`，数据会流向 `sv[1]`，由对端通过 `read(sv[1])` 读取。
- 向 `sv[1]` 执行 `write`，数据会流向 `sv[0]`，由对端通过 `read(sv[0])` 读取。
- 两个方向互相独立，互不干扰。

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

### 示例代码关键细节逐行解析：

1. **`ssize_t bytes_read` 与 `ssize_t bytes_written`**：
   - 必须使用有符号的 `ssize_t`（而不是 `int` 或 `size_t`），因为 `read()` / `write()` 失败时会返回 `-1`，成功时返回正字节数。在 64 位系统下 `ssize_t` 为 8 字节，避免数据截断。
2. **`close(sockets[0])`（子进程）与 `close(sockets[1])`（父进程）**：
   - `fork()` 产生副本后，每个进程都拥有 2 个 fd。
   - 子进程必须立即关掉不属于它的 `sockets[0]`，父进程关掉 `sockets[1]`。
   - **为什么必须关？** 如果父进程不关 `sockets[1]`，那么整个系统中 `sockets[1]` 的引用计数依然是 1（父进程还占着）。当子进程退出或关闭 `sockets[1]` 时，系统认为写端并没有真正彻底关闭，父进程在 `read(sockets[0])` 时就**永远等不到 EOF（返回 0），从而导致程序永久卡死**！
3. **EOF 的产生条件**：
   - 当对端执行 `close(fd)` 后，本端调用 `read()` 读完缓冲区剩余数据后，下一次 `read()` 会**返回 `0`（代表对端已关闭，遇到 EOF）**。
4. **`_exit(EXIT_FAILURE)` 与 `fflush(stdout)`**：
   - 子进程退出推荐使用 `_exit()`（或由 `main` 返回），它不清理父进程继承的标准 I/O 缓冲区；因此在 `_exit()` 前调用 `printf()` 必须手动加 `fflush(stdout)`。

运行结果类似：

~~~text
子进程收到：status
父进程收到：running
子进程退出，状态码：0
~~~

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
