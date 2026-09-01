CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -g

TARGETS := build/uptime build/meminfo build/monitor build/monitor_args build/monitor_log build/monitor_fork build/monitor_exec build/monitor_session build/system_monitor_learn build/pipe_demo build/monitor_two_pipe build/monitor_socketpair

all: $(TARGETS)

build/uptime: uptime.c | build
	$(CC) $(CFLAGS) uptime.c -o $@

build/meminfo: meminfo.c | build
	$(CC) $(CFLAGS) meminfo.c -o $@

build/monitor: monitor.c | build
	$(CC) $(CFLAGS) monitor.c -o $@

build/monitor_args: monitor_args.c | build
	$(CC) $(CFLAGS) monitor_args.c -o $@

build/monitor_log: monitor_log.c | build
	$(CC) $(CFLAGS) monitor_log.c -o $@

build/monitor_fork: monitor_fork.c | build
	$(CC) $(CFLAGS) monitor_fork.c -o $@

build/monitor_exec: monitor_exec.c build/monitor_log | build
	$(CC) $(CFLAGS) monitor_exec.c -o $@

build/monitor_session: monitor_session.c build/monitor_args | build
	$(CC) $(CFLAGS) monitor_session.c -o $@

build/system_monitor_learn: system_monitor_learn.c | build
	$(CC) $(CFLAGS) system_monitor_learn.c -o $@

build/pipe_demo: pipe_demo.c | build
	$(CC) $(CFLAGS) pipe_demo.c -o $@

build/monitor_two_pipe: monitor_two_pipe.c | build
	$(CC) $(CFLAGS) monitor_two_pipe.c -o $@

build/monitor_socketpair: monitor_socketpair.c | build
	$(CC) $(CFLAGS) monitor_socketpair.c -o $@

build:
	mkdir -p build

run: build/uptime
	./build/uptime

run-meminfo: build/meminfo
	./build/meminfo

run-monitor: build/monitor
	./build/monitor

run-monitor-args: build/monitor_args
	./build/monitor_args

run-monitor-log: build/monitor_log
	./build/monitor_log

run-monitor-fork: build/monitor_fork
	./build/monitor_fork

run-monitor-exec: build/monitor_exec
	./build/monitor_exec

run-monitor-session: build/monitor_session
	./build/monitor_session

run-system-monitor-learn: build/system_monitor_learn
	./build/system_monitor_learn

run-pipe-demo: build/pipe_demo
	./build/pipe_demo

run-monitor-two-pipe: build/monitor_two_pipe
	./build/monitor_two_pipe

run-monitor-socketpair: build/monitor_socketpair
	./build/monitor_socketpair

clean:
	rm -rf build

.PHONY: all run run-meminfo run-monitor run-monitor-args run-monitor-log run-monitor-fork run-monitor-exec run-monitor-session run-system-monitor-learn run-pipe-demo run-monitor-two-pipe run-monitor-socketpair clean
