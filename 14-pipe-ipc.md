# pipe 单向进程间通信（练习 10）

## 本课目标

让父进程和子进程通过一个内核通信通道传递数据。

之前学过：

~~~
fork()    创建进程
exec()    启动另一个程序
waitpid() 等待进程
~~~

本课学习最简单的 IPC：pipe()。

## 一、pipe() 是什么

函数原型：

~~~
#include <unistd.h>

int pipe(int pipefd[2]);
~~~

成功后得到两个文件描述符：

~~~
pipefd[0]：读端
pipefd[1]：写端
~~~

数据流方向固定：

~~~
write(pipefd[1], ...) → pipe → read(pipefd[0], ...)
~~~

返回值：成功为 0，失败为 -1，并设置 errno。

## 二、为什么父子进程能使用 pipe

通常先创建 pipe，再 fork：

~~~
父进程创建 pipe
        ↓
      fork()
        ↓
父子进程都继承这两个文件描述符
~~~

于是可以形成：

~~~
父进程 write → pipe → 子进程 read
~~~

## 三、为什么要关闭不使用的一端

如果父进程写、子进程读：

~~~
父进程：close(pipefd[0]); write(pipefd[1], ...); close(pipefd[1]);
子进程：close(pipefd[1]); read(pipefd[0], ...);  close(pipefd[0]);
~~~

关闭不使用的端点既是资源管理，也是通信逻辑的一部分。

## 四、read() 的阻塞和 EOF

如果 pipe 没有数据，但仍有写端打开，read() 可能阻塞等待。

当所有写端都关闭，并且 pipe 中的数据已经读完，read() 返回 0：

~~~
ssize_t count = read(pipefd[0], buffer, sizeof(buffer));

if (count == 0) {
    /* 收到 EOF，写端全部关闭 */
}
~~~

如果父进程忘记关闭写端，子进程可能永远等不到 EOF。

## 五、返回值

~~~
ssize_t written = write(fd, data, length);
ssize_t received = read(fd, buffer, sizeof(buffer));
~~~

- 大于 0：实际读写的字节数
- 0：read() 读到 EOF
- -1：发生错误，查看 errno

一次 read() 不一定得到一次 write() 的全部内容。本练习先传递一条短文本消息。

## 六、write() 可能只写入一部分

不要假设一次 write() 一定写完全部数据：

~~~
size_t total_written = 0;
size_t length = strlen(message);

while (total_written < length) {
    ssize_t count = write(pipefd[1],
                          message + total_written,
                          length - total_written);

    if (count < 0) {
        perror("write");
        break;
    }

    total_written += (size_t)count;
}
~~~

这里的三个关键值是：

- `message + total_written`：从还没有写完的位置继续
- `length - total_written`：本次最多还需要写多少字节
- `total_written += count`：累计已经写入的字节数

### 常见重复写入错误

下面的代码会把同一条消息写两遍：

~~~
write(pipefd[1], message, strlen(message));

size_t total_written = 0;
while (total_written < strlen(message)) {
    total_written += write(pipefd[1],
                           message + total_written,
                           strlen(message) - total_written);
}
~~~

原因是第一次 write() 已经写入了完整消息，但循环中的 total_written 仍然是 0，于是循环又从消息开头写了一遍。

正确做法是删除前面的单独 write()，只使用完整写循环；或者把 total_written 初始化为第一次实际写入的字节数。

## 七、pipe 没有消息边界

pipe 传递的是字节流，不会记录这是第几次 write()。例如：

~~~
write(pipefd[1], "hello", 5);
write(pipefd[1], "world", 5);
~~~

子进程可能一次 read() 读到 `helloworld`，也可能分两次读到 `hello` 和 `world`。

如果程序需要区分多条消息，就必须自己设计格式，例如每条消息以换行符结束，再由接收方按换行符拆分。

## 八、换行符和输出缓冲

如果消息本身已经包含换行，接收方的 printf() 格式字符串末尾又包含换行，输出可能多出空行。

另外，子进程如果使用 `_exit()` 结束，标准输出缓冲区不会自动刷新。调用 printf() 后使用 `_exit()` 时，需要先：

~~~
fflush(stdout);
_exit(EXIT_SUCCESS);
~~~

普通的 main() 返回或调用 exit() 时，标准库会执行正常的输出刷新。

## 九、和监控程序的关系

下一步先让父进程发送一条简单文本，子进程读取并打印：

~~~
父进程 write → pipe → 子进程 read
~~~

理解单向 pipe 后，再考虑把监控数据传给其他进程，以及使用两个 pipe 实现双向通信。

## 常见错误

- 把 pipefd[0] 当成写端
- fork 前没有创建 pipe
- 忘记关闭不使用的一端
- 忘记关闭写端，导致读端一直等不到 EOF
- 不检查 read() 和 write() 的返回值
- 以为一次 read() 必然读到一次 write() 的全部内容
- 先写一次消息，又从 total_written = 0 开始执行完整写循环，导致重复写入
- 消息和 printf() 都带换行，导致输出空行

## 完成本练习后的结果

pipe_demo.c 已经能够：

- 在 fork() 前创建 pipe
- 父进程写入文本，子进程循环读取
- 正确关闭不用的读端和写端
- 通过关闭写端让子进程收到 EOF
- 使用 waitpid() 等待子进程

代码中还有一个待整理的编译警告：ssize_t 与 size_t 比较时需要统一类型。它不影响本次短消息通信，但后续应修正。
