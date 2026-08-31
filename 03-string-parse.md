# 字符串解析

## sscanf —— 按格式从字符串里提取数据（meminfo 的核心）

```c
#include <stdio.h>

int sscanf(const char *str, const char *format, ...);
// 返回：成功转换的个数。== 2 表示两个 % 都成功了
```

常用格式：

| 格式 | 含义 |
|---|---|
| `%d` | int |
| `%lu` | unsigned long |
| `%s` | 字符串（遇空白停下） |
| `%31s` | 字符串，最多 31 字符（防溢出） |
| `%[^:]` | 一直读到 `:` 为止 |
| `%%` | 表示一个 `%` 字符 |

meminfo 里的实际用法：

```c
char name[32];
unsigned long v;

if (sscanf(line, "%31[^:] : %lu kB", name, &v) == 2 && strcmp(name, key) == 0) {
    *value = v;
    return 1;
}
```

拆解 `"%31[^:] : %lu kB"`：

1. `%31[^:]` 读键名（如 `MemTotal`，不含冒号）
2. ` `（空格）跳过冒号后面的连续空格
3. `:` 匹配冒号本身
4. ` `（空格）跳过数字前的空格
5. `%lu` 读数值
6. ` kB` 匹配单位，顺便验证行格式正确

## 相关函数

```c
#include <string.h>
int strcmp(const char *a, const char *b);      // 相等返回 0（判断相等要写 == 0！）
char *strchr(const char *s, int c);            // 找字符位置，找不到返回 NULL

#include <stdlib.h>
unsigned long strtoul(const char *s, char **endp, int base);  // 字符串转无符号数
// base 用 10；endp 传 NULL 忽略
```

## 整数运算陷阱（meminfo 最后一步）

```c
used * 100 / mem_total     // 先乘后除！整数除法会直接截断小数
```

`used / mem_total * 100` 会先算出 0 再乘 100，结果永远是 0。

```c
printf("内存使用: %d%%\n", percent);   // 输出 % 要写 %%
```

## 心得

- sscanf 是"文本行 → 变量"最顺手的方式，嵌入式里解析 /proc、日志、串口数据全靠它
- 判断 sscanf 返回值是**必须**的——格式没匹配上时，变量值是旧的/垃圾值
