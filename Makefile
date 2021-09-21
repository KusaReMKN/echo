PROGRAM = echod
OBJS = echod.o

VPATH = src

CFLAGS = -O2 -Wall
CC = cc
RM = rm -f

.PHONY: all
all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) -o $@ $^

.PHONY: clean
clean:
	$(RM) $(PROGRAM) $(OBJS)
