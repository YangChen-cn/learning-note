/*
 * 练习 3: 每秒刷新显示 运行时间 + 内存使用率, Ctrl+C 优雅退出
 *
 * 目标输出 (每秒刷新, 原地覆盖):
 *   运行时间: 1 小时 06 分 02 秒    内存使用: 45% (471856 / 1024000 kB)
 *
 * 新概念: 定时循环 sleep、信号处理 (SIGINT / Ctrl+C)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> 
#include <unistd.h>     /* sleep */
#include <signal.h>     /* signal, SIGINT */

/* 信号标志:
 * 处理函数里只能做"置标志"这种简单事 (printf 等函数在信号上下文不安全)
 * volatile     : 防止编译器把这个变量的读取优化掉
 * sig_atomic_t : 保证读写在信号中断程序时也是原子操作 */
static volatile sig_atomic_t g_stop = 0;

/* 信号处理函数: 按 Ctrl+C (SIGINT) 时内核会调用它 */
static void on_sigint(int signum)
{
    (void)signum;       /* 这个参数表示收到的是几号信号, 这里用不到 */
    g_stop = 1;
}

/* 练习 2 的 parse_kb_value, 直接抄过来复用 */
static int parse_kb_value(const char *line, const char *key, unsigned long *value)
{
   char name[32];
    unsigned long value_kb; 
    if (sscanf(line, "%31[^:] : %lu kB", name, &value_kb) == 2 &&
        strcmp(name, key) == 0) {
        *value = value_kb;
        return 1;
    }
    return 0;
}

int main(void)
{
    int fd_uptime;
    char buffer_uptime[128];
    ssize_t n;

    FILE *fp_meminfo;
    char line[256];
    unsigned long mem_total = 0;
    unsigned long mem_free = 0;
    /* 提示: 注册信号处理函数
     *   signal(SIGINT, on_sigint);
     * 之后按 Ctrl+C 不再直接杀进程, 而是调用 on_sigint 把 g_stop 置 1 */

    /* TODO: 注册信号 */
    signal(SIGINT, on_sigint);

    /* 提示: ANSI 转义序列
     *   "\033[2J" 清屏, "\033[H" 光标回左上角
     * 第一次先清一次屏; 之后每轮光标回左上角重新打印, 就是"原地刷新" */

    printf("\033[2J\033[H");

    while (!g_stop) {   /* g_stop 被信号置 1 后循环退出 */

        /* 提示: 每轮做三件事
         *
         * 1. 运行时间: 读 /proc/uptime
         *    fopen("r") + fgets + sscanf(line, "%lf", &seconds)
         *    第一个数字是开机秒数(浮点), 显示时转成整数
         *    想显示成 时:分:秒: 时=sec/3600 分=(sec%3600)/60 秒=sec%60
         *
         * 2. 内存: 调用上面的 parse_kb_value 读 MemTotal/MemFree
         *    复用练习 2 的用法, 算 percent = used * 100 / mem_total
         *
         * 3. 刷新: printf("\033[H") 光标回左上角, 再打印两行信息
         *    printf 后加 fflush(stdout) 强制立即输出(终端行缓冲) */

        /* TODO: 你的代码 */
        fd_uptime = open("/proc/uptime", O_RDONLY);
        if (fd_uptime < 0) {
            perror("open /proc/uptime");
            exit(EXIT_FAILURE);
        }
        n = read(fd_uptime, buffer_uptime, sizeof(buffer_uptime) - 1);
        if (n < 0) {
            perror("read /proc/uptime");
            close(fd_uptime);
            exit(EXIT_FAILURE);
        }
        buffer_uptime[n] = '\0';
        double uptime_seconds;
        if (sscanf(buffer_uptime, "%lf", &uptime_seconds) != 1) {
            fprintf(stderr, "解析 /proc/uptime 失败\n");
            close(fd_uptime);
            exit(EXIT_FAILURE);
        }
        close(fd_uptime);
        
        fp_meminfo = fopen("/proc/meminfo", "r");
        if (fp_meminfo == NULL) {
            perror("fopen /proc/meminfo");
            exit(EXIT_FAILURE);
        }
        while(fgets(line, sizeof(line), fp_meminfo) != NULL) {
            if (parse_kb_value(line, "MemTotal", &mem_total) ||
                parse_kb_value(line, "MemFree", &mem_free)) {
                if (mem_total != 0 && mem_free != 0) {
                    break;
                }
            }
        }
        fclose(fp_meminfo); 

        if (mem_total == 0 || mem_free == 0) {
        fprintf(stderr, "解析失败: 未找到 MemTotal / MemFree\n");
        exit(EXIT_FAILURE);
    }
    unsigned long used = mem_total - mem_free;
    int percent = (int)(used * 100 / mem_total);

        printf("\033[H");
        printf("运行时间: %02d 小时 %02d 分 %02d 秒    内存使用: %d%% (%lu / %lu kB)\n",
               (int)(uptime_seconds / 3600),
               (int)((((int)uptime_seconds) % 3600) / 60),
               (int)(((int)uptime_seconds) % 60),
               percent, used, mem_total);
        fflush(stdout);
        //重置
        mem_free = 0;
        mem_total = 0;
        sleep(1);       /* 睡 1 秒, 循环再来一次 → 每秒刷新 */
    }

    printf("\n收到 Ctrl+C, 正常退出\n");

    return 0;
}
