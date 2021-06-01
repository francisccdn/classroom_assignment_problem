#include <ilcplex/ilocplex.h>
#include <iostream>
#include <chrono>

#include "../include/cap.h"
#include "../include/cap_data.h"
#include "../include/utility.h"

#ifndef CAP_CPP
#define CAP_CPP

using namespace std;

Cap::Cap(const CapData &capdata) : data(capdata) {}

int Cap::i_in_A(int i, int is_computer)
{
    return is_computer ? i + data.get_classes_classroom() : i;
}

CapResults Cap::Solve(int time_limit_min, double upper_bound)
{
    IloEnv env;
    IloModel model(env);
    char name[128];

    /*
    * (SUB)SETS
    */

    // A
    int num_classes = data.get_classes();
    // B
    int num_classes_classroom = data.get_classes_classroom();
    // C
    int num_classes_computer = data.get_classes_computer();
    // B_k  -- k in H
    std::vector<std::vector<int>> classes_classroom_per_timeslot = data.get_classes_classroom_per_timeslot();
    // C_k  -- k in H
    std::vector<std::vector<int>> classes_computer_per_timeslot = data.get_classes_computer_per_timeslot();
    // L
    int num_locations = data.get_locations();
    // S
    int num_locations_classroom = data.get_locations_classroom();
    // H
    int num_timeslots = data.get_num_timeslots();
    // H_i  -- i in A
    std::vector<std::vector<int>> lectures_of_class = data.get_lectures_of_class();
    // S_i  -- i in B
    std::vector<std::vector<int>> location_contains_class_classroom = data.get_location_contains_class_classroom();
    // I_i  -- i in C
    std::vector<std::vector<int>> location_contains_class_computer = data.get_location_contains_class_computer();
    // E
    int num_itc_groups = data.get_num_itc_groups();
    // D_l  -- l in E
    std::vector<std::vector<int>> classes_classroom_of_itc_group = data.get_classes_classroom_of_itc_group();

    /*
    * INPUT DATA
    */

    // c_j  -- j in L
    vector<float> location_cost = data.get_location_cost();
    // q_i  -- i in A
    vector<int> num_students_in_class = data.get_num_students_in_class();
    // m_j  -- j in I
    map<int, float> location_computer_cost = data.get_location_computer_cost();
    // s_j  -- j in L
    vector<float> location_setup_cost = data.get_location_setup_cost();
    // d_j  -- j in L
    vector<float> location_setup_duration = data.get_location_setup_duration();

    /*
    * VARIABLES
    */

    auto timer_start_model = chrono::system_clock::now();

    // X_ikj  -- i in B, k in H_i, j in S_i
    map<int, map<int, map<int, IloBoolVar>>> x;
    for (int i = 0; i < num_classes_classroom; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, false)])
        {
            for (int j : location_contains_class_classroom[i])
            {
                sprintf(name, "X_%s_%d_%s", data.get_class_name(i_in_A(i, false)).c_str(), k, data.get_location_name(j).c_str());
                x[i][k][j] = IloBoolVar(env, name);
                model.add(x[i][k][j]);

                if (!data.ValidVar(false, i_in_A(i, false), k, j))
                {
                    IloRange constraint = (x[i][k][j] <= 0);
                    sprintf(name, "InvalidVar(%s,%d,%s)", data.get_class_name(i_in_A(i, false)).c_str(),
                            k, data.get_location_name(j).c_str());
                    constraint.setName(name);

                    model.add(constraint);
                }
            }
        }
    }

    // T_ikj  -- i in C, k in H_i, j in I_i
    map<int, map<int, map<int, IloBoolVar>>> t;
    for (int i = 0; i < num_classes_computer; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, true)])
        {
            for (int j : location_contains_class_computer[i])
            {
                sprintf(name, "T_%s_%d_%s", data.get_class_name(i_in_A(i, true)).c_str(), k, data.get_location_name(j).c_str());
                t[i][k][j] = IloBoolVar(env, name);
                model.add(t[i][k][j]);

                if (!data.ValidVar(true, i_in_A(i, true), k, j))
                {
                    IloRange constraint = (t[i][k][j] <= 0);
                    sprintf(name, "InvalidVar(%s,%d,%s)", data.get_class_name(i_in_A(i, true)).c_str(),
                            k, data.get_location_name(j).c_str());
                    constraint.setName(name);

                    model.add(constraint);
                }
            }
        }
    }

    // Z_ikj  -- i in A, k in H_i, j in L_i
    map<int, map<int, map<int, IloBoolVar>>> z;
    if (data.is_setup())
    {
        for (int i = 0; i < num_classes_classroom; i++)
        {
            for (int k : lectures_of_class[i_in_A(i, false)])
            {
                for (int j : location_contains_class_classroom[i])
                {
                    if (x[i][k].count(j) == 0)
                        continue;

                    sprintf(name, "Z_%s_%d_%s", data.get_class_name(i_in_A(i, false)).c_str(), k, data.get_location_name(j).c_str());
                    z[i][k][j] = IloBoolVar(env, name);
                    model.add(z[i][k][j]);
                }
            }
        }
        for (int i = 0; i < num_classes_computer; i++)
        {
            for (int k : lectures_of_class[i_in_A(i, true)])
            {
                for (int j : location_contains_class_computer[i])
                {
                    if (t[i][k].count(j) == 0)
                        continue;

                    sprintf(name, "Z_%s_%d_%s", data.get_class_name(i_in_A(i, true)).c_str(), k, data.get_location_name(j).c_str());
                    z[i][k][j] = IloBoolVar(env, name);
                    model.add(z[i][k][j]);
                }
            }
        }
    }

    // Y_ij  -- i in B, j in S_i
    map<int, map<int, IloBoolVar>> y;
    for (int i = 0; i < num_classes_classroom; i++)
    {
        for (int j : location_contains_class_classroom[i])
        {
            for (int k : lectures_of_class[i_in_A(i, false)])
            {
                if (x[i][k].count(j) == 0)
                    continue;

                sprintf(name, "Y_%s_%s", data.get_class_name(i_in_A(i, false)).c_str(), data.get_location_name(j).c_str());
                y[i][j] = IloBoolVar(env, name);
                model.add(y[i][j]);

                break;
            }
        }
    }

    // U_ij  -- i in C, j in I_i
    map<int, map<int, IloBoolVar>> u;
    for (int i = 0; i < num_classes_computer; i++)
    {
        for (int j : location_contains_class_computer[i])
        {
            for (int k : lectures_of_class[i_in_A(i, true)])
            {
                if (t[i][k].count(j) == 0)
                    continue;

                sprintf(name, "U_%s_%s", data.get_class_name(i_in_A(i, true)).c_str(), data.get_location_name(j).c_str());
                u[i][j] = IloBoolVar(env, name);
                model.add(u[i][j]);

                break;
            }
        }
    }

    // W_lj  -- l in E, j in S
    map<int, map<int, IloBoolVar>> w;
    for (int l = 0; l < num_itc_groups; l++)
    {
        for (int i : classes_classroom_of_itc_group[l])
        {
            for (int j : location_contains_class_classroom[i])
            {
                for (int k : lectures_of_class[i])
                {
                    if (x[i][k].count(j) == 0)
                        continue;

                    sprintf(name, "W_%d_%s", data.get_itc_group_id(l), data.get_location_name(j).c_str());
                    w[l][j] = IloBoolVar(env, name);
                    model.add(w[l][j]);

                    break;
                }
            }
        }
    }

    /*
    * OBJETIVE FUNCTION
    */

    IloExpr of_sum(env);

    // sum_i_in_B sum_k_in_H_i sum_j_in_S_i (c_j * x_ikj)
    for (int i = 0; i < num_classes_classroom; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, false)])
        {
            for (int j : location_contains_class_classroom[i])
            {
                if (x[i][k].count(j) == 0)
                    continue;

                of_sum += location_cost[j] * x[i][k][j];
            }
        }
    }

    // sum_i_in_C sum_k_in_H_i sum_j_in_I_i ((c_j + q_i m_j)* t_ikj)
    for (int i = 0; i < num_classes_computer; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, true)])
        {
            for (int j : location_contains_class_computer[i])
            {
                if (t[i][k].count(j) == 0)
                    continue;

                of_sum += (location_cost[j] + (num_students_in_class[i_in_A(i, true)] * location_computer_cost[j])) * t[i][k][j];
            }
        }
    }

    // sum_i_in_A sum_k_in_H_i sum_j_in_L_i (s_j * z_ikj)
    if (data.is_setup())
    {
        for (int i = 0; i < num_classes_classroom; i++)
        {
            for (int k : lectures_of_class[i_in_A(i, false)])
            {
                for (int j : location_contains_class_classroom[i])
                {
                    if (z[i][k].count(j) == 0)
                        continue;

                    if (data.is_setup_before_class())
                        of_sum += location_setup_cost[j] * z[i][k][j];
                    else
                        of_sum += (location_setup_cost[j] - (location_cost[j] * location_setup_duration[j])) * z[i][k][j];
                }
            }
        }
        for (int i = 0; i < num_classes_computer; i++)
        {
            for (int k : lectures_of_class[i_in_A(i, true)])
            {
                for (int j : location_contains_class_computer[i])
                {
                    if (z[i][k].count(j) == 0)
                        continue;

                    if (data.is_setup_before_class())
                        of_sum += location_setup_cost[j] * z[i][k][j];
                    else
                        of_sum += (location_setup_cost[j] - (location_cost[j] * location_setup_duration[j])) * z[i][k][j];
                }
            }
        }
    }

    model.add(IloMinimize(env, of_sum));

    /*
    * CONSTRAINTS
    */

    // Basic constraints 1 (BC1): each lecture must be assigned to exactly one lecture location

    for (int i = 0; i < num_classes_classroom; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, false)])
        {
            IloExpr sum_bc1_x(env);
            bool sum_is_empty = true;

            for (int j : location_contains_class_classroom[i])
            {
                if (x[i][k].count(j) == 0)
                    continue;

                sum_bc1_x += x[i][k][j];
                sum_is_empty = false;
            }

            if (!sum_is_empty)
            {
                IloRange constraint = (sum_bc1_x - 1 == 0);
                sprintf(name, "BC1(%s,%d)", data.get_class_name(i_in_A(i, false)).c_str(), k);
                constraint.setName(name);

                model.add(constraint);
            }
        }
    }

    for (int i = 0; i < num_classes_computer; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, true)])
        {
            IloExpr sum_bc1_t(env);
            bool sum_is_empty = true;

            for (int j : location_contains_class_computer[i])
            {
                if (t[i][k].count(j) == 0)
                    continue;

                sum_bc1_t += t[i][k][j];
                sum_is_empty = false;
            }

            if (!sum_is_empty)
            {
                IloRange constraint = (sum_bc1_t - 1 == 0);
                sprintf(name, "BC1(%s,%d)", data.get_class_name(i_in_A(i, true)).c_str(), k);
                constraint.setName(name);

                model.add(constraint);
            }
        }
    }

    // Basic constraints 2 (BC2): each location must have at most one lecture assigned per lecture time slot

    for (int j = 0; j < num_locations; j++)
    {
        for (int k = 0; k < num_timeslots; k++)
        {
            int sum_size = 0;

            IloExpr sum_bc2_x(env);

            for (int i : classes_classroom_per_timeslot[k])
            {
                if (x[i][k].count(j) == 0)
                    continue;

                sum_bc2_x += x[i][k][j];
                sum_size++;
            }

            IloExpr sum_bc2_t(env);

            for (int i : classes_computer_per_timeslot[k])
            {
                if (t[i][k].count(j) == 0)
                    continue;

                sum_bc2_t += t[i][k][j];
                sum_size++;
            }

            if (sum_size > 1)
            {
                IloRange constraint = (sum_bc2_x + sum_bc2_t - 1 <= 0);
                sprintf(name, "BC2(%s,%d)", data.get_location_name(j).c_str(), k);
                constraint.setName(name);

                model.add(constraint);
            }
        }
    }

    // Institutional constraints 1 (IC1): lectures from a class must be assigned to the same location
    if (data.IC1())
    {
        for (int i = 0; i < num_classes_classroom; i++)
        {
            for (int k : lectures_of_class[i_in_A(i, false)])
            {
                for (int j : location_contains_class_classroom[i])
                {
                    if (x[i][k].count(j) == 0 || y[i].count(j) == 0)
                        continue;

                    IloRange constraint = (x[i][k][j] - y[i][j] == 0);
                    sprintf(name, "IC1(%s,%d,%s)", data.get_class_name(i_in_A(i, false)).c_str(), k, data.get_location_name(j).c_str());
                    constraint.setName(name);

                    model.add(constraint);
                }
            }
        }

        for (int i = 0; i < num_classes_computer; i++)
        {
            for (int k : lectures_of_class[i_in_A(i, true)])
            {
                for (int j : location_contains_class_computer[i])
                {
                    if (t[i][k].count(j) == 0 || u[i].count(j) == 0)
                        continue;

                    IloRange constraint = (t[i][k][j] - u[i][j] == 0);
                    sprintf(name, "IC1(%s,%d,%s)", data.get_class_name(i_in_A(i, true)).c_str(), k, data.get_location_name(j).c_str());
                    constraint.setName(name);

                    model.add(constraint);
                }
            }
        }
    }

    // Institutional constraints 2 (IC2): lectures from same ITC group requiring only classrooms must be assigned to the same location
    if (data.IC2())
    {
        for (int l = 0; l < num_itc_groups; l++)
        {
            for (int i : classes_classroom_of_itc_group[l])
            {
                for (int k : lectures_of_class[i])
                {
                    for (int j : location_contains_class_classroom[i])
                    {
                        if (x[i][k].count(j) == 0 || w[l].count(j) == 0)
                            continue;

                        IloRange constraint = (x[i][k][j] - w[l][j] == 0);
                        sprintf(name, "IC2(%d,%s,%d,%s)", data.get_itc_group_id(l), data.get_class_name(i_in_A(i, false)).c_str(),
                                k, data.get_location_name(j).c_str());
                        constraint.setName(name);

                        model.add(constraint);
                    }
                }
            }
        }
    }

    // Setup cost 1: forces Z to 1 if there are no lecture in preceding time slot in the same room, for X
    if (data.is_setup())
    {
        for (int i = 0; i < num_classes_classroom; i++)
        {
            for (int k : lectures_of_class[i_in_A(i, false)])
            {
                for (int j : location_contains_class_classroom[i])
                {
                    if (x[i][k].count(j) == 0 || z[i][k].count(j) == 0)
                        continue;

                    IloExpr sum_zc1_x(env);

                    for (int i1 : classes_classroom_per_timeslot[k - 1])
                    {
                        if (x[i1][k - 1].count(j) == 0)
                            continue;

                        sum_zc1_x += x[i1][k - 1][j];
                    }

                    IloRange constraint = (sum_zc1_x + z[i][k][j] - x[i][k][j] >= 0);
                    sprintf(name, "ZC1(%s,%d,%s)", data.get_class_name(i_in_A(i, false)).c_str(), k, data.get_location_name(j).c_str());
                    constraint.setName(name);

                    model.add(constraint);
                }
            }
        }
    }

    // Setup cost 2: forces Z to 1 if there are no lecture in preceding time slot in the same room, for T
    if (data.is_setup())
    {
        for (int i = 0; i < num_classes_computer; i++)
        {
            for (int k : lectures_of_class[i_in_A(i, true)])
            {
                for (int j : location_contains_class_computer[i])
                {
                    if (t[i][k].count(j) == 0 || z[i][k].count(j) == 0)
                        continue;

                    IloExpr sum_zc2_t(env);

                    for (int i1 : classes_computer_per_timeslot[k - 1])
                    {
                        if (t[i1][k - 1].count(j) == 0)
                            continue;

                        sum_zc2_t += t[i1][k - 1][j];
                    }

                    IloRange constraint = (sum_zc2_t + z[i][k][j] - t[i][k][j] >= 0);
                    sprintf(name, "ZC2(%s,%d,%s)", data.get_class_name(i_in_A(i, true)).c_str(), k, data.get_location_name(j).c_str());
                    constraint.setName(name);

                    model.add(constraint);
                }
            }
        }
    }

    auto timer_end_model = chrono::system_clock::now();
    chrono::duration<double> timer_model = timer_end_model - timer_start_model;

    /*
    * SOLVE
    */

    IloCplex cplex(model);
    // Gap tolerance
    cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-06);
    // Time limit
    if (time_limit_min > 0)
        cplex.setParam(IloCplex::Param::TimeLimit, time_limit_min * 60);
    // Set upper bound
    if (upper_bound >= 0)
        cplex.setParam(IloCplex::Param::MIP::Tolerances::UpperCutoff, upper_bound);
    // Export LP
    string lp_name = "lp/" + data.get_instance_name() + ".lp";
    cplex.exportModel(lp_name.c_str());
    // Timer
    chrono::duration<double> timer_solver;

    try
    {
        auto timer_start_solver = chrono::system_clock::now();

        cplex.solve();

        auto timer_end_solver = chrono::system_clock::now();
        timer_solver = timer_end_solver - timer_start_solver;
    }
    catch (IloException &e)
    {
        std::cerr << e << '\n';
    }

    cout << "status: " << cplex.getStatus() << endl;
    cout << "total value: " << cplex.getObjValue() << endl;

    /*
    * EXPORT
    */

    const double eps = 0.1;

    CapResults results;
    results.objValue = cplex.getObjValue();
    results.gap = cplex.getMIPRelativeGap();
    results.status = cplex.getStatus();
    results.variables = "";
    results.solverTime = timer_solver.count();
    results.modelTime = timer_model.count();

    for (int i = 0; i < num_classes_classroom; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, false)])
        {
            for (int j : location_contains_class_classroom[i])
            {
                if (x[i][k].count(j) != 0 && cplex.getValue(x[i][k][j]) >= 1 - eps)
                {
                    results.variables += x[i][k][j].getName();
                    results.variables += " ";
                }

                if (z[i][k].count(j) != 0 && cplex.getValue(z[i][k][j]) >= 1 - eps)
                {
                    results.variables += z[i][k][j].getName();
                    results.variables += " ";
                }
            }
        }
    }

    for (int i = 0; i < num_classes_computer; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, true)])
        {
            for (int j : location_contains_class_computer[i])
            {
                if (t[i][k].count(j) != 0 && cplex.getValue(t[i][k][j]) >= 1 - eps)
                {
                    results.variables += t[i][k][j].getName();
                    results.variables += " ";
                }

                if (z[i][k].count(j) != 0 && cplex.getValue(z[i][k][j]) >= 1 - eps)
                {
                    results.variables += z[i][k][j].getName();
                    results.variables += " ";
                }
            }
        }
    }

    env.end();

    return results;
}

#endif