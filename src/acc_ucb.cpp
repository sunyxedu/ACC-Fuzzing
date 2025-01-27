//
// Created by Hemlix on 2025/1/25.
//

#include "acc_ucb.h"

// The main problem is I don't quite understand the "context". How should we deal with it??
// I used a simple i/K to show that they are different. It's obviously wrong. But I don't know how to turn it into [0, 1].

// 0. These parameters are the ones which given by the paper

ACC_UCB::ACC_UCB(int K, int T, double exploration)
        : K(K), T(T), v1(std::sqrt(5)), v2(1.0), rho(0.5), 
          exploration_param(exploration), jsFuzzer(), 
          rootContext({0.0}), total_rewards(0.0), total_plays(0) {
    generator.seed(time(0));
    reward_history.resize(K);
    
    for (int i = 0; i < K; ++i) {
        std::vector<double> context = {static_cast<double>(i) / K};
        arms[i] = Arm(context, &rootContext);
    }
}

// 2. Compute indices g^t(x_h,i)
// g = mu(p(x)) + c(p(x)) + v1*ro
int getNodeDepth(const ContextNode* node) {
    int depth = 0;
    while (node->parent) {
        node = node->parent;
        depth++;
    }
    return depth;
}

double ACC_UCB::computeUCBScore(const Arm& arm) {
    if (arm.plays == 0) return std::numeric_limits<double>::infinity();

    double exploitation = arm.reward;
    double exploration = exploration_param * std::sqrt((std::log(total_plays + 1) / arm.plays));
    double node_ucb = exploitation + exploration;

    double parent_ucb = 0.0;
    if (arm.node && arm.node->parent) {
        double parent_mean = arm.node->parent->index;
        double parent_visits = arm.node->parent->count;
        double parent_confidence = (parent_visits > 0) ? std::sqrt((2 * std::log(total_plays + 1)) / parent_visits) : 0.0;
        parent_ucb = parent_mean + parent_confidence + v1 * std::pow(rho, getNodeDepth(arm.node) - 1);
    }

    double bt_xhi = std::min(node_ucb, parent_ucb);
    double adaptive_discretization = v1 * std::pow(rho, getNodeDepth(arm.node));

    return bt_xhi + adaptive_discretization;
}

// 3. Compute indices g^t(x_m^t)
double ACC_UCB::computeDiversity(int armID) {
    const Arm& current_arm = arms[armID];
    double diversity_score = 0.0;

    for (const auto& pair : arms) {
        if (pair.first != armID) {
            double distance = 0.0;
            for (size_t i = 0; i < current_arm.context.size(); ++i) {
                distance += std::pow(current_arm.context[i] - pair.second.context[i], 2);
            }
            diversity_score += std::sqrt(distance);
        }
    }

    double coverageDiff = 0.0;
    for (const auto& pair : arms) {
        if (pair.first != armID) {
            coverageDiff += std::abs(current_arm.coverage - pair.second.coverage);
        }
    }
    coverageDiff /= (arms.size() - 1);

    double partition_size = rho * v1;
    double combined_diversity = diversity_score / partition_size + 0.2 * coverageDiff;

    return combined_diversity + (v1 / v2) * v1 * rho;
}

// 5. Select Super Arm
std::vector<int> ACC_UCB::selectSuperArm() {
    std::vector<std::pair<int, double>> armScores;
    
    if (arms.empty()) {
        std::cout << "Warning: No arms available for selection" << std::endl;
        return std::vector<int>();
    }
    
//    std::cout << "Selecting from " << arms.size() << " arms" << std::endl;
    
    for (const auto& pair : arms) {
        // 4. Identify available active leaf nodes N^t \subseteq L^t
        double ucb_score = computeUCBScore(pair.second);
        double diversity = computeDiversity(pair.first);
        double coverageBoost = pair.second.coverage * 0.5;
        double combined_score = ucb_score + 0.3 * diversity + coverageBoost;
        armScores.push_back({pair.first, combined_score});

        // DEBUGGGGGGGGGG
        /*
        std::cout << "Arm " << pair.first << ": UCB=" << ucb_score
                 << " Div=" << diversity << " Cov=" << pair.second.coverage 
                 << " Total=" << combined_score << std::endl;
        */
    }
    std::sort(armScores.begin(), armScores.end(),
              [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
                  return a.second > b.second;
              });

    std::vector<int> selected;
    for (int i = 0; i < K && i < armScores.size(); ++i) {
        selected.push_back(armScores[i].first);
    }
    return selected;
}

