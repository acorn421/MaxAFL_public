#include "afl-fuzz.h"
#include "afl-lbfgs.hpp"
#include <iostream>
#include <math.h>
#include <random>
#include <vector>

using namespace cppoptlib;
using namespace Eigen;
using namespace std;

// #define MAXAFL_DEBUG

inline double maxd(double a, double b)
{
    return a > b ? a : b;
}
inline double mind(double a, double b)
{
    return a < b ? a : b;
}

class FuzzSampling
{
public:
    u8 mean;
    double stddev;

    FuzzSampling(u8 mean, double stddev)
    {
        this->mean = mean;
        this->stddev = stddev;
    }
};

class FuzzProb : public BoundedProblem<double>
{
public:
    using Superclass = BoundedProblem<double>;
    using typename cppoptlib::BoundedProblem<double>::Scalar;
    using typename cppoptlib::BoundedProblem<double>::TVector;

    char **argv;
    u8 *out_buf;
    s32 len;
    int accuracy;
    int stage;
    u32 upper;
    int mode;
    int prob;
    int mIdx = -1, pIdx = -1;
    // int maxIdx = -1;
    // double maxGrad = -1;
    bool firstMove = false;
    int topK[TOPK] = {
        0,
    };

    FuzzProb(int len) : Superclass(len) {}

    double value(const TVector &x)
    {
#ifdef MAXAFL_DEBUG
        // cerr << "current    " << x.transpose() << endl;
#endif
        int i;
        if (mIdx == -1 && pIdx == -1)
        {
            if (stage == 1)
            {
                for (i = 0; i < len; i++)
                {
                    out_buf[i] = (u8)(s8)x[i];
                }
            }
        }
        else
        {
            if (mIdx != -1)
                out_buf[mIdx] = (u8)(s8)x[mIdx];
            if (pIdx != -1)
                out_buf[pIdx] = (u8)(s8)x[pIdx];
        }

        common_fuzz_stuff(argv, out_buf, len);
        return calculate_obj_func();
    }

