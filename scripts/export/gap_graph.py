import os
import pandas as pd
import matplotlib.pyplot as plt

def get_gap_time_pairs(filepath):
    gap_time_pairs = [(0, 100)]
    
    def time_in_line(line):
        equalpos = line.find('=')
        secpos = line.find('sec', equalpos)
        return line[equalpos+1:secpos]

    def gap_in_line(line):
        gapstart = line.rfind(' ')
        gapend = line.rfind('%')
        return line[gapstart:gapend]

    with open(filepath, 'r') as log:
        lines = log.readlines()

        time_line_indexes = []
        root_line_index = 0

        i = 0
        for line in lines:
            if line.startswith("Root relaxation"):
                if root_line_index != 0:
                    i += 1
                    continue

                gapline = lines[i + 5]
                root_line_index = i

                time = float(time_in_line(line))
                gap = float(gap_in_line(gapline))

                gap_time_pairs.append((time, gap))

            if line.startswith("Elapsed time"):
                time_line_indexes.append(i)
            
            i += 1

        for j in range(0, len(time_line_indexes)):
            beg_time_line_idx = time_line_indexes[j - 1] if j > 0 else root_line_index
            end_time_line_idx = time_line_indexes[j]

            beg_time = float(time_in_line(lines[beg_time_line_idx]))
            end_time = float(time_in_line(lines[end_time_line_idx]))

            if beg_time_line_idx == root_line_index:
                beg_time_line_idx += 4

            del_time = end_time - beg_time
            del_j = end_time_line_idx - beg_time_line_idx

            time_increment = del_time / del_j

            k = 0
            for gapline in lines[beg_time_line_idx + 1 : end_time_line_idx]:

                if (not gapline.startswith("*") and not gapline.startswith(" ")) or "Node" in gapline:
                    k += 1
                    continue

                time = beg_time + (k * time_increment)
                gap = float(gap_in_line(gapline))

                gap_time_pairs.append((time, gap))
                
                k += 1
        
        final_time_line_idx = time_line_indexes[len(time_line_indexes) - 1]
        final_gapline = lines[final_time_line_idx + 1]
        if final_gapline == "\n" or final_gapline.startswith("Nodefile"):
            final_gapline = lines[final_time_line_idx - 1]
    
        final_time = float(time_in_line(lines[final_time_line_idx]))
        final_gap = float(gap_in_line(final_gapline))
        gap_time_pairs.append((final_time, final_gap))

    return gap_time_pairs


def save_scenario_graph(scenario, data, path):
    linestyle = {
        "2018.1": "c-.",
        "2018.2": "r--",
        "2019.1": "g:"
    }
    ax = None

    for instance in ["2018.1", "2018.2", "2019.1"]:
        df = pd.DataFrame(data[instance], columns=["Seconds", instance])
        next_ax = df.plot(ax=ax, x="Seconds", y=instance, style=[linestyle[instance]])
        ax = next_ax
    ax.set(xlabel="Seconds", ylabel="Gap")
    ax.set_title(f"Solution gap development over time - Scenario S{scenario}") 

    if max([max(data[instance])[0] for instance in data]) > 30000:
        ymin, ymax = ax.get_ylim()
        df = pd.DataFrame([(36000, ymax), (36000, ymin)], columns=["Seconds", "Time limit"])
        final_ax = df.plot(ax=ax, x="Seconds", y="Time limit", style=["k-"])
        final_ax.set_ylim(ymin, ymax)
        final_ax.legend(loc="upper right", bbox_to_anchor=(0.95, 1))

    plt.savefig(f"{path}/graph_s{scenario}.png")


def get_data_by_scenario(path):
    data = {}

    for file in os.listdir(path):
        filename = os.fsdecode(file)
        filepath = os.path.join(path, filename)

        pairs = get_gap_time_pairs(filepath)
        instancename = filename.split('_')[0]
        instancename = f"{instancename[:4]}.{instancename[4]}"
        scenario = filename.split('_')[1]

        if scenario not in data:
            data[scenario] = {}
        
        data[scenario][instancename] = pairs

    return data


def main():
    inpath = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))) + '/results_log'
    outpath = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))) + '/gap_development'

    data = get_data_by_scenario(inpath)
    for scenario in data:
        save_scenario_graph(scenario, data[scenario], outpath)


if __name__ == "__main__":
    main()
    