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

## 心得

- 读 `/proc` 这种文本 → 用 **fopen + fgets** 更合适
- **健壮性检查**：解析失败要报错退出，别拿着 0 继续算百分比（meminfo 里 `mem_total == 0` 就报错）
