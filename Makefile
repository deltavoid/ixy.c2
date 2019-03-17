

NIC     ?= 0000:00:08.0


#CC      := /usr/bin/cc
CC      := x86_64-linux-musl-gcc 
CFLAGS  := -g -O2 -march=native -fomit-frame-pointer -std=c11 -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wformat=2 -std=gnu11 -I src
LDFLAGS := -static

INSTALL_DIR := /home/zhangqianyu/WorkSpace/RcoreNetwork/Rcore/biscuit/user/build/x86_64


SRC_DRIVER := device.c ixgbe.c
COMMON_SRC := $(addprefix driver/,$(SRC_DRIVER)) memory.c pci.c stats.c
COMMON_SRC := $(addprefix src/,$(COMMON_SRC))

SRCS             := $(COMMON_SRC) src/app/ixy-pktgen.c src/app/ixy-fwd.c
IXY_PKTGEN_SRCS  := $(COMMON_SRC) src/app/ixy-pktgen.c
IXY_FORWARD_SRCS := $(COMMON_SRC) src/app/ixy-fwd.c


IXY_PKTGEN_NAME  := ixy-pktgen
IXY_PKTGEN_OBJS  := $(patsubst %.c,%.o,$(IXY_PKTGEN_SRCS))
IXY_FORWARD_NAME := ixy-fwd
IXY_FORWARD_OBJS := $(patsubst %.c,%.o,$(IXY_FORWARD_SRCS))



.PHONY: all build run install clean
all: run

build: $(IXY_PKTGEN_NAME) $(IXY_FORWARD_NAME)


$(IXY_PKTGEN_NAME): $(IXY_PKTGEN_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(IXY_FORWARD_NAME): $(IXY_FORWARD_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

include Makefile.dep
Makefile.dep: $(SRC)
	$(CC) $(CFLAGS) -MM $^ > $@

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<


run: build
#	sudo ../setup-hugetlbfs.sh
	sudo ./$(IXY_PKTGEN_NAME) $(NIC)


install: build
	cp ixy-pktgen $(INSTALL_DIR)

clean:
	rm $(patsubst %.c, %.o, $(SRCS))  $(IXY_PKTGEN_NAME) $(IXY_FORWARD_NAME)