#ifndef CONTEXT_TREE_H
#define CONTEXT_TREE_H

#include <vector>
#include <memory>

class Node {
public:
    int h;
    int idx;
    Node* parent;
    std::vector<std::unique_ptr<Node>> children;

    double muHat;
    double C;
    bool isActive;

    Node(int depth, int index, Node* p = nullptr);

    double confidenceRadius(int T) const;
};

#endif // NODE_H
