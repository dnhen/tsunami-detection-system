#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <pthread.h>
#include "only_main.h"
#include "functions.h"
#include "satellite.h"
#include "base_station.h"

extern int pthreadStop;
extern const int SLEEP_TIME, MAX_SEA_LEVEL, SATELLITELOGLENGTH;
extern double **satelliteLog;

// Satellite
void *Satellite(void *satArgs){
	SatelliteArgs *args = satArgs;
	MPI_Comm masterComm = args->masterComm;

	// Get # of nodes
	int numOfNodes;
	MPI_Comm_size(masterComm, &numOfNodes);
	numOfNodes--; // (-1 for root node)

	// Get m and n (m x n)
	int m = args->m, n = args->n;

	printf("TDS // SAT: satellite online!\n");

	while(!pthreadStop){
		sleep(SLEEP_TIME);

		float randNum = generateFloatValue(args->threshold, MAX_SEA_LEVEL);

		pthread_mutex_lock(&satelliteMutex); // to prevent race condition

		int i;
		for(i = SATELLITELOGLENGTH - 1; i > 0; i--){
			int x;
			for(x = 0; x < 4; x++){
				satelliteLog[i][x] = satelliteLog[i-1][x];
			}
		}

		satelliteLog[0][0] = randNum; // Store sea level
		satelliteLog[0][1] = generateInt(0, m-1); // Store coord [m,  ]
		satelliteLog[0][2] = generateInt(0, n-1); // Store coord [ , n]
		satelliteLog[0][3] = (int)time(NULL);

		pthread_mutex_unlock(&satelliteMutex);
	}

	if(pthreadStop){ // If termination signal was found
    	printf("TDS // SAT: received termination signal.. gracefully shutting down..\n");
		pthread_exit(NULL);
    }
	return 0;
}