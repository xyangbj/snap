/*
 * Copyright 2016, International Business Machines
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/time.h>
#include <getopt.h>
#include <ctype.h>
#include <stdbool.h>
#include <linux/random.h>

#include <libdonut.h>
#include <donut_tools.h>

#define CACHELINE_BYTES 128

#define	FW_BASE_ADDR	0x00100
#define	FW_BASE_ADDR8	0x00108

/*	Memcopy Action */
#define	ACTION_BASE		0x10000
#define ACTION_CONTEXT_OFFSET	0x01000	/* Add 4 KB for the next Action */
#define	ACTION_CONTROL		ACTION_BASE
#define	ACTION_CONTROL_START	0x01
#define	ACTION_CONTROL_IDLE	0x04
#define	ACTION_CONTROL_RUN	0x08
#define	ACTION_4		(ACTION_BASE + 0x04)
#define	ACTION_8		(ACTION_BASE + 0x08)
#define	ACTION_CONFIG		(ACTION_BASE + 0x10)
#define	ACTION_CONFIG_COUNT	1	/* Count Mode */
#define	ACTION_CONFIG_COPY_HH	2	/* Memcopy Host to Host */
#define	ACTION_CONFIG_COPY_HD	3	/* Memcopy Host to DDR */
#define	ACTION_CONFIG_COPY_DH	4	/* Memcopy DDR to Host */
#define	ACTION_CONFIG_COPY_DD	5	/* Memcopy DDR to DDR */
#define	ACTION_CONFIG_COPY_HDH	6	/* Memcopy Host to DDR to Host */
#define	ACTION_CONFIG_MEMSET_H	8	/* Memset Host Memory */
#define	ACTION_CONFIG_MEMSET_F	9	/* Memset FPGA Memory */

#define	ACTION_SRC_LOW		(ACTION_BASE + 0x14)
#define	ACTION_SRC_HIGH		(ACTION_BASE + 0x18)
#define	ACTION_DEST_LOW		(ACTION_BASE + 0x1c)
#define	ACTION_DEST_HIGH	(ACTION_BASE + 0x20)
#define	ACTION_CNT		(ACTION_BASE + 0x24)	/* Count Register */

/*	defaults */
#define	DEFAULT_MEMCPY_ITER	1
#define ACTION_WAIT_TIME	1000	/* Default in msec */

#define	KILO_BYTE		(1024)
#define	MEGA_BYTE		(1024*1024)

#define DEFAULT_MEM	0xff

static const char *version = GIT_VERSION;
static	int verbose_level = 0;
static int context_offset = 0;

#define PRINTF0(fmt, ...) do {		\
		printf(fmt, ## __VA_ARGS__);	\
	} while (0)

#define PRINTF1(fmt, ...) do {		\
		if (verbose_level > 0)	\
			printf(fmt, ## __VA_ARGS__);	\
	} while (0)

#define PRINTF2(fmt, ...) do {		\
		if (verbose_level > 1)	\
			printf(fmt, ## __VA_ARGS__);	\
	} while (0)


#define PRINTF3(fmt, ...) do {		\
		if (verbose_level > 2)	\
			printf(fmt, ## __VA_ARGS__);	\
	} while (0)

#define PRINTF4(fmt, ...) do {		\
		if (verbose_level > 3)	\
			printf(fmt, ## __VA_ARGS__);	\
	} while (0)

static uint64_t get_usec(void)
{
        struct timeval t;

        gettimeofday(&t, NULL);
        return t.tv_sec * 1000000 + t.tv_usec;
}

static void print_time(uint64_t elapsed)
{
	if (elapsed > 10000)
		PRINTF1(" T = %d msec" ,(int)elapsed/1000);
	else	PRINTF1(" T = %d usec", (int)elapsed);
}

/*
 *	Check Pattern in buffer
 */
static int check_buffer(uint8_t *hb,	/* Host Buffer */
		int begin,		/* Offset in Buffer */
		int size,
		uint8_t pattern)	/* Patthern to check */
{
	int i;
	int rc, bad;
	uint8_t data;

	PRINTF2("\nCheck %d bytes in Buffer Start at: %d for Pattern: 0x%02x",
		size, begin, pattern); 
	rc = bad = 0;
	for (i = 0; i < begin; i++) {
		data = *hb;	/* Get data */
		if (data != DEFAULT_MEM) {
			PRINTF0("\nError1@: %d Expect: 0x%02x Read: 0x%02x",
				i,		/* Address */
				DEFAULT_MEM,	/* What i expect */
				data);		/* What i got */
			bad++;
			if (bad > 10) rc = 1;	/* Exit */
		}
		if (rc) return 1;
		hb++;
	}
	for (i = 0; i < size; i++) {
		data = *hb;	/* Get data */
		if (data != pattern) {
			PRINTF0("\nError@: %d Expect: 0x%02x Read: 0x%02x",
				i,		/* Address */
				pattern,	/* What i expect */
				data);		/* What i got */
			bad++;
			if (bad > 9) rc = 1;	/* Exit */
		}
		if (rc) return 2;
		hb++;
	}
	return 0;
}

