# 定时循环与信号处理（练习 3: monitor.c）

## 定时循环

```c
#include <unistd.h>

unsigned int sleep(unsigned int seconds);   // 睡 seconds 秒，返回剩余秒数
```

循环体最后 `sleep(1)` → 每秒执行一次循环体，这就是"定时刷新"。

要点：

- `sleep` 被信号打断时会提前返回（返回剩余秒数），练习阶段不用处理
- 需要毫秒/微秒级精度时用 `nanosleep`，遇到再查 man

## 信号：进程被"打断"的机制

```c
#include <signal.h>

sighandler_t signal(int signum, sighandler_t handler);
// 注册：以后收到 signum 信号时，内核会调用 handler
```

常用信号：

| 信号 | 触发 | 默认行为 |
|---|---|---|
| `SIGINT` | 按 Ctrl+C（2 号） | 终止进程 |
| `SIGTERM` | `kill` 命令默认发的（15 号） | 终止进程 |
| `SIGKILL` | 强杀（9 号） | 终止，**无法拦截** |

## 处理函数为什么只能"置标志"

处理函数运行在信号打断程序的**任意时刻**，此时调用 printf 等函数可能死锁或重入出错。标准做法：

```c
static volatile sig_atomic_t g_stop = 0;

static void on_sigint(int signum)
{
    (void)signum;      /* 参数是信号编号，这里用不到 */
    g_stop = 1;        /* 只做这一件事 */
}
```

```c
while (!g_stop) {      /* 主循环里检查标志 */
    ...
}
```

- `volatile`：防止编译器把这个变量优化进寄存器，导致主循环永远读不到新值
- `sig_atomic_t`：保证变量读写是原子的，不会被信号打断到一半

**"信号置标志 + 主循环检查"是监控类程序的标配骨架。**

## ANSI 转义序列（终端控制）

```c
printf("\033[2J");   /* 清屏 */
printf("\033[H");    /* 光标回左上角 */
```

每秒刷新套路：光标回左上角 + 重新打印 = 原地覆盖刷新。

```c
printf("运行时间: %02d 小时 %02d 分 %02d 秒\n", h, m, s);
/* %02d: 数字占 2 位，不足补 0，比如 07 */
```

## fflush(stdout)

`printf` 先写进缓冲区：终端下通常是"行缓冲"（遇到 `\n` 就输出），管道/文件下是全缓冲（攒满才输出）。不放心就强制刷：

```c
printf(...);
fflush(stdout);   /* 立即把缓冲区内容输出 */
```

监控类程序建议每次刷新后都加，避免画面"卡住不更新"的假象。

## 坑：循环里的状态变量要重置（练习 3 亲测）

```c
while (!g_stop) {
    unsigned long mem_total = 0;   /* 方法一：循环内声明 */
    ...
    mem_total = 0;                 /* 方法二：循环末尾清零 */
}
```

不重置的话：第二次循环的 break 条件（`mem_total != 0 && mem_free != 0`）第一行就成立，直接 break，内存数字永远不更新。

## 心得

- 三件套齐了：**读文件 → 解析 → 定时循环 + 信号退出**，这就是监控程序的骨架
- `system_monitor.c` 之后要做的 = monitor.c + 更多数据源 + 命令行参数
