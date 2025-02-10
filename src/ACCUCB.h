#ifndef ACCUCB_H
#define ACCUCB_H

#include <vector>
#include <map>
#include <memory>
#include "Node.h"

// 用于描述基臂(Base Arm)及其上下文信息
struct BaseArm {
    int armID;                // 基臂编号
    std::vector<double> context;  // 上下文(简化为一个向量)
    double trueMean;          // 真实期望奖励(用于模拟)
    double lastReward;        // 最近一次观测到的奖励
};

// ACC-UCB 算法整体管理类
class ACCUCB {
public:
    // 算法主要参数
    int    T;       // 总轮数
    int    K;       // 每轮选 K 个基臂
    double v1;
    double v2;
    double rho;
    int    Nchild;  // refine 时每个叶节点生成子节点数

    // 维护的树根和当前所有"活动叶"集合
    std::unique_ptr<Node> root;
    std::vector<Node*> activeLeaves;

    // 构造函数
    ACCUCB(int _T, int _K, double _v1, double _v2, double _rho, int _Nchild);

    // 计算给定节点的索引值 g^t(node)
    double computeNodeIndex(Node* node);

    // 找到基臂(上下文)所属的叶节点 (简单示例, 随机返回一个活跃叶)
    Node* matchLeaf(const BaseArm &arm);

    // 近似 Oracle：将臂按索引值从大到小排序，选前K个
    std::vector<int> approximateOracle(const std::vector<double> &indices);

    // 更新节点的统计量 \hat{\mu}^t 和 C^t
    void updateNodes(const std::vector<BaseArm> &chosenArms,
                     std::map<Node*, int> &countMap,
                     std::map<Node*, double> &rewardMap);

    // 判断是否需要 refine；若 c^t <= v1*rho^h，则分裂
    void refineCheck();

    // 主流程：循环 T 轮
    void run();

private:
    // 生成基臂(用于示例)
    std::vector<BaseArm> generateBaseArms(int M, int t);

    // 随机产生奖励(此处用伯努利分布)
    double genReward(double mean);

    // 返回 [low, high) 范围内的随机实数
    double uniformReal(double low, double high);
};

#endif // ACCUCB_H
