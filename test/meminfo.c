/*
 * 练习 2: 解析 /proc/meminfo
 *
 * 目标: 读取 /proc/meminfo, 提取 MemTotal 和 MemFree,
 *       计算内存使用率并打印, 例如:
 *       内存使用: 45% (471856 / 1024000 kB)
 *
 * 新概念: 按行读取 + 从字符串行中解析出数字
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMINFO_PATH "/proc/meminfo"

/*
 * parse_kb_value: 判断 line 这一行是不是 key 开头的内存项,
 * 是则把数值(kB)写入 *value 并返回 1, 否则返回 0。
 *
 * 行的格式:
 *     "MemTotal:       1024000 kB"
 *     "MemFree:        524288 kB"
 *
 * 提示(方法一, sscanf 一行完成):
 *     char name[32];
 *     unsigned long v;
 *     if (sscanf(line, "%31[^:] : %lu kB", name, &v) == 2 &&
 *         strcmp(name, key) == 0) {
 *         *value = v;
 *         return 1;
 *     }
 *     return 0;
 *   - "%31[^:]"  读冒号之前的所有字符, 即键名 "MemTotal" (不含冒号)
 *   - "%lu"      读后面的数字, 即 kB 数值
 *   - 格式串里冒号前后各留一个空格: " : " 才能跳过行里的多个空格
 *   - sscanf 返回值 == 2 表示两个转换都成功
 *
 * 提示(方法二, 不用 sscanf 的写法):
 *     用 strchr(line, ':') 找到冒号位置, 用 strncmp 比较键名,
 *     再用 strtoul(冒号后面的位置, NULL, 10) 解析数字
 */
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
    FILE *fp;
    char line[256];
    unsigned long mem_total = 0;
    unsigned long mem_free = 0;

    fp = fopen(MEMINFO_PATH, "r");

    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* 提示:
     * 1. fgets(line, sizeof(line), fp) 每次读一行, 读到文件尾返回 NULL
     * 2. 每一行分别用 parse_kb_value() 检查是不是 MemTotal / MemFree:
     *        parse_kb_value(line, "MemTotal", &mem_total)
     *        parse_kb_value(line, "MemFree", &mem_free)
     * 3. 两个值都拿到了(都不为 0)可以 break 提前退出循环 */
    while (fgets(line, sizeof(line), fp) != NULL) {
        /* TODO: 你的代码 */
        if (parse_kb_value(line, "MemTotal", &mem_total) ||
            parse_kb_value(line, "MemFree", &mem_free)) {
            if (mem_total != 0 && mem_free != 0) {
                break;
            }
        }
    }   

    fclose(fp);

    /* 健壮性检查: 解析失败就别继续算了, 报错退出 */
    if (mem_total == 0 || mem_free == 0) {
        fprintf(stderr, "解析失败: 未找到 MemTotal / MemFree\n");
        exit(EXIT_FAILURE);
    }

    /* 提示:
     * 1. 已用内存 used = mem_total - mem_free
     *    (先不管缓存, MemAvailable 是另一个练习的事)
     * 2. 整数除法会截断成 0, 要先乘 100 再除:
     *        percent = used * 100 / mem_total
     * 3. 打印: printf("内存使用: %d%% (%lu / %lu kB)\n", ...)
     *    (printf 里 %% 才表示输出一个 % 字符) */
    /* TODO: 计算使用率并打印 */
    unsigned long used = mem_total - mem_free;
    int percent = (int)(used * 100 / mem_total);
    printf("内存使用: %d%% (%lu / %lu kB)\n", percent, used, mem_total);    

    return 0;
}
