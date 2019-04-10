

NIC     ?= 0000:02:00.0

CC      := gcc
LD      := ld
CFLAGS  := -g -O2 -march=native -fomit-frame-pointer -std=c11 -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wformat=2 -std=gnu11 -I src
LDFLAGS := -static



SRC_DIR     := src src/driver
COMMON_SRCS := $(wildcard $(addsuffix /*.c, $(SRC_DIR)))

SRCS             := $(COMMON_SRCS) $(wildcard src/app/*.c)
OBJS             := $(patsubst %.c,%.o,$(SRCS))

IXY_LIB_NAME     := bin/libixy.o
IXY_LIB_SRCS     := $(COMMON_SRCS)
IXY_LIB_OBJS     := $(patsubst %.c, %.o, $(IXY_LIB_SRCS))

IXY_PKTGEN_NAME  := bin/ixy-pktgen
IXY_PKTGEN_SRCS  := $(COMMON_SRCS) src/app/ixy-pktgen.c
IXY_PKTGEN_OBJS  := $(patsubst %.c,%.o,$(IXY_PKTGEN_SRCS))

IXY_FORWARD_NAME := bin/ixy-fwd
IXY_FORWARD_SRCS := $(COMMON_SRCS) src/app/ixy-fwd.c
IXY_FORWARD_OBJS := $(patsubst %.c,%.o,$(IXY_FORWARD_SRCS))



.PHONY: all build run clean
all: run

build: $(IXY_PKTGEN_NAME) $(IXY_FORWARD_NAME) $(IXY_LIB_NAME)
lib: $(IXY_LIB_NAME)

$(IXY_PKTGEN_NAME): $(IXY_PKTGEN_OBJS)
	@echo IXY_PKTGEN_SRCS: $(IXY_PKTGEN_SRCS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(IXY_FORWARD_NAME): $(IXY_FORWARD_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(IXY_LIB_NAME): $(IXY_LIB_OBJS)
	$(LD) -r -o $@ $^ $(LDFLAGS)

include Makefile.dep
Makefile.dep: $(SRCS)
	@echo SRCS: $(SRCS)
	$(CC) $(CFLAGS) -MM $^ > $@
	-mkdir bin

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<


run: build
#	sudo ../setup-hugetlbfs.sh
	sudo ./$(IXY_PKTGEN_NAME) $(NIC)


clean:
	rm $(OBJS)  $(IXY_PKTGEN_NAME) $(IXY_FORWARD_NAME) Makefile.dep