#ifndef __ONLY_MAIN_H_

#define __ONLY_MAIN_H_

#include<stdio.h>
#include <pthread.h>

typedef struct{
	int nodeRankMaster, nodeRankCart, numberNodesCompared, alertTimestamp;
	int neighbourRanks[4], nodecoords[2], neighbourCoords[4][2];
	float nodeSeaLevel;
	float neighbourNodeSeaLevel[4];
	char nodeIP[16], processName[MPI_MAX_PROCESSOR_NAME], neighbourProcessNames[4][MPI_MAX_PROCESSOR_NAME];
	double alertTimeTaken;
}AlertReport;

extern MPI_Datatype AlertReportType;

#endif