    void gradient(const TVector &x, TVector &grad)
    {
        if (mode == LBFGS_MODE_ORIGIN) // LBFGS_MODE_ORIGIN
        {
            // accuracy can be 0, 1, 2, 3
            int accuracy = this->accuracy;
            // const Scalar eps = 2.2204e-6;
            const Scalar eps = 1;
            static const std::array<std::vector<Scalar>, 4> coeff =
                {{{1, -1}, {1, -8, 8, -1}, {-1, 9, -45, 45, -9, 1}, {3, -32, 168, -672, 672, -168, 32, -3}}};
            static const std::array<std::vector<Scalar>, 4> coeff2 =
                {{{1, -1}, {-2, -1, 1, 2}, {-3, -2, -1, 1, 2, 3}, {-4, -3, -2, -1, 1, 2, 3, 4}}};
            static const std::array<Scalar, 4> dd = {2, 12, 60, 840};

            grad.resize(x.rows());
            TVector &xx = const_cast<TVector &>(x);

            mIdx = pIdx = -1;
            // maxIdx = -1;
            // maxGrad = -1;
            double vx = value(x);

            const int innerSteps = 2 * (accuracy + 1);
            const Scalar ddVal = dd[accuracy] * eps;

            for (TIndex d = 0; d < x.rows(); d++)
            {
                if (grad[d] == 0 && !firstMove)
                {
                    continue;
                }

                grad[d] = 0;
                pIdx = mIdx;
                mIdx = d;
                double vxx[2] = {
                    0,
                };
                for (int s = 0; s < innerSteps; ++s)
                {
                    Scalar tmp = xx[d];
                    xx[d] += coeff2[accuracy][s] * eps;
                    vxx[s] = value(xx);

                    // #ifdef MAXAFL_DEBUG
                    //                     cerr << "[DBG]\tcheck_branch_hit() is called from gradient() with d = " << d << ", s = " << s << endl;
                    // #endif
                    char chk = check_branch_hit();
                    if (chk != BR_UNCHANGED)
                        vxx[s] = chk;
                    // else
                    // #ifdef MAXAFL_DEBUG
                    //                         cerr << endl
                    //                              << "[DBG]\toriginal f = " << vx << "\t, changed f = " << vxx[s] << endl;
                    // #endif

                    xx[d] = tmp;
                }

                // *Calculate gradient depends on case
                if (vxx[0] == BR_WRONG && vxx[1] == BR_WRONG)
                {
                    grad[d] = 0;
                }
                else if (vxx[0] == BR_CHANGED && vxx[1] == BR_CHANGED)
                {
                    grad[d] = -LBFGS_DEFAULT_GRAD;
                }
                else if (vxx[0] == BR_CHANGED && vxx[1] == BR_WRONG)
                {
                    grad[d] = -LBFGS_DEFAULT_GRAD;
                }
                else if (vxx[0] == BR_WRONG && vxx[1] == BR_CHANGED)
                {
                    grad[d] = LBFGS_DEFAULT_GRAD;
                }
                else if (vxx[0] == BR_WRONG)
                {
                    grad[d] = (coeff[accuracy][0] * mind(vx, vxx[1]) + coeff[accuracy][1] * vxx[1]) / ddVal;
                }
                else if (vxx[1] == BR_WRONG)
                {
                    grad[d] = (coeff[accuracy][0] * vxx[0] + coeff[accuracy][1] * mind(vx, vxx[0])) / ddVal;
                }
                else if (vxx[0] == BR_CHANGED) // BR_CHANGED로 유도하자.
                {
                    grad[d] = (coeff[accuracy][0] * (vx < vxx[1] ? vx : vxx[1] - LBFGS_DOUBLE_GRAD) + coeff[accuracy][1] * vxx[1]) / ddVal;
                }
                else if (vxx[1] == BR_CHANGED) // BR_CHANGED로 유도하자.
                {
                    grad[d] = (coeff[accuracy][0] * vxx[0] + coeff[accuracy][1] * (vx < vxx[0] ? vx : vxx[0] - LBFGS_DOUBLE_GRAD)) / ddVal;
                }
                // else if (vxx[0] >= vx && vxx[1] >= vx)
                // {
                //     grad[d] = 0;
                // }
                // else if (vxx[0] <= vx && vxx[1] <= vx)
                // {
                //     if (vxx[0] < vxx[1])
                //     {
                //         grad[d] = (coeff[accuracy][0] * vxx[0] + coeff[accuracy][1] * vx) / ddVal;
                //     }
                //     else
                //     {
                //         grad[d] = (coeff[accuracy][0] * vx + coeff[accuracy][1] * vxx[1]) / ddVal;
                //     }
                // }
                else
                {
                    grad[d] = (coeff[accuracy][0] * vxx[0] + coeff[accuracy][1] * vxx[1]) / ddVal;
                }

                // if (abs(grad[d]) > maxGrad)
                // {
                //     maxGrad = abs(grad[d]);
                //     maxIdx = d;
                // }

                if (grad[d] > LBFGS_GRAD_MAX)
                {
                    grad[d] = LBFGS_GRAD_MAX;
                }
                else if (LBFGS_GRAD_NORM_MIN < grad[d] && grad[d] < LBFGS_DEFAULT_GRAD)
                {
                    grad[d] = LBFGS_DEFAULT_GRAD;
                }
                else if (-LBFGS_DEFAULT_GRAD < grad[d] && grad[d] < -LBFGS_GRAD_NORM_MIN)
                {
                    grad[d] = -LBFGS_DEFAULT_GRAD;
                }
                else if (grad[d] < -LBFGS_GRAD_MAX)
                {
                    grad[d] = -LBFGS_GRAD_MAX;
                }
            }
        }
        else if (mode == LBFGS_MODE_BOUND) // LBFGS_MODE_BOUND
        {
            // accuracy can be 0, 1, 2, 3
            int accuracy = this->accuracy;
            // const Scalar eps = 2.2204e-6;
            const Scalar eps = 1;
            static const std::array<std::vector<Scalar>, 4> coeff =
                {{{1, -1}, {1, -8, 8, -1}, {-1, 9, -45, 45, -9, 1}, {3, -32, 168, -672, 672, -168, 32, -3}}};
            static const std::array<std::vector<Scalar>, 4> coeff2 =
                {{{1, -1}, {-2, -1, 1, 2}, {-3, -2, -1, 1, 2, 3}, {-4, -3, -2, -1, 1, 2, 3, 4}}};
            static const std::array<Scalar, 4> dd = {2, 12, 60, 840};

            grad.resize(x.rows());
            TVector &xx = const_cast<TVector &>(x);

            mIdx = pIdx = -1;
            // maxIdx = -1;
            // maxGrad = -1;
            double vx = value(x);

            const int innerSteps = 2 * (accuracy + 1);
            const Scalar ddVal = dd[accuracy] * eps;

            for (TIndex d = 0; d < x.rows(); d++)
            {
                if (grad[d] == 0 && !firstMove)
                {
                    continue;
                }

                pIdx = mIdx;
                mIdx = d;
                double vxx[2] = {
                    0,
                };
                for (int s = 0; s < innerSteps; ++s)
                {
                    Scalar tmp = xx[d];
                    xx[d] += coeff2[accuracy][s] * eps;
                    vxx[s] = value(xx);
                    char chk = check_branch_hit();
                    if (chk != BR_UNCHANGED)
                        vxx[s] = vx;
                    xx[d] = tmp;
                }

                grad[d] = (coeff[accuracy][0] * vxx[0] + coeff[accuracy][1] * vxx[1]) / ddVal;

                // if (grad[d] > LBFGS_GRAD_MAX)
                // {
                //     grad[d] = LBFGS_GRAD_MAX;
                // }
                // else if (LBFGS_GRAD_NORM_MIN < grad[d] && grad[d] < LBFGS_DEFAULT_GRAD)
                // {
                //     grad[d] = LBFGS_DEFAULT_GRAD;
                // }
                // else if (-LBFGS_DEFAULT_GRAD < grad[d] && grad[d] < -LBFGS_GRAD_NORM_MIN)
                // {
                //     grad[d] = -LBFGS_DEFAULT_GRAD;
                // }
                // else if (grad[d] < -LBFGS_GRAD_MAX)
                // {
                //     grad[d] = -LBFGS_GRAD_MAX;
                // }
            }
        }
        else if (mode == LBFGS_MODE_NORMAL) // LBFGS_MODE_NORMAL
        {
            // accuracy can be 0, 1, 2, 3
            int accuracy = this->accuracy;
            // const Scalar eps = 2.2204e-6;
            const Scalar eps = 1;
            static const std::array<std::vector<Scalar>, 4> coeff =
                {{{1, -1}, {1, -8, 8, -1}, {-1, 9, -45, 45, -9, 1}, {3, -32, 168, -672, 672, -168, 32, -3}}};
            static const std::array<std::vector<Scalar>, 4> coeff2 =
                {{{1, -1}, {-2, -1, 1, 2}, {-3, -2, -1, 1, 2, 3}, {-4, -3, -2, -1, 1, 2, 3, 4}}};
            static const std::array<Scalar, 4> dd = {2, 12, 60, 840};

            grad.resize(x.rows());
            TVector &xx = const_cast<TVector &>(x);

            const int innerSteps = 2 * (accuracy + 1);
            const Scalar ddVal = dd[accuracy] * eps;

            mIdx = pIdx = -1;

            if (firstMove)
            {
                for (TIndex d = 0; d < x.rows(); d++)
                {
                    pIdx = mIdx;
                    mIdx = d;
                    grad[d] = 0;
                    for (int s = 0; s < innerSteps; ++s)
                    {
                        Scalar tmp = xx[d];
                        xx[d] += coeff2[accuracy][s] * eps;
                        grad[d] += coeff[accuracy][s] * value(xx);
                        xx[d] = tmp;
                    }
                    grad[d] /= ddVal;
                }
            }
            // else
            // {

            // }
        }
#ifdef _MAXAFL_DEBUG
        cerr
            << "Gradient : \n"
            << grad.transpose() << endl;
#endif
        firstMove = false;
    }
};

