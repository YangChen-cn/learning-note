/*
 * 练习 6：把已有的日志监控改成父子进程形式
 *
 * 目标：
 *   父进程负责创建和等待
 *   子进程负责运行原来的监控与日志逻辑
 *
 * 监控主体来自 monitor_log.c，本练习只新增进程控制。
 */

#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define LOG_PATH "monitor.log"

static volatile sig_atomic_t g_stop = 0;
static volatile sig_atomic_t g_parent_stop = 0;

static void on_sigint(int signum)
{
    (void)signum;
    g_stop = 1;
}

static void on_parent_sigint(int signum)
{
    (void)signum;
    g_parent_stop = 1;
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

/* 原来的监控主体：子进程应该运行这个函数。 */
int run_monitor(void)
{
    FILE *log_fp = fopen(LOG_PATH, "a");

    if (log_fp == NULL) {
        perror("fopen monitor.log");
        return EXIT_FAILURE;
    }

    if (signal(SIGINT, on_sigint) == SIG_ERR) {
        perror("signal");
        fclose(log_fp);
        return EXIT_FAILURE;
    }

    printf("监控子进程 PID: %ld\n", (long)getpid());
    printf("按 Ctrl+C 停止监控\n");

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
            fclose(log_fp);
            return EXIT_FAILURE;
        }

        bytes_read = read(fd_uptime, buffer_uptime,
                           sizeof(buffer_uptime) - 1);
        if (bytes_read < 0) {
            perror("read /proc/uptime");
            close(fd_uptime);
            fclose(log_fp);
            return EXIT_FAILURE;
        }
        buffer_uptime[bytes_read] = '\0';
        close(fd_uptime);

        if (sscanf(buffer_uptime, "%lf", &uptime_seconds) != 1) {
            fprintf(stderr, "解析 /proc/uptime 失败\n");
            fclose(log_fp);
            return EXIT_FAILURE;
        }

        fp_meminfo = fopen("/proc/meminfo", "r");
        if (fp_meminfo == NULL) {
            perror("fopen /proc/meminfo");
            fclose(log_fp);
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
            fclose(log_fp);
            return EXIT_FAILURE;
        }

        {
            unsigned long used = mem_total - mem_free;
            int percent = (int)(used * 100 / mem_total);
            int total_seconds = (int)uptime_seconds;

            printf("运行时间: %02d:%02d:%02d    内存使用: %d%%\n",
                   total_seconds / 3600,
                   (total_seconds % 3600) / 60,
                   total_seconds % 60,
                   percent);

            fprintf(log_fp, "运行时间: %02d:%02d:%02d    内存使用: %d%%\n",
                    total_seconds / 3600,
                    (total_seconds % 3600) / 60,
                    total_seconds % 60,
                    percent);

            fflush(log_fp);
        }

        sleep(1);
    }

    if (fclose(log_fp) == EOF) {
        perror("fclose monitor.log");
        return EXIT_FAILURE;
    }

    printf("监控子进程结束\n");
    return EXIT_SUCCESS;
}

int main(void)
{
    pid_t child_pid;
    int status;

    //处理 SIGINT 信号
    if (signal(SIGINT, on_parent_sigint) == SIG_ERR) {
            perror("signal主进程");
            return EXIT_FAILURE;
    }
    /* TODO：在这里加入 fork、父子分支和 waitpid。 */
    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (child_pid == 0) {
        //子进程运行监控
        return run_monitor();
        }
    else {
    //父进程
        printf("父进程pid: %ld\n", (long)getpid());
        //等待子进程结束
        pid_t result;
        do {
            result = waitpid(child_pid, &status, 0);
        } while (result < 0 && errno == EINTR );

        if (result < 0) {
            perror("waitpid");
            return EXIT_FAILURE;
        }

        if (WIFEXITED(status)) {
            printf("监控子进程退出，状态码: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("监控子进程被信号 %d 终止\n", WTERMSIG(status));
        } else {
            printf("监控子进程异常退出\n");
        }

        if (g_parent_stop) {
        printf("父进程收到 SIGINT，子进程已经结束\n");
        }
     return EXIT_SUCCESS;
     }
}
