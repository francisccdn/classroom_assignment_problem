import os
import json
import sys

scenario = sys.argv[1] if len(sys.argv) > 1 else 0

new_path = os.path.dirname(os.path.realpath(__file__)) + '/results_compare_bc3/with_bc3'
old_path = os.path.dirname(os.path.realpath(__file__)) + '/results_compare_bc3/no_bc3'

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


for file in os.listdir(new_path):
    filename = os.fsdecode(file)
    old_filepath = os.path.join(old_path, filename)
    new_filepath = os.path.join(new_path, filename)

    file_scenario = filename.split('_')[1]
    file_scenario = file_scenario.split('.')[0]

    if scenario != 0 and file_scenario != scenario:
        continue

    with open(old_filepath, 'r') as ofr:
        old_data = json.load(ofr)

        with open(new_filepath, 'r') as nfr:
            new_data = json.load(nfr)

            if old_data["results"] == new_data["results"]:
                print(f"{bcolors.OKGREEN}" + filename +
                      " has same results with and without BC3." + f"{bcolors.ENDC}")
                if old_data["status"] == 2 and new_data["status"] == 2:
                    print(f"{bcolors.OKCYAN}" + filename + " are both optimal." + f"{bcolors.ENDC}")
                else:
                    print(f"{bcolors.WARNING}" + filename + " are not both optimal." + f"{bcolors.ENDC}")
            
            elif old_data["value"] == new_data["value"]:
                print(f"{bcolors.OKGREEN}" + filename +
                      " has same value with and without BC3." + f"{bcolors.ENDC}")
                if old_data["status"] == 2 and new_data["status"] == 2:
                    print(f"{bcolors.OKCYAN}" + filename + " are both optimal." + f"{bcolors.ENDC}")
                else:
                    print(f"{bcolors.WARNING}" + filename + " are not both optimal." + f"{bcolors.ENDC}")

            else:
                pct_diff = ((new_data["value"] - old_data["value"])/old_data["value"]) * 100
                print(f"{bcolors.FAIL}" + filename + " has different results and value with and without BC3."
                      + " Increase: " + str(pct_diff) + "%" + f"{bcolors.ENDC}")
                if old_data["status"] == 2 and new_data["status"] == 2:
                    print(f"{bcolors.WARNING}" + filename + " are both optimal." + f"{bcolors.ENDC}")
                