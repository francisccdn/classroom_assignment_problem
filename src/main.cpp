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
    const string instance = "toy";
    const int scenario = 10;
    const bool setup = false;
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
    auto timer_start_solver = chrono::system_clock::now();

    Cap cap = Cap(data);
    CapResults results = cap.Solve(heuristic_obj_value);

    auto timer_end_solver = chrono::system_clock::now();
    chrono::duration<double> timer_solver = timer_end_solver - timer_start_solver;

    // EXPORT DATA //
    nlohmann::json outjson = {
        {"periodo", instance},
        {"cenario", scenario},
        {"setup", setup},
        {"setup antes da aula", setup_before_class},
        {"gulosa", heuristic},
        {"tempo pre processamento", timer_preprocessing.count()},
        {"tempo heuristica", timer_heuristic_count},
        {"tempo resolvedor", timer_solver.count()},
        {"valor heuristica", heuristic_obj_value},
        {"status", results.status},
        {"valor", results.objValue},
        {"resultado", results.variables}};

    ofstream outstream("results/" + data.get_instance_name() + ".json");
    outstream << std::setw(4) << outjson << std::endl;

    return 0;
}
