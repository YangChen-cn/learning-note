# Makefile 基础

## 为什么需要

编译命令多了之后（CFLAGS、多个源文件、每天要敲好几遍），用一个文件自动化。

## 最小结构

```make
目标: 依赖
    规则命令        # 前面必须是 Tab，不能是空格！
```

## test/Makefile 逐行拆解

```make
CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -g
# 变量赋值：:= 立即赋值
# -Wall 打开警告，-Wextra 更多警告，-std=c11 C11 标准，-g 带调试信息

TARGETS := build/uptime build/meminfo    # 列表变量

all: $(TARGETS)        # make / make all 都构建全部目标

build/uptime: uptime.c | build    # | build 是"仅顺序依赖"：先保证目录存在
	$(CC) $(CFLAGS) uptime.c -o $@    # $@ = 当前目标名

run: build/uptime      # make run = 先编译再运行
	./build/uptime

clean:                 # make clean = 清理
	rm -rf build

.PHONY: all run clean  # 声明这些"目标"只是命令名，不是要生成的文件
```

## 常用命令

```sh
make          # 构建第一个目标（默认目标）
make all      # 构建全部
make run      # 编译并运行 uptime
make clean    # 清理 build
```

## 关键机制

- make 靠**文件时间戳**判断要不要重编：源文件比目标新 → 重新编译
- 想加新练习 = 加一个 `build/xxx` 目标 + 加进 `TARGETS`

## 心得

- 规则命令的 Tab 缩进是最常见的报错来源（`missing separator`）
- 看别人项目的 Makefile 时，先找变量和 `.PHONY`，再找目标和依赖
