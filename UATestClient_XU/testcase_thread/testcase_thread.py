from ast import Not
import numpy as np
import matplotlib.pyplot as plt
import array
import random


send_frequency = input("Please enter the send frequency [in ms]: ")
quantity = input("Please enter the quantity of signals: ")

received_signals = array.array('f')
sent_signals = array.array('f')


# Get array of all signal values
file2 = open('testcase_thread_sent_signals.txt', 'r')
for line in file2:
    line = line.strip('\n') #entferne \n
    time = line.split(":")[0]
    value = line.split(":")[1]
    sent_signals.append(float(value))

sent_signals = [round(x,2)for x in sent_signals]
    
file2.close()

# Get missing array of signal values
file1 = open('testcase_thread_received_signals.txt', 'r')
for line in file1:
    if not(line.__contains__("#")):
        line = line.strip('\n') #entferne \n
        time_and_value = line[line.rfind(" "):len(line)] #schneide bis Leerzeichen ab und nimm hinteren Teil
        time = time_and_value.split(":")[0]
        value = time_and_value.split(":")[1]
        received_signals.append(round(float(value),2))

received_signals = [round(x,2)for x in received_signals]
       
file1.close()

# Identify missing signals
missing_signals = list(set(sent_signals) - set(received_signals))

# Indices of the sent signals
sent_indices = list(range(1, (int(quantity)+1)))

print(sent_signals)
# Indices of the received signals
received_indices = [sent_indices[sent_signals.index(signal)] for signal in received_signals]

# Indices of the missing signals
missing_indices = [sent_indices[sent_signals.index(signal)] for signal in missing_signals]

plt.figure(figsize=(12, 6))

# sent signals plot
plt.scatter(sent_indices, sent_signals, c='blue', label=f'sent signals ({len(sent_signals)})')

# received signals plot
plt.scatter(received_indices, received_signals, c='green', label=f'received signals ({len(received_signals)})')

# mark missing signals
plt.scatter(missing_indices, missing_signals, c='red', label=f'missing signals ({len(missing_signals)})', marker='x')

plt.xlabel('signal number')
plt.ylabel('signal value')
plt.title(f'Comparison of sent and received signals with send frequency {send_frequency} in [ms]')
plt.legend()
plt.grid(True)

# Plot als PNG speichern
plt.savefig(f'comparison_sent_received_signals_{send_frequency}.pdf', format = "pdf")

# Plot anzeigen
plt.show()


  
