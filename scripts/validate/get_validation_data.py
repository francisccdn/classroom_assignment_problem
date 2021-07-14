import os
import json

dir_path = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))) + '/data'
file_names = ['diarios_info.json', 'diarios_sala.json',
              'horarios_salas_especificos.json', 'locais_info.json', 'locais_sala.json']


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

            if file_name == 'diarios_info.json' or file_name == 'diarios_sala.json':
                if 'diarios' not in output_final:
                    output_final['diarios'] = {}

                for key in data:
                    class_data = data[key]

                    output_final['diarios'][key] = {}

                    if file_name == 'diarios_sala.json':
                        output_final['diarios'][key]['id_turma'] = class_data['id_turma']
                    output_final['diarios'][key]['id_curso'] = class_data['id_curso']
                    output_final['diarios'][key]['qtd_alunos'] = class_data['qtd_alunos']
                    output_final['diarios'][key]['horarios'] = []

                    for timekey in class_data["horarios_aulas"]:
                        output_final['diarios'][key]['horarios'].append(
                            timeslotToInt(class_data["horarios_aulas"][timekey]["abreviacao"]))

            if file_name == 'locais_info.json' or file_name == 'locais_sala.json':
                if 'locais' not in output_final:
                    output_final['locais'] = {}

                for key in data:
                    location_data = data[key]

                    if key not in output_final['locais']:
                        output_final['locais'][key] = {}
                        output_final['locais'][key]['id_bloco'] = location_data['id_bloco']

                    if file_name == 'locais_info.json':
                        output_final['locais'][key]['qtd_pc'] = location_data['qtd_pc']
                    if file_name == 'locais_sala.json':
                        output_final['locais'][key]['qtd_lugares'] = location_data['qtd_carteiras']

            if file_name == 'horarios_salas_especificos.json':
                output = {}
                for string in data['horarios_salas_especificos']:
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
