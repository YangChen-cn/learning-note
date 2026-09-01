/*
 * 练习 9：复习 system_monitor.c 中已经学过的部分
 *
 * 这个文件是学习版，不是最终产品代码。
 *
 * 保留：
 *   /proc/uptime、/proc/meminfo、/proc/loadavg
 *   open/read/close、fopen/fgets/fclose
 *   sscanf、strcmp、函数和指针输出参数
 *
 * 暂时跳过：
 *   socket、ioctl、inet_ntop、gethostname、uname、网络接口查询
 */

#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * 读取 /proc/uptime 的第一个数字。
 *
 * seconds 不是用来传入数据的，而是让函数把结果写回 main()。
 * 这就是“指针作为输出参数”。
 */
static int read_uptime(double *seconds)
{
    int fd;
    char buffer[128];
    ssize_t bytes_read;

    /* open：打开内核提供的虚拟文件。 */
    fd = open("/proc/uptime", O_RDONLY);
    if (fd < 0) {
        perror("open /proc/uptime");
        return -1;
    }

    /* read：把文件内容从内核读取到用户空间的 buffer。 */
    bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("read /proc/uptime");
        close(fd);
        return -1;
    }

    /* 文本要补上字符串结尾，才能交给 sscanf。 */
    buffer[bytes_read] = '\0';

    /* close：使用完文件描述符后关闭。 */
    if (close(fd) < 0) {
        perror("close /proc/uptime");
        return -1;
    }

    /* sscanf：从文本中提取第一个浮点数。 */
    if (sscanf(buffer, "%lf", seconds) != 1) {
        fprintf(stderr, "cannot parse /proc/uptime\n");
        return -1;
    }

    return 0;
}

/*
 * 读取 /proc/meminfo 中的 MemTotal 和 MemAvailable。
 *
 * 这里使用标准库文件流：
 * fopen → fgets → sscanf → fclose
 */
static int read_memory(unsigned long *total_kb,
                       unsigned long *available_kb)
{
    FILE *file;
    char line[256];
    int found_total = 0;
    int found_available = 0;

    file = fopen("/proc/meminfo", "r");
    if (file == NULL) {
        perror("fopen /proc/meminfo");
        return -1;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char key[64];
        unsigned long value_kb;

        /* 例如：MemTotal:       6543180 kB */
        if (sscanf(line, "%63[^:]: %lu kB", key, &value_kb) != 2) {
            continue;
        }

        if (strcmp(key, "MemTotal") == 0) {
            *total_kb = value_kb;
            found_total = 1;
        } else if (strcmp(key, "MemAvailable") == 0) {
            *available_kb = value_kb;
            found_available = 1;
        }
    }

    if (ferror(file)) {
        perror("fgets /proc/meminfo");
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {
        perror("fclose /proc/meminfo");
        return -1;
    }

    if (!found_total || !found_available) {
        fprintf(stderr, "MemTotal or MemAvailable is missing\n");
        return -1;
    }

    return 0;
}

/* 读取 /proc/loadavg 的前三个负载数值。 */
static int read_load_average(double load[3])
{
    FILE *file;
    char line[256];

    file = fopen("/proc/loadavg", "r");
    if (file == NULL) {
        perror("fopen /proc/loadavg");
        return -1;
    }

    if (fgets(line, sizeof(line), file) == NULL) {
        if (ferror(file)) {
            perror("fgets /proc/loadavg");
        } else {
            fprintf(stderr, "/proc/loadavg is empty\n");
        }
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {
        perror("fclose /proc/loadavg");
        return -1;
    }

    if (sscanf(line, "%lf %lf %lf", &load[0], &load[1], &load[2]) != 3) {
        fprintf(stderr, "cannot parse /proc/loadavg\n");
        return -1;
    }

    return 0;
}

static void print_uptime(double seconds)
{
    unsigned long total_seconds = (unsigned long)seconds;
    unsigned long hours = total_seconds / 3600UL;
    unsigned long minutes = (total_seconds % 3600UL) / 60UL;
    unsigned long remaining_seconds = total_seconds % 60UL;

    printf("Uptime: %02lu:%02lu:%02lu\n",
           hours, minutes, remaining_seconds);
}

int main(void)
{
    double uptime_seconds;
    double load[3];
    unsigned long total_memory_kb = 0;
    unsigned long available_memory_kb = 0;

    printf("学习版 System Monitor\n\n");

    if (read_uptime(&uptime_seconds) == 0) {
        print_uptime(uptime_seconds);
    } else {
        printf("Uptime: unavailable\n");
    }

    if (read_memory(&total_memory_kb, &available_memory_kb) == 0 &&
        total_memory_kb > 0 && available_memory_kb <= total_memory_kb) {
        unsigned long used_kb = total_memory_kb - available_memory_kb;
        double used_percent =
            (double)used_kb * 100.0 / (double)total_memory_kb;

        printf("Memory: %.1f%% used (%lu/%lu kB)\n",
               used_percent, used_kb, total_memory_kb);
    } else {
        printf("Memory: unavailable\n");
    }

    if (read_load_average(load) == 0) {
        printf("Load average: %.2f %.2f %.2f\n",
               load[0], load[1], load[2]);
    } else {
        printf("Load average: unavailable\n");
    }

    return EXIT_SUCCESS;
}
