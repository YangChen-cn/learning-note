# 文件读写（两种方式）

## 方式一：系统调用 open / read / close（练习 1 uptime.c）

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>     // open
#include <unistd.h>    // read, close

int open(const char *path, int flags);           // flags: O_RDONLY 只读
ssize_t read(int fd, void *buf, size_t count);   // 返回实际读到的字节数，-1 出错
int close(int fd);
```

要点：

- `read` **不保证一次读满**，返回 `n` 是实际读到的字节数
- 读文本时要自己补 `'\0'` 结尾才能当字符串打印：`buffer[n] = '\0'`

## 方式二：标准库 fopen / fgets / fclose（练习 2 meminfo.c）

```c
#include <stdio.h>

FILE *fopen(const char *path, const char *mode); // "r" 只读；失败返回 NULL
char *fgets(char *s, int size, FILE *stream);    // 每次读一行（含换行）；读完返回 NULL
int fclose(FILE *fp);
```

要点：

- `fgets` 天然**按行**处理文本，比 read 省心，读 /proc 这种文本文件优先用它
- 行 buffer 要够大（如 256），太小会截断一行

## 错误处理套路（两个练习都在用）

```c
fp = fopen(MEMINFO_PATH, "r");
if (fp == NULL) {
    perror("fopen");        // 打印 "fopen: 具体错误原因"
    exit(EXIT_FAILURE);     // 非 0 退出，表示程序出错
}
```

- `perror`：根据全局变量 `errno` 打印 `你传的字符串: 错误描述`
- `exit(EXIT_FAILURE)` = 退出码 1；`return 0` / `EXIT_SUCCESS` = 成功

## 系统调用 vs C 标准库的核心区别

### 1. 层次结构与缓冲机制

```text
┌─────────────────────────────────────────────────────────┐
│                     你的应用程序代码                      │
└────────────┬───────────────────────────────┬────────────┘
             │ (读写文本/日志/格式化)            │ (操作硬件设备/套接字/管道)
             ▼                               │
┌─────────────────────────────┐              │
│    C 标准库 (libc / stdio)   │              │
│  fopen / fgets / fprintf    │              │
│   【自带用户态缓冲区 buffer】  │              │
└────────────┬────────────────┘              │
             │ 底层最终调用                    │ 直接调用
             ▼                               ▼
═════════════╪═══════════════════════════════╪═════════════ [ 用户态 / 内核态 分界线 ]
             ▼                               ▼
┌─────────────────────────────────────────────────────────┐
│                    Linux 内核系统调用                    │
│                 open / read / write / close             │
│                      (操作文件描述符 fd)                  │
└────────────────────────────┬────────────────────────────┘
                             ▼
                    VFS 虚拟文件系统 / 设备驱动
```

- **系统调用（Unbuffered）**：每次调用直接触发陷入内核（软中断/syscall），频繁小数据读写时上下文切换开销大。
- **标准库（Buffered）**：在用户态维护内存缓冲区（通常 4KB~8KB），攒够一批数据才调用一次系统调用，大幅降低内核态切换开销。
  - 注：`FILE *` 内部包含了对应的 `int fd`，可通过 `fileno(fp)` 获取底层文件描述符。

### 2. 特性对比表

| 维度 | 系统调用 (`open`/`read`/`close`) | 标准库 (`fopen`/`fgets`/`fclose`) |
| :--- | :--- | :--- |
| **所属标准** | POSIX / Linux 内核接口 | ANSI C / ISO C 标准运行时库 |
| **数据句柄** | **文件描述符**（`int fd`，非负整数） | **文件流指针**（`FILE *`，结构体指针） |
| **缓冲机制** | **无缓冲**（直接陷入内核） | **带用户态缓冲**（减少系统调用次数） |
| **数据抽象** | 只处理**原始字节流（Raw Bytes）** | 提供**行（`fgets`）**、**格式化（`fprintf`）**等高级抽象 |
| **文本结尾** | 读入后**不补 `\0`**，须手动 `buf[n] = '\0'` | `fgets` 自动按行读取并补 `\0` 字符串结束符 |
| **适用平台** | POSIX / UNIX 系统专用 | 跨平台通用（Linux / Windows / RTOS） |

---

## 选型建议与心得

### 什么时候用标准库（`fopen` / `fgets` / `fprintf`）？
- **解析文本文件**：读 `/proc` 文本（如 `/proc/meminfo`），用 `fgets` 逐行读取最省心，避免自己解析换行符。
- **记录运行日志**：如 `monitor_log.c`，使用 `fprintf` 格式化时间戳与数据，配合 `fflush` 控制落盘。
- **配置文件读写**：`.ini`、`.json`、`.txt` 等结构化文本。

### 什么时候必须用系统调用（`open` / `read` / `write`）？
- **设备驱动与硬件交互**：嵌入式 Linux 中操作 `/dev/fb0`（显存 Framebuffer）、`/dev/input/event0`（触摸屏）、`/dev/i2c-1`、GPIO 等设备节点。
- **进程间通信与网络 I/O**：`pipe`、`socket`、`socketpair` 原生返回的就是 `int fd`。
- **底层 I/O 精细控制**：非阻塞模式（`O_NONBLOCK`）、原子追加（`O_APPEND`）、异步通知或 `ioctl` 控制。

### 核心心得
- 读 `/proc` 这种文本 → 用 **fopen + fgets** 更合适。
- **健壮性检查**：解析失败要报错退出，别拿着 0 继续算百分比（meminfo 里 `mem_total == 0` 就报错）。

