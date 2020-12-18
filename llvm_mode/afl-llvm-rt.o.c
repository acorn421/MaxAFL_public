/*
  Copyright 2015 Google LLC All rights reserved.

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
   american fuzzy lop - LLVM instrumentation bootstrap
   ---------------------------------------------------

   Written by Laszlo Szekeres <lszekeres@google.com> and
              Michal Zalewski <lcamtuf@google.com>

   LLVM integration design comes from Laszlo Szekeres.

   This code is the rewrite of afl-as.h's main_payload.
*/

#include "../config.h"
#include "../types.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>

/* This is a somewhat ugly hack for the experimental 'trace-pc-guard' mode.
   Basically, we need to make sure that the forkserver is initialized after
   the LLVM-generated runtime initialization pass, not before. */

#ifdef USE_TRACE_PC
#define CONST_PRIO 5
#else
#define CONST_PRIO 0
#endif /* ^USE_TRACE_PC */

/* Globals needed by the injected instrumentation. The __afl_area_initial region
   is used for instrumentation output before __afl_map_shm() has a chance to run.
   It will end up as .comm, so it shouldn't be too wasteful. */

u8 __afl_area_initial[MAP_SIZE];
u8 *__afl_area_ptr = __afl_area_initial;

// maxafl_info_t __maxafl_area_initial[MAXAFL_MX_CMP];
// maxafl_info_t *__maxafl_area_ptr = __maxafl_area_initial;

// u32 __maxafl_ptr_initial[MAXAFL_MX_MODULE];
// u32 *__maxafl_ptr_ptr = __maxafl_ptr_initial;

br_info_t __maxafl_br_info[MAXAFL_MX_BR];
br_info_t *__maxafl_br_info_ptr = __maxafl_br_info;

// u8 __maxafl_br_info[MAXAFL_BR_INFO_SIZE];
// u8 *__maxafl_br_info_ptr = __maxafl_br_info;
u32 __maxafl_br_ptr[MAXAFL_MX_MODULE];
u32 *__maxafl_br_ptr_ptr = __maxafl_br_ptr;
u32 __maxafl_br_hit[MAXAFL_MX_HIT];
u32 *__maxafl_br_hit_ptr = __maxafl_br_hit;
cmp_info_t __maxafl_cmp_info[MAXAFL_MX_CMP];
cmp_info_t *__maxafl_cmp_info_ptr = __maxafl_cmp_info;
u32 __maxafl_cmp_ptr[MAXAFL_MX_MODULE];
u32 *__maxafl_cmp_ptr_ptr = __maxafl_cmp_ptr;
u32 __maxafl_cmp_hit[MAXAFL_MX_HIT];
u32 *__maxafl_cmp_hit_ptr = __maxafl_cmp_hit;
s16 __maxafl_cmpvec[MAXAFL_MX_CMPVEC];
s16 *__maxafl_cmpvec_ptr = __maxafl_cmpvec;
u32 __maxafl_exit_penalty;
u32 *__maxafl_exit_penalty_ptr = &__maxafl_exit_penalty;
u8 obj_mode_cur = 3;

FILE *output_fd;

__thread u32 __afl_prev_loc;

/* Running in persistent mode? */

static u8 is_persistent;

/* SHM setup. */

