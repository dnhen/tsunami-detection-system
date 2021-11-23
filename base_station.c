#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <pthread.h>
#include "only_main.h"
#include "satellite.h"

#define TIMETOLERANCE 30

double **satelliteLog;
int pthreadStop = 0, totalMessagesSent;
pthread_mutex_t satelliteMutex = PTHREAD_MUTEX_INITIALIZER;
extern MPI_Datatype AlertReportType;

const int SATELLITELOGLENGTH = 8, MIN_SEA_LEVEL = 0, MAX_SEA_LEVEL = 500, SLEEP_TIME = 3, STOPTAG = 40, REPORTTAG = 30, SEALTOLERANCE = 100;
const char *KEYPERFMETRICFILE = "keyperformancemetrics.txt",  *BASESTATIONLOGFILENAME = "basestationlog.txt";


// Base Station
int BaseStation(MPI_Comm masterComm, MPI_Comm comm, int argM, int argN, int argThreshold){
	int crank;
	MPI_Comm_rank(comm, &crank);

	int msize, m, n, threshold, i, y, stop = 0, iter = 0, totalAlerts = 0, totalTrueAlerts = 0, totalFalseAlerts = 0;
	m = argM;
	n = argN;
	threshold = argThreshold;

	printf("-= Welcome to the Tsunami Detection System =-\n");
	printf("   Created by Daniel and Xuguang\n");

	MPI_Comm_size(masterComm, &msize); // get the size of all the processes

	// Prompt user for grid size
    printf("Size of grid for m (m x n): %d\n", m);
    printf("Size of grid for n (m x n): %d\n", n);
    if(msize != m * n + 1){ // If not minimum processes are running (grid + base + satellite)
        printf("Please run with %d processes.\n", m * n + 1);
        MPI_Abort(masterComm, 1);
    }

    // Prompt user for threshold height
    printf("Threshold for sea level (between %d and %d): %d\n", MIN_SEA_LEVEL, MAX_SEA_LEVEL, threshold);
    if(threshold >= MAX_SEA_LEVEL || threshold <= MIN_SEA_LEVEL){ // If the threshhold not appropriate
        printf("Threshold of the sea level should within the range from %d to %d.\n", MIN_SEA_LEVEL, MAX_SEA_LEVEL);
        MPI_Abort(masterComm, 1);
    }

    double start = MPI_Wtime(); // store start time

    // Allocate mem for global satellite log
    satelliteLog = malloc(SATELLITELOGLENGTH * sizeof(double *));
    for(i = 0; i < SATELLITELOGLENGTH; i++){
    	satelliteLog[i] = calloc(4, sizeof(double));
    }

    // Create satellite
    SatelliteArgs args;
	args.masterComm = masterComm;
	args.comm = comm;
	args.threshold = threshold;
	args.m = m;
	args.n = n;

	pthread_t satelliteThread;
	pthread_mutex_init(&satelliteMutex, NULL);
	int satelliteThreadErr = pthread_create(&satelliteThread, NULL, Satellite, (void *)&args);

	if(satelliteThreadErr != 0){
		printf("TDS // BASE: ERROR.. unable to create satellite thread..");
		exit(-1);
	}

    int forSlaves[] = {m, n, threshold};

    // Send the grid size and threshold to each slave process
    for(i = 1; i < msize; i++){
    	totalMessagesSent++;
        MPI_Send(&forSlaves, 3, MPI_INT, i, 0, masterComm);
    }

    while(!stop){
    	char shutdownInFile = '0';
    	FILE *shutdownFile = fopen("shutdown.txt", "r");
    	if(shutdownFile != NULL){
    		shutdownInFile = fgetc(shutdownFile);
    		fclose(shutdownFile);
    	}

    	if(shutdownInFile == '1'){ // user has changed shutdown file to 0 to shutdown
    		stop = 1;
    		pthreadStop = 1;
    		printf("TDS // BASE: sending termination signal to satellite and detectors...\n");
    	}

    	// Continually send stop message to all processes
    	for(i = 1; i < msize; i++){
			MPI_Request request;
			// Send stop message to all threads
			totalMessagesSent++;
			MPI_Isend(&stop, 1, MPI_INT, i, STOPTAG, masterComm, &request);
		}

		if(shutdownInFile == '1'){
    		sleep(SLEEP_TIME * 2);
			free(satelliteLog);
			pthread_mutex_destroy(&satelliteMutex);

    		FILE *f = fopen(KEYPERFMETRICFILE, "w");
			fprintf(f, "-------------------------KEY-PERFORMANCE-METRICS-------------------------\n");
			fprintf(f, "Simulation time: %.2f\n", MPI_Wtime() - start);
			fprintf(f, "Iterations: %d\n", iter);
			fprintf(f, "Number of alerts detected:                        %d\n", totalAlerts);
			fprintf(f, "                         : Matched (or True):     %d\n", totalTrueAlerts);
			fprintf(f, "                         : Miss-match (or False): %d\n", totalFalseAlerts);
			fprintf(f, "Number of messages/events with sender's adjacency information: %d\n", totalAlerts);
			fprintf(f, "Total number of messages sent: %d\n", totalMessagesSent);
			fprintf(f, "-------------------------------------------------------------------------\n");
			fclose(f);
    		printf("TDS // BASE: key performance metrics logged...\n");

    		printf("\nTDS // BASE: satellite and detectors shut down...\n");
    		printf("\n-= Thank you for using the Tsunami Detection System =-\n");
    		printf("   Goodbye!\n");

    		return 0;
    	}

    	iter++;
    	printf("\nTDS // BASE: --------- ITERATION %d ---------\n", iter);

    	// Listen for reports
	    for(i = 1; i < msize; i++){
	    	AlertReport report;

	    	MPI_Recv(&report, 1, AlertReportType, i, REPORTTAG, masterComm, MPI_STATUS_IGNORE);

	    	if(report.nodeRankMaster != -10){ // There is a report filed
	    		totalAlerts++;
	    		printf("TDS // BASE: WARNING: received report from node (cart: %d, master: %d)\n", report.nodeRankCart, report.nodeRankMaster);
	    		
	    		// Check with the shared global array (satellite) if there is a match
	    		int matched = 0, x, sateIndex = -1;

	    		for(x = 0; x < SATELLITELOGLENGTH; x++){
	    			if(report.nodecoords[0] == satelliteLog[x][1] && report.nodecoords[1] == satelliteLog[x][2] // Coords match
    				    && abs(report.alertTimestamp - satelliteLog[x][3]) <= TIMETOLERANCE // Time is within tolerance
						&& abs(report.nodeSeaLevel - satelliteLog[x][0]) <= SEALTOLERANCE){ // Sea level is within tolerance
    						sateIndex = x;
    						matched = 1;
    						break;
    				}
	    		}

	    		// Get report time & curr time
	    		struct tm *tmptime;
	    		time_t timestamp = (time_t)report.alertTimestamp;
	    		tmptime = localtime(&timestamp);
	    		char alertTime[256] = {0};
	    		strftime(alertTime, 256, "%a %F %T", tmptime);

	    		timestamp = time(NULL);
	    		tmptime = localtime(&timestamp);
	    		char currTime[256] = {0};
	    		strftime(currTime, 256, "%a %F %T", tmptime);

	    		// Open log file
				FILE *f = fopen(BASESTATIONLOGFILENAME, "a");
				fprintf(f, "------------------------------------------------------------------------\n");
				fprintf(f, "Iteration: %d\n", iter);
				fprintf(f, "Logged time:                        %s\n", currTime);
				fprintf(f, "Alert report time:                  %s\n", alertTime);

	    		if(matched){ // true alert
	    			totalTrueAlerts++;
	    			printf("TDS // BASE: DANGER: true tsunami alert detected. Logging to file...\n");
	    			
	    			// log the report as a matched event (true alert)
	    			fprintf(f, "Alert type: Match (or True)\n\n");

	    		} else { // false alert
	    			totalFalseAlerts++;
	    			printf("TDS // BASE: OK: false tsunami alert detected. Logging to file...\n");
	    			
	    			// log the report as a miss-matched event (false alert)
	    			fprintf(f, "Alert type: Miss-match (or False)\n\n");
	    		}

	    		fprintf(f, "Reporting Node        Coord         Height(m)          IPv4\n");
	    		fprintf(f, "%d                     (%d, %d)        %.2f             %s (%s)\n\n", report.nodeRankCart, report.nodecoords[0], report.nodecoords[1], report.nodeSeaLevel, report.nodeIP, report.processName);

	    		fprintf(f, "Adjacent Nodes        Coord         Height(m)          IPv4\n");
	    		for(y = 0; y < 4; y++){
	    			if(report.neighbourRanks[y] >= 0){
	    				char neighbourIP[16];
		    			sprintf(neighbourIP, "116.182.167.1%d", report.neighbourRanks[y]);
						fprintf(f, "%d                     (%d, %d)        %.2f             %s (%s)\n", report.neighbourRanks[y], report.neighbourCoords[y][0], report.neighbourCoords[y][1], report.neighbourNodeSeaLevel[y], neighbourIP, report.neighbourProcessNames[y]);
	    			}
	    		}

	    		fprintf(f, "\n");

	    		if(matched){
	    			timestamp = (time_t)satelliteLog[sateIndex][3];
		    		tmptime = localtime(&timestamp);
		    		char sateTime[256] = {0};
		    		strftime(sateTime, 256, "%a %F %T", tmptime);

	    			fprintf(f, "Satellite altimeter reporting time: %s\n", sateTime);
	    			fprintf(f, "Satellite altimeter reporting height(m): %.2f\n", satelliteLog[sateIndex][0]);
	    			fprintf(f, "Satellite altimeter reporting coord: (%.0f, %.0f)\n\n", satelliteLog[sateIndex][1], satelliteLog[sateIndex][2]);
	    		}

	    		fprintf(f, "Communication time (seconds): %f\n", report.alertTimeTaken);
	    		fprintf(f, "Total messages sent between reporting node and base station: 1\n");
	    		fprintf(f, "Number of adjacent matches to reporting node: %d\n", report.numberNodesCompared);
	    		fprintf(f, "Max. tolerance range between nodes readings(m): %d\n", SEALTOLERANCE);
	    		fprintf(f, "Max. tolerance range between satellite altimeter and reporting node readings(m): %d\n", SEALTOLERANCE);

	    		fprintf(f, "------------------------------------------------------------------------\n");
	    		fclose(f); // close log file		
			}
	    }
    }
	return 0;
}



