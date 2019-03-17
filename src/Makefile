

NIC     ?= 0000:00:08.0

CC      := /usr/bin/cc
#CC      := x86_64-linux-musl-gcc 
CFLAGS  := -g -O2 -march=native -fomit-frame-pointer -std=c11 -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wformat=2 -std=gnu11 -I.
LDFLAGS := -static



SRC_DRIVER := device.c ixgbe.c virtio.c
COMMON_SRC :=  $(addprefix driver/,$(SRC_DRIVER)) memory.c pci.c stats.c
SRC := $(COMMON_SRC) app/ixy-pktgen.c app/ixy-fwd.c 


IXY_PKTGEN_NAME  := ixy-pktgen
IXY_PKTGEN_OBJS  := app/ixy-pktgen.o $(patsubst %.c,%.o,$(COMMON_SRC))
IXY_FORWARD_NAME := ixy-fwd
IXY_FORWARD_OBJS := app/ixy-fwd.o $(patsubst %.c,%.o,$(COMMON_SRC))



.PHONY: all build run clean
all: run

build: $(IXY_PKTGEN_NAME) $(IXY_FORWARD_NAME)


$(IXY_PKTGEN_NAME): $(IXY_PKTGEN_OBJS)
	echo $(SRC)
	echo $(COMMON_SRC)
	echo $(IXY_PKTGEN_OBJS)
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


clean:
	rm $(patsubst %.c, %.o, $(SRC))  $(IXY_PKTGEN_NAME) $(IXY_FORWARD_NAME)