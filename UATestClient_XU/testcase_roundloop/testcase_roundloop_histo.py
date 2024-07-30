from ast import Not
import numpy as np
import matplotlib.pyplot as plt
import array
from matplotlib.patches import Patch


quantity = input("Enter quantity of signals: ")

X_data = list(range(1, int(quantity)+1))
Y_data = array.array('f')


file1 = open('testcase_roundloop_1.txt', 'r')
for line in file1:
    if not(line.__contains__("#")):
        line = line.strip('\n') #entferne \n
        line = line[line.rfind(" "):len(line)] #schneide bis Leerzeichen ab und nimm hinteren Teil
        Y_data.append(float(line)/1000.0)

file1.close()  

plt.figure(figsize=(12, 6))

#plt.scatter(X_data, Y_data, c='blue', label='signals')
plt.hist(Y_data, bins=30, color='blue', edgecolor='black')

#average_time = round(sum(Y_data)/float(quantity),2)
#plt.axvline(x=average_time, color='red', linestyle='-', label=f'average_duration = {average_time} ms')

# Statistics:
median = np.median(Y_data)
median_line = plt.axvline(median, color='red', linestyle='dashed', linewidth=2, label=f'Median: {median:.2f} ms')

mean = np.mean(Y_data)
mean_line = plt.axvline(mean, color='orange', linestyle='dashed', linewidth=2, label=f'Mean: {mean:.2f} ms')

min_duration = np.min(Y_data)
minduration_line = plt.axvline(min_duration, color='green', linestyle='dashed', linewidth=2, label=f'Shortest duration: {min_duration:.2f} ms')

max_duration = np.max(Y_data)
maxduration_line = plt.axvline(max_duration, color='red', linestyle='dashed', linewidth=2, label=f'Longest duration: {max_duration:.2f} ms')

std_dev = np.std(Y_data)
# Bereich der ersten Standardabweichung hervorheben
highlight = plt.axvspan(mean - std_dev, mean + std_dev, color='orange', alpha=0.3)

# Zusätzlichen Eintrag für den hervorgehobenen Bereich zur Legende hinzufügen
highlight_patch = Patch(color='orange', alpha=0.3, label='Standard Deviation')

# Legende hinzufügen, einschließlich des neuen Eintrags für den hervorgehobenen Bereich
plt.legend(handles=[median_line, mean_line, minduration_line, maxduration_line, highlight_patch])
plt.ylabel('Number of operations')
plt.xlabel('durations[ms]')

plt.title('Time measurement of a loop between sending and receiving a signal')

plt.savefig("testcase_roundloop_histo.pdf", format="pdf");

#plt.grid(True)

plt.show()

  