/* Action or Kernel Write and Read are 32 bit MMIO */
static void action_write(struct dnut_card* h, uint32_t addr, uint32_t data)
{
	int rc;

	addr += context_offset * ACTION_CONTEXT_OFFSET;
	PRINTF4("mmio_write(0x%08x ,0x%08x)\n", addr, data);
	rc = dnut_mmio_write32(h, (uint64_t)addr, data);
	if (0 != rc)
		PRINTF0("Write MMIO 32 Err\n");
	return;
}

static uint32_t action_read(struct dnut_card* h, uint32_t addr)
{
	int rc;
	uint32_t data;

	addr += context_offset * ACTION_CONTEXT_OFFSET;
	rc = dnut_mmio_read32(h, (uint64_t)addr, &data);
	if (0 != rc)
		PRINTF0("Read MMIO 32 Err\n");
	PRINTF4("mmio_read(0x%08x) -> 0x%08x\n", addr, data);
	return data;
}

/*
 *	Start Action and wait for Idle.
 */
static int action_wait_idle(struct dnut_card* h, int timeout_ms)
{
	uint32_t action_data;
	uint64_t t_start;	/* time in usec */
	uint64_t tout = (uint64_t)timeout_ms * 1000;	/* in usec */
	uint64_t td;		/* Diff time in usec */

	action_write(h, ACTION_CONTROL, 0);
	action_write(h, ACTION_CONTROL, ACTION_CONTROL_START);

	/* Wait for Action to go back to Idle */
	t_start = get_usec();
	do {
		action_data = action_read(h, ACTION_CONTROL);
		td = get_usec() - t_start;
		if (td > tout) {
			PRINTF0("Error. Timeout while Waiting for Idle\n");
			return ETIME;
		}
	} while ((action_data & ACTION_CONTROL_IDLE) == 0);
	print_time(td);
	return 0;
}

static int action_start(struct dnut_card* h,
		int action,
		uint64_t dest,	/* End address for MEMSET */
		uint64_t src,	/* Start address for MEMSET */
		int timeout)
{
	uint64_t addr;
	uint8_t pattern;

	pattern = (action & 0xff00) >> 8;
	switch (action & 0xff) {
	case ACTION_CONFIG_MEMSET_H:
		PRINTF2("\n[Host]");
		break;
	case ACTION_CONFIG_MEMSET_F:
		PRINTF2("\n[FPGA]");
		break;
	default:
		PRINTF0("\nInvalid Action\n");
		return 1;
		break;
	}
	PRINTF2(" Set: %lld Bytes with Pattern: 0x%02x ",
		(long long)(dest-src), pattern);
	action_write(h, ACTION_CONFIG,  action);
	addr = src;
	action_write(h, ACTION_SRC_LOW, (uint32_t)(addr & 0xffffffff));
	action_write(h, ACTION_SRC_HIGH, (uint32_t)(addr >> 32));
	addr = dest;
	action_write(h, ACTION_DEST_LOW, (uint32_t)(addr & 0xffffffff));
	action_write(h, ACTION_DEST_HIGH, (uint32_t)(addr >> 32));

	if (action_wait_idle(h, timeout))
		return 1;
	return 0;
}

static void free_mem(void *buffer)
{
	PRINTF3("Free Host Buffer: %p\n", buffer);
	if (buffer)
		free(buffer);
}

static void usage(const char *prog)
{
	PRINTF0("Usage: %s\n"
		"    -h, --help           print usage information\n"
		"    -v, --verbose        verbose mode\n"
		"    -C, --card <cardno>  use this card for operation\n"
		"    -V, --version\n"
		"    -q, --quiet          quiece output\n"
		"    -z, --context        Use this for MMIO + N x 0x1000 (default 0)\n"
		"    -t, --timeout        timeout in msec (default=1000 ms)\n"
		"    -i, --iter           Number of Iterations (default 1)\n"
		"    -H, --host           Set Host Memory (defualt)\n"
		"    -F, --fpga           Set FPGA Memory\n"
		"    -s, --size           Size to fill (default %d)\n"
		"    -b, --begin          Byte offset to begin set (default 0)\n"
		"    -p, --pattern        Byte Pattern to use (default 0x0)\n"
		"\tSNAP Test tool to Set Memory in Host (Option -H) or FPGA (Option -F) Card\n"
		, prog,
		(4 * KILO_BYTE));
}

