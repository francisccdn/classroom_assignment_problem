# Minimizing energy consumption in a real-life classroom assignment problem<!-- omit in toc -->

### Table of Contents<!-- omit in toc -->
- [1. Instances](#1-instances)
  - [1.1. Folder structure](#11-folder-structure)
  - [1.2. Classes file structure](#12-classes-file-structure)
    - [1.2.2. Time slot notation](#122-time-slot-notation)
  - [1.3. Locations file structure](#13-locations-file-structure)
- [2. Manual Solutions](#2-manual-solutions)
  - [2.1. Manual solution file structure](#21-manual-solution-file-structure)
- [3. Optimal Solutions](#3-optimal-solutions)
  - [3.1. Optimal solution file structure](#31-optimal-solution-file-structure)

## 1. Instances
### 1.1. Folder structure
All instances can be found in folder **/instances**. Each instance has its own (sub)folder with an identifiable name. In it are found five files:
- **classes_classroom.json** — Contains data pertaining to classes which require only classrooms;
- **classes_pc.json** — Contains data pertaining to classes which require computer labs;
- **locations_classroom.json** — Contains data pertaining to locations classified as classrooms or hybrid locations;
- **locations_pc.json** — Contains data pertaining to locations classified as computer labs or hybrid locations;
- **occupied_locations.json** — List of timeslot-location pairs, each of which denote a location unavailable for assignments during that timeslot. This are used to represent cases where the lectures of some specific subjects must be assigned to specific locations, and as such are not considered in our instances.

All of these files are formatted as [JSON](https://www.json.org/json-en.html) objects, and their structure is detailed in subsections [1.2.](#12-classes-file-structure) and [1.3.](#13-locations-file-structure)

### 1.2. Classes file structure
Both **classes_classroom.json** and **classes_pc.json** follow the same structure.

Each name/value pair represents a class, where the name is that class' ID number, and the value is an object (hereafter called class data) containing relevant data for that class. The following describe what each element of class data represents.
- **lecture_times_condensed** — A single string which represents all lecture times of this class. The notation used for this is described in subsection [1.2.2.](#122-time-slot-notation)
- **subject** — Contents of a lecture (e.g., Electricity I, Calculus II, Sociology).
- **lectures** — Each element is an object representing a lecture, where its name is that lecture's ID number, and its value is another object containing data relevant to that class. The only data included is in which time slot that lecture takes place, using the notation described in subsection [1.2.2.](#122-time-slot-notation)
- **group_id** — If this class belongs to a TC group, this value is that group's ID number. Otherwise, this value is 0.
- **course_id** — ID number for the class' course, i.e. a set of subjects that are part of a curriculum.
- **subject_code** — ID code for the subject in IFPB's information system.
- **qty_students** — How many students are enrolled in this class.
- **id** — ID number for the class itself.

#### 1.2.1. Class data example<!-- omit in toc -->
```
"11702": {
  "lecture_times_condensed": "6M23",
  "subject": "Recursos Energéticos",
  "lectures": {
    "29278": { "timeslot": "6M2" },
    "29279": { "timeslot": "6M3" }
  },
  "group_id": 0,
  "course_id": 61,
  "subject_code": "TEC.0472",
  "qty_students": 16,
  "id": 11702
}
```
#### 1.2.2. Time slot notation
A time slot string has three sections, divided by a letter:
1. The number that comes before the letter represents which day of the week the lecture takes place in, where 1 is Sunday, 2 is Monday, and so on;
2. The letter in the middle of the string represents in which shift the lecture takes place in, where M is morning, V is afternoon, and N is night; 
3. The numbers after the letter represent during which time slot of that shift the lecture will take place, where 1 is the 1st time slot, 2 is the 2nd, and so on.

For example, "6M23" means this class' lectures will take place on Fridays during the 2nd and 3rd morning time slots.

### 1.3. Locations file structure
Both **locations_classroom.json** and **locations_pc.json** follow the same structure.

Each name/value pair represents a location, where the name is that location's ID number, and the value is an object (hereafter called location data) containing relevant data for that location. The following describe what each element of location data represents.
- **type** — Whether location is a classroom (1), computer lab (2) or hybrid (3).
- **name** — Location's name (e.g. Classroom 3, Materials Lab).
- **cost_per_lecture** — Energy cost of having a lecture take place in this location, in kWh.
- **block_id** — ID number for the academic block in which the location is situated.
- **pc_cost_per_lecture** — Energy cost of using a single computer for the duration of a lecture in this location, in kWh. This value can be null if there are no computers in the location.
- **qty_pc** — How many computers are available in this location. This value can be null if there are no computers in the location.
- **id** — ID number for the location itself. 
- **qty_seats** — How many students this location can hold without a computer available to them. This value can be null if all seats have computers associated with them.
- **volume** — Location's air volume, in m³.
- **air_conditioning** — Each element represents an AC present in the location. For each AC, **power** shows its power in W, and **btu** shows the its capacity for heat removal in BTU (this is converted to kW in setup cost calculation).
- **setup_cost** — Energy cost of cooling down this location to a comfortable temperature (from 30⁰C to 23⁰C). 
- **setup_duration** — Time, in fractions of lecture, required to cool down location to a comfortable temperature.

#### 1.3.1. Location data example<!-- omit in toc -->
```
"282": {
  "type": 1,
  "name": "SALA DE AULA 09",
  "cost_per_lecture": 3.516666667,
  "block_id": 57,
  "pc_cost_per_lecture": null,
  "qty_pc": null,
  "id": 282,
  "qty_seats": 46,
  "volume": 182.27,
  "air_conditioning": {
    "ac_1": { "power": 2440, "btu": 27000 },
    "ac_2": { "power": 2320, "btu": 27000 }
  },
  "setup_cost": 0.12328629937848165,
  "setup_duration": 0.031080579675247477
}
```
## 2. Manual Solutions
All manual solutions (i.e. solutions obtained through the algorithm used in practise) can be found in folder **/manual_solutions**. Each term has an associated solution, and each solution is shown in a file with an identifiable name. Solution files are formatted as [JSON](https://www.json.org/json-en.html) objects, and their structure is detailed in subsection [2.1.](#21-manual-solution-file-structure)

### 2.1. Manual solution file structure
Each name/value pair represents a lecture that took place in that academic term, where the name is that lecture's ID number, and the value is an object (hereafter called lecture data) containing relevant data for that lecture. The following describe what each element of lecture data represents.
- **timeslot** — In which time slot that lecture takes place, using the notation described in subsection [1.2.2.](#122-time-slot-notation)
- **requires** — Whether lecture requires only a regular classroom (**classroom**) or a computer lab (**pc**). 
- **location_id** — ID number of the location to which that lecture was assigned.
- **class_id** — ID number of the class that lecture belong to.

#### 2.1.1. Lecture data example<!-- omit in toc -->
```
"32239": {
  "timeslot": "5M2",
  "requires": "pc",
  "location_id": 360,
  "class_id": 11600
}
```

## 3. Optimal Solutions
Optimal solutions for each scenario/term (with the exception of scenario 9 of term 20182, whose solution is not optimal but still available) can be found in folder **/solutions**. Each solution is shown in a file with an identifiable name. The naming scheme for each file is **[term]_[scenario].json**. Solution files are formatted as [JSON](https://www.json.org/json-en.html) objects, and their structure is detailed in subsection [3.1.]

### 3.1. Optimal solution file structure
A solution object contains 6 elements of "metadata" about the solution, these are:
- **instance** — The term/instance of this solution.
- **gap** — Solution's gap reported by the solver.
- **scenario** — The scenario of this solution.
- **time preprocessing** — Preprocessing time necessary to build all sets as well as the CPLEX model in seconds.
- **time solver** — Time solver took to find solution in seconds.
- **solution value** — Objective value of the solution as reported by the solver. This is the weekly energy consumption of the solution, in kWh.

In addition to these, the value associated with the name **solution** contains the assignments made in this solution. It is an array of objects, each of which represent a lecture (hereafter called lecture data), and contain relevant data for that lecture. The following describe what each element of lecture data represents.
- **timeslot** — In which time slot that lecture takes place, using the notation described in subsection [1.2.2.](#122-time-slot-notation)
- **requires** — Whether lecture requires only a regular classroom (**classroom**) or a computer lab (**pc**). 
- **location_id** — ID number of the location to which that lecture was assigned.
- **class_id** — ID number of the class that lecture belong to.

#### 3.1.1. Solution object example<!-- omit in toc -->
```
{
  "instance": "20181",
  "gap": 9.773514159289885e-7,
  "scenario": 1,
  "time preprocessing": 2.358294807,
  "time solver": 176.834308068,
  "solution value": 8580.880272794515,
  "solution": [
    {
      "type": "classroom",
      "class_id": 11375,
      "timeslot": "6N3",
      "location_id": 323
    },
    ...
  ]
}
```
