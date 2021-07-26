#include <iostream>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <chrono>

#include "../include/cap_data.h"
#include "../include/cap.h"
#include "../include/heuristic.h"

using namespace std;

int main(int argc, char **argv)
{
    // PROBLEM PARAMETERS //
    if (argc < 7)
    {
        cerr << "Invalid number of arguments. Try: ./$EXECUTABLE $INSTANCE $SCENARIO $SETUP $SETUP_BEFORE $TIME_LIMIT $HEURISTIC" << endl;
        return 1;
    }

    const string instance = argv[1];
    const int scenario = atoi(argv[2]);
    const bool setup = atoi(argv[3]) == 0 ? false : true;
    const bool setup_before_class = atoi(argv[4]) == 0 ? false : true;

    const int time_limit = atoi(argv[5]); // In minutes. 0 skips solver, < 0 is no time limit.

    const bool heuristic = atoi(argv[6]) == 0 ? false : true;

    // PRE PROCESSING //
    auto timer_start_preprocessing = chrono::system_clock::now();

    CapData data = CapData(scenario, instance, setup, setup_before_class, heuristic);

    auto timer_end_preprocessing = chrono::system_clock::now();
    chrono::duration<double> timer_preprocessing = timer_end_preprocessing - timer_start_preprocessing;

    // HEURISTIC //
    HeuristicResults heuristic_results = {
        -1, // greedy value
        -1, // num of unfeasible assignments in greedy
        -1, // greedy time
        "" // chosen variables
    };
    if (heuristic)
    {
        Heuristic localsearch = Heuristic(data);
        heuristic_results = localsearch.Solve();
    }

    // SOLVER //
    Cap cap = Cap(data);
    CapResults results = cap.Solve(time_limit, heuristic_results.greedyValue);

    // EXPORT DATA //
    string variables = heuristic ? heuristic_results.variables : results.variables;
    nlohmann::json outjson = {
        {"instance", instance},
        {"scenario", scenario},
        {"setup", setup},
        {"setup before class", setup_before_class},
        {"time pre processing", timer_preprocessing.count()},
        {"time model", results.modelTime},
        {"time solver", results.solverTime},
        {"heuristic", heuristic},
        {"heuristic - time", heuristic_results.greedyTime},
        {"heuristic - value", heuristic_results.greedyValue},
        {"heuristic - num unfeasibilities", heuristic_results.numUnfeasible},
        {"status", results.status},
        {"value", results.objValue},
        {"gap", results.gap},
        {"results", variables}};

    ofstream outstream("results/" + data.get_instance_name() + ".json");
    outstream << std::setw(4) << outjson << std::endl;
    outstream.close();

    return 0;
}
