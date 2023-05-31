CC = gcc
CFLAGS = -g3 -std=c99 -pedantic -Wall


gpuMonitor: gpuMonitor.o
	${CC} ${CFLAGS} -o $@ $^ -ldcgm

gpuMonitor.o: gpu_monitoring.c
	${CC} ${CFLAGS} -o $@ -c $^

