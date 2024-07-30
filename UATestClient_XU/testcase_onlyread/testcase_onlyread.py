from ast import Not
import numpy as np
import matplotlib.pyplot as plt
import array


quantity = input("Enter quantity of signals: ")

X_data = list(range(1, int(quantity)+1))
Y_data = array.array('f')


file1 = open('testcase_onlyread.txt', 'r')
for line in file1:
    if not(line.__contains__("#")):
        line = line.strip('\n') #entferne \n
        line = line[line.rfind(" "):len(line)] #schneide bis Leerzeichen ab und nimm hinteren Teil
        Y_data.append(float(line)/1000.0)

file1.close()  

plt.figure(figsize=(12, 6))

plt.scatter(X_data, Y_data, c='blue', label='signals')

average_time = round(sum(Y_data)/float(quantity),2)
plt.axhline(y=average_time, color='red', linestyle='-', label=f'average_duration = {average_time} ms')

plt.xlabel('Number of operations')
plt.ylabel('duration[ms]')

plt.title('Time measurement of the reading process only')
plt.legend()

plt.savefig("testcase_onlyread.png");

plt.grid(True)

plt.show()

  
