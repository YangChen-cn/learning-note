# setsid 与后台会话（练习 8）

## 本课目标

理解一个进程为什么还“挂在终端上”，以及如何创建一个新的会话，让程序具备后台运行的基础。

本课暂时不做完整守护进程，也不修改启动项。

## 一、`fork()` 和后台运行不是一回事

`fork()` 只负责创建新进程。子进程仍可能属于当前终端的进程组，并继续使用终端相关的文件描述符。

```text
fork()       → 创建子进程
setsid()     → 创建新会话，脱离控制终端
dup2()       → 重定向标准输入输出
```

## 二、会话和进程组

```text
会话 Session
  └── 进程组 Process Group
        └── 进程 Process
```

终端按下 Ctrl+C 时，信号通常会发送给当前前台进程组。这也是父子进程都可能收到 `SIGINT` 的原因。

## 三、`setsid()`

函数原型：

```c
#include <unistd.h>

pid_t setsid(void);
```

成功后，调用进程会创建新会话、成为会话首进程和新进程组组长，并脱离原控制终端。

失败返回 `-1`，可以用 `perror()` 查看原因：

```c
if (setsid() < 0) {
    perror("setsid");
    return EXIT_FAILURE;
}
```

如果调用者已经是进程组组长，`setsid()` 可能失败。因此通常让 `fork()` 后的子进程调用它。

## 四、标准输入输出

`setsid()` 不会自动关闭已经打开的文件描述符。后台程序通常还需要处理：

```text
stdin  → fd 0
stdout → fd 1
stderr → fd 2
```

后续可以打开 `/dev/null`，再使用 `dup2()` 重定向：

```c
int fd = open("/dev/null", O_RDWR);
dup2(fd, STDIN_FILENO);
dup2(fd, STDOUT_FILENO);
dup2(fd, STDERR_FILENO);
```

## 五、下一步

练习 7 完成后，再创建基于 `monitor_exec.c` 的 `monitor_session.c`，在子进程中学习 `setsid()`。

## 完成本练习后的结果

`monitor_session.c` 已经能够：

- 创建新的会话
- 重定向子进程的标准输入、输出和错误输出
- 使用 `exec()` 启动已有的监控程序
- 由父进程等待会话子进程结束

下一步先复习 `system_monitor.c` 中已经学过的 `/proc` 和文件操作部分，网络代码暂时跳过。