template <typename ProblemType>
class GradientDescentFuzzSolver : public GradientDescentSolver<ProblemType>
{
    using Superclass = GradientDescentSolver<ProblemType>;
    using typename Superclass::Scalar;
    using typename Superclass::TVector;

    // Override
public:
    void minimize(ProblemType &objFunc, TVector &x0)
    {
        TVector direction(x0.rows()), delta(x0.rows()), norm(x0.rows());
        this->m_current.reset();
        objFunc.firstMove = true;
        Scalar rate = LBFGS_INITIAL_RATE;
        s8 flag = 0;
        do
        {
            objFunc.mIdx = objFunc.pIdx = -1;
            double origin_f = objFunc.value(x0);
#ifdef _MAXAFL_DEBUG
            std::cerr << "Input : " << x0.transpose() << std::endl;
            std::cerr << "F\t:\t" << origin_f << std::endl;
#endif
            std::cerr << "F : " << origin_f << std::endl;
            save_branch_hit();
            u32 r = UR2(100);
            if (objFunc.prob == PROB_MODE_EPSILON && this->m_current.iterations > 10 && r < EPSILON_BITFLIP + EPSILON_NORMAL)
            {
                //  NORMAL SAMPLING
                if (r < EPSILON_NORMAL)
                {
                    std::cerr << "Epsilon normal occured!!" << std::endl;
                    TVector stddev(x0.rows());
                    default_random_engine generator;

                    stddev = norm / this->m_current.iterations;

                    for (int i = 0; i < stddev.rows(); i++)
                    {
                        double sample = normal_distribution<double>(x0[i], stddev[i])(generator);
                        if (sample < 0)
                            sample = 0;
                        else if (sample > 255.5)
                            sample = 255;
                        x0[i] = sample;
                    }
                }
                //  BITFLIPPING
                else
                {
                    std::cerr << "Epsilon bitflip occured!!" << std::endl;
                    for (int k = 0; k < BITFLIP_CNT; k++)
                    {
                        u32 idx = UR2(objFunc.len);
                        u32 bit = UR2(8);

                        u32 val = (u32)x0[idx];
                        val ^= (1 << bit);
                        x0[idx] = val;
                    }
                }
            }
            else
            {
                std::cerr << "Gradient Calculation..." << std::endl;
                objFunc.gradient(x0, direction);
                // const Scalar rate = MoreThuente<ProblemType, 1>::linesearch(x0, -direction, objFunc);

                if (objFunc.mode == LBFGS_MODE_BOUND)
                {
                    // // rate = MoreThuente<ProblemType, 1>::linesearch(x0, -direction, objFunc);
                    // // x0 = x0 - rate * direction;
                    // // double f2 = objFunc.value(x0);

                    // // if (check_branch_hit() != BR_UNCHANGED)
                    // // {
                    // //     flag = -2;
                    // //     break;
                    // // }
                    int i = 0;
                    do
                    {
                        x0 = x0 - rate * direction;
                        double f2 = objFunc.value(x0);
                        if (origin_f == f2)
                        {
                            if (flag == 1)
                            {
                                flag = -2;
                                x0 = x0 + rate * direction;
                                break;
                            }

                            rate /= LBFGS_GAMMA;
                            flag = -1;
                            x0 = x0 + rate * direction;
                        }
                        else if (check_branch_hit() != BR_UNCHANGED)
                        {
                            if (flag == -1)
                            {
                                flag = -2;
                                x0 = x0 + rate * direction;
                                break;
                            }

                            rate *= LBFGS_GAMMA;
                            flag = 1;
                            x0 = x0 + rate * direction;
                        }
                        else
                            break;
                        i++;
                    } while (i < 5);

                    if (flag == -2 || i == 5)
                        break;

                    // x0 = x0 - rate * direction;
                }
                else
                {
                    delta = rate * direction;
                    if (objFunc.prob != PROB_MODE_NONE)
                    {
                        norm = norm + delta;
                    }
                    x0 = x0 - delta;
                }

                // x0[objFunc.maxIdx] = x0[objFunc.maxIdx] - rate * direction[objFunc.maxIdx];
                // rate *= 0.99;`

                this->m_current.gradNorm = direction.template lpNorm<Eigen::Infinity>();
            }
            // std::cout << "iter: "<<iter<< " f = " <<  objFunc.value(x0) << " ||g||_inf "<<gradNorm  << std::endl;
            ++this->m_current.iterations;
            this->m_status = checkConvergence(this->m_stop, this->m_current);
            // std::cerr << this->m_current.gradNorm << endl;
            // this->m_current.print(std::cerr);
        } while (objFunc.callback(this->m_current, x0) && (this->m_status == Status::Continue));
#ifdef _MAXAFL_DEBUG
        std::cerr << "Stop status was: " << this->m_status << " flag = " << flag << std::endl;
        std::cerr << "Stop criteria were: " << std::endl
                  << this->m_stop << std::endl;
        std::cerr << "Current values are: " << std::endl
                  << this->m_current << std::endl;
#endif
        // if (this->m_debug > DebugLevel::None)
        // {
        //     std::cout << "Stop status was: " << this->m_status << std::endl;
        //     std::cout << "Stop criteria were: " << std::endl
        //               << this->m_stop << std::endl;
        //     std::cout << "Current values are: " << std::endl
        //               << this->m_current << std::endl;
        // }
    }
};

