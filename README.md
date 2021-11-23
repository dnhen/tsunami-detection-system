# Tsunami Detection System

Team project implemented in the C language using POSIX threads and OpenMPI.

A simulated tsunami detection system featuring nodes communicating with a satellite and a base station to broadcast alerts when a tsunami is detected.

![image](https://user-images.githubusercontent.com/69449713/142986317-68989af2-7cb3-435d-85f5-68d1f9cf6272.png)

## System Design

The tsunami detection system works by each node, the satellite, and the base station sending messages to each other to communicate different data points and make decisions based on the data received.

Each time interval, the nodes located in the water generate a random water column height reading (between 0m and 500m), and calculates a moving average with this random height. If the moving average exceeds a user-defined threshold, then the communication begins. The node sends it's moving average to all of the neighbouring nodes (up, down, left, right) and also requests the moving average from the neighbouring nodes.

Once the neighbouring nodes receive the moving average from the original node, they compare this to their own moving average. If they are both within a predefined threshold of each other, the node sends their reading back to the original node; otherwise, a null reading is sent back.

If at least two neighbours have sent their values back (values fell within predefined threshold), then an alert is going to be broadcast to the base station. The node compiles all of the neighbours information (coordinates, reading, IP, etc) and it's own information (log time, time taken to communicate, etc) and this is sent to the base station as an alert package.

The base station receives the alert, and requests a height reading from the satellite. The satellite is constantly generating random sea height values for each node and storing it in an array. The satellite finds the value for the required node, and sends the value to the base station. If this value is within a predefined threshold, the alert is assumed to be a true alert and it is logged as such. If the value is not within the threshold, then it is assumed to be a false alert and logged as such.

## System Termination

The system will run until a termination signal is received. When the system starts, a "shutdown.txt" file is created. To shut the system down, change the integer in this file to 1. This will shut the system down on the next cycle run.

When the system is shut down, you can view the base station log file (which includes all of the alerts), and also view the key performance metrics (which contains key information about the system).

## System Execution
* Execute the command: make
* Execute the command: make run proc=\<# processors\> n=\<n\> m=\<m\> t=\<threshold\>
  * \# processors = how many processors you want to run with
  * n = the int for the grid size for a grid n x m
  * m = the int for the grid size for a grid n x m
  * t = the threshold for the water level to not go over till it broadcasts an alert
  * e.g. make run proc=5 n=2 m=2 t=300
