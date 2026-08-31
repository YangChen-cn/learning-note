# 两个 pipe 实现双向通信（练习 11）

## 本课目标

上一课只有一个 pipe，数据只能沿一个方向流动：

~~~
父进程 write → pipe → 子进程 read
~~~

本课增加第二个 pipe，让父子进程完成一次请求和响应：

~~~
父进程 write → pipe1 → 子进程 read
父进程 read  ← pipe2 ← 子进程 write
~~~

这一步是从“父子进程传消息”走向“父子进程协作”。以后做监控服务时，父进程可以发送命令，子进程可以返回采集结果。

## 一、为什么一个 pipe 不能方便地双向通信

pipe 创建后得到两个文件描述符：

~~~
pipefd[0]：读端
pipefd[1]：写端
~~~

同一个 pipe 的数据方向是固定的。虽然多个进程可以同时打开同一个 pipe，但用一个 pipe 同时承载请求和响应会让协议变得混乱：读到的数据无法简单判断属于哪个方向。

因此使用两个有明确名字的 pipe：

~~~
int parent_to_child[2];
int child_to_parent[2];
~~~

命名不是 C 语言的特殊语法，但能直接表达数据方向。

## 二、两个 pipe 的完整数据流

第一个 pipe 负责父进程发给子进程：

~~~
parent_to_child[1]  父进程写端
        ↓
parent_to_child pipe
        ↓
parent_to_child[0]  子进程读端
~~~

第二个 pipe 负责子进程返回给父进程：

~~~
child_to_parent[1]   子进程写端
        ↓
child_to_parent pipe
        ↓
child_to_parent[0]   父进程读端
~~~

最终通信顺序：

~~~
父进程 write(parent_to_child[1])
子进程 read(parent_to_child[0])

子进程 write(child_to_parent[1])
父进程 read(child_to_parent[0])
~~~

## 三、函数原型和参数

### pipe()

~~~
#include <unistd.h>

int pipe(int pipefd[2]);
~~~

参数 `pipefd` 是一个长度为 2 的整数数组，函数会把两个文件描述符写入数组。

返回值：

- `0`：成功
- `-1`：失败，并设置 `errno`

通常在 `fork()` 之前创建两个 pipe，这样父子进程都能继承它们：

~~~
if (pipe(parent_to_child) < 0) {
    perror("pipe parent_to_child");
    return EXIT_FAILURE;
}

if (pipe(child_to_parent) < 0) {
    perror("pipe child_to_parent");
    return EXIT_FAILURE;
}
~~~

### read()

~~~
ssize_t read(int fd, void *buffer, size_t count);
~~~

- `fd`：要读取的文件描述符
- `buffer`：接收数据的内存地址
- `count`：最多读取多少字节

返回值：

- 大于 `0`：本次实际读取的字节数
- `0`：读到 EOF
- `-1`：读取失败，查看 `errno`

### write()

~~~
ssize_t write(int fd, const void *buffer, size_t count);
~~~

- `fd`：要写入的文件描述符
- `buffer`：要发送的数据地址
- `count`：希望写入的字节数

返回值大于 0 时，表示本次实际写入的字节数，不一定等于 `count`。

### close()

~~~
int close(int fd);
~~~

关闭文件描述符。成功返回 `0`，失败返回 `-1`。

在双向 pipe 中，`close()` 不只是资源清理，也决定另一端什么时候能够收到 EOF。

## 四、fork 后文件描述符如何继承

创建两个 pipe 后，父进程暂时拥有四个描述符：

~~~
parent_to_child[0]  读请求
parent_to_child[1]  写请求
child_to_parent[0]  读响应
child_to_parent[1]  写响应
~~~

调用 `fork()` 后，子进程也拥有这四个描述符：

~~~
fork()
  ├── 父进程：四个描述符
  └── 子进程：四个描述符
~~~

它们指向内核中的同一组 pipe。之后必须根据数据方向关闭不用的端点。

## 五、父子进程各自应该关闭什么

父进程只负责写请求、读响应：

~~~
close(parent_to_child[0]);  /* 父进程不读请求 */
close(child_to_parent[1]);  /* 父进程不写响应 */
~~~

子进程只负责读请求、写响应：

~~~
close(parent_to_child[1]);  /* 子进程不写请求 */
close(child_to_parent[0]);  /* 子进程不读响应 */
~~~

