#ifndef __BASE_STATION_H_

#define __BASE_STATION_H_

#include<stdio.h>
#include <pthread.h>

int BaseStation(MPI_Comm masterComm, MPI_Comm comm, int argM, int argN, int argThreshold);
extern pthread_mutex_t satelliteMutex;

#endif