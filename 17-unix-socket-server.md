# Unix Domain Socket 命名服务端与客户端（练习 13 预习）

## 本课目标

在练习 12 中，我们学习了 `socketpair()`，它能够在一个父子进程对之间建立全双工通信。

但 `socketpair()` 有一个根本局限：**通信双方必须是具有亲缘关系的进程（必须通过 `fork()` 继承描述符）**。

而在真实嵌入式系统（如 `imx6u-edgepanel`）中：
- **监控服务端（Daemon）**：开机后在后台长期运行。
- **查询客户端（CLI / UI）**：用户随时打开终端运行一个命令（如 `monitor_cli`），查询当前的 CPU、内存状态后退出。

这两个进程**完全独立、互不相关**。本课的目标就是学习使用 **命名 Unix Domain Socket（通过文件路径名连接）** 实现独立的 Client-Server 架构。

```text
┌────────────────────────┐                   ┌────────────────────────┐
│  客户端 (monitor_cli)  │                   │  服务端 (monitor_srv)  │
│      独立进程 A        │                   │      独立守护进程 B    │
└───────────┬────────────┘                   └───────────┬────────────┘
            │                                            │
            │  1. connect("/tmp/monitor.sock")           │ 1. socket() -> bind("/tmp/monitor.sock")
            ├───────────────────────────────────────────►│ 2. listen() -> accept() 得到 client_fd
            │                                            │
            │  2. write: "meminfo\n"                     │
            ├───────────────────────────────────────────►│ 3. read: 读取请求
            │                                            │    读取 /proc/meminfo
            │  4. read: 接收内存信息                     │
            │◄───────────────────────────────────────────┤ 4. write: 发送响应
            │                                            │
            │  5. close() 退出                           │ 5. close(client_fd)，继续等待下一个连接
            ▼                                            ▼
```

---

## 一、核心结构体：`struct sockaddr_un`

与网络 Socket 使用 IP+端口（`sockaddr_in`）不同，Unix 域套接字使用**文件系统路径**作为地址。

定义在 `<sys/un.h>`：

```c
#include <sys/un.h>

struct sockaddr_un {
    sa_family_t sun_family;       // 地址族，固定填写 AF_UNIX
    char        sun_path[108];    // 套接字文件在文件系统中的路径名（以 '\0' 结尾）
};
```

使用套路：
```c
struct sockaddr_un addr;
memset(&addr, 0, sizeof(addr));
addr.sun_family = AF_UNIX;
strncpy(addr.sun_path, "/tmp/monitor.sock", sizeof(addr.sun_path) - 1);
```

---

## 二、服务端核心流程与 API

服务端扮演“监听者”角色，标准生命周期分为 5 步：

```text
socket() ──► unlink() ──► bind() ──► listen() ──► accept() ──► read/write ──► close(client_fd)
                                                     ▲                               │
                                                     └───────────────────────────────┘
```

### 1. `socket()`：创建监听套接字
```c
int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
```
- `AF_UNIX`：本地通信。
- `SOCK_STREAM`：流式连接。

### 2. `unlink()`：清理残留的套接字文件
```c
unlink("/tmp/monitor.sock");
```
- **为什么必须调用？** 如果程序上次异常退出，`/tmp/monitor.sock` 文件可能残留在磁盘上。如果直接 `bind()`，会报错 `EADDRINUSE`（Address already in use）。因此在 `bind` 前先主动删除旧文件。

### 3. `bind()`：将套接字绑定到文件路径
```c
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```
- 将 `server_fd` 与 `/tmp/monitor.sock` 绑定。执行成功后，文件系统里会出现一个类型为 `s`（Socket）的特殊文件。

### 4. `listen()`：开启监听队列
```c
int listen(int sockfd, int backlog);
```
- `backlog`：等待处理的连接请求队列最大长度（如 `5` 或 `10`）。

### 5. `accept()`：接收客户端连接
```c
int client_fd = accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```
- **阻塞等待**：直到有客户端执行 `connect()` 连接进来。
- **返回值**：成功返回一个**全新的文件描述符 `client_fd`**，专门用于与该客户端进行数据收发。原 `server_fd` 继续保留用于监听后续新连接。

---

## 三、客户端核心流程与 API

客户端扮演“请求发起者”角色，流程非常简单：

```text
socket() ──► connect() ──► write(请求) ──► read(响应) ──► close()
```

### 1. `connect()`：主动连接服务端
```c
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```
- 客户端填入服务端的路径 `/tmp/monitor.sock`，发起连接。
- 成功返回 `0`；如果服务端没启动或路径不存在，返回 `-1`（`errno == ENOENT` 或 `ECONNREFUSED`）。

---

## 四、最小 Client-Server 示例

### 1. 服务端最小实现 (`server_demo.c`)
```c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/demo.sock"

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_un addr;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    unlink(SOCKET_PATH); // 清理旧文件

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen"); return 1;
    }

    printf("服务端启动，监听: %s\n", SOCKET_PATH);

    // 接收一次客户端连接
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd >= 0) {
        char buf[64];
        ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("收到客户端请求: %s", buf);
            write(client_fd, "pong\n", 5);
        }
        close(client_fd);
    }

    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}
```

### 2. 客户端最小实现 (`client_demo.c`)
```c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/demo.sock"

int main(void) {
    int sock_fd;
    struct sockaddr_un addr;
    char buf[64];

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) { perror("socket"); return 1; }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); return 1;
    }

    write(sock_fd, "ping\n", 5);

    ssize_t n = read(sock_fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("收到服务端响应: %s", buf);
    }

    close(sock_fd);
    return 0;
}
```

---

## 五、常见错误与避坑指南

1. **忘记 `unlink(SOCKET_PATH)`**：
   - 现象：第二次运行服务端时报 `bind: Address already in use`。
   - 解决：在 `bind()` 之前显式调用 `unlink(SOCKET_PATH)`；在服务端正常退出时也调用 `unlink`。
2. **混淆 `server_fd` 与 `client_fd`**：
   - `server_fd` 只用于 `listen()` 和 `accept()`，**绝不能拿它去 `read`/`write` 数据**。
   - 数据通信必须使用 `accept()` 返回的 `client_fd`。
3. **路径过长溢出**：
   - `sun_path` 通常只有 108 字节，不要使用过深的目录路径。
4. **客户端连接时服务端未启动**：
   - 现象：`connect` 报 `No such file or directory` 或 `Connection refused`。
   - 解决：必须先启动服务端建立监听文件，客户端才能连接。

---

## 六、本课练习目标（练习 13）

我们将编写两个独立的程序：
1. **`monitor_server.c`**：
   - 绑定并监听 `/tmp/imx6u_monitor.sock`。
   - 循环 `accept()` 客户端连接。
   - 收到 `"meminfo\n"` 时解析 `/proc/meminfo` 并返回结果给客户端。
2. **`monitor_client.c`**：
   - 连接 `/tmp/imx6u_monitor.sock`。
   - 发送 `"meminfo\n"` 并打印收到的内存指标。
