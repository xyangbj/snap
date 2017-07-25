/*
 * Copyright 2016, 2017, International Business Machines
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


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#include <libsnap.h>
#include <snap_internal.h>
#include <action_bloom.h>
#include <bloom.h>
#include <murmurhash2.h>

static int mmio_write32(struct snap_card *card,
			uint64_t offs, uint32_t data)
{
	act_trace("  %s(%p, %llx, %x)\n", __func__, card,
		  (long long)offs, data);
	return 0;
}

static int mmio_read32(struct snap_card *card,
		       uint64_t offs, uint32_t *data)
{
	act_trace("  %s(%p, %llx, %x)\n", __func__, card,
		  (long long)offs, *data);
	return 0;
}

/** ***************************************************************************
 * A few simple tests to check if it works at all.
 *
 */
//static int basic()
static int basic(struct bloom * bloom)
{
  printf("----- basic -----\n");

  //struct bloom bloom;
  char rc=0;

//  assert(bloom_init(&bloom, 0, 1.0) == 1);
  rc |= (bloom_init(bloom, 0, 1.0) == 1 ? 0 : 1);
//  assert(bloom_init(&bloom, 10, 0) == 1);
  rc |= (bloom_init(bloom, 10, 0) == 1 ? 0 : 1);
//  assert(bloom.ready == 0);
  rc |= (bloom->ready == 0 ? 0 : 1);

//  assert(bloom_add(&bloom, "hello world", 11) == -1);
  rc |= (bloom_add(bloom, "hello world", 11) == -1 ? 0 : 1);
//  assert(bloom_check(&bloom, "hello world", 11) == -1);
  rc |= (bloom_check(bloom, "hello world", 11) == -1 ? 0 : 1);

//  bloom_free(&bloom);

//  assert(bloom_init(&bloom, 1002, 0.1) == 0);
  rc |= (bloom_init(bloom, 1002, 0.1) == 0 ? 0 : 1);

//  assert(bloom.ready == 1);
    rc |= (bloom->ready == 1 ? 0 : 1);

  bloom_print(bloom);

  /* check that "hello world" is NOT found - add it if not found*/
  //assert(bloom_check(&bloom, "hello world", 11) == 0);
  rc |= (bloom_check(bloom, "hello world", 11) == 0 ? 0 : 1);

  //assert(bloom_add(&bloom, "hello world", 11) == 0);
  rc |= (bloom_add(bloom, "hello world", 11) == 0 ? 0 : 1);

  /* check that "hello world" is found */
  //assert(bloom_check(&bloom, "hello world", 11) == 1);
  rc |= (bloom_check(bloom, "hello world", 11) == 1 ? 0 : 1);

  /* add once more "hello world" */
  //assert(bloom_add(&bloom, "hello world", 11) > 0);
  rc |= (bloom_add(bloom, "hello world", 11) > 0 ? 0 : 1);

  /* add twice just "hello" */
  //assert(bloom_add(&bloom, "hello", 5) == 0);
  rc |= (bloom_add(bloom, "hello", 5) == 0 ? 0 : 1);
  //assert(bloom_add(&bloom, "hello", 5) > 0);
  rc |= (bloom_add(bloom, "hello", 5) > 0 ? 0 : 1);

  /* check that "hello" is found */
  //assert(bloom_check(&bloom, "hello", 5) == 1);
  rc |= (bloom_check(bloom, "hello", 5) == 1 ? 0 : 1);
  //bloom_free(&bloom);
  //
  return rc;
}
/** ***************************************************************************
 * Simple loop to compare performance.
 *
 */