端点关系表：

| 进程 | 保留的描述符 | 关闭的描述符 |
|---|---|---|
| 父进程 | `parent_to_child[1]`、`child_to_parent[0]` | `parent_to_child[0]`、`child_to_parent[1]` |
| 子进程 | `parent_to_child[0]`、`child_to_parent[1]` | `parent_to_child[1]`、`child_to_parent[0]` |

## 六、一次请求和响应的完整过程

假设父进程发送 `status`，子进程返回 `running`。

### 第一步：创建通信通道

~~~
父进程创建 parent_to_child
父进程创建 child_to_parent
父进程 fork 子进程
~~~

### 第二步：双方关闭不用的端点

~~~
父进程：保留请求写端和响应读端
子进程：保留请求读端和响应写端
~~~

### 第三步：父进程发送请求

~~~
父进程 write(parent_to_child[1], "status", 6)
父进程 close(parent_to_child[1])
~~~

关闭请求写端表示：父进程不会再发送请求数据。

### 第四步：子进程读取请求

~~~
子进程 read(parent_to_child[0], buffer, ...)
子进程识别出 status
~~~

如果子进程需要通过 EOF 判断请求结束，那么父进程关闭写端是必要的。

### 第五步：子进程发送响应

~~~
子进程 write(child_to_parent[1], "running", 7)
子进程 close(child_to_parent[1])
~~~

### 第六步：父进程读取响应

~~~
父进程 read(child_to_parent[0], buffer, ...)
父进程 close(child_to_parent[0])
~~~

## 七、阻塞和死锁

pipe 默认是阻塞的。没有数据时，`read()` 会等待；写端没有空间时，`write()` 也可能等待。

正确顺序是：

~~~
父进程先 write 请求
        ↓
子进程 read 请求
        ↓
子进程 write 响应
        ↓
父进程 read 响应
~~~

错误顺序可能导致死锁：

~~~
父进程 read，等待子进程写
子进程 read，等待父进程写
~~~

此时双方都在等待，程序不会继续运行。

另一个常见问题是父进程写完后忘记关闭写端。子进程可能已经读完数据，但因为仍然存在打开的写端，继续 `read()` 时不会收到 EOF。

## 八、pipe 传递的是字节流

pipe 不保存消息边界，也不知道数据是字符串、整数还是结构体。

如果父进程执行两次：

~~~
write(fd, "hello", 5);
write(fd, "world", 5);
~~~

子进程可能一次读到：

~~~
helloworld
~~~

也可能分两次读到 `hello` 和 `world`。接收方不能把一次 `write()` 自动等同于一次 `read()`。

简单练习可以使用短文本；真实程序需要设计消息格式，例如：

~~~
status\n
memory\n
quit\n
~~~

接收方再按照换行符拆分消息。

## 九、read() 和 write() 的部分传输

`read()` 返回多少字节，取决于当前 pipe 里已经有多少数据；`write()` 返回多少字节，表示本次实际写入多少。

完整写入通常需要累计计数：

~~~
size_t total = 0;
size_t length = strlen(message);

while (total < length) {
    ssize_t count = write(fd, message + total, length - total);

    if (count < 0) {
        perror("write");
        break;
    }

    total += (size_t)count;
}
~~~

不要在已经写过一次完整消息后，又把 `total` 初始化为 0 再执行一次循环，否则会重复发送。

## 十、最小请求响应示例

下面的示例展示数据流，练习代码不会直接照抄它：

~~~
int request_pipe[2];
int response_pipe[2];

pipe(request_pipe);
pipe(response_pipe);
fork();

父进程：
    close(request_pipe[0]);
    close(response_pipe[1]);
    write(request_pipe[1], "status", 6);
    close(request_pipe[1]);
    read(response_pipe[0], buffer, sizeof(buffer));
    close(response_pipe[0]);

子进程：
    close(request_pipe[1]);
    close(response_pipe[0]);
    read(request_pipe[0], buffer, sizeof(buffer));
    close(request_pipe[0]);
    write(response_pipe[1], "running", 7);
    close(response_pipe[1]);
~~~

预期逻辑结果：

~~~
父进程发送: status
子进程收到: status
子进程发送: running
父进程收到: running
~~~

## 十一、和监控程序的关系

以后可以让父进程发送：

~~~
status
~~~

子进程读取后调用已有的监控读取函数，再返回：

