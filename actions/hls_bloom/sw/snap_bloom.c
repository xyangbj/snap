/*
 * Copyright 2016, 2017 International Business Machines
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <math.h>

#include <snap_tools.h>
#include <action_bloom.h>
#include <libsnap.h>
#include <snap_hls_if.h>

int verbose_flag = 0;

static const char *version = GIT_VERSION;
static const char *bloom_mode_str[] = { "BLOOM" };
static const char *test_choice_str[] = { "SPEED", "BASIC"};

/**
 * @brief	prints valid command line options
 *
 * @param prog	current program's name
 */
static void usage(const char *prog)
{
	printf("Usage: %s [-h] [-v, --verbose] [-V, --version]\n"
	       "  -C, --card <cardno> can be (0...3)\n"
	       "  -x, --threads <threads>   depends on the available CPUs.\n"
	       "  -i, --input <file.bin>    input file.\n"
	       "  -A, --type-in <CARD_RAM, HOST_RAM, ...>.\n"
	       "  -a, --addr-in <addr>      address e.g. in CARD_RAM.\n"
	       "  -s, --size <size>         size of data.\n"
	       "  -c, --choice <SPEED,BASIC> \n"
	       "  -n, --number of perf function calls <nb_calls> \n"
	       "  -N, --number of add/checks calls <count> \n"
	       "  -E, --number of ENTRIES \n"
	       "  -e, --error rate will be 1 divided by value given\n"
	       "  -T, --test                execute a test if available.\n"
	       "  -t, --timeout             Timeout in sec (default 3600 sec).\n"
	       "  -I, --irq                 Enable Interrupts\n"
	       "\n"
	       "Example:\n"
	       "  To run a performance test with 10k add calls with 1M entries, type:\n"
	       "  snap_bloom -I -t200 -cSPEED -n 10000 -E 1000000\n"
	       "  To run a set of predefined basic tests, type:\n"
	       "  snap_bloom -I -t200 -cBASIC\n"
	       "\n",
	       prog);
}

static void snap_prepare_bloom(struct snap_job *cjob,
				  struct bloom_job *mjob_in,
				  struct bloom_job *mjob_out,
				  void *addr_in,
				  uint32_t size_in,
				  uint8_t type_in,
				  uint16_t mode,
				  uint32_t test_choice,
				  uint32_t nb_calls,
				  uint32_t count,
				  uint32_t entries,
				  uint32_t error_rate,
				  uint32_t threads)
{
	snap_addr_set(&mjob_in->in, addr_in, size_in, type_in,
		      SNAP_ADDRFLAG_ADDR | SNAP_ADDRFLAG_SRC |
		      SNAP_ADDRFLAG_END);

	mjob_in->mode = mode;
	mjob_in->test_choice = test_choice;
	mjob_in->nb_calls = nb_calls;
	mjob_in->count = count;
	mjob_in->entries = entries;
	mjob_in->error_rate = error_rate;
	mjob_in->threads = threads; 

	mjob_out->error_rate = 1;
	mjob_out->bits = 0x0;
	mjob_out->bytes = 0x0;
	mjob_out->hashes = 0x0;
	mjob_out->collisions = 0x0;
	snap_job_set(cjob, mjob_in, sizeof(*mjob_in),
		     mjob_out, sizeof(*mjob_out));
}

static inline
ssize_t file_size(const char *fname)
{
	int rc;
	struct stat s;

	rc = lstat(fname, &s);
	if (rc != 0) {
		fprintf(stderr, "err: Cannot find %s!\n", fname);
		return rc;
	}
	return s.st_size;
}

static inline ssize_t
file_read(const char *fname, uint8_t *buff, size_t len)
{
	int rc;
	FILE *fp;

	if ((fname == NULL) || (buff == NULL) || (len == 0))
		return -EINVAL;

	fp = fopen(fname, "r");
	if (!fp) {
		fprintf(stderr, "err: Cannot open file %s: %s\n",
			fname, strerror(errno));
		return -ENODEV;
	}
	rc = fread(buff, len, 1, fp);
	if (rc == -1) {
		fprintf(stderr, "err: Cannot read from %s: %s\n",
			fname, strerror(errno));
		fclose(fp);
		return -EIO;
	}
	fclose(fp);
	return rc;
}

static inline ssize_t
file_write(const char *fname, const uint8_t *buff, size_t len)
{
	int rc;
	FILE *fp;

	if ((fname == NULL) || (buff == NULL) || (len == 0))
		return -EINVAL;

	fp = fopen(fname, "w+");
	if (!fp) {
		fprintf(stderr, "err: Cannot open file %s: %s\n",
			fname, strerror(errno));
		return -ENODEV;
	}
	rc = fwrite(buff, len, 1, fp);
	if (rc == -1) {
		fprintf(stderr, "err: Cannot write to %s: %s\n",
			fname, strerror(errno));
		fclose(fp);
		return -EIO;
	}
	fclose(fp);
	return rc;
}

