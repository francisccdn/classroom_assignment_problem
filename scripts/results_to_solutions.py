import os
import json

dir_path = os.path.dirname(os.path.dirname(
    os.path.realpath(__file__))) + '/results'
output_path = os.path.dirname(os.path.dirname(
    os.path.realpath(__file__))) + '/solutions'


def TimeslotToString(timeslot: int):
    dayless = timeslot % 20
    shiftless = dayless % 7

    day = ((timeslot - dayless) / 20) + 2
    shift_int = dayless - shiftless
    shift = "?"
    if shift_int == 0:
        shift = "M"
    elif shift_int == 7:
        shift = "V"
    elif shift_int == 14:
        shift = "N"
    time = shiftless

    return (str(int(day)) + shift + str(time))


for file in os.listdir(dir_path):
    filename = os.fsdecode(file)
    filepath = os.path.join(dir_path, filename)

    outputname = filename.split("_setup")[0] + ".json"
    output = {}

    with open(filepath, 'r') as fr:
        data = json.load(fr)

        output["instance"] = data["instance"]
        output["gap"] = data["gap"]
        output["scenario"] = data["scenario"]
        output["time preprocessing"] = data["time model"] + \
            data["time pre processing"]
        output["time solver"] = data["time solver"]
        output["solution value"] = data["value"]
        output["solution"] = []

        # Get each variable from results
        results = data['results']
        variables_strings = results.split()

        for var in variables_strings:
            variable_decomposed = var.split('_')

            type = variable_decomposed[0]
            class_id = variable_decomposed[1]
            timeslot = variable_decomposed[2]
            location_id = variable_decomposed[3]

            if type == "Z":
                continue
            elif type == "X":
                type = "classroom"
            elif type == "T":
                type = "pc"

            output["solution"].append({
                "timeslot": TimeslotToString(int(timeslot)),
                "requires": type,
                "location_id": int(location_id),
                "class_id": int(class_id)
            })

    output_filepath = os.path.join(output_path, outputname)
    with open(output_filepath, 'w') as fw:
        json.dump(output, fw)
