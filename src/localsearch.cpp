#include "../include/localsearch.h"
#include "../include/cap_data.h"

#ifndef LOCALSEARCH_CPP
#define LOCALSEARCH_CPP

using namespace std;

LocalSearch::LocalSearch(const CapData &capdata) : data(capdata) 
{
    // Get H_i
    lectures_of_class = data.get_lectures_of_class();

    // Unite S_i and I_i into L_i
    location_contains_class = vector<vector<int>>(data.get_classes(), vector<int>());
    for(int i = 0; i < data.get_classes(); i++)
    {
        // Classroom indexes
        if(i < data.get_classes_classroom())
        {
            location_contains_class[i] = data.get_location_contains_class_classroom()[i];
        }
        // Computer indexes
        else
        {
            location_contains_class[i] = data.get_location_contains_class_computer()[i];
        }
    }
}

double LocalSearch::Solve()
{
    return 0;
}

#endif