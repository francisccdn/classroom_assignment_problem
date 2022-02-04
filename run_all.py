import subprocess
from subprocess import PIPE
import argparse
import os


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--timelim",
        type=int,
        default=600,
        help=
        "Time limit for solver, in minutes. 0 skips solver, < 0 is no time limit. (Default=600)"
    )
    parser.add_argument(
        "--colors",
        choices=['yes', 'no'],
        default='yes',
        help="Enables colors in program log. (Default=yes)"
    )
    parser.add_argument(
        "--scenario",
        type=int,
        default=0,
        help="Specify a scenario to run. 0 runs all scenarios. (Default=0)"
    )
    parser.add_argument(
        "--instance",
        type=str,
        default="default",
        help=
        "Specify an instance to run. 'default' runs '20181', '20182' and '20191'. (Default='default')"
    )

    return parser.parse_args()


def main(args):
    class bcolors:
        HEADER =    '\033[95m' if args.colors == 'yes' else ''
        OKBLUE =    '\033[94m' if args.colors == 'yes' else ''
        OKCYAN =    '\033[96m' if args.colors == 'yes' else ''
        OKGREEN =   '\033[92m' if args.colors == 'yes' else ''
        WARNING =   '\033[93m' if args.colors == 'yes' else ''
        FAIL =      '\033[91m' if args.colors == 'yes' else ''
        ENDC =      '\033[0m'  if args.colors == 'yes' else ''
        BOLD =      '\033[1m'  if args.colors == 'yes' else ''
        UNDERLINE = '\033[4m'  if args.colors == 'yes' else ''

    subprocess.run(["make"])

    print(
        f"{bcolors.HEADER}" +
        "........................| Classroom Assignment Problem |......................."
        + f"{bcolors.ENDC}")

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

    instances = [20181, 20182, 20191]
    if args.instance != "default":
        instances = [args.instance]

    scenarios = range(1, 11)
    if args.scenario != 0:
        scenarios = [args.scenario]

    for instance in instances:
        for scenario in scenarios:
            # for heuristic in range(0, 2):
            heuristic = 0
            setup_before = 1

            cmd = []
            time_limit_str = str(args.timelim) if heuristic == 0 else "0"

            # With setup - setup before class
            # not running no setup or setup during class, as they are not considered in paper
            if [instance, scenario, 1, setup_before, heuristic] not in found:
                print(f"{bcolors.OKBLUE}" + "Rodando -- Instancia: " +
                      str(instance) + " Cenario: " + str(scenario) +
                      " Heuristica: " + str(heuristic) + " Setup antes: " +
                      str(setup_before) + f"{bcolors.ENDC}")

                cmd = ['./cap', str(instance), str(scenario), "1", str(setup_before), time_limit_str, str(heuristic)]
                logname = str(instance) + "_" + str(scenario) + "_setup"

                logname = logname + "_duringclass" if setup_before == 0 else logname
                logname = logname + "_heuristic" if heuristic == 1 else logname

                # Run cap
                result = subprocess.run(cmd, stdout=PIPE, stderr=PIPE)
                # Save log to file
                with open("results_log/" + logname + ".log", 'w') as f:
                    f.write(result.stdout.decode("utf8"))

            else:  # If results were already found
                print(f"{bcolors.OKGREEN}" + "Instancia: " + str(instance) +
                      " Cenario: " + str(scenario) + " Heuristica: " +
                      str(heuristic) + " Setup antes: " + str(setup_before) +
                      " \t Resultados gerados encontrados" + f"{bcolors.ENDC}")

    # Done!
    print(f"{bcolors.OKBLUE}" + "Todas as rodadas foram concluidas" +
          f"{bcolors.ENDC}")

if __name__ == "__main__":
    args = parse_args()
    main(args)