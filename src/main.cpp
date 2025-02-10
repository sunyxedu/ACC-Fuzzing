#include "ACCUCB.h"
#include <iostream>

int main() {
    int    T      = 100;
    int    K      = 2;
    double v1     = 1.0;
    double v2     = 0.5;
    double rho    = 0.5;
    int    Nchild = 2;

    ACCUCB accucbAlg(T, K, v1, v2, rho, Nchild);
    accucbAlg.run();

    std::cout << "ACC-UCB 示例运行完毕。\n";
    return 0;
}
