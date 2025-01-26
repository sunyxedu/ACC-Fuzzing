//
// Created by Hemlix on 2025/1/25.
//

#include "context_tree.h"

void ContextNode::expand(double rho, double v1) {
    if (!isLeaf) return;

    double new_radius = v1 * pow(rho, log(count + 1) / log(1.0 / rho));

    if (new_radius > 0.01) {
        for (int i = 0; i < 2; ++i) {
            std::vector<double> new_ctx = context;
            new_ctx[0] += (i == 0 ? -new_radius : new_radius);
            children.push_back(new ContextNode(new_ctx, this));
        }
        isLeaf = false;
    }
}
