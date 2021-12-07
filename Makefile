#Makefile/lakin/4760/Project6

GCC= gcc
CFLAGS= -g -Wall

all: oss user_proc

clean:
	rm *.o oss user_proc

oss: oss.o queue.o osclock.o shm.o logger.o ipcm.o memory.o
	$(GCC) $(CFLAGS) oss.o queue.o osclock.o shm.o logger.o -o oss

oss.o: oss.c oss.h config.h osclock.h
	$(GCC) $(CFLAGS) -c oss.c

user_proc: user_proc.o shm.o logger.o osclock.o queue.o ipcm.o memory.o
	$(GCC)  $(CFLAGS) user_proc.o shm.o logger.o osclock.o queue.o -o user_proc

user_proc.o: user_proc.c user_proc.h
	$(GCC) $(CFLAGS) -c user_proc.c

osclock.o: osclock.c osclock.h shm.h
	$(GCC)  $(CFLAGS) -c osclock.c

shm.o: shm.c shm.h
	$(GCC)  $(CFLAGS) -c shm.c

queue.o: queue.c queue.h
	$(GCC) $(CFLAGS) -c queue.c

logger.o: logger.c logger.h
	$(GCC) $(CFLAGS) -c logger.c

memory.o: memory.c memory.h
	$(GCC) $(CFLAGS) -c memory.c

ipcm.o: ipcm.c ipcm.h config.h
	$(GCC) $(CFLAGS) -c ipcm.c
