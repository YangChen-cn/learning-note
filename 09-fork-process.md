# fork 父子进程基础（练习 6）

## 本课目标

理解 Linux 中“一个程序如何创建出另一个进程”，并写一个最小的父子进程练习。

本课只学习：

- 进程是什么
- `fork()` 如何创建子进程
- `getpid()` 和 `getppid()` 如何查看进程关系
- `waitpid()` 如何让父进程等待子进程

本课暂时不学习 `setsid()`、守护进程和后台日志。那些内容放在下一课。

## 一、进程是什么
程序是磁盘上的代码和数据，进程是程序运行起来后的实例。

一个进程通常有：

- 自己的进程 ID（PID）
- 自己的内存空间
- 打开的文件描述符或文件流
- 当前执行位置

查看当前进程 ID：

```c
#include <unistd.h>

pid_t pid = getpid();
```

查看父进程 ID：

```c
pid_t parent_pid = getppid();
```

`pid_t` 是 Linux 用来表示进程 ID 的类型。

## 二、`fork()` 创建子进程

函数原型：

```c
#include <sys/types.h>
#include <unistd.h>

pid_t fork(void);
```

调用 `fork()` 后，系统会创建一个子进程。调用点之后，父进程和子进程都会继续执行后面的代码。

最重要的是：同一次 `fork()`，父子进程得到不同的返回值。

| 返回值 | 哪个进程收到 | 含义 |
|---|---|---|
| `-1` | 父进程 | 创建失败，没有子进程 |
| `0` | 子进程 | 当前正在子进程中 |
| 大于 `0` | 父进程 | 返回新创建子进程的 PID |

典型结构：

```c
pid_t pid = fork();

if (pid < 0) {
    perror("fork");
    return EXIT_FAILURE;
}

if (pid == 0) {
    /* 子进程代码 */
} else {
    /* 父进程代码，pid 是子进程 PID */
}
```

注意：`fork()` 不是“跳转”，而是让一个执行流变成两个执行流。

## 三、父子进程的内存

`fork()` 之后，子进程一开始看起来拥有父进程当时的变量副本：

```c
int value = 10;
pid_t pid = fork();

if (pid == 0) {
    value = 20;
}
```

子进程修改自己的 `value`，不会直接修改父进程的 `value`。两者是不同进程，各自拥有自己的地址空间。

可以先把它理解为“复制出一份独立的运行状态”。Linux 实际使用写时复制等优化，但本课暂时不深入内核实现。

## 四、`waitpid()` 等待子进程

如果父进程创建子进程后立即结束，子进程可能仍然继续运行。父进程通常需要等待和回收子进程，避免产生僵尸进程。

函数原型：

```c
#include <sys/types.h>
#include <sys/wait.h>

pid_t waitpid(pid_t pid, int *status, int options);
```

最常见的用法：

```c
int status;

if (waitpid(child_pid, &status, 0) < 0) {
    perror("waitpid");
}
```

参数含义：

- `child_pid`：要等待的子进程 PID
- `&status`：保存子进程退出状态
- `0`：没有特殊选项，阻塞等待

检查子进程是否正常退出：

```c
#include <sys/wait.h>

if (WIFEXITED(status)) {
    printf("子进程退出码: %d\n", WEXITSTATUS(status));
}
```

## 五、最小父子进程示例

```c
pid_t child_pid = fork();

if (child_pid < 0) {
    perror("fork");
    return EXIT_FAILURE;
}

if (child_pid == 0) {
    printf("子进程: pid=%ld, ppid=%ld\n",
           (long)getpid(), (long)getppid());
    return 7;
}

printf("父进程: pid=%ld, child=%ld\n",
       (long)getpid(), (long)child_pid);

int status;
if (waitpid(child_pid, &status, 0) < 0) {
    perror("waitpid");
    return EXIT_FAILURE;
}

if (WIFEXITED(status)) {
    printf("子进程退出码: %d\n", WEXITSTATUS(status));
}
```

父进程和子进程的输出顺序不一定固定，因为它们由操作系统分别调度。

## 六、和当前项目的关系

后续可以让：

- 父进程负责管理和等待
- 子进程负责执行监控或写日志

但是当前不要直接改 `monitor_log.c`。先通过一个独立的 `fork_demo.c` 观察父子进程关系，理解 `fork()` 的返回值，再考虑后台运行。

## 七、常见错误

- 忘记检查 `fork()` 是否返回 `-1`
- 以为 `fork()` 只执行一次后面的代码
- 忘记区分父进程中的返回值和子进程中的返回值
- 父进程不调用 `waitpid()`，导致子进程退出后留下僵尸状态
- 认为父进程和子进程会按固定顺序打印
- 在还没理解 `fork()` 前直接加入 `setsid()`、重定向和后台运行

## 下一步练习

## 完成本练习后的结果

`monitor_fork.c` 已经能够：

- 由父进程创建子进程
- 由子进程运行原来的监控和日志逻辑
- 由父进程使用 `waitpid()` 等待子进程
- 处理 `waitpid()` 被 `SIGINT` 打断的 `EINTR`
- 输出子进程的正常退出状态

下一课先学习如何让子进程使用 `exec()` 启动另一个程序，之后再学习如何脱离当前终端会话。
