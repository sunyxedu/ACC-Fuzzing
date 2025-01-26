//
// Created by Hemlix on 2025/1/25.
//

#ifndef CONTEXT_TREE_H
#define CONTEXT_TREE_H

#include <vector>
#include <cmath>

struct ContextNode {
    std::vector<double> context;
    double index;
    int count;
    ContextNode* parent;
    std::vector<ContextNode*> children;
    bool isLeaf;

    ContextNode(std::vector<double> ctx, ContextNode* p = nullptr)
            : context(ctx), index(0.0), count(0), parent(p), isLeaf(true) {}

    void expand(double rho, double v1);
};

#endif
