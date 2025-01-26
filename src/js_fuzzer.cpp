//
// Created by Hemlix on 2025/1/25.
//

#include "js_fuzzer.h"
#include <iostream>
#include <cstdlib>
#include <vector>
#include <algorithm>

JSFuzzer::JSFuzzer() {
    generator.seed(time(0));
    mutationOperators = {
            "swap_statements", "insert_random_loop", "modify_literals",
            "change_variable_names", "duplicate_statements", "delete_random_part"
    };
    
    templates = {
            "let x = {}; for (let i = 0; i < 100; i++) x[i] = i; console.log(x);",  // to test objects and loops
            "function test() { return Math.random() * 100; } console.log(test());", // to test functions and random numbers
            "try { let a = new Array(1000000).fill(1); } catch (e) { console.log(e); }"  // to test exception handling and large arrays
    };
}

void JSFuzzer::addTemplate(const std::string& templ) {
    templates.push_back(templ);
}

std::string JSFuzzer::applyASTMutation(const std::string& input) {
    std::vector<std::string> mutations = {
            "function foo() { console.log('AST Mutation Applied!'); } foo();",
            "for (let i = 0; i < 100; i++) console.log(i);",
            "try { let a = JSON.parse('{\"key\": }'); } catch (e) { console.log(e); }"
    };
    std::uniform_int_distribution<int> dist(0, mutations.size() - 1);
    return input + "\n" + mutations[dist(generator)];
}

std::string JSFuzzer::applyCorpusMutation(const std::string& input) {
    std::vector<std::string> corpus = {
            "while (true) { console.log('looping'); }",
            "let arr = [1, 2, 3]; arr[-1] = 100; console.log(arr);",
            "Object.prototype.hacked = 'Pwned'; console.log(({}).hacked);"
    };
    std::uniform_int_distribution<int> dist(0, corpus.size() - 1);
    return corpus[dist(generator)];
}

std::string JSFuzzer::applyGrammarMutation(const std::string& input) {
    std::vector<std::string> mutations = {
            "let x = 42 / 0; console.log(x);",
            "const sym = Symbol('test'); console.log(sym.toString());",
            "console.log(Function('return 2 + 2')());"
    };
    std::uniform_int_distribution<int> dist(0, mutations.size() - 1);
    return input + "\n" + mutations[dist(generator)];
}

std::string JSFuzzer::generateFuzzedJS(int mutationType) {
    if (templates.empty()) {
        std::cout << "Warning: No templates available" << std::endl;
        return "";
    }

    std::uniform_int_distribution<int> dist(0, templates.size() - 1);
    std::string baseJS = templates[dist(generator)];

    switch (mutationType) {
        case 0: return applyASTMutation(baseJS);
        case 1: return applyCorpusMutation(baseJS);
        case 2: return applyGrammarMutation(baseJS);
        default: return baseJS;
    }
}

double JSFuzzer::executeJS(const std::string& jsCode) {
    // This is a timeout command. If there is a timeout, then give a good reward.
    std::string command = "timeout 5s node -e \"" + jsCode + "\" 2>&1";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cout << "Failed to execute command" << std::endl;
        return 0.0;
    }

    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    int status = pclose(pipe);

    if (status == 124) {  // My program timed out
        std::cout << "Execution timed out" << std::endl;
        return 1.5;
    }

    // Calculate reward based on execution result
    double reward = 0.0;
    
    if (result.find("Error") != std::string::npos) {
        reward += 1.0;
        if (result.find("TypeError") != std::string::npos) reward += 0.5;
        if (result.find("ReferenceError") != std::string::npos) reward += 0.5;
    }
    
    // Reward for successful execution
    if (status == 0) {
        reward += 0.5;
    }
    
    // Add some randomness to encourage exploration
    std::uniform_real_distribution<double> dist(0.0, 0.3);
    reward += dist(generator);

    std::cout << "Generated reward: " << reward << " for status: " << status << std::endl;
    return reward;
}
