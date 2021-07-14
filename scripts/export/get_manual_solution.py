import os
import json

manual_path = os.path.dirname(os.path.realpath(__file__)) + '/results_manual'
data_path = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))) + '/data'

# Output object
solution_values = {}

# Loop through manual solutions
for file in os.listdir(manual_path):
    manual_filename = os.fsdecode(file)
    manual_filepath = os.path.join(manual_path, manual_filename)

    # Save manual solution to python object
    manual_fr = open(manual_filepath, 'r')
    manual_solution = json.load(manual_fr)
    manual_fr.close()

    # Get instance name
    instance = manual_filename.split(".")[0]

    # Create instance object inside output object
    solution_values[instance] = {}
    solution_values[instance]["setup_during"] = 0
    solution_values[instance]["setup_before"] = 0
    solution_values[instance]["no_setup"] = 0
    solution_values[instance]["n_assignments"] = 0
    solution_values[instance]["highest"] = {
        "room": {"setup_during": 0, "setup_before": 0, "no_setup": 0},
        "pc": {"setup_during": 0, "setup_before": 0, "no_setup": 0}
    }

    # Get data directory for instance
    data_dir = os.path.join(data_path, instance)

    # Get classes and locations data
    pc_rooms_data_fr = open(os.path.join(data_dir, "locais_info.json"), 'r')
    rooms_data_fr = open(os.path.join(data_dir, "locais_sala.json"), 'r')
    pc_classes_data_fr = open(os.path.join(data_dir, "diarios_info.json"), 'r')
    classes_data_fr = open(os.path.join(data_dir, "diarios_sala.json"), 'r')

    pc_rooms_data = json.load(pc_rooms_data_fr)
    rooms_data = json.load(rooms_data_fr)
    pc_classes_data = json.load(pc_classes_data_fr)
    classes_data = json.load(classes_data_fr)

    pc_rooms_data_fr.close()
    rooms_data_fr.close()
    pc_classes_data_fr.close()
    classes_data_fr.close()

    # Loop through each assignment in manual solution
    for assignment in manual_solution.values():
        a_room = str(assignment["id_local"])
        a_class = str(assignment["id_diario"])

        # Calculate cost of assignment
        if assignment["requer"] == "sala":
            base_cost = rooms_data[a_room]["gasto_por_aula"]

            setup_cost = rooms_data[a_room]["gasto_setup"]
            setup_during_cost = rooms_data[a_room]["gasto_setup"] - (
                rooms_data[a_room]["gasto_por_aula"]*rooms_data[a_room]["duracao_setup"])
        else:
            base_cost = pc_rooms_data[a_room]["gasto_por_aula"]

            if a_class in pc_classes_data:
                qtd = pc_classes_data[a_class]["qtd_alunos"]
            else:
                qtd = classes_data[a_class]["qtd_alunos"]

            base_cost = base_cost + \
                (pc_rooms_data[a_room]["gasto_pc_por_aula"] * qtd)

            setup_cost = pc_rooms_data[a_room]["gasto_setup"]
            setup_during_cost = pc_rooms_data[a_room]["gasto_setup"] - (
                pc_rooms_data[a_room]["gasto_por_aula"]*pc_rooms_data[a_room]["duracao_setup"])

        # Add assignment cost to solution cost
        solution_values[instance]["no_setup"] += base_cost
        solution_values[instance]["setup_during"] += base_cost + \
            setup_during_cost
        solution_values[instance]["setup_before"] += base_cost + setup_cost

        solution_values[instance]["n_assignments"] += 1

        # Save highest cost
        if assignment["requer"] == "sala":
            if solution_values[instance]["highest"]["room"]["no_setup"] < base_cost:
                solution_values[instance]["highest"]["room"]["no_setup"] = base_cost

            if solution_values[instance]["highest"]["room"]["setup_during"] < base_cost + setup_during_cost:
                solution_values[instance]["highest"]["room"]["setup_during"] = base_cost + \
                    setup_during_cost

            if solution_values[instance]["highest"]["room"]["setup_before"] < base_cost + setup_cost:
                solution_values[instance]["highest"]["room"]["setup_before"] = base_cost + setup_cost
        else:
            if solution_values[instance]["highest"]["pc"]["no_setup"] < base_cost:
                solution_values[instance]["highest"]["pc"]["no_setup"] = base_cost

            if solution_values[instance]["highest"]["pc"]["setup_during"] < base_cost + setup_during_cost:
                solution_values[instance]["highest"]["pc"]["setup_during"] = base_cost + \
                    setup_during_cost

            if solution_values[instance]["highest"]["pc"]["setup_before"] < base_cost + setup_cost:
                solution_values[instance]["highest"]["pc"]["setup_before"] = base_cost + setup_cost

# Save solution values to output json
output_path = manual_path + '/manual_results.json'
with open(output_path, 'w+') as fw:
    json.dump(solution_values, fw)
