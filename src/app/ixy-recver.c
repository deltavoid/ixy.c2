#include <stdio.h>
#include <unistd.h>

#include "stats.h"
#include "log.h"
#include "memory.h"
#include "driver/device.h"



// number of packets sent simultaneously to our driver
//static const uint32_t BATCH_SIZE = 64;
#define BATCH_SIZE 16

// excluding CRC (offloaded by default)
#define PKT_SIZE 60

static const uint8_t pkt_data[] = {
	0x00, 0x16, 0x31, 0xff, 0xa6, 0xa8, // dst MAC
	0x00, 0x16, 0x31, 0xff, 0xa6, 0xa9, // src MAC
	0x08, 0x00,                         // ether type: IPv4
	
	0x45, 0x00,                         // Version, IHL, TOS
	(PKT_SIZE - 14) >> 8,               // ip len excluding ethernet, high byte
	(PKT_SIZE - 14) & 0xFF,             // ip len exlucding ethernet, low byte
	0x00, 0x00, 0x00, 0x00,             // id, flags, fragmentation
	0x40, 0x11, 0x00, 0x00,             // TTL (64), protocol (UDP), checksum
	0x0A, 0x00, 0x00, 0x01,             // src ip (10.0.0.1)
	0x0A, 0x00, 0x00, 0x02,             // dst ip (10.0.0.2)
	
	0x00, 0x2A, 0x05, 0x31,             // src and dst ports (42 -> 1337)
	(PKT_SIZE - 20 - 14) >> 8,          // udp len excluding ip & ethernet, high byte
	(PKT_SIZE - 20 - 14) & 0xFF,        // udp len exlucding ip & ethernet, low byte
	0x00, 0x00,                         // udp checksum, optional
	'i', 'x', 'y'                       // payload
	// rest of the payload is zero-filled because mempools guarantee empty bufs
};

// calculate a IP/TCP/UDP checksum
static uint16_t calc_ip_checksum(uint8_t* data, uint32_t len) {
	if (len % 1) error("odd-sized checksums NYI"); // we don't need that
	uint32_t cs = 0;
	for (uint32_t i = 0; i < len / 2; i++) {
		cs += ((uint16_t*)data)[i];
		if (cs > 0xFFFF) {
			cs = (cs & 0xFFFF) + 1; // 16 bit one's complement
		}
	}
	return ~((uint16_t) cs);
}

static struct mempool* init_mempool() {
	const int NUM_BUFS = 2048;
	struct mempool* mempool = memory_allocate_mempool(NUM_BUFS, 0);
	// pre-fill all our packet buffers with some templates that can be modified later
	// we have to do it like this because sending is async in the hardware; we cannot re-use a buffer immediately
	struct pkt_buf* bufs[NUM_BUFS];
	for (int buf_id = 0; buf_id < NUM_BUFS; buf_id++) {
		struct pkt_buf* buf = pkt_buf_alloc(mempool);
		buf->size = PKT_SIZE;
		memcpy(buf->data, pkt_data, sizeof(pkt_data));
		buf->data[14 + 20 - 1] = buf_id & 0xff;  // dst ip

		*(uint16_t*) (buf->data + 24) = calc_ip_checksum(buf->data + 14, 20);
		bufs[buf_id] = buf;
	}
	// return them all to the mempool, all future allocations will return bufs with the data set above
	for (int buf_id = 0; buf_id < NUM_BUFS; buf_id++) {
		pkt_buf_free(bufs[buf_id]);
	}

	return mempool;
}


static void show_pkt(struct pkt_buf* pkb)
{
	printf("len: %u  hash: %x\n", pkb->size, pkb->hash);
    for (unsigned int i = 0; i < pkb->size; i++)
    {   printf("%x ", pkb->data[i]);
        if  (i % 4 == 3)  printf("\n");
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("Usage: %s <pci bus id>\n", argv[0]);
		return 1;
	}

    int num_queues = 8;
    struct ixy_device* dev = ixy_init(argv[1], num_queues, num_queues);
    struct pkt_buf* rx_bufs[BATCH_SIZE];

	uint64_t counter = 0;

    while (true)
    {
        usleep(1000000);
		printf("counter: %llu\n", counter++);

        for (unsigned i = 0; i < num_queues; i++)
        {
            uint32_t num_rx = ixy_rx_batch(dev, i, rx_bufs, BATCH_SIZE);
            printf("queue %u, num_rx %u\n", i, num_rx);

            for (unsigned j = 0; j < num_rx; j++)
            {   printf("j: %u\n", j);
                show_pkt(rx_bufs[j]);
                pkt_buf_free(rx_bufs[j]);
            }
        }
    }

    return 0;
}


