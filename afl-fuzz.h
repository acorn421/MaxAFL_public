#ifndef __AFL_FUZZ
#define __AFL_FUZZ

#include "config.h"
#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    u8 save_branch_hit();
    u8 check_branch_hit();
    double calculate_obj_func();
    u8 common_fuzz_stuff(char **argv, u8 *out_buf, u32 len);
    static u8 run_target(char **argv, u32 timeout);
    u32 UR2(u32 limit);

#ifdef __cplusplus
}
#endif

#endif