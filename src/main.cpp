#include <iostream>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <chrono>

#include "../include/cap_data.h"
#include "../include/cap.h"
#include "../include/localsearch.h"

using namespace std;

int main(int argc, char **argv)
{
    // PROBLEM PARAMETERS //
    if (argc < 6)
    {
        cerr << "Invalid number of arguments. Try: ./$EXECUTABLE $INSTANCE $SCENARIO $SETUP $SETUP_BEFORE $TIME_LIMIT" << endl;
        return 1;
    }

    const string instance = argv[1];
    const int scenario = atoi(argv[2]);
    const bool setup = atoi(argv[3]) == 0 ? false : true;
    const bool setup_before_class = atoi(argv[4]) == 0 ? false : true;

    const int time_limit = atoi(argv[5]); // In minutes. 0 skips solver, < 0 is no time limit.

    const bool heuristic = argc > 6;
    const bool first_improvement = (heuristic && atoi(argv[6]) == 0) ? false : true;

    // PRE PROCESSING //
    auto timer_start_preprocessing = chrono::system_clock::now();

    CapData data = CapData(scenario, instance, setup, setup_before_class, heuristic, first_improvement);

    auto timer_end_preprocessing = chrono::system_clock::now();
    chrono::duration<double> timer_preprocessing = timer_end_preprocessing - timer_start_preprocessing;

    // HEURISTIC //
    HeuristicResults heuristic_results = {
        0, // greedy value
        0, // num of unfeasible assignments in greedy
        0, // greedy time
        0, // local search value
        0, // local search time
        "" // chosen variables
    };
    if (heuristic)
    {
        LocalSearch localsearch = LocalSearch(data);
        heuristic_results = localsearch.Solve(first_improvement);
    }

    // SOLVER //
    Cap cap = Cap(data);
    CapResults results = cap.Solve(time_limit, heuristic_results.localsearchValue);

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
        {"heuristic - local search - time", heuristic_results.localsearchTime},
        {"heuristic - local search - value", heuristic_results.localsearchValue},
        {"heuristic - greedy - time", heuristic_results.greedyTime},
        {"heuristic - greedy - value", heuristic_results.greedyValue},
        {"heuristic - greedy - num unfeasibilities", heuristic_results.numUnfeasible},
        {"status", results.status},
        {"value", results.objValue},
        {"gap", results.gap},
        {"results", variables}};

    ofstream outstream("results/" + data.get_instance_name() + ".json");
    outstream << std::setw(4) << outjson << std::endl;
    outstream.close();

    return 0;
}