~~~
uptime=12345 memory=31.9%
~~~

这比单纯用 `waitpid()` 等待子进程更进一步：父进程可以主动请求状态，子进程可以返回数据。

本课仍然只传递短文本，不直接把完整监控程序改造成复杂服务。

## 十二、可以复制运行的观察命令

上一课的单向 pipe 可以先重新运行：

~~~
make -C test build/pipe_demo
./test/build/pipe_demo
~~~

本次练习使用下面的具体功能，而不是只发送 `status` 字符串：

~~~
父进程：发送 "meminfo\n"
子进程：读取 /proc/meminfo，筛选 MemTotal 和 MemAvailable
子进程：通过 response_pipe 返回两行原始文本
父进程：读取到 EOF 后显示结果
~~~

练习框架文件是 `test/monitor_two_pipe.c`。完成后使用类似命令观察请求和响应各出现一次：

~~~
make -C test build/monitor_two_pipe
./test/build/monitor_two_pipe
~~~

预期输出应该能区分四个事件：父发送、子接收、子发送、父接收。

## 常见错误

- 把两个 pipe 的方向命名混乱
- fork 前没有创建两个 pipe
- 忘记关闭某个不用的端点
- 父子进程同时先 read，造成死锁
- 忘记关闭写端，导致读端一直等不到 EOF
- 把一次 write() 错误地当作一条完整消息
- 不检查 read()、write()、close() 和 pipe() 的返回值
- 忘记处理部分读写

## 下一步练习

当前已经进入练习阶段。先完成 `test/monitor_two_pipe.c` 中的 TODO，再运行上面的编译和执行命令。

## 练习完成记录

本练习已完成一次请求和响应：

~~~
父进程发送：meminfo
子进程读取：/proc/meminfo
子进程返回：MemTotal、MemAvailable 两行
父进程读取响应并显示
子进程退出，状态码：0
~~~

当前实现通过 `sscanf()` 分离 `name` 和 `value` 来筛选目标行，但实际写回的是 `/proc/meminfo` 的原始 `line`。这是一种简单且合理的协议设计；如果后续需要统一格式，可以再使用 `snprintf()` 把多个数值组合成新的响应字符串。

还需要记住两个改进点：

- `write()` 不保证一次写完全部数据，正式程序应处理短写。
- 当前请求只有一次，后续可以增加无效命令处理，例如返回 `unknown command`。

## write() 短写：为什么不能只调用一次

`write()` 的第三个参数表示“最多希望写入多少字节”，不是保证一定写完。

例如：

~~~c
ssize_t count = write(fd, buffer, length);
~~~

可能出现三种结果：

- `count < 0`：写入失败
- `count == length`：全部写入
- `0 < count < length`：只写入了一部分，也就是短写

如果发生短写，剩下的数据位置是：

~~~c
buffer + count
~~~

剩余长度是：

~~~c
length - count
~~~

因此需要循环写入，直到全部完成。可以写一个通用函数：

~~~c
static int write_all(int fd, const void *buffer, size_t length)
{
    const char *current = buffer;

    while (length > 0) {
        ssize_t count = write(fd, current, length);

        if (count < 0) {
            if (errno == EINTR) {
                /* 被信号打断，没有写入数据，重新尝试。 */
                continue;
            }

            perror("write");
            return -1;
        }

        if (count == 0) {
            fprintf(stderr, "write returned 0\n");
            return -1;
        }

        current += count;
        length -= (size_t)count;
    }

    return 0;
}
~~~

这个函数的关键是两个变量：

- `current`：当前还没有写入的位置
- `length`：当前还剩多少字节

每次成功写入 `count` 个字节后：

~~~c
current += count;
length -= (size_t)count;
~~~

在本练习中，可以这样使用：

~~~c
if (write_all(response_pipe[1], line, strlen(line)) < 0) {
    /* 处理错误并退出子进程。 */
}
~~~

短字符串通常一次就能写完，所以你的程序可能很难在运行时观察到短写。但程序不能依赖“这次数据很短”这个偶然条件。以后写日志、socket、串口或网络数据时，都应该考虑部分写入。

注意：`write_all()` 只负责把数据写完，不负责关闭文件描述符。写完后仍然需要由调用者执行：

~~~c
close(response_pipe[1]);
~~~

这样父进程读取完数据后，才能收到 EOF。