static void __afl_map_shm(void)
{
  u8 *id_str = getenv(SHM_ENV_VAR);
  u8 *id_str_br_info = getenv(SHM_ENV_VAR_BR_INFO);
  // u8 *id_str_br_info = 0;
  u8 *id_str_br_ptr = getenv(SHM_ENV_VAR_BR_PTR);
  u8 *id_str_br_hit = getenv(SHM_ENV_VAR_BR_HIT);
  u8 *id_str_cmp_info = getenv(SHM_ENV_VAR_CMP_INFO);
  u8 *id_str_cmp_ptr = getenv(SHM_ENV_VAR_CMP_PTR);
  u8 *id_str_cmp_hit = getenv(SHM_ENV_VAR_CMP_HIT);
  u8 *id_str_cmpvec = getenv(SHM_ENV_VAR_CMPVEC);
  u8 *id_str_exit_penalty = getenv(SHM_ENV_VAR_EXIT_PENALTY);

  /* If we're running under AFL, attach to the appropriate region, replacing the
     early-stage __afl_area_initial region that is needed to allow some really
     hacky .init code to work correctly in projects such as OpenSSL. */

  if (id_str)
  {
    u32 shm_id = atoi(id_str);

    __afl_area_ptr = shmat(shm_id, NULL, 0);

    if (__afl_area_ptr == (void *)-1)
    {
      // fprintf(output_fd, "[ERR] Failed to call shmat of id_str\n");
      _exit(1);
    }

    __afl_area_ptr[0] = 1;
  }
  // else
  // {
  //   fprintf(output_fd, "[ERR] Cannot get id_str from envvar\n");
  // }
  if (id_str_br_info)
  {
    u32 shm_id = atoi(id_str_br_info);

    __maxafl_br_info_ptr = shmat(shm_id, NULL, 0);

    // fprintf(output_fd, "Ret value of shmat at id_str_br_info = %p\n", shmat(shm_id, NULL, 0));

    // if (shmat(shm_id, NULL, 0) <= -1)
    // {
    //   fprintf(output_fd, "[ERR] ERRNO = %d", errno);
    // }

    if (__maxafl_br_info_ptr == (void *)-1)
    {
      // fprintf(output_fd, "[ERR] Failed to call shmat of id_str_br_info\n");
      _exit(1);
    }
    // fprintf(output_fd, "[SUCC] Successed to call shmat of id_str_br_info, addr = %p\n", __maxafl_br_info_ptr);
  }
  // else
  // {
  //   fprintf(output_fd, "[ERR] Cannot get id_str_br_info from envvar\n");
  // }
  if (id_str_br_ptr)
  {
    u32 shm_id = atoi(id_str_br_ptr);

    __maxafl_br_ptr_ptr = shmat(shm_id, NULL, 0);

    // fprintf(output_fd, "Ret value of shmat at id_str_br_ptr = %p\n", shmat(shm_id, NULL, 0));

    if (__maxafl_br_ptr_ptr == (void *)-1)
    {
      // fprintf(output_fd, "[ERR] Failed to call shmat of id_str_br_ptr\n");
      _exit(1);
    }
    // fprintf(output_fd, "[SUCC] Successed to call shmat of id_str_br_ptr, addr = %p\n", __maxafl_br_ptr_ptr);
  }
  // else
  // {
  //   fprintf(output_fd, "[ERR] Cannot get id_str_br_ptr from envvar\n");
  // }
  if (id_str_br_hit)
  {
    u32 shm_id = atoi(id_str_br_hit);

    __maxafl_br_hit_ptr = shmat(shm_id, NULL, 0);

    // fprintf(output_fd, "Ret value of shmat at id_str_br_ptr = %p\n", shmat(shm_id, NULL, 0));

    if (__maxafl_br_hit_ptr == (void *)-1)
    {
      // fprintf(output_fd, "[ERR] Failed to call shmat of id_str_br_ptr\n");
      _exit(1);
    }
    // fprintf(output_fd, "[SUCC] Successed to call shmat of id_str_br_ptr, addr = %p\n", __maxafl_br_ptr_ptr);
  }
  // else
  // {
  //   fprintf(output_fd, "[ERR] Cannot get id_str_br_ptr from envvar\n");
  // }
  if (id_str_cmp_info)
  {
    u32 shm_id = atoi(id_str_cmp_info);

    __maxafl_cmp_info_ptr = shmat(shm_id, NULL, 0);

    if (__maxafl_cmp_info_ptr == (void *)-1)
    {
      // fprintf(output_fd, "[ERR] Failed to call shmat of id_str_cmp_info\n");
      _exit(1);
    }
    // fprintf(output_fd, "[SUCC] Successed to call shmat of id_str_cmp_info, addr = %p\n", __maxafl_cmp_info_ptr);
  }
  // else
  // {
  //   fprintf(output_fd, "[ERR] Cannot get id_str_cmp_info from envvar\n");
  // }
  if (id_str_cmp_ptr)
  {
    u32 shm_id = atoi(id_str_cmp_ptr);

    __maxafl_cmp_ptr_ptr = shmat(shm_id, NULL, 0);

    if (__maxafl_cmp_ptr_ptr == (void *)-1)
    {
      // fprintf(output_fd, "[ERR] Failed to call shmat of id_str_cmp_ptr\n");
      _exit(1);
    }
    // fprintf(output_fd, "[SUCC] Successed to call shmat of id_str_cmp_ptr, addr = %p\n", __maxafl_cmp_ptr_ptr);
  }
  // else
  // {
  //   fprintf(output_fd, "[ERR] Cannot get id_str_cmp_ptr from envvar\n");
  // }
  if (id_str_cmp_hit)
  {
    u32 shm_id = atoi(id_str_cmp_hit);

    __maxafl_cmp_hit_ptr = shmat(shm_id, NULL, 0);

    if (__maxafl_cmp_hit_ptr == (void *)-1)
    {
      // fprintf(output_fd, "[ERR] Failed to call shmat of id_str_cmp_ptr\n");
      _exit(1);
    }
    // fprintf(output_fd, "[SUCC] Successed to call shmat of id_str_cmp_ptr, addr = %p\n", __maxafl_cmp_ptr_ptr);
  }
  // else
  // {
  //   fprintf(output_fd, "[ERR] Cannot get id_str_cmp_ptr from envvar\n");
  // }
  if (id_str_cmpvec)
  {
    u32 shm_id = atoi(id_str_cmpvec);

    __maxafl_cmpvec_ptr = shmat(shm_id, NULL, 0);

    if (__maxafl_cmpvec_ptr == (void *)-1)
    {
      // fprintf(output_fd, "[ERR] Failed to call shmat of id_str_cmpvec\n");
      _exit(1);
    }
    // fprintf(output_fd, "[SUCC] Successed to call shmat of id_str_cmpvec, addr = %p\n", __maxafl_cmpvec_ptr);
  }
  // else
  // {
  //   fprintf(output_fd, "[ERR] Cannot get id_str_cmpvec from envvar\n");
  // }
  if (id_str_exit_penalty)
  {
    u32 shm_id = atoi(id_str_exit_penalty);

    __maxafl_exit_penalty_ptr = shmat(shm_id, NULL, 0);

    if (__maxafl_exit_penalty_ptr == (void *)-1)
    {
      // fprintf(output_fd, "[ERR] Failed to call shmat of id_str_exit_penalty\n");
      _exit(1);
    }
    // fprintf(output_fd, "[SUCC] Successed to call shmat of id_str_exit_penalty, addr = %p\n", __maxafl_exit_penalty_ptr);
  }
  // else
  // {
  //   fprintf(output_fd, "[ERR] Cannot get id_str_exit_penalty from envvar\n");
  // }
}

