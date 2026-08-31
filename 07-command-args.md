# 命令行参数（练习 4）

## 目标

让程序可以从命令行接收刷新间隔和刷新次数：

```sh
./build/monitor_args -i 2 -n 5
```

## getopt

```c
#include <unistd.h>

int getopt(int argc, char * const argv[], const char *optstring);
```

选项字符串中的冒号表示该选项必须带参数：

```c
getopt(argc, argv, "i:n:h")
```

- `-i` 后面必须有参数，例如 `-i 2`
- `-n` 后面必须有参数，例如 `-n 5`
- `-h` 不需要参数

循环中：

```c
while ((opt = getopt(argc, argv, "i:n:h")) != -1) {
    switch (opt) {
    case 'i':
        /* optarg 指向 -i 后面的字符串 */
        break;
    }
}
```

- `opt`：当前选项字母
- `optarg`：当前选项携带的字符串参数
- `optind`：下一个待处理的参数下标

## 字符串转整数

`optarg` 是字符串，不能直接当成整数使用。练习中使用：

```c
int value = (int)strtol(optarg, NULL, 10);
```

实际程序还应该检查转换是否成功，以及数值是否大于 0。

## 心得

命令行参数让同一个程序可以在不同场景下运行，而不需要修改源码重新编译。嵌入式设备上的测试工具经常使用这种方式调整采样间隔、输出次数和设备路径。
