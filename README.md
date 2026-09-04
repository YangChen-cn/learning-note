# Linux C 学习笔记

**目标**：打好 Linux 用户态 C 基础，最终能读懂并独立开发 `imx6u-edgepanel` 项目（ATK I.MX6U 边缘监控终端）。

- 练习代码在 `test/`
- 学习笔记在 `learning-note/`
- 每个练习只引入一个清晰的新概念，先在 Ubuntu 本机验证，再上板

## 学习路线

| 步骤 | 练习 | 新概念 | 状态 |
|---|---|---|---|
| 1 | 读 /proc/uptime 并打印 | open/read/close、perror | ✅ 完成 |
| 2 | 解析 /proc/meminfo 算内存使用率 | fopen/fgets、sscanf、整数运算陷阱 | ✅ 完成 |
| 3 | 每秒刷新显示 uptime+meminfo | 定时循环 sleep、信号 Ctrl+C | ✅ 完成 |
| 4 | 工具加参数：`-i 间隔`、`-n 次数` | getopt 命令行参数 | ✅ 完成 |
| 5 | 前台运行 + 写日志文件 | fopen 追加、fprintf、fflush | ✅ 完成 |
| 6 | fork 父子进程基础 | fork、getpid、getppid、waitpid | ✅ 完成 |
| 7 | fork + exec 启动另一个程序 | execl、execvp、进程映像替换 | ✅ 完成 |
| 8 | 后台会话 | setsid、标准输入输出重定向 | ✅ 完成 |
| 9 | 回看 system_monitor.c 的已学部分 | /proc、文件操作、解析、函数拆分 | ✅ 完成 |
| 10 | pipe 单向进程间通信 | pipe、read、write、close、EOF | ✅ 完成 |
| 11 | 两个 pipe 实现双向通信 | 请求、响应、双向数据流 | ✅ 完成 |
| 12 | Unix Domain Socket | socketpair、本机进程间全双工通信 | ✅ 完成 |
| 13 | Unix Domain Socket 服务端/客户端 | bind/listen/accept/connect、sockaddr_un、unlink | ✅ 完成 |
| 14 | I/O 多路复用 poll | poll、struct pollfd、POLLIN、事件主循环 | 🚧 **当前预习** |

## 主题笔记

- [01 /proc 文件系统](01-proc.md) — Linux 用户态最重要的数据来源
- [02 文件读写](02-file-io.md) — open/read 与 fopen/fgets 两种方式
- [03 字符串解析](03-string-parse.md) — sscanf、strcmp、strchr、strtoul
- [04 Makefile](04-makefile.md) — 构建自动化
- [05 Git 与 GitHub](05-git.md) — 版本控制与免密推送
- [06 定时循环与信号](06-sleep-signal.md) — sleep、SIGINT、volatile、ANSI 终端控制
- [07 命令行参数](07-command-args.md) — getopt、optarg、optind
- [08 文件日志基础](08-file-log.md) — FILE *、fopen 模式、fprintf、fflush、fclose
- [09 fork 父子进程](09-fork-process.md) — 进程、fork、getpid、getppid、waitpid
- [10 errno 与 EINTR](10-errno-eintr.md) — errno、perror、strerror、被信号打断的系统调用
- [11 exec 启动程序](11-exec-process.md) — exec 家族、参数、进程映像替换
- [12 setsid 与后台会话](12-setsid-session.md) — 会话、进程组、setsid、标准输入输出
- [13 system_monitor 复习](13-system-monitor-review.md) — 抽取已学的 /proc 监控部分
- [14 pipe 进程间通信](14-pipe-ipc.md) — 文件描述符、单向数据流、EOF
- [15 两个 pipe 双向通信](15-two-pipes.md) — 请求、响应、端点关闭和阻塞
- [16 Unix Domain Socket](16-unix-domain-socket.md) — socketpair、本机进程间全双工通信
- [17 Unix Socket 服务端与客户端](17-unix-socket-server.md) — 命名套接字、bind/listen/accept/connect
- [18 I/O 多路复用 poll](18-io-multiplexing-poll.md) — poll、struct pollfd、非阻塞事件驱动



## 使用约定

- 完成一个练习后更新本文件的进度表
- 遇到新函数/新概念，在对应主题文件里补一段"函数原型 + 用法 + 心得"
- 笔记里的例子优先用自己写过的代码（uptime.c、meminfo.c），理解最深

## 新练习流程

每个新练习分为两个阶段：

1. **预习阶段**：先在本目录创建对应的知识材料，学习新 API、函数原型、参数、返回值、错误处理和最小示例。
2. **练习阶段**：理解基础知识后，再创建带注释提示的代码框架，自己完成 TODO。

练习完成后，再更新进度表和心得。没有完成当前练习并确认之前，不自动进入下一个练习。

教材文件统一放在本目录中，使用连续编号命名，例如 `07-command-args.md`、`08-file-log.md`，不另建重复的学习资料目录。
