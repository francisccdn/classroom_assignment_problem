#include <fstream>
#include <iostream>
#include <chrono>
#include <nlohmann/json.hpp>

#include "../include/heuristic.h"
#include "../include/cap_data.h"

#ifndef HEURISTIC_CPP
#define HEURISTIC_CPP

using namespace std;

Heuristic::Heuristic(const CapData &capdata) : data(capdata)
{
    // Get H_i
    lectures_of_class = data.get_lectures_of_class();
    for (int i = 0; i < data.get_classes(); i++)
    {
        sort(lectures_of_class[i].begin(), lectures_of_class[i].end());
    }

    // Unite S_i and I_i into L_i
    location_contains_class = vector<vector<int>>(data.get_classes(), vector<int>());
    // Classroom indexes
    for (int i = 0; i < data.get_classes_classroom(); i++)
    {
        int i_in_A = i;
        location_contains_class[i_in_A] = data.get_location_contains_class_classroom()[i];
    }
    // Computer indexes
    for (int i = 0; i < data.get_classes_computer(); i++)
    {
        int i_in_A = i + data.get_classes_classroom();
        location_contains_class[i_in_A] = data.get_location_contains_class_computer()[i];
    }

    // Get twins of each lecture
    twin_sets_of_class = vector<vector<int>>(data.get_classes(), vector<int>());
    for (int i = 0; i < data.get_classes(); i++)
    {
        for (int k : lectures_of_class[i])
        {
            // If k has a lecture right before it
            if (find(lectures_of_class[i].begin(), lectures_of_class[i].end(), k - 1) != lectures_of_class[i].end())
            {
                // Twins of k are the same as twins of k - 1
                twins_of_lecture[i][k] = twins_of_lecture[i][k - 1];
                continue;
            }

            // Else, while there are subsequent lectures, add them to twins of k
            twin_sets_of_class[i].push_back(k);
            twins_of_lecture[i][k] = vector<int>();
            for (int t = k; find(lectures_of_class[i].begin(), lectures_of_class[i].end(), t) != lectures_of_class[i].end(); t++)
            {
                twins_of_lecture[i][k].push_back(t);
            }
        }
    }

    // Initialize variables
    for (int i = 0; i < data.get_classes(); i++)
    {
        for (int k : lectures_of_class[i])
        {
            for (int j = 0; j < data.get_locations(); j++)
            {
                x[i][k][j] = false;
            }
        }
    }
    for (int i = 0; i < data.get_classes(); i++)
    {
        for (int k : lectures_of_class[i])
        {
            has_assigned_location[i][k] = false;
        }
    }
    for (int j = 0; j < data.get_locations(); j++)
    {
        for (int k = 0; k < data.get_num_timeslots(); k++)
        {
            has_assigned_class[k][j] = false;
        }
    }
}

// Cost of assigning lecture i k and its twins to location j
double Heuristic::AssignCost(int i, vector<int> k_and_twins, int j)
{
    double cost = 0;

    cost += data.get_location_cost()[j] * k_and_twins.size();

    // If it is a computer class
    if (i >= data.get_classes_classroom())
    {
        cost += data.get_num_students_in_class()[i] * data.get_location_computer_cost()[j] * k_and_twins.size();
    }

    if (data.is_setup())
    {
        double setup_cost = data.get_location_setup_cost()[j];
        if (!data.is_setup_before_class())
        {
            setup_cost -= data.get_location_cost()[j] * data.get_location_setup_duration()[j];
        }

        int lowest_twin = k_and_twins[0];

        // Add setup cost if lecture needs setup
        if (has_assigned_class[lowest_twin - 1][j] == false)
        {
            cost += setup_cost;
        }
        
        // Remove setup cost of lecture after it, if there is one
        for (int t : k_and_twins)
        {
            // vector<pair<int, int>> removed_setup_of_lecture = vector<pair<int, int>>();

            // Avoid removing setup of same lecture twice
            if (has_assigned_class[t + 1][j] == true)
            {
                /* Check if setup of lecture after was already removed either by 
                ** another class in same location and time, or by another of k's twins */
                bool already_removed = false;
                for (int ii = 0; ii < data.get_classes(); ii++)
                {
                    if (ii == i) continue;

                    // Check for other classes in same location and time
                    if (x[ii][t][j] == true)
                    {
                        already_removed = true;
                        break;
                    }
                }

                // Get num of classes that are sharing this lecture and location, to remove setup of them all
                int num_classes_after = 0;
                for (int ii = 0; ii < data.get_classes(); ii++)
                {
                    if (ii == i) continue;

                    if (x[ii][t + 1][j] == true)
                        num_classes_after++;
                }                

                if (!already_removed)
                {
                    /* As setup cost is only dependent on the location, the 
                    ** setup cost of lecture after it is the same as this one's */
                    cost -= (setup_cost * num_classes_after);
                }
            }
        }
    }

    return cost;
}

