import json
import os
import subprocess

dirpath20191 = os.path.dirname(os.path.realpath(__file__)) + '/data/20191'
dirpathout = os.path.dirname(os.path.realpath(__file__)) + '/data/test'
dirpathresults = os.path.dirname(os.path.realpath(__file__)) + '/results'

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

    json.dump(new_c_r_dict, f_new_c_r)
    json.dump(new_c_c_dict, f_new_c_c)
    json.dump(new_l_r_dict, f_new_l_r)
    json.dump(new_l_c_dict, f_new_l_c)

    f_new_c_r.close()
    f_new_c_c.close()
    f_new_l_r.close()
    f_new_l_c.close()

    infeasible = False

    #Run both cap and cap_raphael
    try:
        subprocess.run(["./cap"], check = True)
    except subprocess.CalledProcessError:
        infeasible = True

    try:
        subprocess.run(["./cap_raphael"], check = True)
    except subprocess.CalledProcessError:
        infeasible = True

    #Check their results at ./results
    f_results_me = open(dirpathresults + "/test_1.json", "r")
    f_results_raphael = open(dirpathresults + "/raphael_test.json", "r")

    results_me = json.load(f_results_me)
    results_raphael = json.load(f_results_raphael)

    #If theyre different
    tolerance = 0.1
    if (results_me["value"] >= results_raphael["value raphael"] + tolerance or results_me["value"] <= results_raphael["value raphael"] - tolerance ):
        #End program, instance with error should be the one left at data/test
        print(f"\033[91m" + "Different results" + f"\033[0m")
        create_new = False
        break

    #If both are infeasible (if only one -> theyre different)
    if (infeasible or (results_me["status"] != 2 and results_raphael["status raphael"] != 2)):
        #Probably means we have too many classes and not enough locations
        if (n_l_r >= len(loc_room.items())):
            print(f"\033[91m" + "Infeasible results, ran out of locations" + f"\033[0m")
            create_new = False
            break
        
        #Make bigger instance: Add 1 loc classroom and 1 loc computer
        n_l_r = n_l_r + 1

        #If it runs out of loc computer, just dont add one
        if (n_l_c < len(loc_comp.items())):
            n_l_c = n_l_c + 1

        #Go to creating a new instance
        continue

    #If theyre both feasible and equal

    #If no more classes, we've exahausted all sub-instances
    if (n_c_r >= len(class_room.items())):
        print(f"\033[92m" + "Ran out of sub-instances, all results are equal" + f"\033[0m")
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