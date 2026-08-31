# 文件日志基础（练习 5A）

## 本练习的目标

让监控程序在终端显示数据的同时，把每次采集结果保存到日志文件：

```text
/proc/uptime、/proc/meminfo
        ↓
      解析
        ↓
printf()       → 终端
fprintf()      → monitor.log
```

本阶段只学习**前台程序写日志**，暂时不学习 `fork()`、`setsid()` 和真正的后台守护进程。

## 一、`FILE *` 是什么

C 标准库使用 `FILE *` 表示一个已经打开的文件流：

```c
FILE *log_fp;
```

它和前面学习的文件描述符不同：

| 文件描述符方式 | 标准库文件流方式 |
|---|---|
| `int fd` | `FILE *fp` |
| `open()` | `fopen()` |
| `read()` | `fgets()`、`fscanf()`、`fread()` |
| `write()` | `fprintf()`、`fputs()`、`fwrite()` |
| `close()` | `fclose()` |

`FILE *` 内部通常还带有缓冲区，因此标准库函数使用起来更方便，但写入并不一定立即到达文件。

## 二、`fopen()` 打开文件

函数原型：

```c
#include <stdio.h>

FILE *fopen(const char *path, const char *mode);
```

返回值：

- 成功：返回文件流指针
- 失败：返回 `NULL`，并设置 `errno`

基本写法：

```c
FILE *fp = fopen("monitor.log", "a");

if (fp == NULL) {
    perror("fopen monitor.log");
    return EXIT_FAILURE;
}
```

## 三、文件打开模式

| 模式 | 文件不存在 | 文件已存在 | 写入位置 |
|---|---|---|---|
| `"r"` | 失败 | 保留 | 读取用 |
| `"w"` | 创建 | 清空原内容 | 文件开头 |
| `"a"` | 创建 | 保留原内容 | 文件末尾 |
| `"r+"` | 失败 | 保留 | 可读写 |
| `"w+"` | 创建 | 清空原内容 | 可读写 |
| `"a+"` | 创建 | 保留原内容 | 写入在末尾 |

日志通常使用：

```c
fopen("monitor.log", "a");
```

### 什么是追加模式

追加模式就是每次写入都放在文件末尾，不覆盖历史内容。

第一次运行：

```text
uptime=100 mem_used=400
```

第二次运行后：

```text
uptime=100 mem_used=400
uptime=200 mem_used=420
```

如果使用 `"w"`，第二次打开时第一行会被清空。因此日志一般不能使用 `"w"`。

## 四、`fprintf()` 写入格式化文本

函数原型：

```c
int fprintf(FILE *stream, const char *format, ...);
```

它和 `printf()` 的区别是：`fprintf()` 可以指定输出目标。

```c
printf("内存使用率: %d%%\n", percent);
```

等价于：

```c
fprintf(stdout, "内存使用率: %d%%\n", percent);
```

输出到日志文件：

```c
fprintf(log_fp, "内存使用率: %d%%\n", percent);
```

第一个参数的常见取值：

```c
stdout       /* 标准输出，通常是终端 */
stderr       /* 标准错误输出，通常也是终端 */
log_fp       /* 通过 fopen() 打开的日志文件 */
```

`fprintf()` 返回值：

- 成功：返回写入的字符数
- 失败：返回负数

基础阶段可以先这样检查：

```c
if (fprintf(log_fp, "一条日志\n") < 0) {
    perror("fprintf");
}
```

## 五、`fflush()` 和缓冲区

函数原型：

```c
int fflush(FILE *stream);
```

标准库为了提高效率，可能先把 `fprintf()` 的内容放在内存缓冲区中，稍后再真正写入文件。

```c
fprintf(log_fp, "一条日志\n");
fflush(log_fp);
```

`fflush(log_fp)` 的作用是把这个文件流中暂存的数据立刻交给操作系统。

返回值：

- 成功：`0`
- 失败：`EOF`

监控程序每轮刷新后执行 `fflush()`，便于使用另一个终端实时观察日志变化：

```sh
tail -f monitor.log
```

注意：`fflush()` 主要解决 C 标准库缓冲区问题，不等于已经物理写入磁盘。`fsync()` 是更进一步的持久化知识，本练习暂时不学。

## 六、`fclose()` 关闭文件

函数原型：

```c
int fclose(FILE *stream);
```

`fclose()` 会关闭文件流，并尝试刷新剩余缓冲区：

```c
if (fclose(log_fp) == EOF) {
    perror("fclose");
}
```

资源配对关系：

```text
fopen  → fclose
```

文件打开成功后，无论程序正常退出还是遇到错误，都应该关闭它。

## 七、最小示例

```c
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    FILE *fp = fopen("demo.log", "a");

    if (fp == NULL) {
        perror("fopen demo.log");
        return EXIT_FAILURE;
    }

    if (fprintf(fp, "程序运行了一次\n") < 0) {
        perror("fprintf");
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (fflush(fp) == EOF) {
        perror("fflush");
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (fclose(fp) == EOF) {
        perror("fclose");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

## 八、在 `monitor_log.c` 中的任务

只完成下面四件事：

1. 用 `fopen(LOG_PATH, "a")` 打开日志
2. 用 `fprintf()` 写入本轮的 uptime、内存使用量和百分比
3. 用 `fflush()` 让日志可以实时观察
4. 退出循环后用 `fclose()` 关闭日志

验证方式：

```sh
make -C test build/monitor_log
cd test
./build/monitor_log
```

另一个终端执行：

```sh
tail -f /home/yang/projects/test/monitor.log
```

按 Ctrl+C 后再次运行程序，确认旧日志仍然存在，新内容被追加到了末尾。

## 常见错误

- 把 `"a"` 写成 `"w"`，导致旧日志被清空
- 忘记检查 `fopen()` 是否返回 `NULL`
- 把 `fprintf()` 的第一个参数漏掉
- 忘记 `fflush()`，导致 `tail -f` 看不到最新内容
- 忘记 `fclose()`，造成资源泄漏
- 从项目根目录运行时，`monitor.log` 会出现在当前工作目录，而不一定是 `test/`

## 完成本练习后的结果

`monitor_log.c` 已经能够：

- 在终端显示 uptime 和内存使用率
- 使用追加模式保存多轮监控结果
- 使用 `fflush()` 让日志可以实时观察
- 收到 Ctrl+C 后关闭日志文件并退出

下一课再学习如何创建子进程；暂时不要把 `fork()` 直接加入这个文件。
