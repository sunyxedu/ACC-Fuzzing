//
// Created by Hemlix on 2025/1/25.
//

#include "acc_ucb.h"

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

double ACC_UCB::computeUCBScore(const Arm& arm) {
    if (arm.plays == 0) return std::numeric_limits<double>::infinity();
    
    double exploitation = arm.reward;
    double exploration = exploration_param * std::sqrt(std::log(total_plays + 1) / arm.plays);
    return exploitation + exploration;
}

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

    double combined_diversity = diversity_score + 0.2 * coverageDiff;
    return combined_diversity;
}

std::vector<int> ACC_UCB::selectSuperArm() {
    std::vector<std::pair<int, double>> armScores;
    
    // 添加保护：确保arms不为空
    if (arms.empty()) {
        std::cout << "Warning: No arms available for selection" << std::endl;
        return std::vector<int>();
    }
    
    // 添加调试信息
    std::cout << "Selecting from " << arms.size() << " arms" << std::endl;
    
    for (const auto& pair : arms) {
        double ucb_score = computeUCBScore(pair.second);
        double diversity = computeDiversity(pair.first);
        double coverageBoost = pair.second.coverage * 0.5;
        double combined_score = ucb_score + 0.3 * diversity + coverageBoost;
        armScores.push_back({pair.first, combined_score});
        
        // 添加调试信息
        std::cout << "Arm " << pair.first << ": UCB=" << ucb_score 
                 << " Div=" << diversity << " Cov=" << pair.second.coverage 
                 << " Total=" << combined_score << std::endl;
    }

    std::sort(armScores.begin(), armScores.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<int> selected;
    for (int i = 0; i < K && i < armScores.size(); ++i) {
        selected.push_back(armScores[i].first);
    }
    return selected;
}

void ACC_UCB::updateArm(int armID, double reward) {
    auto it = arms.find(armID);
    if (it != arms.end()) {
        Arm& arm = it->second;
        
        double oldReward = arm.reward * arm.plays;
        arm.plays++;
        arm.reward = (oldReward + reward) / arm.plays;
        
        // Example bandit logic: pick a mutation index in [0..2]
        int mutationType = armID % 3;  // or any logic you prefer
        std::string fuzzedJS = jsFuzzer.generateFuzzedJS(mutationType);
        arm.coverage = getCoverage(fuzzedJS);
        
        total_rewards += reward;
        total_plays++;

        // Expand context if usage threshold is high
        if (arm.node) {
            arm.node->count++;
            if (arm.node->count > 5) {
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
    // 添加保护：确保初始化成功
    if (arms.empty()) {
        std::cout << "Error: No arms initialized. Aborting." << std::endl;
        return;
    }

    for (int t = 1; t <= T; ++t) {
        try {
            // std::cout << "Starting round " << t << std::endl;
            
            std::vector<int> chosenArms = selectSuperArm();
            if (chosenArms.empty()) {
                std::cout << "Warning: No arms selected in round " << t << std::endl;
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
                std::cout << "Round " << t << " completed. Average reward: " 
                         << getAverageReward() << std::endl;
                // 打印每个arm的状态
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