// 7. Identify the set of selected nodes
void ACC_UCB::updateArm(int armID, double reward) {
    auto it = arms.find(armID);
    if (it != arms.end()) {
        Arm& arm = it->second;

        double previous_count = arm.plays;
        double previous_mean = arm.reward;

        // 8. Update mu & C
        arm.plays++;  // C^t(x_{h,i}) update
        arm.reward = (previous_count * previous_mean + reward) / arm.plays;  // μ̂^t(x_{h,i}) update

        // Example bandit logic: pick a mutation index in [0..2]
        int mutationType = armID % 3;  // or any logic you prefer
        // 6. Observe outcomes and collect reward
        std::string fuzzedJS = jsFuzzer.generateFuzzedJS(mutationType);
        arm.coverage = getCoverage(fuzzedJS);
        
        total_rewards += reward;
        total_plays++;

        // Expand context if usage threshold is high
        if (arm.node) {
            // 8. Update mu & C
            double prev_C = arm.node->count;
            arm.node->count++;
            arm.node->index = (prev_C * arm.node->index + reward) / arm.node->count;

            double confidence_bound = sqrt(2 * log(total_plays + 1) / arm.node->count);
            int node_depth = getNodeDepth(arm.node);
            double threshold = v1 * pow(rho, node_depth);
            if (confidence_bound <= threshold) {
                arm.node->expand(rho, v1);
            }
        }
        
        updateRewardHistory(armID, reward);
    }
}

void ACC_UCB::updateRewardHistory(int armID, double reward) {
    if (armID < reward_history.size()) {
        reward_history[armID].push_back(reward);
    }
}

double ACC_UCB::getAverageReward() const {
    return total_plays > 0 ? total_rewards / total_plays : 0.0;
}

double ACC_UCB::getCoverage(const std::string& jsCode) {
    // Generate coverage score based on code complexity
    double baseScore = 0.3; // minimum coverage
    
    // Add score for each interesting pattern found in code
    if (jsCode.find("try") != std::string::npos) baseScore += 0.1;
    if (jsCode.find("while") != std::string::npos) baseScore += 0.1;
    if (jsCode.find("for") != std::string::npos) baseScore += 0.1;
    if (jsCode.find("function") != std::string::npos) baseScore += 0.1;
    if (jsCode.find("if") != std::string::npos) baseScore += 0.1;
    
    // Add some randomness
    std::uniform_real_distribution<double> dist(0.0, 0.2);
    return std::min(1.0, baseScore + dist(generator));
}

void ACC_UCB::run() {
    // --- TEST ---
    /*
    if (arms.empty()) {
        std::cout << "Error: No arms initialized. Aborting." << std::endl;
        return;
    }
     */

    for (int t = 1; t <= T; ++t) {
        try {
            // 1. Observe base arms in M^t and their contexts X^t
            std::vector<int> chosenArms = selectSuperArm();
            if (chosenArms.empty()) {
                std::cout << "What????? No arms selected in round" << t << std::endl;
                continue;
            }

            for (int armID : chosenArms) {
                if (arms.find(armID) == arms.end()) {
                    std::cout << "Error: Invalid arm ID " << armID << std::endl;
                    continue;
                }

                Arm& arm = arms[armID];

                int mutationType = (arm.plays < 10) ? (t % 3) : (arm.node ? (arm.node->count % 3) : (armID % 3));
                
                try {
                    std::string fuzzedJS = jsFuzzer.generateFuzzedJS(mutationType);
                    double reward = jsFuzzer.executeJS(fuzzedJS);
                    
                    std::cout << "Arm " << armID << " received reward: " << reward << std::endl;  // 调试信息
                    
                    updateArm(armID, reward);
                } catch (const std::exception& e) {
                    std::cout << "Error in fuzzing/execution: " << e.what() << std::endl;
                    continue;
                }
            }

            if (t % 100 == 0) {
                std::cout << "Round " << t << " completed. Average reward: "  << getAverageReward() << std::endl;
                // Print Infos
                for (const auto& pair : arms) {
                    std::cout << "Arm " << pair.first << " stats - Plays: " << pair.second.plays 
                             << " Reward: " << pair.second.reward 
                             << " Coverage: " << pair.second.coverage << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "Error in round " << t << ": " << e.what() << std::endl;
            continue;
        }
    }
}