/* Fork server logic. */

static void __afl_start_forkserver(void)
{
  static u8 tmp[4];
  s32 child_pid;

  u8 child_stopped = 0;

  /* Phone home and tell the parent that we're OK. If parent isn't there,
     assume we're not running in forkserver mode and just execute program. */

  if (write(FORKSRV_FD + 1, tmp, 4) != 4)
    return;

  while (1)
  {

    u32 was_killed;
    int status;

    /* Wait for parent by reading from the pipe. Abort if read fails. */

    if (read(FORKSRV_FD, &was_killed, 4) != 4)
      _exit(1);

    /* If we stopped the child in persistent mode, but there was a race
       condition and afl-fuzz already issued SIGKILL, write off the old
       process. */

    if (child_stopped && was_killed)
    {
      child_stopped = 0;
      if (waitpid(child_pid, &status, 0) < 0)
        _exit(1);
    }

    if (!child_stopped)
    {

      /* Once woken up, create a clone of our process. */

      child_pid = fork();
      if (child_pid < 0)
        _exit(1);

      /* In child process: close fds, resume execution. */

      if (!child_pid)
      {

        close(FORKSRV_FD);
        close(FORKSRV_FD + 1);
        return;
      }
    }
    else
    {

      /* Special handling for persistent mode: if the child is alive but
         currently stopped, simply restart it with SIGCONT. */

      kill(child_pid, SIGCONT);
      child_stopped = 0;
    }

    /* In parent process: write PID to pipe, then wait for child. */

    if (write(FORKSRV_FD + 1, &child_pid, 4) != 4)
      _exit(1);

    if (waitpid(child_pid, &status, is_persistent ? WUNTRACED : 0) < 0)
      _exit(1);

    /* In persistent mode, the child stops itself with SIGSTOP to indicate
       a successful run. In this case, we want to wake it up without forking
       again. */

    if (WIFSTOPPED(status))
      child_stopped = 1;

    /* Relay wait status to pipe, then loop back. */

    if (write(FORKSRV_FD + 1, &status, 4) != 4)
      _exit(1);
  }
}