Criteria<double> criteria;

FuzzProb *f = NULL;
LbfgsbSolver<FuzzProb> solver;
LbfgsSolver<FuzzProb> solver2;
GradientDescentFuzzSolver<FuzzProb> solver3;

vector<FuzzSampling *> fuzz_dist;

extern "C" int init_lbfgs(char **argv, u8 *out_buf, s32 len, int stage, int accuracy, int mode, int prob)
{

    f = new FuzzProb(len);

    if (stage == 1)
    {
        f->upper = 0xffff;
        f->setLowerBound(FuzzProb::TVector::Zero(len));
        f->setUpperBound(FuzzProb::TVector::Ones(len) * 255.5);
    }

    f->argv = argv;
    f->out_buf = out_buf;

    f->len = len / stage;
    if (f->len == 0)
    {
        return -1;
    }
    f->accuracy = accuracy;
    f->stage = stage;
    f->mode = mode;
    f->prob = prob;

    criteria.iterations = LBFGS_ITERATION_MAX;
    criteria.gradNorm = LBFGS_GRAD_NORM_MIN;

    // solver1.setStopCriteria(criteria);
    // solver2.setStopCriteria(criteria);
    solver3.setStopCriteria(criteria);

    // solver.setDebug(DebugLevel::High);
    // solver2.setDebug(DebugLevel::High);
    // solver3.setDebug(DebugLevel::High);
    return 1;
}

