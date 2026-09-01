/* 让 glibc 在 -std=c11 下声明 POSIX 接口，例如 getopt()。 */
#define _POSIX_C_SOURCE 200809L

/*
 * 练习 4 框架：给 monitor 增加命令行参数
 *
 * 目标：
 *   ./build/monitor_args              每 1 秒刷新，持续运行
 *   ./build/monitor_args -i 2 -n 5   每 2 秒刷新，共刷新 5 次
 *   ./build/monitor_args -h           显示帮助
 *
 * 新概念：getopt(argc, argv, "i:n:h")
 *
 * 提示：
 *   -i 后面需要一个参数，所以选项字符串中写成 "i:"
 *   -n 后面需要一个参数，所以选项字符串中写成 "n:"
 *   -h 不需要参数，所以只写 "h"
 */

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t g_stop = 0;

static void on_sigint(int signum)
{
    (void)signum;
    g_stop = 1;
}

static void print_usage(const char *program)
{
    printf("用法: %s [-i 间隔秒数] [-n 刷新次数]\n", program);
    printf("  -i N    每 N 秒刷新一次，默认 1 秒\n");
    printf("  -n N    刷新 N 次后退出，默认持续运行\n");
    printf("  -h      显示帮助\n");
}

static int parse_kb_value(const char *line, const char *key,
                          unsigned long *value)
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

int main(int argc, char *argv[])
{
    int interval = 1;  /* -i：刷新间隔，默认 1 秒 */
    int max_count = 0; /* -n：最大刷新次数，0 表示不限次数 */
    int count = 0;
    int opt;

    /*
     * TODO 1：用 getopt() 解析命令行参数
     *
     * 参考结构：
     *   while ((opt = getopt(argc, argv, "i:n:h")) != -1) {
     *       switch (opt) {
     *       case 'i':
     *           // optarg 是 -i 后面的字符串，例如 "2"
     *           // 提示：用 strtol() 转成整数，并检查结果是否大于 0
     *           break;
     *       case 'n':
     *           // optarg 是 -n 后面的字符串
     *           // 提示：同样转成整数；允许正数，不要接受 0 或负数
     *           break;
     *       case 'h':
     *           print_usage(argv[0]);
     *           return EXIT_SUCCESS;
     *       default:
     *           print_usage(argv[0]);
     *           return EXIT_FAILURE;
     *       }
     *   }
     *
     * 先想清楚：
     *   1. getopt() 返回的 opt 是选项字母
     *   2. optarg 指向该选项携带的参数字符串
     *   3. 参数非法时应该打印错误并退出，而不是继续运行
     */
    while ((opt = getopt(argc, argv, "i:n:h")) != -1) {
        switch (opt) {
        case 'i':
            /* TODO：把 optarg 转成 interval，并检查 interval > 0 */

            interval = (int)strtol(optarg, NULL, 10);
            if (interval <= 0) {
                fprintf(stderr, "错误：-i 参数必须是正整数\n");
                return EXIT_FAILURE;
            }
            break;
        case 'n':
            /* TODO：把 optarg 转成 max_count，并检查 max_count > 0 */
            max_count = (int)strtol(optarg, NULL, 10);
            if (max_count <= 0) {
                fprintf(stderr, "错误：-n 参数必须是正整数\n");
                return EXIT_FAILURE;
            }
            break;
        case 'h':
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        default:
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    /* 暂时不处理位置参数；本练习只关注 -i、-n、-h。 */
    if (optind < argc) {
        fprintf(stderr, "不支持的位置参数: %s\n", argv[optind]);
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (signal(SIGINT, on_sigint) == SIG_ERR) {
        perror("signal");
        return EXIT_FAILURE;
    }

    printf("\033[2J\033[H");

    while (!g_stop && (max_count == 0 || count < max_count)) {
        int fd_uptime;
        char buffer_uptime[128];
        ssize_t bytes_read;
        double uptime_seconds = 0.0;
        FILE *fp_meminfo;
        char line[256];
        unsigned long mem_total = 0;
        unsigned long mem_free = 0;

        /* 练习 3 的数据流：open -> read -> sscanf -> close。 */
        fd_uptime = open("/proc/uptime", O_RDONLY);
        if (fd_uptime < 0) {
            perror("open /proc/uptime");
            return EXIT_FAILURE;
        }

        bytes_read = read(fd_uptime, buffer_uptime,
                           sizeof(buffer_uptime) - 1);
        if (bytes_read < 0) {
            perror("read /proc/uptime");
            close(fd_uptime);
            return EXIT_FAILURE;
        }
        buffer_uptime[bytes_read] = '\0';

        /* TODO 2：检查 sscanf() 的返回值，确认 uptime 解析成功。 */
        if (sscanf(buffer_uptime, "%lf", &uptime_seconds) != 1) {
            fprintf(stderr, "解析 /proc/uptime 失败\n");
            close(fd_uptime);
            return EXIT_FAILURE;
        }
        close(fd_uptime);

        /* 练习 2 的数据流：fopen -> fgets -> sscanf -> fclose。 */
        fp_meminfo = fopen("/proc/meminfo", "r");
        if (fp_meminfo == NULL) {
            perror("fopen /proc/meminfo");
            return EXIT_FAILURE;
        }

        while (fgets(line, sizeof(line), fp_meminfo) != NULL) {
            if (parse_kb_value(line, "MemTotal", &mem_total) ||
                parse_kb_value(line, "MemFree", &mem_free)) {
                if (mem_total != 0 && mem_free != 0) {
                    break;
                }
            }
        }
        fclose(fp_meminfo);

        if (mem_total == 0 || mem_free > mem_total) {
            fprintf(stderr, "解析内存信息失败\n");
            return EXIT_FAILURE;
        }

        {
            unsigned long used = mem_total - mem_free;
            int percent = (int)(used * 100 / mem_total);
            int total_seconds = (int)uptime_seconds;

            printf("\033[H");
            printf("运行时间: %02d 小时 %02d 分 %02d 秒    "
                   "内存使用: %d%% (%lu / %lu kB)\n",
                   total_seconds / 3600,
                   (total_seconds % 3600) / 60,
                   total_seconds % 60,
                   percent, used, mem_total);
            fflush(stdout);
        }

        count++;

        if (max_count > 0 && count >= max_count) {
            break;
        }   
        sleep((unsigned int)interval);
    }

    printf("\n监控结束，共刷新 %d 次\n", count);
    return EXIT_SUCCESS;
}
