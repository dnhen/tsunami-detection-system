#ifndef __SATELLITE_H_

#define __SATELLITE_H_

#include<stdio.h>
#include <pthread.h>
#include <mpi.h>

void *Satellite(void *satArgs);

typedef struct{
	MPI_Comm masterComm;
	MPI_Comm comm;
	int threshold;
	int m;
	int n;
}SatelliteArgs;

#endif