double Heuristic::UnassignCost(int i, vector<int> k_and_twins, int j)
{
    return (-1) * AssignCost(i, k_and_twins, j);
}

double Heuristic::AssignPriority(int i, std::vector<int> k_and_twins, int j)
{
    const double multiplier = 100;
    double priority = 0;

    // Higher priority to morning classes
    for (int t : k_and_twins)
    {
        if (t % 20 < 10)
        {
            priority -= multiplier;
        }
    }
    
    // Higher priority to longer twins
    priority -= multiplier * k_and_twins.size(); 

    // Higher priority to many lectures in class
    if (data.IC1())
    {
        priority -= multiplier * lectures_of_class[i].size();
    }

    // Higher priority to ITC
    if (data.IC2())
    {
        // Find i's itc group
        for (int l = 0; l < data.get_num_itc_groups(); l++)
        {
            vector<int> classes_in_itc = data.get_classes_classroom_of_itc_group()[l];
            bool i_is_in_classes_in_itc = find(classes_in_itc.begin(), classes_in_itc.end(), i) != classes_in_itc.end();
            // If l is i's itc group
            if (i_is_in_classes_in_itc)
            {
                for (int ii : classes_in_itc)
                {
                    priority -= multiplier * lectures_of_class[ii].size();
                }
            }
        }
    }

    return priority;
}

// Checks for ICs, PCs and Blocked Timeslots
int Heuristic::ValidAssignment(int i, vector<int> k_and_twins, int j)
{
    int ic1 = 0, ic2 = 0, pcs_and_blocked = 0;

    // (IC1) If class is already assigned to another location
    if (data.IC1())
    {
        for (int t : lectures_of_class[i])
        {
            // Don't check this lecture
            bool t_is_in_k_and_twins = find(k_and_twins.begin(), k_and_twins.end(), t) != k_and_twins.end();
            if (t_is_in_k_and_twins || has_assigned_location[i][t] == false)
                continue;

            // Find location that other lecture of same class is assigned to
            for (int jj = 0; jj < data.get_locations(); jj++)
            {
                // If it's the same location as we want to assign, don't penalize
                if (jj == j) continue;

                // If it's assigned to a different location, penalize
                if (x[i][t][jj] == true)
                {
                    ic1++;
                }
            }
        }
    }

    // (IC2) If any other lecture in class's ITC group is assigned to another location
    if (data.IC2() && i < data.get_classes_classroom())
    {
        // Find i's itc group
        for (int l = 0; l < data.get_num_itc_groups(); l++)
        {
            vector<int> classes_in_itc = data.get_classes_classroom_of_itc_group()[l];
            bool i_is_in_classes_in_itc = find(classes_in_itc.begin(), classes_in_itc.end(), i) != classes_in_itc.end();
            // If l is i's itc group
            if (i_is_in_classes_in_itc)
            {
                // Iterating through classes in ic group
                for (int c : classes_in_itc)
                {
                    // Checking only other classes
                    if (c == i)
                        continue;

                    for (int t : lectures_of_class[c])
                    {
                        if (has_assigned_location[c][t] == false)
                            continue;

                        // Find location that other class is assigned to
                        for (int jj = 0; jj < data.get_locations(); jj++)
                        {
                            // If it's the same location as we want to assign, don't penalize
                            if (jj == j) continue;
                            
                            // If it's assigned to a different location, penalize
                            if (x[c][t][jj] == true)
                            {
                                ic2++;
                                break;
                            }
                        }
                    }
                }

                break; // Found i's itc group, no need to check others
            }
        }
    }

    // PC1, PC2 and blocked timeslots
    for (int t : k_and_twins)
    {
        if (!data.ValidVar(i >= data.get_classes_classroom(), i, t, j))
            pcs_and_blocked++;
    }

    return ic1 + ic2 + pcs_and_blocked;
}

