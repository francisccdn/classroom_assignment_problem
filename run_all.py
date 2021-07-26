import subprocess
import os


time_limit = 0  # In minutes. 0 skips solver, < 0 is no time limit.


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


subprocess.run(["make"])

print(f"{bcolors.HEADER}" +
      "........................| Classroom Assignment Problem |......................." + f"{bcolors.ENDC}")

print("Gerando lista de instancias já rodadas")

found = []

dirpath = os.path.dirname(os.path.realpath(__file__)) + "/results"
for file in os.listdir(dirpath):
    filename = os.fsdecode(file)
    filename = filename.split('.')[0]

    filecomponents = filename.split('_')
    filearray = [int(filecomponents[0]), int(filecomponents[1])]

    # No setup
    if "setup" not in filecomponents:
        filearray.append(0)
        filearray.append(0)

    # Setup
    else:
        filearray.append(1)

        # Setup during class
        if "duringclass" in filecomponents:
            filearray.append(0)
        # Setup before class
        else:
            filearray.append(1)

    # No heuristic
    if "heuristic" not in filecomponents:
        filearray.append(0)

    # Heuristic
    else:
        filearray.append(1)

    found.append(filearray)

print("Rodando todas as instâncias...")

instances = [20181]  # [20181, 20182, 20191]

for scenario in range(1, 11):
    for instance in instances:
        for heuristic in range(0, 2):
            args = []
            time_limit_str = str(time_limit) if heuristic == 0 else "0"

            # No setup

            if [instance, scenario, 0, 0, heuristic] not in found:
                print(f"{bcolors.OKBLUE}" + "Rodando -- Instancia: " + str(instance) + " Cenario: " +
                      str(scenario) + " Heuristica: " + str(heuristic) + " Sem setup" + f"{bcolors.ENDC}")

                args = ['./cap', str(instance), str(scenario), "0", "0", time_limit_str, str(heuristic)]
                logname = str(instance) + "_" + str(scenario)

                logname = logname + "_heuristic" if heuristic == 1 else logname

                # Run cap
                result = subprocess.run(args, capture_output=True)
                # Save log to file
                with open("results_log/" + logname + ".log", 'w') as f:
                    f.write(result.stdout.decode("utf8"))

            else:  # If results were already found
                print(f"{bcolors.OKGREEN}" + "Instancia: " + str(instance) + " Cenario: " + str(scenario) +
                      " Heuristica: " + str(heuristic) + " Sem setup \t\t Resultados gerados encontrados" + f"{bcolors.ENDC}")

            # With setup

            for setup_before in range(0, 2):

                if [instance, scenario, 1, setup_before, heuristic] not in found:
                    print(f"{bcolors.OKBLUE}" + "Rodando -- Instancia: " + str(instance) + " Cenario: " + str(
                        scenario) + " Heuristica: " + str(heuristic) + " Setup antes: " + str(setup_before) + f"{bcolors.ENDC}")

                    args = ['./cap', str(instance), str(scenario), "1", str(setup_before), time_limit_str, str(heuristic)]
                    logname = str(instance) + "_" + str(scenario) + "_setup"

                    logname = logname + "_duringclass" if setup_before == 0 else logname
                    logname = logname + "_heuristic" if heuristic == 1 else logname

                    # Run cap
                    result = subprocess.run(args, capture_output=True)
                    # Save log to file
                    with open("results_log/" + logname + ".log", 'w') as f:
                        f.write(result.stdout.decode("utf8"))

                else:  # If results were already found
                    print(f"{bcolors.OKGREEN}" + "Instancia: " + str(instance) + " Cenario: " + str(scenario) + " Heuristica: " + str(
                        heuristic) + " Setup antes: " + str(setup_before) + " \t Resultados gerados encontrados" + f"{bcolors.ENDC}")

# Done!
print(f"{bcolors.OKBLUE}" +
      "Todas as rodadas foram concluidas" + f"{bcolors.ENDC}")
