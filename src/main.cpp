#include <iostream>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <chrono>

#include "../include/cap_data.h"
#include "../include/cap.h"

using namespace std;

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "Invalid number of arguments. Try: ./$EXECUTABLE $INSTANCE $SCENARIO" << endl;
        return 1;
    }

    const string instance = argv[1];
    const int scenario = atoi(argv[2]);
    const bool setup = true;
    const bool setup_before_class = false;
    const bool heuristic = false;

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
        //heuristic_obj_value =

        auto timer_end_heuristic = chrono::system_clock::now();
        chrono::duration<double> timer_heuristic = timer_end_heuristic - timer_start_heuristic;
        timer_heuristic_count = timer_heuristic.count();
    }

    // SOLVER //
    Cap cap = Cap(data);
    CapResults results = cap.Solve(heuristic_obj_value);

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
        {"results", results.variables}};

    ofstream outstream("results/" + data.get_instance_name() + ".json");
    outstream << std::setw(4) << outjson << std::endl;
    outstream.close();

    return 0;
}
