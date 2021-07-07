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
    if (argc < 5)
    {
        cerr << "Invalid number of arguments. Try: ./$EXECUTABLE $INSTANCE $SCENARIO $SETUP $SETUP_BEFORE" << endl;
        return 1;
    }

    const string instance = argv[1];
    const int scenario = atoi(argv[2]);
    const bool setup = atoi(argv[3]) == 0 ? false : true;
    const bool setup_before_class = atoi(argv[4]) == 0 ? false : true;
    const bool heuristic = false;

    const int time_limit = 0; // In minutes. 0 skips solver, < 0 is no time limit.

    // PRE PROCESSING //
    auto timer_start_preprocessing = chrono::system_clock::now();

    CapData data = CapData(scenario, instance, setup, setup_before_class, heuristic);

    auto timer_end_preprocessing = chrono::system_clock::now();
    chrono::duration<double> timer_preprocessing = timer_end_preprocessing - timer_start_preprocessing;

    // HEURISTIC //
    double timer_heuristic_count = 0;
    double heuristic_obj_value = -1;
    if (heuristic)
    {
        auto timer_start_heuristic = chrono::system_clock::now();

        // TODO
        LocalSearch localsearch = LocalSearch(data);
        heuristic_obj_value = localsearch.Solve();

        auto timer_end_heuristic = chrono::system_clock::now();
        chrono::duration<double> timer_heuristic = timer_end_heuristic - timer_start_heuristic;
        timer_heuristic_count = timer_heuristic.count();
    }

    // SOLVER //
    Cap cap = Cap(data);
    CapResults results = cap.Solve(time_limit, heuristic_obj_value);

    // EXPORT DATA //
    nlohmann::json outjson = {
        {"instance", instance},
        {"scenario", scenario},
        {"setup", setup},
        {"setup before class", setup_before_class},
        {"heuristic", heuristic},
        {"time pre processing", timer_preprocessing.count()},
        {"time heuristic", timer_heuristic_count},
        {"time model", results.modelTime},
        {"time solver", results.solverTime},
        {"value heuristic", heuristic_obj_value},
        {"status", results.status},
        {"value", results.objValue},
        {"gap", results.gap},
        {"results", results.variables}};

    ofstream outstream("results/" + data.get_instance_name() + ".json");
    outstream << std::setw(4) << outjson << std::endl;
    outstream.close();

    return 0;
}
