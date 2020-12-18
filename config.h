/*
  Copyright 2013 Google LLC All rights reserved.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at:

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

/*
   american fuzzy lop - vaguely configurable bits
   ----------------------------------------------

   Written and maintained by Michal Zalewski <lcamtuf@google.com>
*/

#ifndef _HAVE_CONFIG_H
#define _HAVE_CONFIG_H

#include "types.h"

/* Version string: */

#define VERSION "2.56b"

/******************************************************
 *                                                    *
 *  Settings that may be of interest to power users:  *
 *                                                    *
 ******************************************************/

/* Comment out to disable terminal colors (note that this makes afl-analyze
   a lot less nice): */

#define USE_COLOR

/* Comment out to disable fancy ANSI boxes and use poor man's 7-bit UI: */

#define FANCY_BOXES

/* Default timeout for fuzzed code (milliseconds). This is the upper bound,
   also used for detecting hangs; the actual value is auto-scaled: */

#define EXEC_TIMEOUT 1000

/* Timeout rounding factor when auto-scaling (milliseconds): */

#define EXEC_TM_ROUND 20

/* Default memory limit for child process (MB): */

#ifndef __x86_64__
#define MEM_LIMIT 25
#else
#define MEM_LIMIT 50
#endif /* ^!__x86_64__ */

/* Default memory limit when running in QEMU mode (MB): */

#define MEM_LIMIT_QEMU 200

/* Number of calibration cycles per every new test case (and for test
   cases that show variable behavior): */

#define CAL_CYCLES 8
#define CAL_CYCLES_LONG 40

/* Number of subsequent timeouts before abandoning an input file: */

#define TMOUT_LIMIT 250

/* Maximum number of unique hangs or crashes to record: */

#define KEEP_UNIQUE_HANG 500
#define KEEP_UNIQUE_CRASH 5000

/* Baseline number of random tweaks during a single 'havoc' stage: */

#define HAVOC_CYCLES 256
#define HAVOC_CYCLES_INIT 1024

/* Maximum multiplier for the above (should be a power of two, beware
   of 32-bit int overflows): */

#define HAVOC_MAX_MULT 16

/* Absolute minimum number of havoc cycles (after all adjustments): */

#define HAVOC_MIN 16

/* Maximum stacking for havoc-stage tweaks. The actual value is calculated
   like this: 

   n = random between 1 and HAVOC_STACK_POW2
   stacking = 2^n

   In other words, the default (n = 7) produces 2, 4, 8, 16, 32, 64, or
   128 stacked tweaks: */

#define HAVOC_STACK_POW2 7

/* Caps on block sizes for cloning and deletion operations. Each of these
   ranges has a 33% probability of getting picked, except for the first
   two cycles where smaller blocks are favored: */

#define HAVOC_BLK_SMALL 32
#define HAVOC_BLK_MEDIUM 128
#define HAVOC_BLK_LARGE 1500

/* Extra-large blocks, selected very rarely (<5% of the time): */

#define HAVOC_BLK_XL 32768

/* Probabilities of skipping non-favored entries in the queue, expressed as
   percentages: */

#define SKIP_TO_NEW_PROB 99   /* ...when there are new, pending favorites */
#define SKIP_NFAV_OLD_PROB 95 /* ...no new favs, cur entry already fuzzed */
#define SKIP_NFAV_NEW_PROB 75 /* ...no new favs, cur entry not fuzzed yet */

/* Splicing cycle count: */

#define SPLICE_CYCLES 15

/* Nominal per-splice havoc cycle length: */

#define SPLICE_HAVOC 32

/* Maximum offset for integer addition / subtraction stages: */

#define ARITH_MAX 35

/* Limits for the test case trimmer. The absolute minimum chunk size; and
   the starting and ending divisors for chopping up the input file: */

#define TRIM_MIN_BYTES 4
#define TRIM_START_STEPS 16
#define TRIM_END_STEPS 1024

/* Maximum size of input file, in bytes (keep under 100MB): */

#define MAX_FILE (1 * 1024 * 1024)

