# MaxAFL

## Overview
- Maximize code coverage using optimization algorithm.

## Main Idea
- construct global objective function for optimization based on static code analysis.
- implement fast optimization algorithm using gradient descent.

## Implementation
- implement instrumentation for fuzzing using [LLVM Pass](http://llvm.org/docs/WritingAnLLVMPass.html).
    ### Dev environment
    - Ubuntu 18.04
    - Visual Studio Code
    ### Dependencies
    - LLVM 8.0.0
    - Boost Graph Library 1.6.2

## Test
### LLVM Pass Test
- opt -load (path_to_so_file)/FuncBlockCount.so -funcblockcount sample.ll
- clang -O0 -S -emit-llvm sample.c -o sample.ll

## Paper Work
- [Google Docs](https://docs.google.com/document/d/1SmVOV06Il6nrnBRXcNqVQ92Bwn9k4HVG-np6sr8_zI0/edit?usp=sharing)

## Related Works
1. Angora
   - use gradient descent to explore path.
   - use taint analysis and type inference for efficient gradient descent.
   - [pdf](https://web.cs.ucdavis.edu/~hchen/paper/chen2018angora.pdf), [github](https://github.com/AngoraFuzzer/Angora)
2. NEUZZ
   - train Deep Neural Network that predict branch coverage(TBA).
   - [pdf](https://arxiv.org/pdf/1807.05620.pdf), [github](https://github.com/Dongdongshe/neuzz)
3. GRsan(Proximal gradient analysis)
   - use chain rule to compute gradient of variable w.r.t. input.
   - [pdf](https://arxiv.org/pdf/1909.03461.pdf), source code is not available yet.


## Links
1. Fuzzing corpus
   - [corpus](https://koreaoffice-my.sharepoint.com/:u:/g/personal/acorn421_korea_edu/EdC_WNnLzqdFuVCRTD8F_B4BucnoUlo69K9BUrFrmhPkcg?e=OgYhsZ)
2. Fuzzing targets
   - [targets](https://koreaoffice-my.sharepoint.com/:u:/g/personal/acorn421_korea_edu/EakuMm6DgDVNuOuRFoOJMcAB_dRDumIh2dw3BVUlUhDLcA?e=7uPm3z)


## Analyze Result
- gen_cov.py -> coverage.py -> plot.py