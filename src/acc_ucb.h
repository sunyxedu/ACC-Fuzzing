//
// Created by Hemlix on 2025/1/25.
//

#ifndef ACC_UCB_H
#define ACC_UCB_H

#include <vector>
#include <unordered_map>
#include <cmath>
#include <random>
#include <iostream>
#include "context_tree.h"
#include "js_fuzzer.h"

struct Arm {
    std::vector<double> context;
    double reward;
    int plays;
    ContextNode* node;
    double coverage;
    Arm() : reward(0.0), plays(0), node(nullptr), coverage(0.0) {}
    Arm(std::vector<double> ctx, ContextNode* n) : context(ctx), reward(0.0), plays(0), node(n), coverage(0.0) {}
};

class ACC_UCB {
private:
    int K, T;
    double v1, v2, rho;
    double exploration_param;
    ContextNode rootContext;
    std::unordered_map<int, Arm> arms;
    std::default_random_engine generator;
    
    std::vector<std::vector<double>> reward_history;
    double total_rewards;
    int total_plays;

    double getCoverage(const std::string& jsCode);
    double computeIndex(const Arm& arm);
    std::vector<int> selectSuperArm();
    void updateArm(int armID, double reward);
    
    double computeUCBScore(const Arm& arm);
    double computeDiversity(int armID);
    void updateRewardHistory(int armID, double reward);

public:
    JSFuzzer jsFuzzer;
    ACC_UCB(int K, int T, double exploration = 0.5);
    void run();
    double getAverageReward() const;
};

#endif
