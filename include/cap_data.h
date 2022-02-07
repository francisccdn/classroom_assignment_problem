#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>

#ifndef CAP_DATA_H
#define CAP_DATA_H

class CapData
{
private:
    /* PROBLEM PARAMETERS */

    std::string instance_full_name;
    bool setup;
    bool setup_before_class;

    /* INPUT FILES */

    nlohmann::json classes_classroom_json;
    nlohmann::json locations_classroom_json;
    nlohmann::json classes_computer_json;
    nlohmann::json locations_computer_json;

    /* MODEL CONSTRAINTS */

    // Lectures from a class must be assigned to the same location
    bool ic1;
    // Lectures from the same ITC group must be assigned to the same location
    bool ic2;
    // Lectures from ITC classes requiring only classrooms must be assigned to classroom block
    bool pc1;
    // Lectures from classes of a given course must be assigned to block of such course
    bool pc2;

    /* (SUB)SETS */

    // A
    int num_classes;
    // B
    int num_classes_classroom;
    // C
    int num_classes_computer;
    // B_k  -- k in H
    std::vector<std::vector<int>> classes_classroom_per_timeslot;
    // C_k  -- k in H
    std::vector<std::vector<int>> classes_computer_per_timeslot;
    // L
    int num_locations;
    // S
    int num_locations_classroom;
    // I
    int num_locations_computer;
    // H_i  -- i in A
    std::vector<std::vector<int>> lectures_of_class;
    // S_i  -- i in B
    std::vector<std::vector<int>> location_contains_class_classroom;
    // I_i  -- i in C
    std::vector<std::vector<int>> location_contains_class_computer;
    // E
    int num_itc_groups;
    // D_l  -- l in E
    std::vector<std::vector<int>> classes_classroom_of_itc_group;
    // G_i  -- i in A
    std::vector<std::vector<int>> twin_lectures_of_class;

    /* INPUT DATA */

    std::map<std::string, std::vector<int>> blocked_timeslot_per_location;
    int num_timeslots;

    // Translates i in A to json key
    std::vector<std::string> classes_json_key;
    // Translates j in L to json key
    std::vector<std::string> locations_json_key;
    // Translates l in E to itc group index
    std::vector<int> itc_group_key;

    // c_j  -- j in L
    std::vector<float> location_cost;
    // q_i  -- i in A
    std::vector<int> num_students_in_class;
    // m_j  -- j in I
    std::map<int, float> location_computer_cost;
    // s_j  -- j in L
    std::vector<float> location_setup_cost;
    // d_j  -- j in L
    std::vector<float> location_setup_duration;
    // p_j  -- j in L
    std::vector<float> location_cost_per_person;

    /* METHODS */

    int TimeslotToInt(std::string timeslot);
    void PreProcessing(bool is_computer, int *classes, std::vector<std::vector<int>> *classes_per_timeslot,
                       int *locations, std::vector<std::vector<int>> *location_contains_class);

public:
    CapData(int scenario, std::string instance_name, bool setup, bool setup_before_class, bool heuristic);

    // i in A, j in L, k in H
    bool ValidVar(bool is_computer, int i, int k, int j);

    // Lectures from a class must be assigned to the same location
    bool IC1() { return ic1; }
    // Lectures from the same ITC group must be assigned to the same location
    bool IC2() { return ic2; }

    // B
    int get_classes_classroom() { return num_classes_classroom; }
    // C
    int get_classes_computer() { return num_classes_computer; }
    // B_k  -- k in H
    std::vector<std::vector<int>> get_classes_classroom_per_timeslot() { return classes_classroom_per_timeslot; }
    // C_k  -- k in H
    std::vector<std::vector<int>> get_classes_computer_per_timeslot() { return classes_computer_per_timeslot; }
    // S
    int get_locations_classroom() { return num_locations_classroom; }
    // I
    int get_locations_computer() { return num_locations_computer; }
    // H_i  -- i in A
    std::vector<std::vector<int>> get_lectures_of_class() { return lectures_of_class; }
    // S_i  -- i in B
    std::vector<std::vector<int>> get_location_contains_class_classroom() { return location_contains_class_classroom; }
    // I_i  -- i in C
    std::vector<std::vector<int>> get_location_contains_class_computer() { return location_contains_class_computer; }
    // c_j  -- j in L
    std::vector<float> get_location_cost() { return location_cost; }
    // q_i  -- i in A
    std::vector<int> get_num_students_in_class() { return num_students_in_class; }
    // m_j  -- j in I
    std::map<int, float> get_location_computer_cost() { return location_computer_cost; }
    // s_j  -- j in L
    std::vector<float> get_location_setup_cost() { return location_setup_cost; }
    // d_j  -- j in L
    std::vector<float> get_location_setup_duration() { return location_setup_duration; }
    // p_j  -- j in L
    std::vector<float> get_location_cost_per_person() { return location_cost_per_person; }
    // E
    int get_num_itc_groups() { return num_itc_groups; }
    // D_l  -- l in E
    std::vector<std::vector<int>> get_classes_classroom_of_itc_group() { return classes_classroom_of_itc_group; }
    // G_i  -- i in A
    std::vector<std::vector<int>> get_twin_lectures_of_class() { return twin_lectures_of_class; };

    /* A
    When accessing a class i in A:
    if class is in B, let b be class' index in B; then i = b
    if class is in C, let c be class' index in C; then i = c + num_classes_classroom */
    int get_classes() { return num_classes; }

    /* L
    When accessing a location j in L:
    if location is in S, let s be location's index in S; then j = s
    if location is in I, let l be location's index in I; then j = l */
    int get_locations() { return num_locations; }

    std::string get_class_name(int i_in_a) { return classes_json_key[i_in_a]; }
    std::string get_location_name(int j_in_l) { return locations_json_key[j_in_l]; }
    int get_itc_group_id(int l_in_e) { return itc_group_key[l_in_e]; }
    int get_num_timeslots() { return num_timeslots; }

    std::string get_instance_name() { return instance_full_name; }

    bool is_setup() { return setup; }
    bool is_setup_before_class() { return setup_before_class; }
};

#endif