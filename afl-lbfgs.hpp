#ifndef __AFL_LBFGS
#define __AFL_LBFGS

#include "config.h"
#include "types.h"

#ifdef __cplusplus
#include "cppoptlib/problem.h"
#include "cppoptlib/meta.h"
#include "cppoptlib/solver/lbfgsbsolver.h"
#include "cppoptlib/solver/lbfgssolver.h"
#include "cppoptlib/solver/gradientdescentsolver.h"
#include "eigen3/Eigen/StdVector"

extern "C"
{
#endif

    int init_lbfgs(char **argv, u8 *out_buf, s32 len, int stage, int accuracy, int mode, int prob);
    int free_lbfgs();
    double solve_lbfgs(u8 *in_buf, int len);
    int init_normal_sampling(u8 *mean, int len, double stddev);
    int free_normal_sampling(int len);
    void modify_dist(u8 *mean, int len);
    void sample_dist(u8 *out_buf, int len);

#ifdef __cplusplus
}
#endif

#endif