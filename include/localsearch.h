#include <map>
#include <vector>

#include "./cap_data.h"

#ifndef LOCALSEARCH_H
#define LOCALSEARCH_H

typedef struct heuristicResults
{
    float greedyValue;
    int numUnfeasible;
    double greedyTime;

    float localsearchValue;
    double localsearchTime;

    std::string variables;
} HeuristicResults;

typedef struct assignmentData
{
    int i;
    std::vector<int> twinlectures;
    int j;
    float cost;
} AssignmentData;

typedef struct swapData
{
    int i_1;
    int j_1;
    
    int i_2;
    int j_2;
    
    int k;
} SwapData;

typedef struct walkData
{
    int i;
    int k;
    int j_from;
    int j_to;
} WalkData;

typedef struct movementData
{
    enum move_type
    {
        Walk_t,
        Swap_t
    } type;
    union
    {
        WalkData walk;
        SwapData swap;
    };

    float cost;
    float penalty;
} MovementData;

class LocalSearch
{
private:
    CapData data;

    // X_ikj -- i in A, k in H_i, j in L_i
    std::map<int, std::map<int, std::map<int, bool>>> x;
    // L_i   -- i in A
    std::vector<std::vector<int>> location_contains_class;
    // H_i   -- i in A
    std::vector<std::vector<int>> lectures_of_class;
    // Holds lecture times that represents that lecture and all of its twins, for each class i
    std::vector<std::vector<int>> twin_sets_of_class;
    std::map<int, std::map<int, std::vector<int>>> twins_of_lecture;

    // Returns j in L of location assigned to lecture [i][k], or -1 if no assignment was made
    std::map<int, std::map<int, bool>> has_assigned_location;
    // Returns i in A of class assigned to location [j] at time [k], or -1 if no assignment was made
    std::map<int, std::map<int, bool>> has_assigned_class;

    int Greedy(float *greedy_cost);
    float FirstImprovement(float initial_cost, int num_unfeasibilities);
    float BestImprovement(float initial_cost, int num_unfeasibilities);
    
    std::vector<MovementData> Search();
    void UpdateSearch(std::vector<MovementData> *neighbourhood, MovementData move);

    std::pair<float, float> WalkCost(WalkData move);
    std::pair<float, float> SwapCost(SwapData move);

    void Walk(WalkData move);
    void Swap(SwapData move);

    float AssignCost(int i, std::vector<int> k_and_twins, int j);
    float UnassignCost(int i, std::vector<int> k_and_twins, int j);
    
    int ValidAssignment(int i, std::vector<int> k_and_twins, int j);
    
    bool AllAssigned();
    void Assign(int i, std::vector<int> k_and_twins, int j);

    std::string SolutionToString();

public:
    LocalSearch(const CapData &capdata);

    HeuristicResults Solve(bool first_improvement);
};

#endif