import json
import random
import os 

lecture_duration_seconds = 60.0 * 50
temp_initial = 30
temp_final = 23
delta_temp = temp_final - temp_initial

air_pressure = 100
specific_heat = 1.006

dir_path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
semesters = ["20181", "20182", "20191"]
filenames = ["/locais_info.json", "/locais_sala.json"]

for semester in semesters:
    for filename in filenames:

        filepath = dir_path + "/data/" + semester + filename

        with open(filepath, 'r') as fr:
            data = json.load(fr)

            for key in data:
                location = data[key]

                airmass = (air_pressure * location['volume']) / (0.287 * (temp_initial + 273.15))
                thermal_energy = airmass * specific_heat * delta_temp * (-1)
                
                ac_btu = 0
                ac_power = 0
                ac_list = location['ares']
                for ac_key in ac_list:
                    ac = ac_list[ac_key]

                    ac_btu += ac['btus']
                    ac_power += ac['potencia']

                heat_removal_capacity = (ac_btu * 0.293)

                ac_setup_cost = (thermal_energy / (heat_removal_capacity / ac_power)) / 3600
                setup_duration_lectures = ( thermal_energy / (heat_removal_capacity / 1000) ) / lecture_duration_seconds

                lecture_cost_no_ac = location['gasto_por_aula'] - (((((ac_power/1000)*21)*50)/60)/30)

                location['gasto_setup'] = (lecture_cost_no_ac * setup_duration_lectures) + ac_setup_cost
                location['duracao_setup'] = setup_duration_lectures

            with open(filepath, 'w+') as fw:
                json.dump(data, fw)