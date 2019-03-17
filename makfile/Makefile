

NIC     ?= 0000:00:08.0

#CC      := /usr/bin/cc
CC      := x86_64-linux-musl-gcc 
CFLAGS  := -g -O2 -march=native -fomit-frame-pointer -std=c11 -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wformat=2 -std=gnu11
LDFLAGS := -static



IXY_PKTGEN_NAME  := ixy-pktgen
IXY_PKTGEN_OBJS  := ixy-pktgen.o device.o ixgbe.o memory.o pci.o stats.o virtio.o
IXY_FORWARD_NAME := ixy-fwd
IXY_FORWARD_OBJS := ixy-fwd.o device.o ixgbe.o memory.o pci.o stats.o virtio.o



.PHONY: all build run clean
all: run

build: $(IXY_PKTGEN_NAME) $(IXY_FORWARD_NAME)


$(IXY_PKTGEN_NAME): $(IXY_PKTGEN_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(IXY_FORWARD_NAME): $(IXY_FORWARD_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

include Makefile.dep
Makefile.dep:
	$(CC) $(CFLAGS) -MM *.c > Makefile.dep

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<


run: build
#	sudo ../setup-hugetlbfs.sh
	sudo ./$(IXY_PKTGEN_NAME) $(NIC)


clean:
	rm *.o Makefile.dep $(IXY_PKTGEN_NAME) $(IXY_FORWARD_NAME)