//static int perf_loop(int entries, int count)
static int perf_loop(int entries, int count,
		unsigned int error_rate, struct bloom * bloom)
{
  char rc;
  unsigned int j;
  double error = 1.0;
  printf("----- perf_loop -----\n");

  //struct bloom bloom;
  //assert(bloom_init(&bloom, entries, 0.001) == 0);
  for(j = 0; j < error_rate; j++)
  	error = error / 10.;
  rc = (bloom_init(bloom, entries, error) == 0 ? 0 : 1);
  bloom_print(bloom);

  int i;
  int collisions = 0;

  //struct timeval tp;
  //gettimeofday(&tp, NULL);
  //long before = (tp.tv_sec * 1000L) + (tp.tv_usec / 1000L);

  for (i = 0; i < count; i++) {
//#pragma HLS UNROLL factor=16      // PARALLELIZATION FACTOR
    //if (bloom_add(&bloom, (const char *) &i, sizeof(int))) { collisions++; }
	char buffer[32];
	int k;
	for (k = 0; k < 32; k++)
//#pragma HLS PIPELINE
		buffer[k] = (char)((i) >> k*8);

	if (bloom_add(bloom, buffer, sizeof(int))) { collisions++; }
  }

  //gettimeofday(&tp, NULL);
  //long after = (tp.tv_sec * 1000L) + (tp.tv_usec / 1000L);

  printf("Added %d elements of size %d,  (collisions=%d)\n",
         count, (int)sizeof(int), collisions);

  printf("%d,%d\n", entries, bloom->bytes);

  bloom_print(bloom);
  bloom_free(bloom);

  return collisions;
}


#if defined(CONFIG_USE_NO_PTHREADS)
static uint64_t bloom_main(uint32_t test_choice, uint32_t nb_calls, 
                            uint32_t count, uint32_t entries,
                            uint32_t error_rate, struct bloom * bloom,
                            uint32_t threads __attribute__((unused)))
{
        uint32_t run_number;
        uint32_t collisions=0; 
        int rc=1;

        act_trace("%s(%d, %d)\n", __func__, nb_calls, entries);
        act_trace("  sw: NB_CALLS=%d COUNT=%d\n", nb_calls, count);
        switch(test_choice) {
        case(BLOOM_SPEED):
        {
                for (run_number = 0; run_number < nb_calls; run_number++) {
                       uint64_t collisions_tmp;

                       act_trace("  run_number=%d\n", run_number);
                       collisions_tmp = perf_loop(entries, count, error_rate, bloom);
                       collisions += collisions_tmp;
                       act_trace("    %016llx %016llx\n",
                               (long long)collisions_tmp,
                               (long long)collisions);
                }
                rc = 0;
        }
                break;
        case(BLOOM_BASIC):
                rc = basic(bloom); // no parameters
                collisions = 0;
                break;
        default:
                rc = 1;
                break;
        }

        act_trace("collisions=%016llx\n", (unsigned long long)collisions);
        return collisions;
}

