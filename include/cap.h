#include <string>

#include "./cap_data.h"

#ifndef CAP_H
#define CAP_H

typedef struct capResults
{
    double objValue;
    int status;
    std::string variables;
    double solverTime;
    double modelTime;
} CapResults;

class Cap
{
private:
    CapData data;

    int i_in_A(int i, int is_computer);
    int j_in_L(int j, int is_computer);

public:
    Cap(const CapData &capdata);

    CapResults Solve(double upper_bound);
};

#endif