bool Heuristic::AllAssigned()
{
    for (int i = 0; i < data.get_classes(); i++)
    {
        for (int k : lectures_of_class[i])
        {
            if (has_assigned_location[i][k] == false)
            {
                return false;
            }
        }
    }
    return true;
}

// Assign k and all its twins
void Heuristic::Assign(int i, vector<int> k_and_twins, int j)
{
    for (int t : k_and_twins)
    {
        x[i][t][j] = true;
        has_assigned_location[i][t] = true;
        has_assigned_class[t][j] = true;
    }
}

bool CompareAssignments(AssignmentData a, AssignmentData b) 
{
    return ((a.cost  + a.priority) < (b.cost + b.priority)); 
}

int Heuristic::Greedy(double *greedy_cost)
{
    double total_cost = 0;
    int num_unfeasibilities = 0;

    // Save every possible assignment to vector
    vector<AssignmentData> sorted_assignments = vector<AssignmentData>();
    for (int i = 0; i < data.get_classes(); i++)
    {
        for (int k : twin_sets_of_class[i])
        {
            for (int j : location_contains_class[i])
            {
                double cost = AssignCost(i, twins_of_lecture[i][k], j);
                double penalty = AssignPriority(i, twins_of_lecture[i][k], j);
                sorted_assignments.push_back({i, twins_of_lecture[i][k], j, cost, penalty});
            }
        }
    }

    while (!AllAssigned())
    {
        // Sort assignments
        sort(sorted_assignments.begin(), sorted_assignments.end(), CompareAssignments);

        // Choose the first viable assignment
        int best_index = -1;
        for (int n = 0; n < sorted_assignments.size(); n++)
        {
            bool basic_constrains = true;
            // If location has no class assigned at that time and lecture isn't assigned to any location
            for (int t : sorted_assignments[n].twinlectures)
            {
                if (has_assigned_class[t][sorted_assignments[n].j] == true
                || has_assigned_location[sorted_assignments[n].i][t] == true)
                {
                    basic_constrains = false;
                    break;
                }
            }
            // If location wont fit class
            if (find(location_contains_class[sorted_assignments[n].i].begin(), location_contains_class[sorted_assignments[n].i].end(), sorted_assignments[n].j) == location_contains_class[sorted_assignments[n].i].end())
                basic_constrains = false;

            // If assignment is viable
            if (basic_constrains && ValidAssignment(sorted_assignments[n].i, sorted_assignments[n].twinlectures, sorted_assignments[n].j) == 0)
            {
                best_index = n;
                break;
            }
        }

        // If all viable assignments were already made
        if (best_index == -1)
        {
            // Find inviable assignment with lowest cost
            for (int n = 0; n < sorted_assignments.size(); n++)
            {
                // With a lecture that wasn't already assigned to another location
                bool lecture_assigned_to_another_location = false;
                for (int t : sorted_assignments[n].twinlectures)
                {
                    if (has_assigned_location[sorted_assignments[n].i][t] == true)
                    {
                        lecture_assigned_to_another_location = true;
                        break;
                    }
                }

                if (!lecture_assigned_to_another_location)
                {
                    best_index = n;
                    num_unfeasibilities += sorted_assignments[best_index].twinlectures.size();
                    num_unfeasibilities += ValidAssignment(sorted_assignments[best_index].i, sorted_assignments[best_index].twinlectures, sorted_assignments[best_index].j);
                    break;
                }
            }
        }

        // Assign
        AssignmentData best_assignment = sorted_assignments[best_index];
        Assign(best_assignment.i, best_assignment.twinlectures, best_assignment.j);
        sorted_assignments.erase(sorted_assignments.begin() + best_index);

        // Add assignment cost to total cost
        total_cost += best_assignment.cost;

        // Update assignment costs that have changed after the assignment
        int best_lowest_twin = best_assignment.twinlectures[0];
        int best_highest_twin = best_assignment.twinlectures[best_assignment.twinlectures.size() - 1];

        for (int n = 0; n < sorted_assignments.size(); n++)
        {
            AssignmentData a = sorted_assignments[n];
            bool get_cost = false;

            bool a_is_before_best = find(a.twinlectures.begin(), a.twinlectures.end(), best_lowest_twin - 1) != a.twinlectures.end();
            if (a_is_before_best && a.j == best_assignment.j)
                get_cost = true;            

            bool a_is_after_best = find(a.twinlectures.begin(), a.twinlectures.end(), best_highest_twin + 1) != a.twinlectures.end();
            if (a_is_after_best && a.j == best_assignment.j)
                get_cost = true;

            bool a_is_same_time_as_best = false;
            for (int t : best_assignment.twinlectures)
            {
                if (find(a.twinlectures.begin(), a.twinlectures.end(), t) != a.twinlectures.end())
                {
                    a_is_same_time_as_best = true;
                    break;
                }
            }
            if (a_is_same_time_as_best && a.j == best_assignment.j)
                get_cost = true;

            if (get_cost)
                sorted_assignments[n].cost = AssignCost(a.i, a.twinlectures, a.j);
        }
    }

    *greedy_cost = total_cost;
    return num_unfeasibilities;
}