#else
/* === START ============ PART NOT PORTED AND DEBUGGED YET ============= */
/*#include <pthread.h>

struct thread_data {
        pthread_t thread_id;    ** Thread id assigned by pthread_create() **
        unsigned int run_number;
        uint32_t test_choice;
        uint32_t nb_calls;
        uint32_t freq;
        uint64_t checksum;
        int thread_rc;
};

static struct thread_data *d;

static void *bloom_thread(void *data)
{
        struct thread_data *d = (struct thread_data *)data;

d  checksum = 0;
        d->threa _rc = 0;
        switch(d->test_choice) {
        case(BLOOM_SPEED):
                d->checksum = test_speed(d->run_number, d->nb_calls, d->freq);
                break;
        case(BLOOM_BASIC):
                d->checksum = (uint64_t)test_sha3();
                d->checksum += (uint64_t)test_shake();
                break;
        default:
                d->checksum = 1;
                break;
        }
        pthread_exit(&d->thread_rc);
}
static uint64_t bloom_main(uint32_t test_choice, uint32_t nb_calls, uint32_t freq, uint32_t _threads)
{
       int rc;
        uint32_t run_number;
        uint64_t checksum = 0;

        if (_threads == 0) {
                fprintf(stderr, "err: Min threads must be 1\n");
                return 0;
        }

        d = calloc(_threads * sizeof(struct thread_data), 1);
        if (d == NULL) {
                fprintf(stderr, "err: No memory available\n");
                return 0;
        }

        act_trace("%s(%d, %d, %d)\n", __func__, nb_calls, freq, _threads);
        act_trace("  NB_TEST_RUNS=%d NB_ROUNDS=%d\n", NB_TEST_RUNS, NB_ROUNDS);
        for (run_number = 0; run_number < NB_TEST_RUNS; ) {
                unsigned int i;
                unsigned int remaining_run_number = NB_TEST_RUNS - run_number;
                unsigned int threads = MIN(remaining_run_number, _threads);

                act_trace("  [X] run_number=%d remaining=%d threads=%d\n",
                          run_number, remaining_run_number, threads);

                for (i = 0; i < threads; i++) {
                        if (nb_calls <= ((run_number + i) % freq)) 
                                continue;

                        d[i].run_number = run_number + i;
                        d[i].test_choice = test_choice;
                        d[i].nb_calls = nb_calls;
                        d[i].freq = freq;
                        rc = pthread_create(&d[i].thread_id, NULL,
                                            &sha3_thread, &d[i]);
                        if (rc != 0) {
                                free(d);
                                fprintf(stderr, "starting %d failed!\n", i);
                                return EXIT_FAILURE;
                        }
                }
                for (i = 0; i < threads; i++) {
                        act_trace("      run_number=%d checksum=%016llx\n",
                                  run_number + i, (long long)d[i].checksum);

                        if (nb_calls <= ((run_number + i) % freq))
                                continue;

                        rc = pthread_join(d[i].thread_id, NULL);
                        if (rc != 0) {
                                free(d);
                                fprintf(stderr, "joining threads failed!\n");
                                return EXIT_FAILURE;
                        }
                        checksum ^= d[i].checksum;
                }
                run_number += threads;
        }

        free(d);

        act_trace("checksum=%016llx\n", (unsigned long long)checksum);
        return checksum;

}
*/
/* == END ============ PART NOT PORTED AND DEBUGGED YET ============= */
#endif /* CONFIG_USE_NO_PTHREADS */

static int action_main(struct snap_sim_action *action, void *job,
		       unsigned int job_len)
{
	struct bloom_job *js = (struct bloom_job *)job;
        struct bloom bloom;

	act_trace("%s(%p, %p, %d) [%d]\n", __func__, action, job, job_len,
		  (int)js->mode);

	act_trace("test_choice=%d nb_calls=%d count=%d entries=%d\n", js->test_choice, 
                  js->nb_calls, js->count, js->entries);

        if(js->test_choice == BLOOM_SPEED) {
              if (js->entries < 1000)
                    return 0;
        }
        else {
              js->nb_calls = 0;
              js->count = 0;
        }

        js->collisions = bloom_main(js->test_choice, js->nb_calls, 
                    js->count, js->entries, js->error_rate, &bloom, js->threads);

  	js->entries = bloom.entries;
  	js->error_rate = -log10(bloom.error);
  	js->bits = bloom.bits;
  	js->bytes = bloom.bytes;
  	js->hashes = bloom.hashes;

	action->job.retc = SNAP_RETC_SUCCESS;
	return 0;
}

static struct snap_sim_action action = {
	.vendor_id = SNAP_VENDOR_ID_ANY,
	.device_id = SNAP_DEVICE_ID_ANY,
	.action_type = BLOOM_ACTION_TYPE,

	.job = { .retc = SNAP_RETC_FAILURE, },
	.state = ACTION_IDLE,
	.main = action_main,
	.priv_data = NULL,	/* this is passed back as void *card */
	.mmio_write32 = mmio_write32,
	.mmio_read32 = mmio_read32,

	.next = NULL,
};

static void _init(void) __attribute__((constructor));

static void _init(void)
{
	snap_action_register(&action);
}
