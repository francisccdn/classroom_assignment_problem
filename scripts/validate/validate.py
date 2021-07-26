import os
import json
import sys

verbose = 'v' in sys.argv
all_instances = 'a' in sys.argv

dir_path = os.path.dirname(os.path.realpath(__file__)) + '/results_filtered'
if all_instances:
    dir_path = os.path.dirname(os.path.dirname(
        os.path.dirname(os.path.realpath(__file__)))) + '/results'


class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


def get_previous_timeslot(timeslot):
    return str(int(timeslot) - 1)


for file in os.listdir(dir_path):
    filename = os.fsdecode(file)
    filepath = os.path.join(dir_path, filename)

    if verbose:
        print("Validating " + filename)
    ideal = True
    feasible = True

    instance_name = filename.split('_')[0]
    input_dir_path = os.path.dirname(os.path.realpath(
        __file__)) + '/validation_data/' + instance_name
    data_dir_path = os.path.dirname(os.path.dirname(
        os.path.dirname(os.path.realpath(__file__)))) + '/data/' + instance_name

    with open(filepath, 'r') as fr:
        data = json.load(fr)
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

        # Check if heuristic cost is accurate
        if data["heuristic"] == True:
            fcl = open(data_dir_path + '/diarios_info.json', 'r')
            flc = open(data_dir_path + '/locais_sala.json', 'r')
            fll = open(data_dir_path + '/locais_info.json', 'r')

            cl_data = json.load(fcl)
            lc_data = json.load(flc)
            ll_data = json.load(fll)

            fcl.close()
            flc.close()
            fll.close()

            accurate_cost = 0

            for var in other_variables:
                if var["type"] == 'X':
                    accurate_cost += lc_data[var["room"]]["gasto_por_aula"]
                if var["type"] == 'T':
                    accurate_cost += ll_data[var["room"]]["gasto_por_aula"]
                    accurate_cost += ll_data[var["room"]]["gasto_pc_por_aula"] * \
                        cl_data[var["class"]]["qtd_alunos"]

            for var in z_variables:
                location = {}
                if var["room"] in lc_data:
                    location = lc_data[var["room"]]
                if var["room"] in ll_data:
                    location = ll_data[var["room"]]

                accurate_cost += location["gasto_setup"]
                if data["setup before class"] == False:
                    accurate_cost -= location["gasto_por_aula"] * \
                        location["duracao_setup"]

            solution_cost = data["heuristic - value"]
            if not (accurate_cost - 0.1 < solution_cost and solution_cost < accurate_cost + 0.1):
                ideal = False
                if verbose:
                    print(f"{bcolors.FAIL}Heuristic cost " + str(solution_cost) 
                    + " is inaccurate. Accurate cost is " + str(accurate_cost) + f"{bcolors.ENDC}")

        # Check if all lectures have rooms assigned to them
        with open(input_dir_path + '/diarios.json', 'r') as fclasses:
            classes_data = json.load(fclasses)
            for class_name in classes_data:
                for timeslot in classes_data[class_name]["horarios"]:
                    found = False

                    for var in other_variables:
                        if var["class"] == class_name and var["timeslot"] == str(timeslot):
                            found = True
                            break

                    if not found:
                        feasible = False
                        if verbose:
                            print(f"{bcolors.FAIL}Lecture " + class_name + "_" +
                                  str(timeslot) + " has no assigned room" + f"{bcolors.ENDC}")

        # BC1
        for i in range(0, len(other_variables)):
            for j in range(i, len(other_variables)):
                var = other_variables[i]
                var2 = other_variables[j]
                if var == var2:
                    continue

                if var["class"] == var2["class"] and var["timeslot"] == var2["timeslot"] and var["room"] != var2["room"]:
                    feasible = False
                    if verbose:
                        print(f"{bcolors.FAIL}Lecture " + var["class"] + "_" + var["timeslot"] +
                              " is assigned to two different rooms" + f"{bcolors.ENDC}")

        # BC2
        for i in range(0, len(other_variables)):
            for j in range(i, len(other_variables)):
                var = other_variables[i]
                var2 = other_variables[j]
                if var == var2:
                    continue

                if var["timeslot"] == var2["timeslot"] and var["room"] == var2["room"]:
                    feasible = False
                    if verbose:
                        print(f"{bcolors.FAIL}Lectures " + var["class"] + "_" + var["timeslot"] + " and " +
                              var2["class"] + "_" + var2["timeslot"] + " are sharing the same room" + f"{bcolors.ENDC}")

        # BC3
        for i in range(0, len(other_variables)):
            for j in range(i, len(other_variables)):
                var = other_variables[i]
                var2 = other_variables[j]
                if var == var2:
                    continue

                if var["class"] == var2["class"] and int(var["timeslot"]) + 1 == int(var2["timeslot"]) and var["room"] != var2["room"]:
                    feasible = False
                    if verbose:
                        print(f"{bcolors.FAIL}Lecture " + var["class"] + "_" + var["timeslot"] + " has twin lecture " +
                              var2["class"] + "_" + var2["timeslot"] + " assigned to a different room" + f"{bcolors.ENDC}")

        # IC1
        if data['scenario'] == 1 or data['scenario'] == 2 or data['scenario'] == 3 or data['scenario'] == 5 or data['scenario'] == 9:
            # Check if there are two lectures from same class in different locations
            for i in range(0, len(other_variables)):
                for j in range(i, len(other_variables)):
                    var = other_variables[i]
                    var2 = other_variables[j]
                    if var == var2:
                        continue

                    if var['class'] == var2['class'] and var['room'] != var2['room']:
                        feasible = False
                        if verbose:
                            print(f"{bcolors.FAIL}Class " + var["class"] + " has two lectures in different rooms: " +
                                  var["class"] + "_" + var["timeslot"] + "_" + var["room"] + " and " +
                                  var2["class"] + "_" + var2["timeslot"] + "_" + var2["room"] + f"{bcolors.ENDC}")

        # IC2
        if data['scenario'] == 1 or data['scenario'] == 2 or data['scenario'] == 3 or data['scenario'] == 5 or data['scenario'] == 8:
            # Check if there are two lectures from same ITC group in different locations
            with open(input_dir_path + '/diarios.json', 'r') as fclasses:
                classes_data = json.load(fclasses)

                for i in range(0, len(other_variables)):
                    var = other_variables[i]
                    if var['type'] == 'T':
                        continue
                
                    for j in range(i, len(other_variables)):
                        var2 = other_variables[j]
                        if var2['type'] == 'T':
                            continue

                        if var == var2:
                            continue

                        if classes_data[var['class']]['id_turma'] == 0 or classes_data[var2['class']]['id_turma'] == 0:
                            continue

                        if classes_data[var['class']]['id_turma'] == classes_data[var2['class']]['id_turma'] and var['room'] != var2['room']:
                            feasible = False
                            if verbose:
                                print(f"{bcolors.FAIL}ITC group " + str(classes_data[var['class']]['id_turma']) + " has two lectures in different rooms: " +
                                      var["class"] + "_" + var["timeslot"] + "_" + var["room"] + " and " +
                                      var2["class"] + "_" + var2["timeslot"] + "_" + var2["room"] + f"{bcolors.ENDC}")

        # PC1
        if data['scenario'] == 1 or data['scenario'] == 3 or data['scenario'] == 4 or data['scenario'] == 7:
            # Check if there are lectures from an ITC group in non-ITC-block room
            with open(input_dir_path + '/diarios.json', 'r') as fclasses:
                classes_data = json.load(fclasses)

                with open(input_dir_path + '/locais.json', 'r') as frooms:
                    rooms_data = json.load(frooms)

                    for var in other_variables:
                        if var['type'] == 'T':
                            continue

                        if classes_data[var['class']]['id_turma'] != 0 and rooms_data[var['room']]['id_bloco'] != 57:
                            feasible = False
                            if verbose:
                                print(f"{bcolors.FAIL}ITC group " + str(classes_data[var['class']]['id_turma']) + " has lecture in non-ITC-block room: " +
                                      var["class"] + "_" + var["timeslot"] + "_" + var["room"] + f"{bcolors.ENDC}")

        # PC2
        if data['scenario'] == 1 or data['scenario'] == 2 or data['scenario'] == 4 or data['scenario'] == 6:
            # Check if lectures from class are in the wrong block
            with open(input_dir_path + '/diarios.json', 'r') as fclasses:
                classes_data = json.load(fclasses)

                with open(input_dir_path + '/locais.json', 'r') as frooms:
                    rooms_data = json.load(frooms)

                    for var in other_variables:
                        course_id = classes_data[var['class']]['id_curso']
                        block_id = rooms_data[var['room']]['id_bloco']

                        if (course_id == 206 and block_id != 58) or (course_id == 56 and block_id != 58) or (course_id == 192 and block_id != 56) or (course_id == 59 and block_id != 59):
                            feasible = False
                            if verbose:
                                print(f"{bcolors.FAIL}Lecture " + var["class"] + "_" + var["timeslot"] + "_" + var["room"] +
                                      " is not in course's block. Course: " + str(course_id) + " Block: " + str(block_id) + f"{bcolors.ENDC}")

        # Check if rooms are being used during timeslots that theyre blocked
        with open(input_dir_path + '/salas_bloqueadas.json', 'r') as fblock:
            blocked_data = json.load(fblock)

            for var in other_variables:
                if var['room'] not in blocked_data:
                    continue

                if int(var['timeslot']) in blocked_data[var['room']]:
                    feasible = False
                    if verbose:
                        print(f"{bcolors.FAIL}Room " + var['room'] + " is blocked for timeslot " + var['timeslot'] +
                              ". Lecture: " + var["class"] + "_" + var["timeslot"] + "_" + var["room"] + f"{bcolors.ENDC}")

        # Check if room capacities are being violated
        with open(input_dir_path + '/diarios.json', 'r') as fclasses:
            classes_data = json.load(fclasses)

            with open(input_dir_path + '/locais.json', 'r') as frooms:
                rooms_data = json.load(frooms)

                for var in other_variables:
                    capacity_str = "qtd_lugares"
                    if var['type'] == 'T':
                        capacity_str = "qtd_pc"

                    if capacity_str not in rooms_data[var['room']]:
                        print(f"{bcolors.FAIL}Room " + var['room'] + " has no capacity of type " + capacity_str + ". Lecture: "
                              + var['type'] + '_' + var["class"] + "_" + var["timeslot"] + "_" + var["room"] + f"{bcolors.ENDC}")
                        continue

                    if classes_data[var['class']]['qtd_alunos'] > rooms_data[var['room']][capacity_str]:
                        feasible = False
                        if verbose:
                            print(f"{bcolors.FAIL}Class " + var['class'] + " has too many students for room " + var['room'] +
                                  ". Lecture: " + var['type'] + '_' + var["class"] + "_" + var["timeslot"] + "_" + var["room"] + f"{bcolors.ENDC}")

        # Setup
        if data['setup'] == True:
            # Go through Zs and check if they're active at the right time
            for z_var in z_variables:
                found_var = False
                found_lecture_before = False

                for var in other_variables:
                    if var["class"] == z_var["class"] and var["timeslot"] == z_var["timeslot"] and var["room"] == z_var["room"]:
                        found_var = True

                        for inner_var in other_variables:
                            var_timeslot_before = get_previous_timeslot(
                                var["timeslot"])
                            if inner_var["timeslot"] == var_timeslot_before and inner_var["room"] == var["room"]:
                                found_lecture_before = True

                        break

                if not found_var:
                    ideal = False
                    if verbose:
                        print(f"{bcolors.WARNING}Corresponding variable for Z " +
                              z_var["class"] + "_" + z_var["timeslot"] + "_" + z_var["room"] + " isn't active" + f"{bcolors.ENDC}")

                if found_lecture_before:
                    ideal = False
                    if verbose:
                        print(f"{bcolors.WARNING}Found lecture immediately before " +
                              z_var["class"] + "_" + z_var["timeslot"] + "_" + z_var["room"] + " and Z is active" + f"{bcolors.ENDC}")

            # Go through Xs and Ts and check if no Zs are missing
            for var in other_variables:
                found_z = False
                should_have_z = True
                var_previous_timeslot = get_previous_timeslot(var["timeslot"])

                for var2 in other_variables:
                    if var == var2:
                        continue

                    if var["room"] == var2["room"] and var_previous_timeslot == var2["timeslot"]:
                        should_have_z = False
                        break

                if should_have_z:
                    for z_var in z_variables:
                        if z_var["class"] == var["class"] and z_var["timeslot"] == var["timeslot"] and z_var["room"] == var["room"]:
                            found_z = True
                            break

                if should_have_z and not found_z:
                    feasible = False
                    if verbose:
                        print(f"{bcolors.FAIL}Z not found for " +
                              var["class"] + "_" + var["timeslot"] + "_" + var["room"] + f"{bcolors.ENDC}")

    if ideal and feasible:
        print(f"{bcolors.OKGREEN}" + filename +
              " -- passed" + f"{bcolors.ENDC}")
    elif feasible and data['status'] == 2:
        print(f"{bcolors.WARNING}" + filename +
              " -- suboptimal but feasible" + " -- status is optimal" + f"{bcolors.ENDC}")
    elif feasible and data['status'] != 2:
        print(f"{bcolors.OKCYAN}" + filename +
              " -- suboptimal but feasible" + " -- status is not optimal" + f"{bcolors.ENDC}")
    else:
        print(f"{bcolors.FAIL}" + filename +
              " -- infeasible" + f"{bcolors.ENDC}")
