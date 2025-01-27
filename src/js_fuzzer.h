//
// Created by Hemlix on 2025/1/25.
//

#ifndef JS_FUZZER_H
#define JS_FUZZER_H

#include <string>
#include <random>
#include <vector>
#include <set>

class JSFuzzer {
private:
    std::default_random_engine generator;
    
    // AST mutation helpers
    std::string insertStatement(const std::string& input);
    std::string deleteStatement(const std::string& input);
    std::string modifyCondition(const std::string& input);
    
    // Corpus mutation helpers
    std::vector<std::string> corpus;  // Store known interesting test cases
    
    // Grammar mutation helpers
    std::vector<std::string> operators = {"+", "-", "*", "/", "%"};
    std::vector<std::string> comparators = {"<", ">", "<=", ">=", "==", "==="};
    
    std::string applyASTMutation(const std::string& input);    // 修改AST结构
    std::string applyCorpusMutation(const std::string& input); // 使用已知的测试用例
    std::string applyGrammarMutation(const std::string& input);// 修改语法元素

public:
    JSFuzzer();
    void addTemplate(const std::string& templ);
    std::vector<std::string> templates;
    std::string generateFuzzedJS(int mutationType);
    double executeJS(const std::string& jsCode);
};

#endif