/* A simplified persistent mode handler, used as explained in README.llvm. */

int __afl_persistent_loop(unsigned int max_cnt)
{
  static u8 first_pass = 1;
  static u32 cycle_cnt;

  if (first_pass)
  {

    /* Make sure that every iteration of __AFL_LOOP() starts with a clean slate.
       On subsequent calls, the parent will take care of that, but on the first
       iteration, it's our job to erase any trace of whatever happened
       before the loop. */

    if (is_persistent)
    {

      memset(__afl_area_ptr, 0, MAP_SIZE);
      __afl_area_ptr[0] = 1;
      __afl_prev_loc = 0;
    }

    cycle_cnt = max_cnt;
    first_pass = 0;
    return 1;
  }

  if (is_persistent)
  {

    if (--cycle_cnt)
    {

      raise(SIGSTOP);

      __afl_area_ptr[0] = 1;
      __afl_prev_loc = 0;

      return 1;
    }
    else
    {

      /* When exiting __AFL_LOOP(), make sure that the subsequent code that
         follows the loop is not traced. We do that by pivoting back to the
         dummy output region. */

      __afl_area_ptr = __afl_area_initial;
    }
  }

  return 0;
}

/* This one can be called from user code when deferred forkserver mode
    is enabled. */

void __afl_manual_init(void)
{
  static u8 init_done;

  if (!init_done)
  {
#ifdef _MAXAFL_DEBUG
    output_fd = fopen("output_fd.out", "w");
#endif
    __afl_map_shm();
    __afl_start_forkserver();
    init_done = 1;
    obj_mode_cur = *__maxafl_exit_penalty_ptr;
    *__maxafl_exit_penalty_ptr = 0;
  }
}

/* Proper initialization routine. */

__attribute__((constructor(CONST_PRIO))) void __afl_auto_init(void)
{
  is_persistent = !!getenv(PERSIST_ENV_VAR);

  if (getenv(DEFER_ENV_VAR))
    return;

  __afl_manual_init();
}

/* The following stuff deals with supporting -fsanitize-coverage=trace-pc-guard.
   It remains non-operational in the traditional, plugin-backed LLVM mode.
   For more info about 'trace-pc-guard', see README.llvm.

   The first function (__sanitizer_cov_trace_pc_guard) is called back on every
   edge (as opposed to every basic block). */

void __sanitizer_cov_trace_pc_guard(uint32_t *guard)
{
  __afl_area_ptr[*guard]++;
}

/* Init callback. Populates instrumentation IDs. Note that we're using
   ID of 0 as a special value to indicate non-instrumented bits. That may
   still touch the bitmap, but in a fairly harmless way. */

void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop)
{
  u32 inst_ratio = 100;
  u8 *x;

  if (start == stop || *start)
    return;

  x = getenv("AFL_INST_RATIO");
  if (x)
    inst_ratio = atoi(x);

  if (!inst_ratio || inst_ratio > 100)
  {
    fprintf(stderr, "[-] ERROR: Invalid AFL_INST_RATIO (must be 1-100).\n");
    abort();
  }

  /* Make sure that the first element in the range is always set - we use that
     to avoid duplicate calls (which can happen as an artifact of the underlying
     implementation in LLVM). */

  *(start++) = R(MAP_SIZE - 1) + 1;

  while (start < stop)
  {

    if (R(100) < inst_ratio)
      *start = R(MAP_SIZE - 1) + 1;
    else
      *start = 0;

    start++;
  }
}

void __maxafl_visit_etc(u32 moduleId, u32 id, s32 real)
{
  if (__maxafl_cmp_info_ptr == __maxafl_cmp_info)
  {
    return;
  }

  // fprintf(output_fd, "[ETC]\tmoduleId : %d, id : %d, real : %d\n", moduleId, id, real);

  // maxafl_info_t *cmp_info = &__maxafl_area_ptr[id];
  cmp_info_t *cmp_info = __maxafl_cmp_info_ptr + __maxafl_cmp_ptr_ptr[moduleId] + id;

  cmp_info->real = real;

  return;
}

void (*__maxafl_visit_etc_func)(u32, u32, s32) = &__maxafl_visit_etc;

void __maxafl_enter_internal_call(u32 penalty)
{
  *__maxafl_exit_penalty_ptr += penalty;
  return;
}

