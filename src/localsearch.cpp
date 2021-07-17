#include <fstream>
#include <iostream>
#include <chrono>
#include <nlohmann/json.hpp>

#include "../include/localsearch.h"
#include "../include/cap_data.h"

#ifndef LOCALSEARCH_CPP
#define LOCALSEARCH_CPP

#define PENALTY_PER_UNF 50

using namespace std;

LocalSearch::LocalSearch(const CapData &capdata) : data(capdata)
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
float LocalSearch::AssignCost(int i, vector<int> k_and_twins, int j)
{
    float cost = 0;

    cost += data.get_location_cost()[j] * k_and_twins.size();

    // If it is a computer class
    if (i >= data.get_classes_classroom())
    {
        cost += data.get_num_students_in_class()[i] * data.get_location_computer_cost()[j] * k_and_twins.size();
    }

    if (data.is_setup())
    {
        float setup_cost = data.get_location_setup_cost()[j];
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

float LocalSearch::UnassignCost(int i, vector<int> k_and_twins, int j)
{
    return (-1) * AssignCost(i, k_and_twins, j);
}

// Checks for ICs, PCs and Blocked Timeslots
int LocalSearch::ValidAssignment(int i, vector<int> k_and_twins, int j)
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

bool LocalSearch::AllAssigned()
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
void LocalSearch::Assign(int i, vector<int> k_and_twins, int j)
{
    for (int t : k_and_twins)
    {
        x[i][t][j] = true;
        has_assigned_location[i][t] = true;
        has_assigned_class[t][j] = true;
    }
}

bool CompareAssignments(AssignmentData a, AssignmentData b) { return (a.cost < b.cost); }

int LocalSearch::Greedy(float *greedy_cost)
{
    float total_cost = 0;
    int num_unfeasibilities = 0;

    // Save every possible assignment to vector
    vector<AssignmentData> sorted_assignments = vector<AssignmentData>();
    for (int i = 0; i < data.get_classes(); i++)
    {
        for (int k : twin_sets_of_class[i])
        {
            // Only assign computer classes to computer rooms and classroom classes to classrooms
            bool i_is_computer = (i >= data.get_classes_classroom());
            int start_j = i_is_computer ? data.get_locations_classroom() : 0;
            int end_j = i_is_computer ? data.get_locations() : data.get_locations_classroom();

            for (int j = start_j; j < end_j; j++)
            {
                float cost = AssignCost(i, twins_of_lecture[i][k], j);
                sorted_assignments.push_back({i, twins_of_lecture[i][k], j, cost});
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

pair<float, float> LocalSearch::WalkCost(WalkData move)
{
    float movement_cost = 0;
    float penalty = 0;

    // Base cost
    movement_cost += UnassignCost(move.i, twins_of_lecture[move.i][move.k], move.j_from);
    movement_cost += AssignCost(move.i, twins_of_lecture[move.i][move.k], move.j_to);

    // Penalize invalid movement

    // If new location wont fit class
    if (find(location_contains_class[move.i].begin(), location_contains_class[move.i].end(), move.j_to) == location_contains_class[move.i].end())
        // Add penalty
        penalty += PENALTY_PER_UNF;

    // If new location already has lecture assigned to it at that time
    for (int t : twins_of_lecture[move.i][move.k])
    {
        if (has_assigned_class[t][move.j_to])
            // Add penalty
            penalty += PENALTY_PER_UNF;
    }

    // If new location violates ICs PCs and blocked timeslots, add penalty
    penalty += (PENALTY_PER_UNF * ValidAssignment(move.i, twins_of_lecture[move.i][move.k], move.j_to));
    
    // If old location didnt fit class
    if (find(location_contains_class[move.i].begin(), location_contains_class[move.i].end(), move.j_from) == location_contains_class[move.i].end())
        // Remove penalty
        penalty -= PENALTY_PER_UNF;

    // If old location has another lecture assigned to it at that time
    for (int t : twins_of_lecture[move.i][move.k])
    {
        for (int i = 0; i < data.get_classes(); i++)
        {
            if (i == move.i) continue;

            if (x[i][t][move.j_from])
            {
                // Remove penalty
                penalty -= PENALTY_PER_UNF;
                break; // Only one removed penalty per lecture time, to keep consistent with added penalty
            }
        }
    }

    // If old location violated ICs PCs and blocked timeslots, remove penalty
    penalty -= (PENALTY_PER_UNF * ValidAssignment(move.i, twins_of_lecture[move.i][move.k], move.j_from));
    
    return pair<float, float>(movement_cost, penalty);
}

pair<float, float> LocalSearch::SwapCost(SwapData move) 
{
    float movement_cost = 0;
    float penalty = 0;

    bool i_1_is_computer = move.i_1 >= data.get_classes_classroom();

    // Movement cost
    if (i_1_is_computer)
    {
        vector<int> k_and_twins = twins_of_lecture[move.i_1][move.k];

        // Remove computer cost of class i_1 in location j_1
        movement_cost -= data.get_num_students_in_class()[move.i_1] * data.get_location_computer_cost()[move.j_1] * k_and_twins.size();
        // Add computer cost of class i_1 in location j_2
        movement_cost += data.get_num_students_in_class()[move.i_1] * data.get_location_computer_cost()[move.j_2] * k_and_twins.size();
        // Remove computer cost of class i_2 in location j_2
        movement_cost -= data.get_num_students_in_class()[move.i_2] * data.get_location_computer_cost()[move.j_2] * k_and_twins.size();
        // Add computer cost of class i_2 in location j_1
        movement_cost += data.get_num_students_in_class()[move.i_2] * data.get_location_computer_cost()[move.j_1] * k_and_twins.size();
    }

    // Penalty

    // If j_1 didnt fit i_1
    if (find(location_contains_class[move.i_1].begin(), location_contains_class[move.i_1].end(), move.j_1) == location_contains_class[move.i_1].end())
        // Remove penalty
        penalty -= PENALTY_PER_UNF;

    // If j_2 didnt fit i_2
    if (find(location_contains_class[move.i_2].begin(), location_contains_class[move.i_2].end(), move.j_2) == location_contains_class[move.i_2].end())
        // Remove penalty
        penalty -= PENALTY_PER_UNF;

    // If j_2 wont fit i_1
    if (find(location_contains_class[move.i_1].begin(), location_contains_class[move.i_1].end(), move.j_2) == location_contains_class[move.i_1].end())
        // Add penalty
        penalty += PENALTY_PER_UNF;

    // If j_1 wont fit i_2
    if (find(location_contains_class[move.i_2].begin(), location_contains_class[move.i_2].end(), move.j_1) == location_contains_class[move.i_2].end())
        // Add penalty
        penalty += PENALTY_PER_UNF;

    // If i_1 violated ICs PCs and blocked timeslots in j_1, remove penalty
    penalty -= (PENALTY_PER_UNF * ValidAssignment(move.i_1, twins_of_lecture[move.i_1][move.k], move.j_1));
    
    // If i_2 violated ICs PCs and blocked timeslots in j_2, remove penalty
    penalty -= (PENALTY_PER_UNF * ValidAssignment(move.i_2, twins_of_lecture[move.i_2][move.k], move.j_2));

    // If i_1 violates ICs PCs and blocked timeslots in j_2, add penalty
    penalty += (PENALTY_PER_UNF * ValidAssignment(move.i_1, twins_of_lecture[move.i_1][move.k], move.j_2));

    // If i_2 violates ICs PCs and blocked timeslots in j_1, add penalty
    penalty += (PENALTY_PER_UNF * ValidAssignment(move.i_2, twins_of_lecture[move.i_2][move.k], move.j_1));

    return pair<float, float>(movement_cost, penalty);
}

void LocalSearch::Walk(WalkData move) 
{
    for (int t : twins_of_lecture[move.i][move.k])
    {
        x[move.i][t][move.j_from] = false;
        x[move.i][t][move.j_to] = true;     

        has_assigned_class[t][move.j_to] = true;
        has_assigned_class[t][move.j_from] = false;
        for (int i = 0; i < data.get_classes(); i++)
        {
            if (x[i][t][move.j_from])
            {
                has_assigned_class[t][move.j_from] = true;
                break;
            }
        }
    }
}

void LocalSearch::Swap(SwapData move) 
{
    for (int t : twins_of_lecture[move.i_1][move.k])
    {
        x[move.i_1][t][move.j_1] = false;
        x[move.i_2][t][move.j_2] = false;

        x[move.i_1][t][move.j_2] = true;
        x[move.i_2][t][move.j_1] = true;
    }
}

bool CompareMovements(MovementData a, MovementData b) { return ((a.cost + a.penalty) < (b.cost + b.penalty)); }

vector<MovementData> LocalSearch::Search()
{
    vector<MovementData> neighbourhood = vector<MovementData>();

    // Get all possible walk movements
    for (int i = 0; i < data.get_classes(); i++)
    {
        for (int k : twin_sets_of_class[i])
        {
            vector<int> k_and_twins = twins_of_lecture[i][k];

            // Only assign computer classes to computer rooms and classroom classes to classrooms
            bool i_is_computer = (i >= data.get_classes_classroom());
            int start_j = i_is_computer ? data.get_locations_classroom() : 0;
            int end_j = i_is_computer ? data.get_locations() : data.get_locations_classroom();

            for (int j_to = start_j; j_to < end_j; j_to++)
            {
                int j_from = -1;
                for (int j = start_j; j < end_j; j++)
                {
                    if (x[i][k][j])
                    {
                        j_from = j;
                        break;
                    }
                }

                if (j_from == j_to || j_from == -1)
                    continue;

                MovementData movement;
                movement.type = movement.Walk_t;
                movement.walk = {
                    i,
                    k,
                    j_from,
                    j_to};

                pair<float, float> cost = WalkCost(movement.walk);
                movement.cost = cost.first;
                movement.penalty = cost.second;

                neighbourhood.push_back(movement);
            }
        }
    }

    // Get all possible swap movements
    for (int i_1 = 0; i_1 < data.get_classes(); i_1++)
    {
        bool i_1_is_computer = i_1 >= data.get_classes_classroom();

        for (int i_2 = 0; i_2 < data.get_classes(); i_2++)
        {
            bool i_2_is_computer = i_2 >= data.get_classes_classroom();

            if (i_1_is_computer != i_2_is_computer)
                continue;
            
            for (int k_1 : twin_sets_of_class[i_1])
            {
                for (int k_2 : lectures_of_class[i_2])
                {
                    if (twins_of_lecture[i_1][k_1] != twins_of_lecture[i_2][k_2])
                        continue;
                    
                    int k = k_1;
                    vector<int> k_and_twins = twins_of_lecture[i_1][k];
                    
                    int j_1 = -1;
                    for (int j = 0; j < data.get_locations(); j++)
                    {
                        if (x[i_1][k][j])
                        {
                            j_1 = j;
                            break;
                        }
                    }
                    int j_2 = -1;
                    for (int j = 0; j < data.get_locations(); j++)
                    {
                        if (x[i_2][k][j])
                        {
                            j_2 = j;
                            break;
                        }
                    }

                    if (i_1 == i_2 || j_1 == j_2 || j_1 == -1 || j_2 == -1)
                        continue;


                    MovementData movement;
                    movement.type = movement.Swap_t;
                    movement.swap = {
                        i_1,
                        j_1,
                        i_2,
                        j_2,
                        k};
                    
                    pair<float, float> cost = SwapCost(movement.swap);

                    movement.cost = cost.first;
                    movement.penalty = cost.second;

                    neighbourhood.push_back(movement);
                }
            }
        }
    }

    return neighbourhood;
}

void LocalSearch::UpdateSearch(std::vector<MovementData> *neighbourhood, MovementData move)
{
    // Classes, times and locations affected by movement
    int affected_i1 = -1, affected_i2 = -1, affected_j1 = -1, affected_j2 = -1;

    if (move.type == move.Walk_t)
    {
        affected_i1 = move.walk.i;
        affected_j1 = move.walk.j_from;
        affected_j2 = move.walk.j_to;
    }
    if (move.type == move.Swap_t)
    {
        affected_i1 = move.swap.i_1;
        affected_i2 = move.swap.i_2;
        affected_j1 = move.swap.j_1;
        affected_j2 = move.swap.j_2;
    }
    
    // Remove from neighbourhood all movements which are now inaccurate
    for (int n = 0; n < neighbourhood->size(); n++)
    {
        MovementData m = (*neighbourhood)[n];

        if (m.type == m.Walk_t)
        {
            if (m.walk.i == affected_i1 || m.walk.i == affected_i2 
            || m.walk.j_to == affected_j1 || m.walk.j_to == affected_j2
            || m.walk.j_from == affected_j1 || m.walk.j_from == affected_j2)
            {
                neighbourhood->erase(neighbourhood->begin() + n);
            }
        }
        if (m.type == m.Swap_t)
        {
            if(m.swap.i_1 == affected_i1 || m.swap.i_1 == affected_i2
            || m.swap.i_2 == affected_i1 || m.swap.i_2 == affected_i2 
            || m.swap.j_1 == affected_j1 || m.swap.j_1 == affected_j2 
            || m.swap.j_2 == affected_j1 || m.swap.j_2 == affected_j2)
            {
                neighbourhood->erase(neighbourhood->begin() + n);
            }
        }
    }

    // Recalculate cost and add back all movements which were rendered inaccurate

    vector<int> affected_is = { affected_i1 };
    if (affected_i2 != -1) affected_is.push_back(affected_j2);

    vector<int> affected_js = { affected_j1, affected_j2 };
    
    // Walk
    for (int i : affected_is)
    {
        for (int k : twin_sets_of_class[i])
        {
            vector<int> k_and_twins = twins_of_lecture[i][k];

            // Only assign computer classes to computer rooms and classroom classes to classrooms
            bool i_is_computer = (i >= data.get_classes_classroom());
            int start_j = i_is_computer ? data.get_locations_classroom() : 0;
            int end_j = i_is_computer ? data.get_locations() : data.get_locations_classroom();

            for (int j_to = start_j; j_to < end_j; j_to++)
            {
                int j_from = -1;
                for (int j = start_j; j < end_j; j++)
                {
                    if (x[i][k][j])
                    {
                        j_from = j;
                        break;
                    }
                }

                if (j_from == j_to || j_from == -1)
                    continue;

                MovementData movement;
                movement.type = movement.Walk_t;
                movement.walk = {
                    i,
                    k,
                    j_from,
                    j_to};

                pair<float, float> cost = WalkCost(movement.walk);
                movement.cost = cost.first;
                movement.penalty = cost.second;

                neighbourhood->push_back(movement);
            }
        }
    }
    
    for (int j : affected_js)
    {
        // Only assign computer classes to computer rooms and classroom classes to classrooms
        bool j_is_computer = (j >= data.get_locations_classroom());
        int start_i = j_is_computer ? data.get_classes_classroom() : 0;
        int end_i = j_is_computer ? data.get_classes() : data.get_classes_classroom();
        int start_j = j_is_computer ? data.get_locations_classroom() : 0;
        int end_j = j_is_computer ? data.get_locations() : data.get_locations_classroom();

        for (int i = start_i; i < end_i; i++)
        {
            for (int k : twin_sets_of_class[i])
            {
                vector<int> k_and_twins = twins_of_lecture[i][k];

                int j_from = -1;
                if (x[i][k][j])
                {
                    j_from = j;
                

                    for (int j_to = start_j; j_to < end_j; j_to++)
                    {
                        if (j_from == j_to || j_from == -1)
                            continue;

                        MovementData movement;
                        movement.type = movement.Walk_t;
                        movement.walk = {
                            i,
                            k,
                            j_from,
                            j_to};

                        pair<float, float> cost = WalkCost(movement.walk);
                        movement.cost = cost.first;
                        movement.penalty = cost.second;

                        neighbourhood->push_back(movement);
                    }
                
                }
                else
                {
                    int j_to = j;

                    for (int j = start_j; j < end_j; j++)
                    {
                        if (x[i][k][j])
                        {
                            j_from = j;
                            break;
                        }
                    }

                    if (j_from == j_to || j_from == -1)
                            continue;

                        MovementData movement;
                        movement.type = movement.Walk_t;
                        movement.walk = {
                            i,
                            k,
                            j_from,
                            j_to};

                        pair<float, float> cost = WalkCost(movement.walk);
                        movement.cost = cost.first;
                        movement.penalty = cost.second;

                        neighbourhood->push_back(movement);
                }
            }
        }
    }

    // Swap
    for (int i_1 : affected_is)
    {
        bool i_1_is_computer = i_1 >= data.get_classes_classroom();

        for (int i_2 = 0; i_2 < data.get_classes(); i_2++)
        {
            bool i_2_is_computer = i_2 >= data.get_classes_classroom();

            if (i_1_is_computer != i_2_is_computer)
                continue;
            
            for (int k_1 : twin_sets_of_class[i_1])
            {
                for (int k_2 : lectures_of_class[i_2])
                {
                    if (twins_of_lecture[i_1][k_1] != twins_of_lecture[i_2][k_2])
                        continue;
                    
                    int k = k_1;
                    vector<int> k_and_twins = twins_of_lecture[i_1][k];
                    
                    int j_1 = -1;
                    for (int j = 0; j < data.get_locations(); j++)
                    {
                        if (x[i_1][k][j])
                        {
                            j_1 = j;
                            break;
                        }
                    }
                    int j_2 = -1;
                    for (int j = 0; j < data.get_locations(); j++)
                    {
                        if (x[i_2][k][j])
                        {
                            j_2 = j;
                            break;
                        }
                    }

                    if (i_1 == i_2 || j_1 == j_2 || j_1 == -1 || j_2 == -1)
                        continue;


                    MovementData movement;
                    movement.type = movement.Swap_t;
                    movement.swap = {
                        i_1,
                        j_1,
                        i_2,
                        j_2,
                        k};
                    
                    pair<float, float> cost = SwapCost(movement.swap);

                    movement.cost = cost.first;
                    movement.penalty = cost.second;

                    neighbourhood->push_back(movement);
                }
            }
        }
    }

    for (int j_1 : affected_js)
    {
        // Only assign computer classes to computer rooms and classroom classes to classrooms
        bool j_is_computer = (j_1 >= data.get_locations_classroom());
        int start_i = j_is_computer ? data.get_classes_classroom() : 0;
        int end_i = j_is_computer ? data.get_classes() : data.get_classes_classroom();

        for (int i_1 = start_i; i_1 < end_i; i_1++)
        {
            bool i_1_is_computer = j_is_computer;

            for (int k_1 : twin_sets_of_class[i_1])
            {
                if (!x[i_1][k_1][j_1])
                    continue;

                for (int i_2 = 0; i_2 < data.get_classes(); i_2++)
                {
                    bool i_2_is_computer = i_2 >= data.get_classes_classroom();

                    if (i_1_is_computer != i_2_is_computer)
                        continue;

                    for (int k_2 : lectures_of_class[i_2])
                    {
                        if (twins_of_lecture[i_1][k_1] != twins_of_lecture[i_2][k_2])
                            continue;
                        
                        int k = k_1;
                        vector<int> k_and_twins = twins_of_lecture[i_1][k];
                        
                        int j_2 = -1;
                        for (int j = 0; j < data.get_locations(); j++)
                        {
                            if (x[i_2][k][j])
                            {
                                j_2 = j;
                                break;
                            }
                        }

                        if (i_1 == i_2 || j_1 == j_2 || j_1 == -1 || j_2 == -1)
                            continue;


                        MovementData movement;
                        movement.type = movement.Swap_t;
                        movement.swap = {
                            i_1,
                            j_1,
                            i_2,
                            j_2,
                            k};
                        
                        pair<float, float> cost = SwapCost(movement.swap);

                        movement.cost = cost.first;
                        movement.penalty = cost.second;

                        neighbourhood->push_back(movement);
                    }   
                }
            }
        }
    }
}

float LocalSearch::BestImprovement(float initial_cost, int num_unfeasibilities)
{
    // Initial solution
    float current_cost = initial_cost;
    float current_penalty = num_unfeasibilities * PENALTY_PER_UNF;

    // Find all possible movements 
    //  vector<MovementData> neighbourhood = Search();
    while (true)
    {
        // TODO move this out of while
        vector<MovementData> neighbourhood = Search();

        // Sort neighbourhood by cost
        sort(neighbourhood.begin(), neighbourhood.end(), CompareMovements);

        // Get best movement in neighbourhood
        MovementData best = neighbourhood[0];
        
        // If lowest cost doesn't improve solution, we have arrived at local best
        if (best.cost + best.penalty > - 0.1)
        {
            break;
        }

        current_cost += best.cost;
        current_penalty += best.penalty;

        if (current_penalty < 0)
        {
            cout << "Penalty went negative: " << current_penalty << endl;
        }

        // Do movement
        cout << "Current cost = " << current_cost << " - Done movement: ";

        if (best.type == best.Walk_t)
        {
            cout << "Walk, i = " << best.walk.i << " k = " << best.walk.k << " j_to = " << best.walk.j_to << " j_from = " << best.walk.j_from;
            Walk(best.walk);
        }
        else // Swap
        {
            cout << "Swap, i_1 = " << best.swap.i_1 << " i_2 = " << best.swap.i_2 << " j_1 = " << best.swap.j_1 << " j_2 = " << best.swap.j_2 << " k = " << best.swap.k;
            Swap(best.swap);
        }
        cout << endl;

        // Update neighbourhood based on movement made TODO
        // UpdateSearch(&neighbourhood, best);
    }

    return current_cost;
}

float LocalSearch::FirstImprovement(float initial_cost, int num_unfeasibilities)
{
    // Initial solution
    float current_cost = initial_cost;
    float current_penalty = num_unfeasibilities * PENALTY_PER_UNF;

    // Find all possible movements
    vector<MovementData> neighbourhood = Search();

    // Iterate through movements
    int n = 0, last_improvement = 0;
    do {
        // On first improvement
        if (neighbourhood[n].cost + neighbourhood[n].penalty < -0.1)
        {
            // Do movement
            if (neighbourhood[n].type == neighbourhood[n].Walk_t)
            {
                Walk(neighbourhood[n].walk);
            }
            else // Swap
            {
                Swap(neighbourhood[n].swap);
            }

            // Update neighbourhood based on movement made
            UpdateSearch(&neighbourhood, neighbourhood[n]);

            // Set last improvement
            last_improvement = n;
        }

        // Increment and loop around
        n++;
        n = n % neighbourhood.size();
    
    } while (n != last_improvement);

    return current_cost;
}

string LocalSearch::SolutionToString()
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

HeuristicResults LocalSearch::Solve(bool first_improvement)
{
    float initial_cost, cost;

    // Greedy
    auto timer_start_greedy = chrono::system_clock::now();
    int num_unfeasibilities = Greedy(&initial_cost);
    auto timer_end_greedy = chrono::system_clock::now();

    cout << "Finished greedy with cost " << initial_cost << endl;
   
    // Local search
    auto timer_start_localsearch = chrono::system_clock::now();
    //if (first_improvement)
        // cost = FirstImprovement(initial_cost);
    //else
        cost = BestImprovement(initial_cost, num_unfeasibilities);
    auto timer_end_localsearch = chrono::system_clock::now();

    // Export solution
    string solution = SolutionToString();

    chrono::duration<double> timer_greedy = timer_end_greedy - timer_start_greedy;
    chrono::duration<double> timer_localsearch = timer_end_localsearch - timer_start_localsearch;

    HeuristicResults results = {
        initial_cost,              // greedy value
        num_unfeasibilities,       // num of unfeasible assignments in greedy
        timer_greedy.count(),      // greedy time
        cost,                      // local search value
        timer_localsearch.count(), // local search time
        solution                   // chosen variables
    };
    return results;
}

#endif