/* The same, for the test case minimizer: */

#define TMIN_MAX_FILE (10 * 1024 * 1024)

/* Block normalization steps for afl-tmin: */

#define TMIN_SET_MIN_SIZE 4
#define TMIN_SET_STEPS 128

/* Maximum dictionary token size (-x), in bytes: */

#define MAX_DICT_FILE 128

/* Length limits for auto-detected dictionary tokens: */

#define MIN_AUTO_EXTRA 3
#define MAX_AUTO_EXTRA 32

/* Maximum number of user-specified dictionary tokens to use in deterministic
   steps; past this point, the "extras/user" step will be still carried out,
   but with proportionally lower odds: */

#define MAX_DET_EXTRAS 200

/* Maximum number of auto-extracted dictionary tokens to actually use in fuzzing
   (first value), and to keep in memory as candidates. The latter should be much
   higher than the former. */

#define USE_AUTO_EXTRAS 50
#define MAX_AUTO_EXTRAS (USE_AUTO_EXTRAS * 10)

/* Scaling factor for the effector map used to skip some of the more
   expensive deterministic steps. The actual divisor is set to
   2^EFF_MAP_SCALE2 bytes: */

#define EFF_MAP_SCALE2 3

/* Minimum input file length at which the effector logic kicks in: */

#define EFF_MIN_LEN 128

/* Maximum effector density past which everything is just fuzzed
   unconditionally (%): */

#define EFF_MAX_PERC 90

/* UI refresh frequency (Hz): */

#define UI_TARGET_HZ 5

/* Fuzzer stats file and plot update intervals (sec): */

#define STATS_UPDATE_SEC 60
#define PLOT_UPDATE_SEC 5
#define PLOT_MY_UPDATE_SEC 60       // 1 minutes
#define PLOT_EXEC_UPDATE_CNT 100000 // 0.1M

/* Smoothing divisor for CPU load and exec speed stats (1 - no smoothing). */

#define AVG_SMOOTHING 16

/* Sync interval (every n havoc cycles): */

#define SYNC_INTERVAL 5

/* Output directory reuse grace period (minutes): */

#define OUTPUT_GRACE 25

/* Uncomment to use simple file names (id_NNNNNN): */

// #define SIMPLE_FILES

/* List of interesting values to use in fuzzing. */

#define INTERESTING_8                                     \
   -128,    /* Overflow signed 8-bit when decremented  */ \
       -1,  /*                                         */ \
       0,   /*                                         */ \
       1,   /*                                         */ \
       16,  /* One-off with common buffer size         */ \
       32,  /* One-off with common buffer size         */ \
       64,  /* One-off with common buffer size         */ \
       100, /* One-off with common buffer size         */ \
       127  /* Overflow signed 8-bit when incremented  */

#define INTERESTING_16                                     \
   -32768,   /* Overflow signed 16-bit when decremented */ \
       -129, /* Overflow signed 8-bit                   */ \
       128,  /* Overflow signed 8-bit                   */ \
       255,  /* Overflow unsig 8-bit when incremented   */ \
       256,  /* Overflow unsig 8-bit                    */ \
       512,  /* One-off with common buffer size         */ \
       1000, /* One-off with common buffer size         */ \
       1024, /* One-off with common buffer size         */ \
       4096, /* One-off with common buffer size         */ \
       32767 /* Overflow signed 16-bit when incremented */

#define INTERESTING_32                                           \
   -2147483648LL,  /* Overflow signed 32-bit when decremented */ \
       -100663046, /* Large negative number (endian-agnostic) */ \
       -32769,     /* Overflow signed 16-bit                  */ \
       32768,      /* Overflow signed 16-bit                  */ \
       65535,      /* Overflow unsig 16-bit when incremented  */ \
       65536,      /* Overflow unsig 16 bit                   */ \
       100663045,  /* Large positive number (endian-agnostic) */ \
       2147483647  /* Overflow signed 32-bit when incremented */

/***********************************************************
 *                                                         *
 *  Really exotic stuff you probably don't want to touch:  *
 *                                                         *
 ***********************************************************/

