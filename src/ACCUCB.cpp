#include "ACCUCB.h"
#include <algorithm>
#include <random>
#include <cmath>
#include <iostream>

static std::mt19937 g_rng(12345);

ACCUCB::ACCUCB(int _T, int _K, double _v1, double _v2, double _rho, int _Nchild)
        : T(_T), K(_K), v1(_v1), v2(_v2), rho(_rho), Nchild(_Nchild)
{
    // Create root node (h=0, idx=1)
    root = std::make_unique<Node>(0, 1, nullptr);
    activeLeaves.push_back(root.get());
}

double ACCUCB::computeNodeIndex(Node* node) {
    // Calculate confidence interval c^t
    double cVal = node->confidenceRadius(T);
    // Parent node mean (if no parent, use self)
    double parentMuHat = node->parent ? node->parent->muHat : node->muHat;

    // b^t = min( muHat + c, parentMuHat + v1*rho^(h-1) )
    double bound1 = node->muHat + cVal;
    double bound2 = parentMuHat + v1 * std::pow(rho, (node->h > 0 ? node->h - 1 : 0));
    double bVal   = std::min(bound1, bound2);

    // Add extra term (according to paper definition, using v2 * rho^h here)
    double extra  = v2 * std::pow(rho, node->h);

    return bVal + extra;
}

Node* ACCUCB::matchLeaf(const BaseArm &arm) {
    // Simplified: randomly return an active leaf (in real case should determine based on which subregion the context falls into)
    if (!activeLeaves.empty()) {
        std::uniform_int_distribution<int> dist(0, (int)activeLeaves.size()-1);
        int r = dist(g_rng);
        return activeLeaves[r];
    }
    return nullptr;
}

std::vector<int> ACCUCB::approximateOracle(const std::vector<double> &indices) {
    int M = (int)indices.size();
    std::vector<int> idx(M);
    for(int i=0; i<M; i++) {
        idx[i] = i;
    }
    // Sort by g^t values in descending order
    std::sort(idx.begin(), idx.end(), [&](int a, int b){
        return indices[a] > indices[b];
    });
    // Take top K
    idx.resize(std::min(K, M));
    return idx;
}

void ACCUCB::updateNodes(const std::vector<BaseArm> & /*chosenArms*/,
                         std::map<Node*, int> &countMap,
                         std::map<Node*, double> &rewardMap)
{
    // Update \hat{\mu}^t, C^t according to equations (1)(2)
    for (auto &kv : countMap) {
        Node* nd = kv.first;
        int numChosen = kv.second;
        double sumReward = rewardMap[nd];
        if (numChosen <= 0) continue;

        double oldC = nd->C;
        double oldMuHat = nd->muHat;
        double numerator = oldC * oldMuHat + sumReward;
        double denominator = oldC + numChosen;
        nd->muHat = numerator / denominator;
        nd->C     = denominator;
    }
}

void ACCUCB::refineCheck() {
    std::vector<Node*> newLeaves;
    for (auto *leaf : activeLeaves) {
        if (!leaf->isActive) {
            continue;
        }
        double cVal = leaf->confidenceRadius(T);
        // Refine if c^t(x_{h,i}) <= v1 * rho^h
        if (cVal <= (v1 * std::pow(rho, leaf->h))) {
            leaf->isActive = false;
            for(int j=1; j<=Nchild; j++){
                auto child = std::make_unique<Node>(leaf->h + 1, j, leaf);
                // Child node initial statistics (can inherit from parent or set to 0)
                child->muHat = leaf->muHat;
                child->C     = 0.0;
                child->isActive = true;

                Node* rawPtr = child.get();
                leaf->children.push_back(std::move(child));
                newLeaves.push_back(rawPtr);
            }
        } else {
            newLeaves.push_back(leaf);
        }
    }
    // Update new active leaf set
    activeLeaves = newLeaves;
}

void ACCUCB::run() {
    for(int t=1; t<=T; t++){
        // 1) Generate base arms for this round
        int Mt = 10;
        std::vector<BaseArm> arms = generateBaseArms(Mt, t);

        // 2) Calculate g^t for each arm's corresponding leaf node
        std::vector<double> indices(Mt, 0.0);
        std::vector<Node*>  matchedNode(Mt, nullptr);
        for(int m=0; m<Mt; m++){
            Node* leaf = matchLeaf(arms[m]);
            matchedNode[m] = leaf;
            indices[m] = computeNodeIndex(leaf);
        }

        // 3) Super arm selection (greedily take K arms with highest scores)
        std::vector<int> chosen = approximateOracle(indices);

        // 4) Count rewards and update nodes
        std::map<Node*, int>    countMap;
        std::map<Node*, double> rewardMap;
        for(int cidx : chosen) {
            Node* nd = matchedNode[cidx];
            double rew = genReward(arms[cidx].trueMean);
            arms[cidx].lastReward = rew;

            countMap[nd] += 1;
            rewardMap[nd] += rew;
        }
        updateNodes(arms, countMap, rewardMap);

        // 5) refine
        refineCheck();
    }
}

// Generate M base arms (for demonstration)
std::vector<BaseArm> ACCUCB::generateBaseArms(int M, int t) {
    std::vector<BaseArm> arms(M);
    for(int i=0; i<M; i++){
        arms[i].armID = i;
        // Example: random context (2D)
        arms[i].context = { uniformReal(0.0, 1.0), uniformReal(0.0, 1.0) };
        // Randomly assign an expected reward
        arms[i].trueMean = uniformReal(0.0, 1.0);
        arms[i].lastReward = 0.0;
    }
    return arms;
}

// Generate Bernoulli random reward
double ACCUCB::genReward(double mean) {
    std::bernoulli_distribution dist(mean);
    return dist(g_rng) ? 1.0 : 0.0;
}

// Return random number in range [low, high)
double ACCUCB::uniformReal(double low, double high) {
    std::uniform_real_distribution<double> dist(low, high);
    return dist(g_rng);
}
