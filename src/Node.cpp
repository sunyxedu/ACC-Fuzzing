#include "Node.h"
#include <cmath>
#include <limits>

// 构造函数实现
Node::Node(int depth, int index, Node* p)
        : h(depth), idx(index), parent(p),
          muHat(0.0), C(0.0), isActive(true)
{
}

// 置信半径计算
double Node::confidenceRadius(int T) const {
    if (C <= 0.0) {
        return 1e9;
    }
    // 示例公式: sqrt(2 * ln(T) / C)
    double val = std::sqrt( (2.0 * std::log((double)T)) / C );
    return val;
}
