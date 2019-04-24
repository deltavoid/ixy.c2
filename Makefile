

NIC     ?= 0000:02:00.0
NIC1    ?= 0000:02:00.1

CC      := /usr/bin/cc 
CFLAGS  := -g -O2 -march=native -fomit-frame-pointer -std=c11 -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wformat=2 -std=gnu11 -I src
LDFLAGS := -static



SRC_DIR     := src src/driver
COMMON_SRCS := $(wildcard $(addsuffix /*.c, $(SRC_DIR)))

SRCS             := $(COMMON_SRCS) $(wildcard src/app/*.c)
OBJS             := $(patsubst %.c,%.o,$(SRCS))



IXY_PKTGEN_NAME  := bin/ixy-pktgen
IXY_PKTGEN_SRCS  := $(COMMON_SRCS) src/app/ixy-pktgen.c
IXY_PKTGEN_OBJS  := $(patsubst %.c,%.o,$(IXY_PKTGEN_SRCS))

IXY_FORWARD_NAME := bin/ixy-fwd
IXY_FORWARD_SRCS := $(COMMON_SRCS) src/app/ixy-fwd.c
IXY_FORWARD_OBJS := $(patsubst %.c,%.o,$(IXY_FORWARD_SRCS))

IXY_LOOP_NAME := bin/ixy-loop
IXY_LOOP_SRCS := $(COMMON_SRCS) src/app/ixy-loop.c
IXY_LOOP_OBJS := $(patsubst %.c,%.o,$(IXY_LOOP_SRCS))

IXY_RECVER_NAME := bin/ixy-recver
IXY_RECVER_SRCS := $(COMMON_SRCS) src/app/ixy-recver.c
IXY_RECVER_OBJS := $(patsubst %.c,%.o,$(IXY_RECVER_SRCS))



.PHONY: all build run clean
all: run

build: $(IXY_PKTGEN_NAME) $(IXY_FORWARD_NAME) $(IXY_LOOP_NAME) $(IXY_RECVER_NAME)


$(IXY_PKTGEN_NAME): $(IXY_PKTGEN_OBJS)
	@echo IXY_PKTGEN_SRCS: $(IXY_PKTGEN_SRCS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(IXY_FORWARD_NAME): $(IXY_FORWARD_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(IXY_LOOP_NAME): $(IXY_LOOP_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(IXY_RECVER_NAME): $(IXY_RECVER_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)


include Makefile.dep
Makefile.dep: $(SRCS)
	@echo SRCS: $(SRCS)
	$(CC) $(CFLAGS) -MM $^ > $@
	-mkdir bin

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<


run: build
#	sudo ../setup-hugetlbfs.sh
#	sudo ./$(IXY_PKTGEN_NAME) $(NIC)
#	sudo ./$(IXY_LOOP_NAME) $(NIC) $(NIC1)
	sudo ./$(IXY_RECVER_NAME) $(NIC);


clean:
	rm $(OBJS)  $(IXY_PKTGEN_NAME) $(IXY_FORWARD_NAME) $(IXY_LOOP_NAME) Makefile.dep