/* Call count interval between reseeding the libc PRNG from /dev/urandom: */

#define RESEED_RNG 10000

/* Maximum line length passed from GCC to 'as' and used for parsing
   configuration files: */

#define MAX_LINE 8192

/* Environment variable used to pass SHM ID to the called program. */

#define SHM_ENV_VAR "__AFL_SHM_ID"
// #define SHM_ENV_VAR_MAX "__MAXAFL_SHM_ID"
// #define SHM_ENV_VAR_PTR "__MAXAFL_SHM_PTR_ID"
#define SHM_ENV_VAR_BR_INFO "__MAXAFL_SHM_BR_INFO"
#define SHM_ENV_VAR_BR_PTR "__MAXAFL_SHM_BR_PTR"
#define SHM_ENV_VAR_BR_HIT "__MAXAFL_SHM_BR_HIT"
#define SHM_ENV_VAR_CMP_INFO "__MAXAFL_SHM_CMP_INFO"
#define SHM_ENV_VAR_CMP_PTR "__MAXAFL_SHM_CMP_PTR"
#define SHM_ENV_VAR_CMP_HIT "__MAXAFL_SHM_CMP_HIT"
#define SHM_ENV_VAR_CMPVEC "__MAXAFL_SHM_CMPVEC"
#define SHM_ENV_VAR_EXIT_PENALTY "__MAXAFL_SHM_EXIT_PENALTY"

/* Other less interesting, internal-only variables. */

#define CLANG_ENV_VAR "__AFL_CLANG_MODE"
#define AS_LOOP_ENV_VAR "__AFL_AS_LOOPCHECK"
#define PERSIST_ENV_VAR "__AFL_PERSISTENT"
#define DEFER_ENV_VAR "__AFL_DEFER_FORKSRV"

/* In-code signatures for deferred and persistent mode. */

#define PERSIST_SIG "##SIG_AFL_PERSISTENT##"
#define DEFER_SIG "##SIG_AFL_DEFER_FORKSRV##"

/* Distinctive bitmap signature used to indicate failed execution: */

#define EXEC_FAIL_SIG 0xfee1dead

/* Distinctive exit code used to indicate MSAN trip condition: */

#define MSAN_ERROR 86

/* Designated file descriptors for forkserver commands (the application will
   use FORKSRV_FD and FORKSRV_FD + 1): */

#define FORKSRV_FD 198

/* Fork server init timeout multiplier: we'll wait the user-selected
   timeout plus this much for the fork server to spin up. */

#define FORK_WAIT_MULT 10

/* Calibration timeout adjustments, to be a bit more generous when resuming
   fuzzing sessions or trying to calibrate already-added internal finds.
   The first value is a percentage, the other is in milliseconds: */

#define CAL_TMOUT_PERC 125
#define CAL_TMOUT_ADD 50

/* Number of chances to calibrate a case before giving up: */

#define CAL_CHANCES 3

/* Map size for the traced binary (2^MAP_SIZE_POW2). Must be greater than
   2; you probably want to keep it under 18 or so for performance reasons
   (adjusting AFL_INST_RATIO when compiling is probably a better way to solve
   problems with complex programs). You need to recompile the target binary
   after changing this - otherwise, SEGVs may ensue. */

#define MAP_SIZE_POW2 16
#define MAP_SIZE (1 << MAP_SIZE_POW2)

/* Maximum allocator request size (keep well under INT_MAX): */

#define MAX_ALLOC 0x40000000

/* A made-up hashing seed: */

#define HASH_CONST 0xa5b35705

/* Constants for afl-gotcpu to control busy loop timing: */

#define CTEST_TARGET_MS 5000
#define CTEST_CORE_TRG_MS 1000
#define CTEST_BUSY_CYCLES (10 * 1000 * 1000)

/* Uncomment this to use inferior block-coverage-based instrumentation. Note
   that you need to recompile the target binary for this to have any effect: */

// #define COVERAGE_ONLY

