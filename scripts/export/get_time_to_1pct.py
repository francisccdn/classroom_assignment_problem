import json
import os

log_path = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))) + '/results_log'
all_results_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))) + '/results'

for file in os.listdir(log_path):
    filename = os.fsdecode(file)
    filepath = os.path.join(log_path, filename)

    jsonfilepath = os.path.join(
        all_results_dir, filename.split('.')[0] + ".json")

    timeTo1Pct = 9999999
    timeToDot1Pct = 9999999

    with open(filepath, 'r') as log:
        lines = log.readlines()

        i = 0
        for line in lines:
            if line.startswith("Elapsed time"):
                equalpos = line.find('=')
                secpos = line.find('sec', equalpos)

                time = float(line[equalpos+1:secpos])

                pctline = lines[i + 1]
                if pctline == "\n":
                    pctline = lines[i - 1]

                pctstart = pctline.rfind(' ')
                pctend = pctline.rfind('%')
                try:
                    pct = float(pctline[pctstart:pctend])
                except ValueError:
                    pct = 9999999

                if pct <= 1 and timeTo1Pct > time:
                    timeTo1Pct = time
                if pct <= 0.1 and timeToDot1Pct > time:
                    timeToDot1Pct = time

            i += 1

    # print(filename + "Time to < 1%: " + str(timeTo1Pct) + " Time to < 0.1%: " + str(timeToDot1Pct))
    with open(jsonfilepath, 'r+') as f:
        data = json.load(f)
        data["time to 1%"] = timeTo1Pct
        data["time to 0.1%"] = timeToDot1Pct
        f.seek(0)
        f.truncate(0)
        json.dump(data, f)