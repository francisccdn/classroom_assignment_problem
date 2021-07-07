#include <map>
#include <vector>

#include "./cap_data.h"

#ifndef LOCALSEARCH_H
#define LOCALSEARCH_H

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

public:
    LocalSearch(const CapData &capdata);

    double Solve();
};

#endif