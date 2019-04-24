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
		//buf->data[14 + 20 - 1] = buf_id & 0xff;  // dst ip
        buf->data[14 + 20 - 1] = buf_id & 0x2f;  // dst ip
		*(uint16_t*) (buf->data + 24) = calc_ip_checksum(buf->data + 14, 20);
		bufs[buf_id] = buf;
	}
	// return them all to the mempool, all future allocations will return bufs with the data set above
	for (int buf_id = 0; buf_id < NUM_BUFS; buf_id++) {
		pkt_buf_free(bufs[buf_id]);
	}

	return mempool;
}

// int main(int argc, char* argv[]) {
// 	if (argc != 2) {
// 		printf("Usage: %s <pci bus id>\n", argv[0]);
// 		return 1;
// 	}

// 	struct mempool* mempool = init_mempool();
// 	struct ixy_device* dev = ixy_init(argv[1], 8, 8);

// 	// uint64_t last_stats_printed = monotonic_time();
// 	// uint64_t counter = 0;
// 	// struct device_stats stats_old, stats;
// 	// stats_init(&stats, dev);
// 	// stats_init(&stats_old, dev);
// 	uint32_t seq_num = 0;

// 	// array of bufs sent out in a batch
// 	struct pkt_buf* bufs[BATCH_SIZE];

// 	// tx loop
// 	while (true) {
// 		// we cannot immediately recycle packets, we need to allocate new packets every time
// 		// the old packets might still be used by the NIC: tx is async
// 		pkt_buf_alloc_batch(mempool, bufs, BATCH_SIZE);
// 		for (uint32_t i = 0; i < BATCH_SIZE; i++) {
// 			// packets can be modified here, make sure to update the checksum when changing the IP header
// 			*(uint32_t*)(bufs[i]->data + PKT_SIZE - 4) = seq_num++;
// 		}
// 		// the packets could be modified here to generate multiple flows
// 		ixy_tx_batch_busy_wait(dev, counter & 0x3, bufs, BATCH_SIZE);

// 		// // don't check time for every packet, this yields +10% performance :)
// 		// if ((counter++ & 0xFFF) == 0) {
// 		// 	uint64_t time = monotonic_time();
// 		// 	if (time - last_stats_printed > 1000 * 1000 * 1000) {
// 		// 		// every second
// 		// 		ixy_read_stats(dev, &stats);
// 		// 		print_stats_diff(&stats, &stats_old, time - last_stats_printed);
// 		// 		stats_old = stats;
// 		// 		last_stats_printed = time;
// 		// 	}
// 		// }
// 		// track stats
// 	}
// 	return 0;
// }



// static void forward(struct ixy_device* rx_dev, uint16_t rx_queue, struct ixy_device* tx_dev, uint16_t tx_queue) {
// 	struct pkt_buf* bufs[BATCH_SIZE];
// 	uint32_t num_rx = ixy_rx_batch(rx_dev, rx_queue, bufs, BATCH_SIZE);
// 	if (num_rx > 0) {
// 		// touch all packets, otherwise it's a completely unrealistic workload if the packet just stays in L3
// 		for (uint32_t i = 0; i < num_rx; i++) {
// 			bufs[i]->data[1]++;
// 		}
// 		uint32_t num_tx = ixy_tx_batch(tx_dev, tx_queue, bufs, num_rx);
// 		// there are two ways to handle the case that packets are not being sent out:
// 		// either wait on tx or drop them; in this case it's better to drop them, otherwise we accumulate latency
// 		for (uint32_t i = num_tx; i < num_rx; i++) {
// 			pkt_buf_free(bufs[i]);
// 		}
// 	}
// }


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
	if (argc != 3) {
		printf("%s forwards packets between two ports.\n", argv[0]);
		printf("Usage: %s <pci bus id2> <pci bus id1>\n", argv[0]);
		return 1;
	}


    struct mempool* mempool = init_mempool();

    uint32_t num_queues = 8;
    //tx dev
	struct ixy_device* dev1 = ixy_init(argv[1], 1, 1);
	//rx dev
    struct ixy_device* dev2 = ixy_init(argv[2], num_queues, num_queues);


    struct pkt_buf* tx_bufs[BATCH_SIZE];
    struct pkt_buf* rx_bufs[BATCH_SIZE];

    uint32_t seq_num = 0;


	// uint64_t last_stats_printed = monotonic_time();
	// struct device_stats stats1, stats1_old;
	// struct device_stats stats2, stats2_old;
	// stats_init(&stats1, dev1);
	// stats_init(&stats1_old, dev1);
	// stats_init(&stats2, dev2);
	// stats_init(&stats2_old, dev2);

	uint64_t counter = 0;
	// while (true) {
	// 	forward(dev1, 0, dev2, 0);
	// 	forward(dev2, 0, dev1, 0);

	// 	// // don't poll the time unnecessarily
	// 	// if ((counter++ & 0xFFF) == 0) {
	// 	// 	uint64_t time = monotonic_time();
	// 	// 	if (time - last_stats_printed > 1000 * 1000 * 1000) {
	// 	// 		// every second
	// 	// 		ixy_read_stats(dev1, &stats1);
	// 	// 		print_stats_diff(&stats1, &stats1_old, time - last_stats_printed);
	// 	// 		stats1_old = stats1;
	// 	// 		if (dev1 != dev2) {
	// 	// 			ixy_read_stats(dev2, &stats2);
	// 	// 			print_stats_diff(&stats2, &stats2_old, time - last_stats_printed);
	// 	// 			stats2_old = stats2;
	// 	// 		}
	// 	// 		last_stats_printed = time;
	// 	// 	}
	// 	// }
	// }


    while (true)
    {
		printf("%d\n", counter++);
        //tx
		// we cannot immediately recycle packets, we need to allocate new packets every time
		// the old packets might still be used by the NIC: tx is async
		pkt_buf_alloc_batch(mempool, tx_bufs, BATCH_SIZE);
		for (uint32_t i = 0; i < BATCH_SIZE; i++) {
			// packets can be modified here, make sure to update the checksum when changing the IP header
			*(uint32_t*)(tx_bufs[i]->data + PKT_SIZE - 4) = seq_num++;
		}
		// the packets could be modified here to generate multiple flows
		ixy_tx_batch_busy_wait(dev1,0, tx_bufs, BATCH_SIZE);


        usleep(1000000);
        //rx
        for (int i = 0; i < num_queues; i++)
        {
            uint32_t num_rx = ixy_rx_batch(dev2, i, rx_bufs, BATCH_SIZE);
            printf("queue %d, num_rx: %d\n", i, num_rx);
            
			for (unsigned int j = 0; j < num_rx; j++)
            {   printf("j:%d\n", j);
				show_pkt(rx_bufs[j]);
                pkt_buf_free(rx_bufs[j]);
            }
        }


    }
}