void (*__maxafl_enter_internal_call_func)(u32) = &__maxafl_enter_internal_call;

void __maxafl_exit_internal_call(u32 penalty)
{
  *__maxafl_exit_penalty_ptr -= penalty;
  return;
}

void (*__maxafl_exit_internal_call_func)(u32) = &__maxafl_exit_internal_call;

void __maxafl_visit_br(u32 moduleId, u32 id)
{
  if (__maxafl_br_info_ptr == __maxafl_br_info)
  {
    return;
  }

  u32 br_id = __maxafl_br_ptr_ptr[moduleId] + id;
  br_info_t *br_info = __maxafl_br_info_ptr + br_id;
  cmp_info_t *cmp_info = __maxafl_cmp_info_ptr + __maxafl_cmp_ptr_ptr[moduleId];
  // u32 *br_hit_cnt = __maxafl_br_hit_ptr;
  u32 *br_hit = __maxafl_br_hit_ptr;
  s32 i, top = 0, left = 0, right = 0;
  u16 leftMul, rightMul, leftMax, rightMax;
  double stack[20];
  s8 sign;
  u8 binop_or, binop_and;

  // fprintf(output_fd, "moduleId : %d, brId : %d, br_info : %p, calc : %d, sizeof(br_info_t) : %d, calc2 : %d\n", moduleId, id, br_info, __maxafl_br_ptr_ptr[moduleId] + id, sizeof(br_info_t), ((u64)br_info - (u64)__maxafl_br_info_ptr) / sizeof(br_info_t));
  // fprintf(output_fd, "moduleId : %d, brId : %d\n", moduleId, id);
  // fprintf(output_fd, "moduleId : %d, brId : %d, left : %d, right : %d, hit : %d, real : %f, cmpSize : %d, cmpVec : %p\n", br_info->moduleId, br_info->brId, br_info->left, br_info->right, br_info->hit, br_info->real, br_info->cmpSize, br_info->cmpVec);

  if (br_info->real == BR_SUCC || br_info->real == BR_FINISH)
  {
    return;
  }

  if (br_info->real == BR_NOHIT)
  {
    // ! Something is wrong in this code. If i remove fprintf line, SIGSEGV is occured. But when i print it, it's fine.
    br_hit[br_hit[0]++] = br_id;
  }

  switch (obj_mode_cur)
  {
  case OBJ_MODE_ORIGIN:
    left = br_info->left;
    right = br_info->right;
    break;
  case OBJ_MODE_ADP1:
    leftMax = MAX_MUL, rightMax = MAX_MUL;
    leftMul = br_info->leftMul, rightMul = br_info->rightMul;
    left = br_info->left * (1 + rightMul * 0.1);
    right = br_info->right * (1 + leftMul * 0.1);
    break;
  case OBJ_MODE_ADP2:
    leftMax = br_info->left, rightMax = br_info->right;
    leftMul = br_info->leftMul, rightMul = br_info->rightMul;
    left = br_info->left - (leftMul - rightMul);
    right = br_info->right - (rightMul - leftMul);
    break;
  }

  sign = (left >= right) ? 1 : 0;

  for (i = br_info->cmpSize - 1; i >= 0; i--)
  {
    int cmpId = __maxafl_cmpvec_ptr[br_info->cmpVec + i * 3];
    int cmpType = __maxafl_cmpvec_ptr[br_info->cmpVec + i * 3 + 1];
    int cmpNo = __maxafl_cmpvec_ptr[br_info->cmpVec + i * 3 + 2] ^ sign;

    binop_or = cmpNo ? BINOP_OR : BINOP_AND, binop_and = cmpNo ? BINOP_AND : BINOP_OR;

    // fprintf(output_fd, "\t%d\t%d", cmpId, cmpType);

    // Binary Operators(and, or, xor)
    if (cmpId == -1)
    {
      double real1, real2, res;
      real1 = stack[--top];
      real2 = stack[--top];
      if (cmpType == binop_or)
      {
        if (real1 == BR_TRUE || real2 == BR_TRUE)
          res = BR_TRUE;
        else if (real1 == BR_FALSE)
          res = real2;
        else if (real2 == BR_FALSE)
          res = real1;
        else
          res = real1 < real2 ? real1 : real2;
        stack[top++] = res;
      }
      else if (cmpType == binop_and)
      {
        if (real1 == BR_FALSE || real2 == BR_FALSE)
          res = BR_FALSE;
        else if (real1 == BR_TRUE)
          res = real2;
        else if (real2 == BR_TRUE)
          res = real1;
        else
          res = real1 > real2 ? real1 : real2;
        stack[top++] = res;
      }
      // else
      // {
      //   fprintf(output_fd, "[FATAL 1]\n");
      // }
    }
    //  ETC instructions(load ...)
    else if (cmpType == -1)
    {
      // double real = -cmp_info[cmpId].real + 0.5;
      s8 real = cmp_info[cmpId].real == 1 ? 1 : 0;

      if (!(real ^ cmpNo))
      {
        stack[top++] = BR_FALSE;
      }
      else
      {
        stack[top++] = BR_TRUE;
      }
    }
    //  Cmp instructions
    else
    {

      double real = cmp_info[cmpId].real;

      if (cmpNo)
      {
        real *= -1;
      }

      stack[top++] = real;
    }
  }

  // if (top != 1)
  // {
  //   fprintf(output_fd, "[ERR] Stack cannot be -1\n");
  //   // assert(0);
  // }

  br_info->real = stack[0];

  //  left
  if (br_info->real == BR_FALSE || br_info->real == BR_TRUE || br_info->left == 0 || br_info->right == 0)
  {
    return;
  }
  if (signbit(br_info->real) && br_info->leftMul < leftMax)
  {
    br_info->leftHit++;
    if (br_info->leftHit >= br_info->left * br_info->left)
    {
      br_info->leftHit = 0;
      br_info->leftMul++;
#ifdef _MAXAFL_DEBUG
      fprintf(output_fd, "[DBG]\tleftMul of brId : %d has been increased to %d!\n", br_info->brId, br_info->leftMul);
#endif
    }
  }
  //  right
  // else if ((sign == -1 && br_info->real == BR_SUCC && br_info->rightMul < MAX_MUL) || (sign == 1 && br_info->real != BR_SUCC && br_info->rightMul < MAX_MUL))
  else if (!signbit(br_info->real) && br_info->rightMul < rightMax)
  {
    br_info->rightHit++;
    if (br_info->rightHit >= br_info->right * br_info->right)
    {
      br_info->rightHit = 0;
      br_info->rightMul++;
#ifdef _MAXAFL_DEBUG
      fprintf(output_fd, "[DBG]\trightMul of brId : %d has been increased to %d!\n", br_info->brId, br_info->rightMul);
#endif
    }
  }

  return;
}

