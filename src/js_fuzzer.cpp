//
// Created by Hemlix on 2025/1/25.
//

#include "js_fuzzer.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <regex>

JSFuzzer::JSFuzzer() {
    generator.seed(time(0));
    
    templates = {
            "let x = {}; for (let i = 0; i < 100; i++) x[i] = i; console.log(x);",  // to test objects and loops
            "function test() { return Math.random() * 100; } console.log(test());", // to test functions and random numbers
            "try { let a = new Array(1000000).fill(1); } catch (e) { console.log(e); }"  // to test exception handling and large arrays
    };
}
void JSFuzzer::addTemplate(const std::string& templ) { templates.push_back(templ); }
std::string JSFuzzer::applyASTMutation(const std::string& input) {
    std::vector<std::string> nodes = {
        "if (COND) { STMT }",
        "while (COND) { STMT }",
        "try { STMT } catch(e) { }",
        "function NAME() { STMT }",
        "for (let i = 0; i < N; i++) { STMT }"
    };

    std::uniform_int_distribution<int> dist(0, nodes.size() - 1);
    std::string node = nodes[dist(generator)];
    
    // Replace placeholders
    node = std::regex_replace(node, std::regex("STMT"), input);
    node = std::regex_replace(node, std::regex("COND"), "Math.random() > 0.5");
    node = std::regex_replace(node, std::regex("NAME"), "fn" + std::to_string(dist(generator)));
    node = std::regex_replace(node, std::regex("N"), std::to_string(dist(generator) + 1));
    
    return node;
}

std::string JSFuzzer::applyCorpusMutation(const std::string& input) {
    if (corpus.empty()) {
        corpus = {
            "Object.prototype.toString.call(undefined)",
            "Array.prototype.slice.call(arguments)",
            "Object.create(null)",
            "new Proxy({}, {})",
            "Object.defineProperty({}, 'x', {get: () => {}})"
        };
    }
    
    std::uniform_int_distribution<int> dist(0, corpus.size() - 1);
    std::string selected = corpus[dist(generator)];
    
    return "try { " + input + "\n" + selected + "; } catch(e) { console.log(e); }";
}
std::string JSFuzzer::applyGrammarMutation(const std::string& input) {
    std::string output = input;

    std::uniform_int_distribution<int> dist(0, 2);
    int choice = dist(generator);

    switch(choice) {
        case 0: {
            // Replace one '+' with one random operator
            std::uniform_int_distribution<int> opDist(0, operators.size() - 1);
            std::string op = operators[opDist(generator)];

            // Replace only the first occurrence of '+'
            output = std::regex_replace(
                    output,
                    std::regex("\\+"),
                    op,
                    std::regex_constants::format_first_only
            );
            break;
        }
        case 1: {
            std::uniform_int_distribution<int> compDist(0, comparators.size() - 1);
            std::string comp = comparators[compDist(generator)];

            // Replace only the first occurrence of '=='
            output = std::regex_replace(
                    output,
                    std::regex("=="),
                    comp,
                    std::regex_constants::format_first_only
            );
            break;
        }
        case 2: {
            std::vector<std::string> types = {"Number", "String", "Boolean"};
            std::uniform_int_distribution<int> typeDist(0, types.size() - 1);
            std::string chosenType = types[typeDist(generator)];

            output = chosenType + "(" + output + ")";
            break;
        }
    }
    return output;
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

    // This do happen !!! Sometimes, it will change ++ to %+ ... So No reward for this !!!!!
    /*
    if (status == 256) {
        std::cout << "Code execution failed." << std::endl;
        std::cout << jsCode << std::endl;
        return 0;
    }
     */

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
