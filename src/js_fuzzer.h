//
// Created by Hemlix on 2025/1/25.
//

#ifndef JS_FUZZER_H
#define JS_FUZZER_H

#include <string>
#include <random>
#include <vector>

class JSFuzzer {
private:
    std::default_random_engine generator;
    std::vector<std::string> mutationOperators;

    std::string applyASTMutation(const std::string& input);
    std::string applyCorpusMutation(const std::string& input);
    std::string applyGrammarMutation(const std::string& input);

public:
    JSFuzzer();
    void addTemplate(const std::string& templ);
    std::vector<std::string> templates;
    std::string generateFuzzedJS(int mutationType);
    double executeJS(const std::string& jsCode);
};

#endif