void (*__maxafl_visit_br_func)(u32, u32) = &__maxafl_visit_br;

void __maxafl_visit_cmp_float(u32 moduleId, u32 id, u32 cmpType, u32 argType, u32 size, double arg1, double arg2)
{
  if (__maxafl_cmp_info_ptr == __maxafl_cmp_info)
  {
    return;
  }

  // maxafl_info_t *cmp_info = &__maxafl_area_ptr[id];
  u32 cmp_id = __maxafl_cmp_ptr_ptr[moduleId] + id;
  cmp_info_t *cmp_info = __maxafl_cmp_info_ptr + cmp_id;
  // u32 *cmp_hit_cnt = __maxafl_cmp_hit_ptr;
  u32 *cmp_hit = __maxafl_cmp_hit_ptr;
  double not = 1.0;

  // fprintf(output_fd, "ptr of cmp_hit : %p, cmp_hit[0] = %d\n", cmp_hit, cmp_hit[0]);

  if (cmp_info->real == BR_SUCC)
  {
    return;
  }

  if (cmp_info->real == BR_NOHIT)
  {
    cmp_hit[cmp_hit[0]++] = cmp_id;
  }

  switch (cmpType)
  {
  case FCMP_ONE:
  case FCMP_UNE:
    not = 1.0;
  case FCMP_OEQ:
  case FCMP_UEQ:
    cmp_info->real = arg1 > arg2 ? arg1 - arg2 : arg2 - arg1;
    if (cmp_info->real == +0.0)
      cmp_info->real = -0.0;
    break;

  case FCMP_OLE:
  case FCMP_ULE:
    not = -1.0;
  case FCMP_OGT:
  case FCMP_UGT:
    cmp_info->real = arg2 - arg1;
    break;

  case FCMP_OLT:
  case FCMP_ULT:
    not = -1.0;
  case FCMP_OGE:
  case FCMP_UGE:
    cmp_info->real = arg2 - arg1;
    if (cmp_info->real == +0.0)
      cmp_info->real = -0.0;
    break;

  default:
    break;
  }

  return;
}