extern "C" int free_lbfgs()
{
    delete f;

    return 1;
}

extern "C" double solve_lbfgs(u8 *in_buf, int len)
{
    double fx;
    FuzzProb::TVector x(len);
    int i;
    if (f->stage == 1)
    {
        for (i = 0; i < len; i++)
        {
            x(i) = (double)in_buf[i];
        }
    }

    // f->value(x);
    // save_branch_hit();

    // solver.minimize(*f, x);
    // solver2.minimize(*f, x);
    // cerr << "m_status : " << solver2.status() << endl;
    solver3.minimize(*f, x);
    // cerr << "m_status : " << solver3.status() << endl;
    fx = (*f)(x);
#ifdef MAXAFL_DEBUG
    cerr << "argmin    " << x.transpose() << endl;
    cerr << "f in argmin " << fx << endl;
#endif

    return fx;
}

extern "C" int init_normal_sampling(u8 *mean, int len, double stddev)
{
    for (int i = 0; i < len; i++)
    {
        fuzz_dist.push_back(new FuzzSampling(mean[i], stddev));
    }

    return 1;
}

extern "C" int free_normal_sampling(int len)
{
    for (vector<FuzzSampling *>::iterator iter = fuzz_dist.begin(); iter != fuzz_dist.end(); iter++)
    {
        delete (*iter);
    }
    fuzz_dist.clear();

    return 1;
}

extern "C" void modify_dist(u8 *mean, int len)
{
    for (int i = 0; i < len; i++)
    {
        fuzz_dist[i]->mean = mean[i];
    }
}

extern "C" void sample_dist(u8 *out_buf, int len)
{
    default_random_engine generator;

#ifdef MAXAFL_DEBUG
    cerr << "sampling\t";
#endif
    for (int i = 0; i < len; i++)
    {
        double sample = normal_distribution<double>(fuzz_dist[i]->mean, fuzz_dist[i]->stddev)(generator);
        if (sample < 0)
            sample = 0;
        if (sample > 255)
            sample = 255;
        out_buf[i] = (u8)sample;
#ifdef MAXAFL_DEBUG
        cerr << dec << out_buf[i] << " ";
#endif
    }
#ifdef MAXAFL_DEBUG
    cerr << endl;
#endif

    return;
}