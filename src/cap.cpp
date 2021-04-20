#include <ilcplex/ilocplex.h>
#include <iostream>

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

CapResults Cap::Solve(double upper_bound)
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
    vector<float> location_computer_cost = data.get_location_cost();
    // s_j  -- j in L
    vector<float> location_setup_cost = data.get_location_setup_cost();
    // d_j  -- j in L
    vector<float> location_setup_duration = data.get_location_setup_duration();

    /*
    * VARIABLES
    */

    // X_ikj  -- i in B, k in H_i, j in S_i
    IloBoolVar *x[num_classes_classroom][num_timeslots][num_locations_classroom];
    for (int i = 0; i < num_classes_classroom; i++)
    {
        for (int k = 0; k < num_timeslots; k++)
        {
            for (int j = 0; j < num_locations_classroom; j++)
            {
                x[i][k][j] = NULL;
            }
        }
    }

    for (int i = 0; i < num_classes_classroom; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, false)])
        {
            for (int j : location_contains_class_classroom[i])
            {
                if (data.ValidVar(false, i_in_A(i, false), k, j))
                {
                    sprintf(name, "X_%s_%d_%s", data.get_class_name(i_in_A(i, false)).c_str(), k, data.get_location_name(j).c_str());
                    x[i][k][j] = new IloBoolVar(env, name);
                    model.add(*x[i][k][j]);
                }
            }
        }
    }

    // T_ikj  -- i in C, k in H_i, j in I_i
    IloBoolVar *t[num_classes_computer][num_timeslots][num_locations];
    for (int i = 0; i < num_classes_computer; i++)
    {
        for (int k = 0; k < num_timeslots; k++)
        {
            for (int j = 0; j < num_locations; j++)
            {
                t[i][k][j] = NULL;
            }
        }
    }

    for (int i = 0; i < num_classes_computer; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, true)])
        {
            for (int j : location_contains_class_computer[i])
            {
                if (data.ValidVar(true, i_in_A(i, true), k, j))
                {
                    sprintf(name, "T_%s_%d_%s", data.get_class_name(i_in_A(i, true)).c_str(), k, data.get_location_name(j).c_str());
                    t[i][k][j] = new IloBoolVar(env, name);
                    model.add(*t[i][k][j]);
                }
            }
        }
    }

    // Y_ij  -- i in B, j in S_i
    IloBoolVar *y[num_classes_classroom][num_locations_classroom];
    for (int i = 0; i < num_classes_classroom; i++)
    {
        for (int j = 0; j < num_locations_classroom; j++)
        {
            y[i][j] = NULL;
        }
    }

    for (int i = 0; i < num_classes_classroom; i++)
    {
        for (int j : location_contains_class_classroom[i])
        {
            sprintf(name, "Y_%s_%s", data.get_class_name(i_in_A(i, false)).c_str(), data.get_location_name(j).c_str());
            y[i][j] = new IloBoolVar(env, name);
            model.add(*y[i][j]);
        }
    }

    // U_ij  -- i in C, j in I_i
    IloBoolVar *u[num_classes_computer][num_locations];
    for (int i = 0; i < num_classes_computer; i++)
    {
        for (int j = 0; j < num_locations; j++)
        {
            u[i][j] = NULL;
        }
    }

    for (int i = 0; i < num_classes_computer; i++)
    {
        for (int j : location_contains_class_computer[i])
        {
            sprintf(name, "U_%s_%s", data.get_class_name(i_in_A(i, true)).c_str(), data.get_location_name(j).c_str());
            u[i][j] = new IloBoolVar(env, name);
            model.add(*u[i][j]);
        }
    }

    // W_lj  -- l in E, j in S
    IloBoolVar *w[num_itc_groups][num_locations_classroom];
    for (int l = 0; l < num_itc_groups; l++)
    {
        for (int j = 0; j < num_locations_classroom; j++)
        {
            sprintf(name, "W_%d_%s", data.get_itc_group_id(l), data.get_location_name(j).c_str());
            w[l][j] = new IloBoolVar(env, name);
            model.add(*w[l][j]);
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
                if (x[i][k][j] == NULL)
                    continue;

                of_sum += location_cost[j] * *x[i][k][j];
            }
        }
    }

    // sum_i_in_C sum_k_in_H_i sum_j_in_I_i ((c_j + q_i m_j)* x_ikj)
    for (int i = 0; i < num_classes_computer; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, true)])
        {
            for (int j : location_contains_class_computer[i])
            {
                if (t[i][k][j] == NULL)
                    continue;

                of_sum += (location_cost[j] + (num_students_in_class[i_in_A(i, true)] * location_computer_cost[j])) * *t[i][k][j];
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

            for (int j : location_contains_class_classroom[i])
            {
                if (x[i][k][j] == NULL)
                    continue;

                sum_bc1_x += *x[i][k][j];
            }

            IloRange constraint = (sum_bc1_x - 1 == 0);
            sprintf(name, "BC1(%s,%d)", data.get_class_name(i_in_A(i, false)).c_str(), k);
            constraint.setName(name);

            model.add(constraint);
        }
    }

    for (int i = 0; i < num_classes_computer; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, true)])
        {
            IloExpr sum_bc1_t(env);

            for (int j : location_contains_class_computer[i])
            {
                if (t[i][k][j] == NULL)
                    continue;

                sum_bc1_t += *t[i][k][j];
            }

            IloRange constraint = (sum_bc1_t - 1 == 0);
            sprintf(name, "BC1(%s,%d)", data.get_class_name(i_in_A(i, true)).c_str(), k);
            constraint.setName(name);

            model.add(constraint);
        }
    }

    // Basic constraints 2 (BC2): each location must have at most one lecture assigned per lecture time slot

    for (int j = 0; j < num_locations; j++)
    {
        for (int k = 0; k < num_timeslots; k++)
        {
            IloExpr sum_bc2_x(env);

            for (int i : classes_classroom_per_timeslot[k])
            {
                if (x[i][k][j] == NULL)
                    continue;

                sum_bc2_x += *x[i][k][j];
            }

            IloExpr sum_bc2_t(env);

            for (int i : classes_computer_per_timeslot[k])
            {
                if (t[i][k][j] == NULL)
                    continue;

                sum_bc2_t += *t[i][k][j];
            }

            IloRange constraint = (sum_bc2_x + sum_bc2_t - 1 <= 0);
            sprintf(name, "BC2(%s,%d)", data.get_location_name(j).c_str(), k);
            constraint.setName(name);

            model.add(constraint);
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
                    if (x[i][k][j] == NULL || y[i][j] == NULL)
                        continue;

                    IloRange constraint = (*x[i][k][j] - *y[i][j] == 0);
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
                    if (t[i][k][j] == NULL || u[i][j] == NULL)
                        continue;

                    IloRange constraint = (*t[i][k][j] - *u[i][j] == 0);
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
                        if (x[i][k][j] == NULL || w[l][j] == NULL)
                            continue;

                        IloRange constraint = (*x[i][k][j] - *w[l][j] == 0);
                        sprintf(name, "IC2(%d,%s,%d,%s)", data.get_itc_group_id(l), data.get_class_name(i_in_A(i, false)).c_str(),
                                k, data.get_location_name(j).c_str());
                        constraint.setName(name);

                        model.add(constraint);
                    }
                }
            }
        }
    }

    /*
    * SOLVE
    */

    IloCplex cplex(model);
    // Time limit
    cplex.setParam(IloCplex::Param::TimeLimit, 1 * 60 * 60);
    // Set upper bound
    if (upper_bound >= 0)
        cplex.setParam(IloCplex::Param::MIP::Tolerances::UpperCutoff, upper_bound);
    // Export LP
    string lp_name = "lp/" + data.get_instance_name() + ".lp";
    cplex.exportModel(lp_name.c_str());

    try
    {
        cplex.solve();
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
    results.status = cplex.getStatus();
    results.variables = "";

    for (int i = 0; i < num_classes_classroom; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, false)])
        {
            for (int j : location_contains_class_classroom[i])
            {
                if (x[i][k][j] == NULL)
                    continue;

                if (cplex.getValue(*x[i][k][j]) >= 1 - eps)
                {
                    results.variables += x[i][k][j]->getName();
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
                if (t[i][k][j] == NULL)
                    continue;

                if (cplex.getValue(*t[i][k][j]) >= 1 - eps)
                {
                    results.variables += t[i][k][j]->getName();
                    results.variables += " ";
                }
            }
        }
    }

    /*
    * CLEANUP
    */

    // X_ikj  -- i in B, k in H_i, j in S_i
    for (int i = 0; i < num_classes_classroom; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, false)])
        {
            for (int j : location_contains_class_classroom[i])
            {
                if (x[i][k][j] == NULL)
                    continue;

                delete x[i][k][j];
            }
        }
    }

    // T_ikj  -- i in C, k in H_i, j in I_i
    for (int i = 0; i < num_classes_computer; i++)
    {
        for (int k : lectures_of_class[i_in_A(i, true)])
        {
            for (int j : location_contains_class_computer[i])
            {
                if (t[i][k][j] == NULL)
                    continue;

                delete t[i][k][j];
            }
        }
    }

    // Y_ij  -- i in B, j in S_i
    for (int i = 0; i < num_classes_classroom; i++)
    {
        for (int j : location_contains_class_classroom[i])
        {
            delete y[i][j];
        }
    }

    // U_ij  -- i in C, j in I_i
    for (int i = 0; i < num_classes_computer; i++)
    {
        for (int j : location_contains_class_computer[i])
        {
            delete u[i][j];
        }
    }

    // W_lj  -- l in E, j in S
    for (int l = 0; l < num_itc_groups; l++)
    {
        for (int j = 0; j < num_locations_classroom; j++)
        {
            delete w[l][j];
        }
    }

    env.end();

    return results;
}

#endif