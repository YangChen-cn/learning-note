# exec 启动另一个程序（练习 7）

## 本课目标

理解 `fork()` 和 `exec()` 的区别，并让父进程创建一个子进程，再由子进程启动已有的监控程序。

本课暂时不学习 `setsid()` 和后台会话。

## 一、`fork()` 和 `exec()` 的区别

```text
fork()
    创建一个新的进程

exec()
    用另一个程序替换当前进程的代码和数据
```

`fork()` 之后，父进程和子进程都继续执行当前程序；`exec()` 成功后，当前进程会变成另一个程序。

常见组合：

```text
父进程
  └── fork()
        └── 子进程 exec(另一个程序)
```

注意：`exec()` 不会创建新进程，它只替换当前进程的程序映像。进程 PID 通常保持不变。

## 二、为什么需要 `fork()` 加 `exec()`

父进程可以继续负责管理，子进程负责运行具体程序：

```text
父进程：创建、等待、检查退出状态
子进程：启动 monitor_log 程序
```

这正是 Linux 中启动外部命令的基本方式。Shell 执行命令时，也会使用类似的进程创建和程序替换机制。

## 三、`execl()`

函数原型：

```c
#include <unistd.h>

int execl(const char *path, const char *arg0, ...);
```

例子：

```c
execl("./build/monitor_log",
      "monitor_log",
      (char *)NULL);
```

参数含义：

- 第一个参数：要执行的程序路径
- 第二个参数：新程序看到的 `argv[0]`
- 后面的参数：传给新程序的命令行参数
- 最后必须用 `(char *)NULL` 结束

## 四、`exec` 成功和失败

这是必须记住的规则：

```text
exec 成功：不返回
exec 失败：返回 -1
```

所以通常这样写：

```c
execl("./build/monitor_log",
      "monitor_log",
      (char *)NULL);

perror("execl monitor_log");
_exit(EXIT_FAILURE);
```

如果 `execl()` 成功，后面的 `perror()` 和 `_exit()` 都不会执行。

## 五、为什么失败后使用 `_exit()`

子进程是通过 `fork()` 复制出来的，可能继承父进程还没有刷新的标准库缓冲区。

在 `exec()` 失败后，子进程通常使用：

```c
_exit(EXIT_FAILURE);
```

本课先记住区别：

- `exit()`：执行标准库清理和刷新
- `_exit()`：直接结束进程，不执行标准库清理

在普通 `main()` 中通常使用 `return` 或 `exit()`；在 `fork()` 后、`exec()` 失败的子进程分支中，通常使用 `_exit()`。

## 六、`exec` 家族的命名规律

常见函数：

| 函数 | 特点 |
|---|---|
| `execl` | 参数逐个写在函数调用中 |
| `execv` | 参数放在字符串数组中 |
| `execlp` | 通过 `PATH` 查找程序 |
| `execvp` | 参数数组 + 通过 `PATH` 查找 |
| `execve` | 最底层形式，可以指定环境变量 |

字母含义可以这样记：

- `l`：list，参数逐个列出
- `v`：vector，参数放在数组中
- `p`：使用 `PATH` 查找程序
- `e`：可以传入环境变量数组

本练习优先使用 `execl()`，因为程序路径和参数都比较简单。

## 七、和当前监控程序的关系

练习 6 中，子进程直接调用了：

```c
run_monitor();
```

练习 7 要改成这样的思路：

```text
父进程 fork()
        ↓
子进程不再直接调用 run_monitor()
        ↓
子进程 exec() 启动 ./build/monitor_log
        ↓
父进程 waitpid()
```
这样可以观察到：子进程的 PID 没变，但执行的程序已经变成了另一个可执行文件。

## 八、常见错误

- 误以为 `exec()` 会创建新进程
- 忘记 `exec()` 成功后不会返回
- `execl()` 的参数末尾忘记 `(char *)NULL`
- 程序路径写错却没有检查返回值
- 在 `fork()` 后的 `exec()` 失败分支使用普通 `return`，没有理解 `_exit()` 的作用
- 还没理解 `fork + exec` 就直接加入 `setsid()`

## 下一步练习

## 完成本练习后的结果

`monitor_exec.c` 已经能够：

- 由父进程创建子进程
- 由子进程使用 `execl()` 启动 `monitor_log`
- 由父进程使用 `waitpid()` 等待
- 处理 `waitpid()` 被 `SIGINT` 打断的 `EINTR`
- 检查被启动程序的退出状态

下一课学习 `setsid()`，让子进程具备脱离当前终端会话的基础。