void (*__maxafl_visit_float_func)(u32, u32, u32, u32, u32, double, double) = &__maxafl_visit_cmp_float;

void __maxafl_visit_cmp_integer(u32 moduleId, u32 id, u32 cmpType, u32 argType, u32 size, s64 sarg1, s64 sarg2, u64 zarg1, u64 zarg2)
{
  double sreal, zreal;

  if (__maxafl_cmp_info_ptr == __maxafl_cmp_info)
  {
    return;
  }

  // maxafl_info_t *cmp_info = &__maxafl_area_ptr[id];
  u32 cmp_id = __maxafl_cmp_ptr_ptr[moduleId] + id;
  cmp_info_t *cmp_info = __maxafl_cmp_info_ptr + cmp_id;
  // u32 *cmp_hit_cnt = __maxafl_cmp_hit_ptr;
  u32 *cmp_hit = __maxafl_cmp_hit_ptr;
  double not = 1.0;

  // fprintf(output_fd, "ptr of cmp_hit : %p, cmp_hit[0] = %d\n", cmp_hit, cmp_hit[0]);

  if (cmp_info->real == BR_SUCC)
  {
    return;
  }

  if (cmp_info->real == BR_NOHIT)
  {
    cmp_hit[cmp_hit[0]++] = cmp_id;
  }

  if (size == 32)
  {
    sarg1 = (s64)(s32)sarg1;
    sarg2 = (s64)(s32)sarg2;
    zarg1 = (u64)(u32)zarg1;
    zarg2 = (u64)(u32)zarg2;
  }
  else if (size == 16)
  {
    sarg1 = (s64)(s16)sarg1;
    sarg2 = (s64)(s16)sarg2;
    zarg1 = (u64)(u16)zarg1;
    zarg2 = (u64)(u16)zarg2;
  }
  else if (size == 8)
  {
    sarg1 = (s64)(s8)sarg1;
    sarg2 = (s64)(s8)sarg2;
    zarg1 = (u64)(u8)zarg1;
    zarg2 = (u64)(u8)zarg2;
  }
  // else if (size == 1)
  // {
  //   sarg1 = (s64)()sarg1;
  //   sarg2 = (s64)(s32)sarg2;
  //   zarg1 = (u64)(u32)uarg1;
  //   zarg2 = (u64)(u32)uarg2;
  // }

  switch (cmpType)
  {
  case ICMP_NE:
    not = -1.0;
  case ICMP_EQ:
    sreal = (double)sarg1 > (double)sarg2 ? (double)sarg1 - (double)sarg2 : (double)sarg2 - (double)sarg1;
    zreal = (double)zarg1 > (double)zarg2 ? (double)zarg1 - (double)zarg2 : (double)zarg2 - (double)zarg1;
    cmp_info->real = (sreal > 0 ? sreal : -sreal) < (zreal > 0 ? zreal : -zreal) ? sreal : zreal;
    if (cmp_info->real == +0.0)
      cmp_info->real = -0.0;
    break;

  case ICMP_ULE:
    not = -1.0;
  case ICMP_UGT:
    cmp_info->real = ((double)zarg2 - (double)zarg1);
    break;

  case ICMP_SLE:
    not = -1.0;
  case ICMP_SGT:
    cmp_info->real = ((double)sarg2 - (double)sarg1);
    break;

  case ICMP_ULT:
    not = -1.0;
  case ICMP_UGE:
    cmp_info->real = ((double)zarg2 - (double)zarg1);
    if (cmp_info->real == +0.0)
      cmp_info->real = -0.0;
    break;

  case ICMP_SLT:
    not = -1.0;
  case ICMP_SGE:
    cmp_info->real = ((double)sarg2 - (double)sarg1);
    if (cmp_info->real == +0.0)
      cmp_info->real = -0.0;
    break;

  default:
    break;
  }

  cmp_info->real *= not;

  return;
}

void (*__maxafl_visit_integer_func)(u32, u32, u32, u32, u32, s64, s64, u64, u64) = &__maxafl_visit_cmp_integer;