int main(int argc, char *argv[])
{
	char device[64];
	struct dnut_card *dn;	/* lib dnut handle */
	int card_no = 0;
	int cmd;
	int rc = 1;
	int i, iter = 1;
	int timeout = ACTION_WAIT_TIME;
	uint32_t size = (4 * KILO_BYTE);
	uint64_t begin = 0;
	uint8_t pattern = 0x0;
	int func = ACTION_CONFIG_MEMSET_H;
	void *hb = NULL;
	int h_mem_size = 0;
	int h_begin = 0;
	uint64_t start, stop;

	while (1) {
                int option_index = 0;
		static struct option long_options[] = {
			{ "card",     required_argument, NULL, 'C' },
			{ "verbose",  no_argument,       NULL, 'v' },
			{ "help",     no_argument,       NULL, 'h' },
			{ "version",  no_argument,       NULL, 'V' },
			{ "quiet",    no_argument,       NULL, 'q' },
			{ "iter",     required_argument, NULL, 'i' },
			{ "context",  required_argument, NULL, 'z' },
			{ "timeout",  required_argument, NULL, 't' },
			{ "host",     required_argument, NULL, 'H' },
			{ "fpga",     required_argument, NULL, 'F' },
			{ "size",     required_argument, NULL, 's' },
			{ "begin",    required_argument, NULL, 'b' },
			{ "pattern",  required_argument, NULL, 'p' },
			{ 0,          no_argument,       NULL, 0   },
		};
		cmd = getopt_long(argc, argv, "C:i:z:t:s:b:p:FHqvVh",
			long_options, &option_index);
		if (cmd == -1)  /* all params processed ? */
			break;

		switch (cmd) {
		case 'v':	/* verbose */
			verbose_level++;
			break;
		case 'V':	/* version */
			PRINTF0("%s\n", version);
			exit(EXIT_SUCCESS);;
		case 'h':	/* help */
			usage(argv[0]);
			exit(EXIT_SUCCESS);;
		case 'C':	/* card */
			card_no = strtol(optarg, (char **)NULL, 0);
			break;
		case 'i':	/* iter */
			iter = strtol(optarg, (char **)NULL, 0);
			break;
		case 'z':	/* context */
			context_offset = strtol(optarg, (char **)NULL, 0);
			break;
		case 't':	/* timeout */
			timeout = strtol(optarg, (char **)NULL, 0) * 1000;
			break;
		case 'H':	/* host */
			func = ACTION_CONFIG_MEMSET_H;
			break;
		case 'F':	/* fpga */
			func = ACTION_CONFIG_MEMSET_F;
			break;
		case 's':	/* size */
			size = strtol(optarg, (char **)NULL, 0);
			break;
		case 'b':	/* begin */
			begin = strtoll(optarg, (char **)NULL, 0);
			break;
		case 'p':	/* pattern */
			pattern = strtol(optarg, (char **)NULL, 0);
			break;
		default:
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (card_no > 3) {
		usage(argv[0]);
		exit(1);
	}

	if (0 == size) {
		PRINTF0("Please Provide a Valid Size. (--size, -s)\n");
		exit(1);
	}

	sprintf(device, "/dev/cxl/afu%d.0m", card_no);
	PRINTF2("Start Memset Test. Timeout: %d msec Device: %s\n",
		timeout, device);

	dn = dnut_card_alloc_dev(device, 0, 0);
	if (NULL == dn) {
		perror("dnut_card_alloc_dev()");
		return -1;
	}

	if (ACTION_CONFIG_MEMSET_H == func) {
		if (size > (32 * MEGA_BYTE)) {
			PRINTF0("Size %d must be less than %d\n", size, (32*MEGA_BYTE));
			goto __exit1;
		}
		h_begin = (int)(begin & 0xF) + 16;
		h_mem_size = (h_begin + size + 32);

		/* Allocate Host Buffer */
		if (posix_memalign((void **)&hb, 4096, h_mem_size) != 0) {
			perror("FAILED: posix_memalign source");
			goto __exit1;
		}
		PRINTF1("Allocate Host Buffer at %p Size: %d Bytes\n", hb, h_mem_size);
	}

	PRINTF1("Fill %d Bytes from %lld to %lld with Pattern 0x%02x\n",
		size, (long long)begin, (long long)begin+size-1, pattern);

	for (i = 0; i < iter; i++) {
		PRINTF1("[%d/%d] Start Memset ", i+1, iter);
		if (ACTION_CONFIG_MEMSET_F == func) {
			start = begin;
			stop = begin + size -1;
			if (action_start(dn, func | (pattern << 8),
					stop, start, timeout))
				goto __exit1;
		}
		if (ACTION_CONFIG_MEMSET_H == func) {
			/* Set Host Buffer */
			memset(hb, DEFAULT_MEM, h_mem_size);
			start = (uint64_t)hb + h_begin;	/* Host Start Address */
			stop = start + size - 1;	/* Host End Address */
			if (action_start(dn, func | (pattern << 8),
					stop, start, timeout))
				goto __exit1;
			if (0 != check_buffer(hb, h_begin, size, pattern))
				goto __exit1;
		}
		PRINTF1(" done\n");
	}
	rc = 0;

__exit1:
	PRINTF3("Close Card Handle: %p\n", dn);
	dnut_card_free(dn);
	free_mem(hb);

	PRINTF2("Exit rc: %d\n", rc);
	return rc;
}