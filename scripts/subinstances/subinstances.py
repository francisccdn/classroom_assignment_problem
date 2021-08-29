import json
import os
import subprocess

dirpath = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
dirpath20191 = dirpath + '/data/20191'
dirpathout = dirpath + '/data/subinstance'
dirpathresults = dirpath + '/results'

#Create instance based off of 20191
f_class_room = open(dirpath20191 + "/classes_classroom.json", "r")
f_class_comp = open(dirpath20191 + "/classes_pc.json", "r")
f_loc_room = open(dirpath20191 + "/locations_classroom.json", "r")
f_loc_comp = open(dirpath20191 + "/locations_pc.json", "r")

class_room = json.load(f_class_room)
class_comp = json.load(f_class_comp)
loc_room = json.load(f_loc_room)
loc_comp = json.load(f_loc_comp)

#Initially, 1 class classroom 1 class computer 1 loc classroom 1 loc computer
n_c_r = 1
n_c_c = 1
n_l_r = 1
n_l_c = 1

create_new = True

while (create_new):
    #Create the new instance
    new_class_room = []
    new_class_comp = []
    new_loc_room = []
    new_loc_comp = []

    for i in range(n_c_r):
        new_class_room.append([*class_room.items()][i])
    for i in range(n_c_c):
        new_class_comp.append([*class_comp.items()][i])
    for i in range(n_l_r):
        new_loc_room.append([*loc_room.items()][i])
    for i in range(n_l_c):
        new_loc_comp.append([*loc_comp.items()][i])

    new_c_r_dict = dict(new_class_room)
    new_c_c_dict = dict(new_class_comp)
    new_l_r_dict = dict(new_loc_room)
    new_l_c_dict = dict(new_loc_comp)

    f_new_c_r = open(dirpathout + "/classes_classroom.json", "w")
    f_new_c_c = open(dirpathout + "/classes_pc.json", "w")
    f_new_l_r = open(dirpathout + "/locations_classroom.json", "w")
    f_new_l_c = open(dirpathout + "/locations_pc.json", "w")
    f_new_blocked = open(dirpathout + "/occupied_locations.json", "w")

    json.dump(new_c_r_dict, f_new_c_r)
    json.dump(new_c_c_dict, f_new_c_c)
    json.dump(new_l_r_dict, f_new_l_r)
    json.dump(new_l_c_dict, f_new_l_c)
    json.dump({"occupied_locations": []}, f_new_blocked)

    f_new_c_r.close()
    f_new_c_c.close()
    f_new_l_r.close()
    f_new_l_c.close()
    f_new_blocked.close()

    infeasible = False

    #Run cap
    try:
        subprocess.run([dirpath + "/cap", "subinstance", "9", "1", "1", "0", "0"], check = True)
    except subprocess.CalledProcessError:
        infeasible = True

    #Check results
    f_results = open(dirpathresults + "/subinstance_9_setup_heuristic_best.json", "r")

    data = json.load(f_results)
    result = data['results']

    all_variables_strings = result.split()
    all_variables = []

    for variable in all_variables_strings:
        variable_decomposed = variable.split('_')

        variable_recomposed = {
            "type": variable_decomposed[0],
            "class": variable_decomposed[1],
            "timeslot": variable_decomposed[2],
            "room": variable_decomposed[3]
        }
        all_variables.append(variable_recomposed)

    # Isolate Zs
    z_variables = []
    other_variables = []
    for variable in all_variables:
        if variable['type'] == 'Z':
            z_variables.append(variable)
        else:
            other_variables.append(variable)

    #Get accurate cost
    fcl = open(dirpath20191 + '/classes_pc.json', 'r')
    flc = open(dirpath20191 + '/locations_classroom.json', 'r')
    fll = open(dirpath20191 + '/locations_pc.json', 'r')

    cl_data = json.load(fcl)
    lc_data = json.load(flc)
    ll_data = json.load(fll)

    fcl.close()
    flc.close()
    fll.close()

    accurate_cost = 0

    for var in other_variables:
        if var["type"] == 'X':
            accurate_cost += lc_data[var["room"]]["cost_per_lecture"]
        if var["type"] == 'T':
            accurate_cost += ll_data[var["room"]]["cost_per_lecture"]
            accurate_cost += ll_data[var["room"]]["pc_cost_per_lecture"] * \
                cl_data[var["class"]]["qty_students"]

    for var in z_variables:
        location = {}
        if var["room"] in lc_data:
            location = lc_data[var["room"]]
        if var["room"] in ll_data:
            location = ll_data[var["room"]]

        accurate_cost += location["setup_cost"]
        if data["setup before class"] == False:
            accurate_cost -= location["cost_per_lecture"] * \
                location["setup_duration"]

    solution_cost = data["heuristic - greedy - value"]
    # solution_cost = data["heuristic - local search - value"]

    #If cost is inaccurate
    if not (accurate_cost - 1 < solution_cost and solution_cost < accurate_cost + 1):
        print("Heuristic cost " + str(solution_cost) + " is inaccurate. Accurate cost is " + str(accurate_cost))
        print("Exiting without creating new sub-instance...")
        #Exit and leave subinstance
        create_new = False

    #If theyre both equal

    #If no more classes, we've exahausted all sub-instances
    if (n_c_r >= len(class_room.items())):
        print(f"\033[92m" + "Ran out of sub-instances, all results are valid" + f"\033[0m")
        create_new = False
        break

    #Make bigger instance: Add 1 class classroom and 1 class computer (in order of the original 20191)
    n_c_r = n_c_r + 1
    
    #If it runs out of class computer, just dont add one
    if (n_c_c < len(class_comp.items())):
            n_c_c = n_c_c + 1

    #Go to creating a new instance

f_class_room.close()
f_class_comp.close()
f_loc_room.close()
f_loc_comp.close()