static int call_bloom(int card_no, unsigned long timeout,
		       unsigned int threads,
		       unsigned long addr_in,
		       unsigned char type_in,  unsigned long size,
		       uint16_t mode,
		       uint16_t test_choice, 
                       uint32_t nb_calls, 
                       uint32_t count, 
                       uint32_t entries,
                       uint32_t error_rate,
		       uint64_t *_usec,
		       uint32_t *_bits,
		       uint32_t *_bytes,
		       uint32_t *_hashes,
		       uint32_t *_collisions,
		       FILE *fp,
		       snap_action_flag_t action_irq)
{
	int rc;
	char device[128];
	struct snap_card *card = NULL;
	struct snap_action *action = NULL;
	struct snap_job cjob;
	struct bloom_job mjob_in, mjob_out;
	struct timeval etime, stime;

	fprintf(fp, "PARAMETERS:\n"
		"  type_in:  %x\n"
		"  addr_in:  %016llx\n"
		"  size:     %08lx\n"
		"  mode:     %d %s\n"
		"  test_choice:%d %s\n"
		"  nb_calls:    %0d\n"
		"  count:    %0d\n"
		"  entries:    %0d\n"
		"  error_rate:  %f\n"
		"  job_size: %ld bytes\n",
		type_in, (long long)addr_in,
		size, mode, bloom_mode_str[mode % BLOOM_MODE_MAX], 
                test_choice, test_choice_str[test_choice % BLOOM_TYPE_MAX],
		nb_calls, count, entries, (double)1./pow(10,error_rate), 
                sizeof(struct bloom_job));

	snprintf(device, sizeof(device)-1, "/dev/cxl/afu%d.0s", card_no);
	card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM,
				   SNAP_DEVICE_ID_SNAP);
	if (card == NULL) {
		fprintf(stderr, "err: failed to open card %u: %s\n",
			card_no, strerror(errno));
		goto out_error;
	}

	action = snap_attach_action(card, BLOOM_ACTION_TYPE, action_irq, 60);
	if (action == NULL) {
		fprintf(stderr, "err: failed to attach action %u: %s\n",
			card_no, strerror(errno));
		goto out_error1;
	}

	snap_prepare_bloom(&cjob, &mjob_in, &mjob_out,
			     (void *)addr_in, size, type_in,
			      mode, test_choice, nb_calls, count, entries,
			      error_rate, threads);

	gettimeofday(&stime, NULL);
	rc = snap_action_sync_execute_job(action, &cjob, timeout);
	gettimeofday(&etime, NULL);
	if (rc != 0) {
		fprintf(stderr, "err: job execution %d: %s!\n", rc,
			strerror(errno));
		goto out_error2;
	}

	fprintf(fp, "------------------\n"
                "RETC=%x => %s\n"
		"NB_CALLS=%d\n"
		"COUNT=%d\n"
		"NB_ENTRIES=%d\n"
		"ERROR_RATE=%f\n"
		"NB_BITS=%d\n"
		"NB_BYTES=%d\n"
		"NB_HASHES=%d\n"
		"COLLISIONS=%0d\n"
		"%lld usec\n"
		"------------------\n",
		cjob.retc, (cjob.retc == SNAP_RETC_SUCCESS ? "SUCCESS" : "FAILURE"), 
                mjob_out.nb_calls,
		mjob_out.count,
		mjob_out.entries,
		(double)1./pow(10,error_rate), 
		mjob_out.bits,
		mjob_out.bytes,
		mjob_out.hashes,
		mjob_out.collisions,
		(long long)timediff_usec(&etime, &stime));

        if(test_choice == BLOOM_SPEED) {

	    fprintf(fp, "%lld bloom filter elements added calls\n", 
                 (long long)(mjob_out.nb_calls*mjob_out.count));
	    fprintf(fp, "%.3f bloom filter elements added / Second.\n",
		((double)(1000000 * mjob_out.nb_calls*mjob_out.count)) / 
                 (double)(timediff_usec(&etime, &stime)));
        }

	snap_detach_action(action);
	snap_card_free(card);

	if (_usec)
		*_usec = timediff_usec(&etime, &stime);
	if (_bits)
		*_bits = mjob_out.bits;
	if (_bytes)
		*_bytes = mjob_out.bytes;
	if (_hashes)
		*_hashes = mjob_out.hashes;
	if (_collisions)
		*_collisions = mjob_out.collisions;

	return 0;

 out_error2:
	snap_detach_action(action);
 out_error1:
	snap_card_free(card);
 out_error:
	return -1;
}


/**
 * Read accelerator specific registers. Must be called as root!
 */
