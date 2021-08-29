#include <fstream>
#include <iostream>

#include "../include/cap_data.h"
#include "../include/utility.h"

#ifndef CAP_DATA_CPP
#define CAP_DATA_CPP

using namespace std;

CapData::CapData(int scenario, string instance_name, bool setup, bool setup_before_class, bool heuristic)
{
    num_timeslots = TimeslotToInt("6N5") + 1;

    ifstream classes_classroom_if("data/" + instance_name + "/classes_classroom.json");
    classes_classroom_if >> classes_classroom_json;
    classes_classroom_if.close();

    ifstream locations_classroom_if("data/" + instance_name + "/locations_classroom.json");
    locations_classroom_if >> locations_classroom_json;
    locations_classroom_if.close();

    ifstream classes_computer_if("data/" + instance_name + "/classes_pc.json");
    classes_computer_if >> classes_computer_json;
    classes_computer_if.close();

    ifstream locations_computer_if("data/" + instance_name + "/locations_pc.json");
    locations_computer_if >> locations_computer_json;
    locations_computer_if.close();

    nlohmann::json blocked_timeslot_location_json;
    ifstream blocked_timeslot_location_if("data/" + instance_name + "/occupied_locations.json");
    blocked_timeslot_location_if >> blocked_timeslot_location_json;
    blocked_timeslot_location_if.close();

    for (string timeslot_location : blocked_timeslot_location_json["occupied_locations"])
    {
        vector<string> spliced_timeslot_location = francisccdn::splice(timeslot_location, '_');

        int timeslot = TimeslotToInt(spliced_timeslot_location[0]);
        string location = spliced_timeslot_location[1];

        if (blocked_timeslot_per_location[location].empty())
            blocked_timeslot_per_location[location] = vector<int>();

        blocked_timeslot_per_location[location].push_back(timeslot);
    }

    switch (scenario)
    {
    case 1:
        ic1 = true;
        ic2 = true;
        pc1 = true;
        pc2 = true;
        break;

    case 2:
        ic1 = true;
        ic2 = true;
        pc1 = false;
        pc2 = true;
        break;

    case 3:
        ic1 = true;
        ic2 = true;
        pc1 = true;
        pc2 = false;
        break;

    case 4:
        ic1 = false;
        ic2 = false;
        pc1 = true;
        pc2 = true;
        break;

    case 5:
        ic1 = true;
        ic2 = true;
        pc1 = false;
        pc2 = false;
        break;

    case 6:
        ic1 = false;
        ic2 = false;
        pc1 = false;
        pc2 = true;
        break;

    case 7:
        ic1 = false;
        ic2 = false;
        pc1 = true;
        pc2 = false;
        break;

    case 8:
        ic1 = false;
        ic2 = true;
        pc1 = false;
        pc2 = false;
        break;

    case 9:
        ic1 = true;
        ic2 = false;
        pc1 = false;
        pc2 = false;
        break;

    case 10:
    default:
        ic1 = false;
        ic2 = false;
        pc1 = false;
        pc2 = false;
        break;
    }

    num_classes_classroom = 0;
    num_classes_computer = 0;
    classes_classroom_per_timeslot = vector<vector<int>>(num_timeslots, vector<int>());
    classes_computer_per_timeslot = vector<vector<int>>(num_timeslots, vector<int>());
    num_locations_classroom = 0;
    num_locations_computer = 0;
    lectures_of_class = vector<vector<int>>(0, vector<int>());
    twin_lectures_of_class = vector<vector<int>>(0, vector<int>());

    location_cost = vector<float>();
    num_students_in_class = vector<int>();
    location_setup_cost = vector<float>();
    location_setup_duration = vector<float>();

    classes_json_key = vector<string>();
    locations_json_key = vector<string>();

    num_itc_groups = 0;
    classes_classroom_of_itc_group = vector<vector<int>>();
    itc_group_key = vector<int>();

    // Classrooms
    PreProcessing(false, &num_classes_classroom, &classes_classroom_per_timeslot, &num_locations_classroom, &location_contains_class_classroom);

    // Computer labs
    PreProcessing(true, &num_classes_computer, &classes_computer_per_timeslot, &num_locations, &location_contains_class_computer);

    num_classes = num_classes_classroom + num_classes_computer;
    num_locations_computer = num_locations - num_locations_classroom;

    this->setup = setup;
    this->setup_before_class = setup_before_class;

    instance_full_name = instance_name + "_" + to_string(scenario);

    if (setup)
    {
        instance_full_name += "_setup";

        if (!setup_before_class)
            instance_full_name += "_duringclass";
    }

    if (heuristic)
    {
        instance_full_name += "_heuristic";
    }
}

int CapData::TimeslotToInt(string timeslot)
{
    char d = timeslot[0];
    char shift = timeslot[1];
    char h = timeslot[2];
    int day = d - '0';
    int hour = h - '0';

    int day_mod = (day - 2) * 20;
    int shift_mod;

    if (shift == 'M')
        shift_mod = 0;
    else if (shift == 'V')
        shift_mod = 7;
    else if (shift == 'N')
        shift_mod = 14;
    else
        cerr << "Timeslot shift not M V or N" << endl;

    return day_mod + shift_mod + hour;
}

