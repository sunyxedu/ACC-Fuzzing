//
// Created by Hemlix on 2025/1/25.
//

#include "acc_ucb.h"

int main() {
    int K = 3;
    int T = 100;

    ACC_UCB acc_ucb(K, T);
    
    acc_ucb.jsFuzzer.addTemplate("function foo(x) { if(x > 0) return x; else return -x; }");
    acc_ucb.jsFuzzer.addTemplate("for(let i=0; i<10; i++) { console.log(i*i); }");
    acc_ucb.jsFuzzer.addTemplate("let arr = [1,2,3]; arr.forEach(x => console.log(x));");
    
    acc_ucb.run();

    return 0;
}