int main(int argc, char *argv[])
{
	int ch, rc = 0;
	int card_no = 0;
	const char *input = NULL;
	unsigned long timeout = 60 * 60; /* 1 hour */
	const char *space = "CARD_RAM";
	ssize_t size = 1024 * 1024;
	uint8_t *ibuff = NULL;
	uint8_t type_in = SNAP_ADDRTYPE_HOST_DRAM;
	uint64_t addr_in = 0x0ull;
	uint16_t mode = BLOOM;
	uint16_t test_choice = BLOOM_SPEED;
	uint32_t nb_calls = 1, count = 1000, entries = 1000;
	uint32_t error_rate = 1;
	int test = 0;
	unsigned int threads = 160;
	snap_action_flag_t action_irq = 0;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{ "card",	 required_argument, NULL, 'C' },
			{ "threads",	 required_argument, NULL, 'x' },
			{ "input",	 required_argument, NULL, 'i' },
			{ "src-type",	 required_argument, NULL, 'A' },
			{ "src-addr",	 required_argument, NULL, 'a' },
			{ "size",	 required_argument, NULL, 's' },
			{ "timeout",	 required_argument, NULL, 't' },
			{ "test",	 no_argument,       NULL, 'T' },
			{ "test_choice", required_argument, NULL, 'c' },
			{ "nb_calls",    required_argument, NULL, 'n' },
			{ "count",       required_argument, NULL, 'N' },
			{ "entries",     required_argument, NULL, 'E' },
			{ "error_rate",  required_argument, NULL, 'e' },
			{ "version",	 no_argument,	    NULL, 'V' },
			{ "verbose",	 no_argument,	    NULL, 'v' },
			{ "help",	 no_argument,	    NULL, 'h' },
			{ "irq", 	 no_argument,	    NULL, 'I' },
			{ 0,		 no_argument,	    NULL, 0   },
		};

		ch = getopt_long(argc, argv,
				 "A:C:i:a:S:x:c:n:N:E:e:s:t:x:VqvhIT",
				 long_options, &option_index);
		if (ch == -1)
			break;

		switch (ch) {
		case 'C':
			card_no = strtol(optarg, (char **)NULL, 0);
			break;
		case 'x':
			threads = strtol(optarg, (char **)NULL, 0);
			break;
		case 'i':
			input = optarg;
			break;
		case 's':
			size = __str_to_num(optarg);
			break;
		case 'T':
			test++;
			break;
		case 'c':
			if (strcmp(optarg, "SPEED") == 0) {
				test_choice = BLOOM_SPEED;
				break;
			}
			if (strcmp(optarg, "BASIC") == 0) {
				test_choice = BLOOM_BASIC;
				break;
			}
			test_choice = strtol(optarg, (char **)NULL, 0);
			break;
		case 'n':
			nb_calls =  __str_to_num(optarg);
			break;
		case 'N':
			count =  __str_to_num(optarg);
			break;
		case 'E':
			entries =  __str_to_num(optarg);
			break;
		case 'e':
			error_rate =  __str_to_num(optarg);
			break;
		case 't':
			timeout = strtol(optarg, (char **)NULL, 0);
			break;
			/* input data */
		case 'A':
			space = optarg;
			if (strcmp(space, "CARD_DRAM") == 0)
				type_in = SNAP_ADDRTYPE_CARD_DRAM;
			else if (strcmp(space, "HOST_DRAM") == 0)
				type_in = SNAP_ADDRTYPE_HOST_DRAM;
			break;
		case 'a':
			addr_in = strtol(optarg, (char **)NULL, 0);
			break;
			/* service */
		case 'V':
			printf("%s\n", version);
			exit(EXIT_SUCCESS);
		case 'v':
			verbose_flag = 1;
			break;
		case 'h':
			usage(argv[0]);
			exit(EXIT_SUCCESS);
			break;
		case 'I':
			action_irq = (SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);
			break;
		default:
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (optind != argc) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	/* if input file is defined, use that as input */
	if (input != NULL) {
		size = file_size(input);
		if (size < 0)
			goto out_error1;

		/* source buffer */
		ibuff = snap_malloc(size);
		if (ibuff == NULL)
			goto out_error;

		fprintf(stdout, "reading input data %d bytes from %s\n",
			(int)size, input);

		rc = file_read(input, ibuff, size);
		if (rc < 0)
			goto out_error1;

		type_in = SNAP_ADDRTYPE_HOST_DRAM;
		addr_in = (unsigned long)ibuff;
	}

	if (test) {
		switch (mode) {
		default:
			goto out_error1;
		}
	} else {
		rc = call_bloom(card_no, timeout, threads, addr_in,
				 type_in, size, mode,
				 test_choice, nb_calls, count, 
                                 entries, error_rate, NULL, NULL, NULL,
				 NULL, NULL, stderr, action_irq);
		if (rc != 0)
			goto out_error1;
	}

	if (ibuff)
		free(ibuff);

	exit(EXIT_SUCCESS);

 out_error1:
	if (ibuff)
		free(ibuff);
 out_error:
	exit(EXIT_FAILURE);
}