void CapData::PreProcessing(bool is_computer, int *classes, vector<vector<int>> *classes_per_timeslot,
                            int *locations, vector<vector<int>> *location_contains_class)
{
    // Define which jsons will be used
    nlohmann::json *classes_json = is_computer ? &classes_computer_json : &classes_classroom_json;
    nlohmann::json *locations_json = is_computer ? &locations_computer_json : &locations_classroom_json;

    // Index for each class - i in B || i in C
    int i = 0;

    // Loop through each class, adding themselves and their data to corresponding subsets
    for (auto class_i : classes_json->items())
    {
        int i_in_a = is_computer ? i + num_classes_classroom : i;

        nlohmann::json class_data = class_i.value();
        classes_json_key.push_back(class_i.key());

        num_students_in_class.push_back(class_data["qty_students"]);
        lectures_of_class.push_back(vector<int>());
        twin_lectures_of_class.push_back(vector<int>());

        for (auto timeslot_data : class_data["lectures"].items())
        {
            string timeslot = timeslot_data.value()["timeslot"];
            int k = TimeslotToInt(timeslot);

            (*classes_per_timeslot)[k].push_back(i);

            lectures_of_class[i_in_a].push_back(k);
        }

        // Populate twin lectures
        for (int k : lectures_of_class[i_in_a])
        {
            // If k+1 is in lectures_of_class[i_in_a]
            if (std::find(lectures_of_class[i_in_a].begin(), lectures_of_class[i_in_a].end(), k + 1) != lectures_of_class[i_in_a].end())
            {
                twin_lectures_of_class[i_in_a].push_back(k);
            }
        }

        // Create new ITC group if class belongs to one
        int group_id = class_data["group_id"];
        if (!is_computer && group_id != 0)
        {
            // l in E
            int group_index = -1;
            vector<int>::iterator group_iterator = find(itc_group_key.begin(), itc_group_key.end(), group_id);

            // If its the first class from this group
            if (group_iterator == itc_group_key.end())
            {
                num_itc_groups++;
                itc_group_key.push_back(group_id);
                classes_classroom_of_itc_group.push_back(vector<int>());
                group_index = itc_group_key.size() - 1;
            }
            else
            {
                group_index = group_iterator - itc_group_key.begin();
            }

            classes_classroom_of_itc_group[group_index].push_back(i_in_a);
        }

        i++;
    }

    // i = final index + 1 == how many classes were evaluated
    *classes = i;

    // Initialize to num_classes size, so we can access a vector for each class index
    *location_contains_class = vector<vector<int>>(*classes, vector<int>());

    // Index for each location - j in L
    int j = is_computer ? num_locations_classroom : 0;

    // Loop through each location, adding themselves and their data to corresponding subsets
    for (auto location_j : locations_json->items())
    {
        nlohmann::json location_data = location_j.value();

        int j_in_l = j;

        vector<string>::iterator location_iterator = find(locations_json_key.begin(), locations_json_key.end(), location_j.key());

        // If location wasn't processed yet
        if (location_iterator == locations_json_key.end())
        {
            locations_json_key.push_back(location_j.key());

            location_cost.push_back(location_data["cost_per_lecture"]);
            location_setup_cost.push_back(location_data["setup_cost"]);
            location_setup_duration.push_back(location_data["setup_duration"]);
            if (!location_data["pc_cost_per_lecture"].is_null())
            {
                location_computer_cost[j] = location_data["pc_cost_per_lecture"];
            }

            j++;
        }
        else
        {
            j_in_l = location_iterator - locations_json_key.begin();
        }

        for (int i = 0; i < *classes; i++)
        {
            int i_in_a = is_computer ? i + num_classes_classroom : i;
            int capacity = is_computer ? location_data["qty_pc"] : location_data["qty_chairs"];

            if (num_students_in_class[i_in_a] <= capacity)
            {
                (*location_contains_class)[i].push_back(j_in_l);
            }
        }
    }

    // j = final index + 1, which is the same as locations->size()
    *locations = j;
}

bool CapData::ValidVar(bool is_computer, int i, int k, int j)
{
    string class_json_key = classes_json_key[i];
    string location_json_key = locations_json_key[j];

    vector<int> blocked_timeslots = blocked_timeslot_per_location[location_json_key];

    // Timeslot k is blocked for location j
    if (find(blocked_timeslots.begin(), blocked_timeslots.end(), k) != blocked_timeslots.end())
    {
        return false;
    }

    nlohmann::json class_data = is_computer ? classes_computer_json[class_json_key] : classes_classroom_json[class_json_key];
    nlohmann::json location_data = is_computer ? locations_computer_json[location_json_key] : locations_classroom_json[location_json_key];

    int course_id = class_data["course_id"];
    int itc_id = class_data["group_id"];
    int block_id = location_data["block_id"];

    if (pc1)
    {
        // Doesnt need a PC & belongs to ITC & is not on ITC block
        if (!is_computer && itc_id != 0 && block_id != 57)
        {
            return false;
        }
    }

    if (pc2)
    {
        // Location j is not in course's block
        if ((course_id == 206 && block_id != 58) ||
            (course_id == 56 && block_id != 58) ||
            (course_id == 192 && block_id != 56) ||
            (course_id == 59 && block_id != 59))
        {
            return false;
        }
    }

    return true;
}

#endif