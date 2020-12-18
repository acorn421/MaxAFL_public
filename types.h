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
   american fuzzy lop - type definitions and minor macros
   ------------------------------------------------------

   Written and maintained by Michal Zalewski <lcamtuf@google.com>
*/

#ifndef _HAVE_TYPES_H
#define _HAVE_TYPES_H

#include <stdint.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

/*

   Ugh. There is an unintended compiler / glibc #include glitch caused by
   combining the u64 type an %llu in format strings, necessitating a workaround.

   In essence, the compiler is always looking for 'unsigned long long' for %llu.
   On 32-bit systems, the u64 type (aliased to uint64_t) is expanded to
   'unsigned long long' in <bits/types.h>, so everything checks out.

   But on 64-bit systems, it is #ifdef'ed in the same file as 'unsigned long'.
   Now, it only happens in circumstances where the type happens to have the
   expected bit width, *but* the compiler does not know that... and complains
   about 'unsigned long' being unsafe to pass to %llu.

 */

#ifdef __x86_64__
typedef unsigned long long u64;
#else
typedef uint64_t u64;
#endif /* ^__x86_64__ */

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#ifndef MIN
#define MIN(_a, _b) ((_a) > (_b) ? (_b) : (_a))
#define MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#endif /* !MIN */

#define SWAP16(_x) ({               \
  u16 _ret = (_x);                  \
  (u16)((_ret << 8) | (_ret >> 8)); \
})

#define SWAP32(_x) ({                 \
  u32 _ret = (_x);                    \
  (u32)((_ret << 24) | (_ret >> 24) | \
        ((_ret << 8) & 0x00FF0000) |  \
        ((_ret >> 8) & 0x0000FF00));  \
})

#ifdef AFL_LLVM_PASS
#define AFL_R(x) (random() % (x))
#else
#define R(x) (random() % (x))
#endif /* ^AFL_LLVM_PASS */

#define STRINGIFY_INTERNAL(x) #x
#define STRINGIFY(x) STRINGIFY_INTERNAL(x)

#define MEM_BARRIER()    \
  __asm__ volatile("" :: \
                       : "memory")

#define likely(_x) __builtin_expect(!!(_x), 1)
#define unlikely(_x) __builtin_expect(!!(_x), 0)

enum Predicate
{
  // Opcode              U L G E    Intuitive operation
  FCMP_FALSE = 0, ///< 0 0 0 0    Always false (always folded)
  FCMP_OEQ = 1,   ///< 0 0 0 1    True if ordered and equal
  FCMP_OGT = 2,   ///< 0 0 1 0    True if ordered and greater than
  FCMP_OGE = 3,   ///< 0 0 1 1    True if ordered and greater than or equal
  FCMP_OLT = 4,   ///< 0 1 0 0    True if ordered and less than
  FCMP_OLE = 5,   ///< 0 1 0 1    True if ordered and less than or equal
  FCMP_ONE = 6,   ///< 0 1 1 0    True if ordered and operands are unequal
  FCMP_ORD = 7,   ///< 0 1 1 1    True if ordered (no nans)
  FCMP_UNO = 8,   ///< 1 0 0 0    True if unordered: isnan(X) | isnan(Y)
  FCMP_UEQ = 9,   ///< 1 0 0 1    True if unordered or equal
  FCMP_UGT = 10,  ///< 1 0 1 0    True if unordered or greater than
  FCMP_UGE = 11,  ///< 1 0 1 1    True if unordered, greater than, or equal
  FCMP_ULT = 12,  ///< 1 1 0 0    True if unordered or less than
  FCMP_ULE = 13,  ///< 1 1 0 1    True if unordered, less than, or equal
  FCMP_UNE = 14,  ///< 1 1 1 0    True if unordered or not equal
  FCMP_TRUE = 15, ///< 1 1 1 1    Always true (always folded)
  FIRST_FCMP_PREDICATE = FCMP_FALSE,
  LAST_FCMP_PREDICATE = FCMP_TRUE,
  BAD_FCMP_PREDICATE = FCMP_TRUE + 1,
  ICMP_EQ = 32,  ///< equal
  ICMP_NE = 33,  ///< not equal
  ICMP_UGT = 34, ///< unsigned greater than
  ICMP_UGE = 35, ///< unsigned greater or equal
  ICMP_ULT = 36, ///< unsigned less than
  ICMP_ULE = 37, ///< unsigned less or equal
  ICMP_SGT = 38, ///< signed greater than
  ICMP_SGE = 39, ///< signed greater or equal
  ICMP_SLT = 40, ///< signed less than
  ICMP_SLE = 41, ///< signed less or equal
  FIRST_ICMP_PREDICATE = ICMP_EQ,
  LAST_ICMP_PREDICATE = ICMP_SLE,
  BAD_ICMP_PREDICATE = ICMP_SLE + 1
};

enum BinryOP
{
  BINOP_NOT = 25,
  BINOP_AND = 26,
  BINOP_OR = 27,
  BINOP_XOR = 28
};

// need to define struct
// 1. cmpid
// 2. left meic
// 3. right meic
// 4. real value
// 5. maximum range value

typedef struct maxafl_info
{
  u32 moduleId;
  u32 cmpType;
  u32 left;
  u32 right;
  s8 hit;
  double real;
  double range;
} maxafl_info_t;

typedef struct cmp_info
{
  u32 moduleId;
  u32 cmpId;
  u32 cmpType;
  double real;
} cmp_info_t;

typedef struct br_info
{
  u32 moduleId;
  u32 brId;
  u32 left;
  u32 right;
  u32 leftHit;
  u32 rightHit;
  u16 leftMul;
  u16 rightMul;
  s8 hit;
  double real;
  u32 cmpSize;
  u32 cmpVec;
} br_info_t;

// #define BR_PASS_FAIL 0
#define BR_CHANGED 1
#define BR_UNCHANGED 0
#define BR_WRONG -1

#define BR_FAIL 2
#define BR_HIT 1
#define BR_SUCC -2
#define BR_NOHIT -3.032
#define BR_UNCONTROL -4
#define BR_FINISH -5.032

#define BR_TRUE -0.00293
#define BR_FALSE +0.00239

#define IS_HIT(_x) (_x == BR_FAIL || _x == BR_SUCC)

#endif /* ! _HAVE_TYPES_H */
