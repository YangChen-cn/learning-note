M# errno 与 EINTR（练习 6 补充知识）

## 本课目标

理解 Linux 函数失败后如何查看具体原因，并处理“系统调用被信号打断”的情况。

本课重点：

- `errno` 是什么
- `perror()` 如何使用 `errno`
- `strerror()` 如何把错误编号转换成文字
- `EINTR` 为什么会出现在 `waitpid()` 中
- 如何安全地重试被信号打断的系统调用

## 一、`errno` 是什么

`errno` 是一个错误编号，定义在：

```c
#include <errno.h>
```

很多 Linux/POSIX 函数失败时会返回特殊值，并把失败原因写入 `errno`。

例如：

```c
pid_t result = waitpid(child_pid, &status, 0);

if (result < 0) {
    printf("错误编号: %d\n", errno);
}
```

注意：`errno` 不是函数，它通常表现得像一个整数变量。它只应该在函数报告失败后读取。

不要这样判断成功：

```c
if (errno != 0) {
    /* 错误 */
}
```

正确方式是先判断函数返回值：

```c
if (result < 0) {
    /* 现在 errno 才有参考意义 */
}
```

成功调用不一定会把 `errno` 清零，所以不能单独通过 `errno` 判断最近一次调用是否成功。

## 二、`perror()`

函数原型：

```c
#include <stdio.h>

void perror(const char *message);
```

`perror()` 会读取当前的 `errno`，输出自定义说明和系统错误文字。

例如：

```c
if (fd < 0) {
    perror("open /proc/uptime");
    return EXIT_FAILURE;
}
```

可能输出：

```text
open /proc/uptime: No such file or directory
```

前半部分是你传入的说明，后半部分来自 `errno`。

所以 `perror()` 必须紧跟在失败判断之后，避免中间调用其他函数改变错误信息。

## 三、`strerror()`

函数原型：

```c
#include <string.h>

char *strerror(int error_number);
```

它把错误编号转换成字符串：

```c
fprintf(stderr, "waitpid 失败: %s\n", strerror(errno));
```

通常简单报错使用 `perror()` 就够了；需要把错误文字拼接到自己的日志格式中时，可以使用 `strerror()`。

## 四、什么是 `EINTR`

`EINTR` 的全称是“系统调用被信号中断”。

例如父进程正在等待子进程：

```c
waitpid(child_pid, &status, 0);
```

此时用户按下 Ctrl+C，信号先到达父进程，`waitpid()` 还没有完成，就可能返回：

```text
返回值: -1
errno: EINTR
```

这不一定表示 `waitpid()` 真正失败了，而是“刚才被信号打断，还没有等完”。

## 五、正确处理被打断的 `waitpid()`

基本结构：

```c
#include <errno.h>

pid_t result;

do {
    result = waitpid(child_pid, &status, 0);
} while (result < 0 && errno == EINTR);

if (result < 0) {
    perror("waitpid");
    return EXIT_FAILURE;
}
```

逻辑是：

```text
waitpid
  ├── 成功：继续处理子进程状态
  ├── EINTR：重新等待
  └── 其他错误：报告并退出
```

## 六、为什么信号处理函数只设置标志

父进程的信号处理函数应该保持简单：

```c
static volatile sig_atomic_t g_parent_stop = 0;

static void on_parent_sigint(int signum)
{
    (void)signum;
    g_parent_stop = 1;
}
```

不要在信号处理函数里调用：

- `printf()`
- `fprintf()`
- `malloc()`
- `fopen()`
- `waitpid()`

信号处理函数结束后，主流程再根据标志打印消息或处理资源。

## 七、当前 `monitor_fork.c` 的具体思路

当前程序的职责应该是：

```text
父进程：创建子进程 → 等待子进程 → 检查退出状态
子进程：运行原来的监控循环 → 收到 Ctrl+C 后退出
```

建议按这个顺序思考：

1. 加入 `<errno.h>`。
2. 把父进程的信号处理函数改成只设置一个标志，不直接 `printf()`。
3. 父进程使用 `waitpid()` 等待子进程。
4. 如果 `waitpid()` 因 `EINTR` 返回，就重新等待。
5. `waitpid()` 成功后，再在主流程中打印“父进程收到信号”或子进程退出信息。
6. 子进程继续使用原来的 `run_monitor()` 和 `on_sigint()`。

本阶段暂时不需要父进程调用 `kill()` 转发信号，因为终端 Ctrl+C 通常会同时发送给同一个前台进程组中的父子进程。

## 常见错误

- 只看 `errno`，不先检查函数返回值
- `perror()` 前先调用其他函数，导致错误原因可能被覆盖
- 把所有 `waitpid()` 返回 `-1` 都当成致命错误
- 忽略 `EINTR`，导致父进程误报等待失败
- 在信号处理函数里调用 `printf()`
- 在 `waitpid()` 完成前直接退出父进程，无法回收子进程
