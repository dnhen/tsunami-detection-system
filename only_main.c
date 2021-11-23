#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "base_station.h"
#include "only_main.h"
#include "node.h"


MPI_Datatype AlertReportType;
extern const char *BASESTATIONLOGFILENAME;

int main(int argc, char **argv){
	int rank, provided;
	MPI_Comm new_comm;

	if(argc != 4){
		printf("Please put the m, n, and threshold in the command line when executing the file.\n");
		printf("e.g. output 5 5 300 (5x5 grid, 300 threshold)\n");
		return 0;
	}

	int argM = atoi(argv[1]), argN = atoi(argv[2]), argThreshold = atoi(argv[3]), i;

   	MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	srand(time(NULL) + rank); // Initialise random number

	// Create alert report type
	AlertReport offsetCalcs;
	MPI_Aint AlertReportAddresses[13];
	MPI_Aint offsets[13];
	MPI_Get_address(&offsetCalcs, &AlertReportAddresses[0]);
	MPI_Get_address(&offsetCalcs.nodeRankCart, &AlertReportAddresses[1]);
	MPI_Get_address(&offsetCalcs.numberNodesCompared, &AlertReportAddresses[2]);
	MPI_Get_address(&offsetCalcs.nodeSeaLevel, &AlertReportAddresses[3]);
	MPI_Get_address(&offsetCalcs.nodeIP, &AlertReportAddresses[4]);
	MPI_Get_address(&offsetCalcs.neighbourRanks, &AlertReportAddresses[5]);
	MPI_Get_address(&offsetCalcs.neighbourNodeSeaLevel, &AlertReportAddresses[6]);
	MPI_Get_address(&offsetCalcs.alertTimeTaken, &AlertReportAddresses[7]);
	MPI_Get_address(&offsetCalcs.alertTimestamp, &AlertReportAddresses[8]);
	MPI_Get_address(&offsetCalcs.nodecoords, &AlertReportAddresses[9]);
	MPI_Get_address(&offsetCalcs.processName, &AlertReportAddresses[10]);
	MPI_Get_address(&offsetCalcs.neighbourProcessNames, &AlertReportAddresses[11]);
	MPI_Get_address(&offsetCalcs.neighbourCoords, &AlertReportAddresses[12]);
	offsets[0] = 0;
	for(i = 1; i < 13; i++){
		offsets[i] = AlertReportAddresses[i] - AlertReportAddresses[0];
	}

	int lengths[13] = {1, 1, 1, 1, 16, 4, 4, 1, 1, 2, MPI_MAX_PROCESSOR_NAME, MPI_MAX_PROCESSOR_NAME * 4, 8};
	MPI_Datatype types[13] = {MPI_INT, MPI_INT, MPI_INT, MPI_FLOAT, MPI_CHAR, MPI_INT, MPI_FLOAT, MPI_DOUBLE, MPI_INT, MPI_INT, MPI_CHAR, MPI_CHAR, MPI_INT};
	MPI_Type_create_struct(13, lengths, offsets, types, &AlertReportType);
	MPI_Type_commit(&AlertReportType);

	// Clear log file
	FILE *f = fopen(BASESTATIONLOGFILENAME, "w");
	fclose(f);

	// Create empty shutdown file
	FILE *sdFile = fopen("shutdown.txt", "w");
	if(sdFile != NULL){
		fprintf(sdFile, "%d", 0);
		fclose(sdFile);
	}

	MPI_Comm_split(MPI_COMM_WORLD, rank == 0, 0, &new_comm); // base station and satellite

	if(rank == 0){ // base station & satellite
		BaseStation(MPI_COMM_WORLD, new_comm, argM, argN, argThreshold);
	} else if(rank >= 1) { // (first rank = base, second rank = satellite)
		Node(MPI_COMM_WORLD, new_comm);
	}

	sleep(1);
	MPI_Comm_free( &new_comm );

	MPI_Finalize();
	return 0;
}







