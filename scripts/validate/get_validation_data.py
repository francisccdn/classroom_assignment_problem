import os
import json

dir_path = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))) + '/data'
file_names = ['classes_pc.json', 'classes_classroom.json',
              'occupied_locations.json', 'locations_pc.json', 'locations_classroom.json']


def timeslotToInt(timeslot):
    day = int(timeslot[0])
    shift = timeslot[1]
    hour = int(timeslot[2])

    day_mod = (day - 2) * 20
    shift_mod = 0

    if shift == 'M':
        shift_mod = 0
    elif shift == 'V':
        shift_mod = 7
    elif shift == 'N':
        shift_mod = 14

    return day_mod + shift_mod + hour


for subdir in os.listdir(dir_path):
    subdir_name = os.fsdecode(subdir)
    subdir_path = os.path.join(dir_path, subdir_name)

    output_final = {}

    for file_name in file_names:
        file_path = os.path.join(subdir_path, file_name)

        with open(file_path, 'r') as fr:
            data = json.load(fr)

            if file_name == 'classes_pc.json' or file_name == 'classes_classroom.json':
                if 'diarios' not in output_final:
                    output_final['diarios'] = {}

                for key in data:
                    class_data = data[key]

                    output_final['diarios'][key] = {}

                    if file_name == 'classes_classroom.json':
                        output_final['diarios'][key]["group_id"] = class_data["group_id"]
                    output_final['diarios'][key]["course_id"] = class_data["course_id"]
                    output_final['diarios'][key]["qty_students"] = class_data["qty_students"]
                    output_final['diarios'][key]['horarios'] = []

                    for timekey in class_data["lectures"]:
                        output_final['diarios'][key]['horarios'].append(
                            timeslotToInt(class_data["lectures"][timekey]["timeslot"]))

            if file_name == 'locations_pc.json' or file_name == 'locations_classroom.json':
                if 'locais' not in output_final:
                    output_final['locais'] = {}

                for key in data:
                    location_data = data[key]

                    if key not in output_final['locais']:
                        output_final['locais'][key] = {}
                        output_final['locais'][key]["block_id"] = location_data["block_id"]

                    if file_name == 'locations_pc.json':
                        output_final['locais'][key]["qty_pc"] = location_data["qty_pc"]
                    if file_name == 'locations_classroom.json':
                        output_final['locais'][key]["qty_lugares'] = location_data["qty_seats"]

            if file_name == 'occupied_locations.json':
                output = {}
                for string in data["occupied_locations"]:
                    pair = string.split('_')

                    if pair[1] not in output:
                        output[pair[1]] = []

                    timeslot = timeslotToInt(pair[0])

                    output[pair[1]].append(timeslot)

                output_final['salas_bloqueadas'] = output

    output_dir = os.path.dirname(os.path.realpath(
        __file__)) + '/validation_data/' + subdir_name

    if not os.path.exists(output_dir):
        os.mkdir(output_dir)

    for key in output_final:
        output_path = output_dir + '/' + key + '.json'

        if os.path.exists(output_path):
            with open(output_path, 'r') as fr2:
                existing = json.load(fr2)

            output_final[key] = {**existing, **output_final[key]}

        with open(output_path, 'w+') as fw:
            json.dump(output_final[key], fw)
