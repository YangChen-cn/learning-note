# system_monitor 已学部分复习（练习 9）

## 复习目标

原始的 `imx6u-edgepanel/src/system_monitor.c` 还包含网络接口查询、`socket()`、`ioctl()` 和 `uname()` 等尚未系统学习的内容。

本次先抽取已经学过的部分：

- `open()`、`read()`、`close()`
- `fopen()`、`fgets()`、`fclose()`
- `sscanf()`、`strcmp()`
- `/proc/uptime`
- `/proc/meminfo`
- `/proc/loadavg`
- 函数拆分和指针输出参数

## 原始文件中暂时跳过的内容

```c
socket()
ioctl()
inet_ntop()
gethostname()
uname()
```

这些内容不是当前练习的重点，之后单独学习网络和系统信息接口。

## 学习版的数据流

```text
/proc/uptime  → open → read → sscanf → close → uptime
/proc/meminfo → fopen → fgets → sscanf → fclose → total/available
/proc/loadavg → fopen → fgets → sscanf → fclose → load[3]
                                                       ↓
                                                     printf
```

## 复习重点

观察三个读取函数的共同结构：

```text
打开文件
检查返回值
读取内容
解析内容
关闭文件
把结果交给 main()
```

注意 `main()` 不直接处理所有细节，而是调用小函数。这是从练习代码走向项目代码的重要一步。

学习版代码：`test/system_monitor_learn.c`

## 完成本练习后的结果

学习版已经能够独立读取并打印 uptime、内存和 load average，并且把原项目中的网络部分隔离出来。

下一课开始学习父子进程之间如何通过 pipe() 传递数据。
