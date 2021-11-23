# Tsunami Detection System

Team project implemented in the C language using POSIX threads and OpenMPI.

A simulated tsunami detection system featuring nodes communicating with satellites and a base station to broadcast alerts when a tsunami is detected.

**How to execute:**
* Execute the command: make
* Execute the command: make run proc=\<# processors\> n=\<n\> m=\<m\> t=\<threshold\>
  * \# processors = how many processors you want to run with
  * n = the int for the grid size for a grid n x m
  * m = the int for the grid size for a grid n x m
  * t = the threshold for the water level to not go over till it broadcasts an alert
  * e.g. make run proc=5 n=2 m=2 t=300
