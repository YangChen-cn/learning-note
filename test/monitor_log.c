/*
 * 练习 5A 框架：在前台运行的同时，把监控结果追加写入日志文件
 *
 * 目标：
 *   终端继续显示监控信息
 *   monitor.log 追加保存每次刷新结果
 *   Ctrl+C 后正确关闭日志文件
 *
 * 本练习暂时不使用 fork() / setsid()，先只学习文件追加和刷新。
 */

#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOG_PATH "monitor.log"

static volatile sig_atomic_t g_stop = 0;

static void on_sigint(int signum)
{
    (void)signum;
    g_stop = 1;
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

int main(void)
{
    FILE *log_fp;

    /* TODO 1：以追加模式打开日志文件。 */
    /* 提示：fopen(LOG_PATH, "a")；失败后用 perror() 并退出。 */
    log_fp = NULL;
    log_fp = fopen(LOG_PATH, "a");
    if (log_fp == NULL) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    if (signal(SIGINT, on_sigint) == SIG_ERR) {
        perror("signal");
        if (log_fp != NULL) {
            fclose(log_fp);
        }
        return EXIT_FAILURE;
    }

    printf("日志文件: %s\n", LOG_PATH);
    printf("按 Ctrl+C 退出\n");

    while (!g_stop) {
        int fd_uptime;
        char buffer_uptime[128];
        ssize_t bytes_read;
        double uptime_seconds = 0.0;
        FILE *fp_meminfo;
        char line[256];
        unsigned long mem_total = 0;
        unsigned long mem_free = 0;

        fd_uptime = open("/proc/uptime", O_RDONLY);
        if (fd_uptime < 0) {
            perror("open /proc/uptime");
            break;
        }

        bytes_read = read(fd_uptime, buffer_uptime,
                           sizeof(buffer_uptime) - 1);
        if (bytes_read < 0) {
            perror("read /proc/uptime");
            close(fd_uptime);
            break;
        }
        buffer_uptime[bytes_read] = '\0';
        close(fd_uptime);

        if (sscanf(buffer_uptime, "%lf", &uptime_seconds) != 1) {
            fprintf(stderr, "解析 /proc/uptime 失败\n");
            break;
        }

        fp_meminfo = fopen("/proc/meminfo", "r");
        if (fp_meminfo == NULL) {
            perror("fopen /proc/meminfo");
            break;
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
            break;
        }else {
            unsigned long used = mem_total - mem_free;
            int percent = (int)(used * 100 / mem_total);
            int total_seconds = (int)uptime_seconds;

            printf("运行时间: %02d:%02d:%02d    内存使用: %d%%（%lu kB / %lu kB）\n",
                   total_seconds / 3600,
                   (total_seconds % 3600) / 60,
                   total_seconds % 60,
                   percent,
                   used,
                   mem_total);

            /* TODO 2：把本次结果写入 log_fp。 */
            /* 提示：使用 fprintf()，一行写完并以 '\n' 结尾。 */
            fprintf(log_fp, "运行时间: %02d:%02d:%02d    内存使用: %d%%（%lu kB / %lu kB）\n",
                   total_seconds / 3600,
                   (total_seconds % 3600) / 60,
                   total_seconds % 60,
                   percent,
                   used,
                   mem_total);
            /* TODO 3：让日志立即落到文件中。 */
            /* 提示：使用 fflush(log_fp)。 */
            fflush(log_fp);
        }

        sleep(1);
    }

    /* TODO 4：退出前关闭日志文件。 */
    /* 提示：只有 log_fp != NULL 时才能调用 fclose()。 */
    if (log_fp != NULL) {
        fclose(log_fp);
    }

    printf("\n监控结束\n");
    return EXIT_SUCCESS;
}