/* Uncomment this to ignore hit counts and output just one bit per tuple.
   As with the previous setting, you will need to recompile the target
   binary: */

// #define SKIP_COUNTS

/* Uncomment this to use instrumentation data to record newly discovered paths,
   but do not use them as seeds for fuzzing. This is useful for conveniently
   measuring coverage that could be attained by a "dumb" fuzzing algorithm: */

// #define IGNORE_FINDS

// MAXAFL_CONFIG
#define MAXAFL_VERSION "1.01"

#define MAXAFL_MX_MODULE 300
#define PTR_SIZE MAXAFL_MX_MODULE * sizeof(u32)
// #define PTR_SIZE MAXAFL_MX_MODULE * sizeof(maxafl_info_t *)

#define MAXAFL_MX_HIT 100000
#define HIT_SIZE MAXAFL_MX_HIT * sizeof(u32)

// #define MAXAFL_MX_CMP 10000
// // #define MAXAFL_INFO_SIZE 28
// #define MAXAFL_INFO_SIZE sizeof(maxafl_info_t)
// #define INFO_SIZE MAXAFL_MX_CMP *MAXAFL_INFO_SIZE
// typedef u32 maxafl_mxexp_instr_t;

#define MAXAFL_MX_BR 100000
#define MAXAFL_BR_INFO_SIZE MAXAFL_MX_BR * sizeof(br_info_t)

#define MAXAFL_MX_CMP 100000
#define MAXAFL_CMP_INFO_SIZE MAXAFL_MX_CMP * sizeof(cmp_info_t)

#define MAXAFL_MX_CMPVEC MAXAFL_MX_BR * 3 * 5
#define MAXAFL_CMPVEC_SIZE MAXAFL_MX_CMPVEC * sizeof(s16)

// Height in cost function
#define MAXAFL_COST_H 20

#define MAX_MUL 20

// initial learning rate for L-BFGS
#define LBFGS_ALPHA_INIT 10.0

// accuracy to calculate gradient(0~3)
#define LBFGS_ACC 0

// count of L-BFGS mutation in one stage
#define LBFGS_MAX_LOOP 1
#define LBFGS_MAX_STAGE 1

#define LBFGS_MODE_FLIP 0
#define LBFGS_MODE_ARITH 1
#define LBFGS_MODE_ORIGIN 2
#define LBFGS_MODE_NORMAL 3
#define LBFGS_MODE_BOUND 4

#define LBFGS_MODE_CURRENT LBFGS_MODE_ORIGIN
#define LBFGS_DEFAULT_GRAD 10
#define LBFGS_DOUBLE_GRAD 20
#define LBFGS_GRAD_MAX 200
#define LBFGS_ITERATION_MAX 50
#define LBFGS_GRAD_NORM_MIN 1e-3

#define LBFGS_INITIAL_RATE 0.1
#define LBFGS_GAMMA 0.7

#define OBJ_MODE_ORIGIN 1
#define OBJ_MODE_ADP1 2
#define OBJ_MODE_ADP2 3
#define OBJ_MODE_DEFAULT OBJ_MODE_ADP2

#define COST_MODE_ORIGIN 1
#define COST_MODE_ROOT 2
#define COST_MODE_LOG 3
#define COST_MODE_SIGMOID 4
#define COST_MODE_DEFAULT COST_MODE_SIGMOID

#define PROB_MODE_NONE 1
#define PROB_MODE_MCMC 2
#define PROB_MODE_EPSILON 3
#define PROB_MODE_MCMC_STAGE 4
#define PROB_MODE_DEFAULT PROB_MODE_EPSILON

#define EPSILON_NORMAL 10
#define EPSILON_BITFLIP 5
#define BITFLIP_CNT 20

#define NORMAL_STDDEV 10.0

// #define MAXAFL_DEBUG
// #define _MAXAFL_DEBUG
#define MAXAFL_RELEASE

#define TOPK 100

// count of Normal Distribution sampling in one stage
// #define NORMAL_DIST_LOOP 1

#endif /* ! _HAVE_CONFIG_H */