string Heuristic::SolutionToString()
{
    string solution = "";

    for (int i = 0; i < data.get_classes(); i++)
    {
        for (int k : lectures_of_class[i])
        {
            for (int j = 0; j < data.get_locations(); j++)
            {
                if (x[i][k][j])
                {
                    string var_type = i < data.get_classes_classroom() ? "X" : "T";
                    string var_name = var_type + "_" + data.get_class_name(i) + "_" + to_string(k) + "_" + data.get_location_name(j);

                    solution += var_name;
                    solution += " ";

                    bool z = true;
                    for (int ii = 0; ii < data.get_classes(); ii++)
                    {
                        if (x[ii][k - 1][j])
                        {
                            z = false;
                            break;
                        }
                    }

                    if (z && data.is_setup())
                    {
                        string var_name = "Z_" + data.get_class_name(i) + "_" + to_string(k) + "_" + data.get_location_name(j);

                        solution += var_name;
                        solution += " ";
                    }
                }
            }
        }
    }

    return solution;
}

HeuristicResults Heuristic::Solve()
{
    double cost;

    // Greedy
    auto timer_start_greedy = chrono::system_clock::now();
    int num_unfeasibilities = Greedy(&cost);
    auto timer_end_greedy = chrono::system_clock::now();

    cout << "Finished greedy with cost " << cost << endl;

    // Export solution
    string solution = SolutionToString();

    chrono::duration<double> timer_greedy = timer_end_greedy - timer_start_greedy;

    HeuristicResults results = {
        cost,                   // greedy value
        num_unfeasibilities,    // num of unfeasible assignments in greedy
        timer_greedy.count(),   // greedy time
        solution                // chosen variables
    };
    return results;
}

#endif