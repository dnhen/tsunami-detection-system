#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include "functions.h"
#include "only_main.h"


#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1
#define RQSTAVG 10
#define SENDAVG 20
#define SENDCOORDS 30
#define SENDPROCNAME 50


extern const int MIN_SEA_LEVEL, MAX_SEA_LEVEL, SLEEP_TIME, STOPTAG, REPORTTAG, SEALTOLERANCE;
extern MPI_Datatype AlertReportType;

// Tsunameter Sensor Node
int Node(MPI_Comm masterComm, MPI_Comm comm){
	MPI_Comm comm2D;
	int info[3], dims[2], periods[2] = {0, 0}, coords[2], recvVals[4], neighbourCoords[4][2];
	int i, m, n, threshold, csize, ndims = 2, ierr = 0, reorder = 1, mrank, crank, cartrank, stop = 0, iter = 0;
	float movings[3], neighbourAverages[4] = {0, 0, 0, 0};
	float movingAverage = 0;
	
	// Receive all the info from the base station
	MPI_Recv(&info, 3, MPI_INT, 0, 0, masterComm, MPI_STATUS_IGNORE);
    
    m = info[0];
    n = info[1];
    threshold = info[2];

    MPI_Comm_size(comm, &csize);

    // CREATE CART GRID
    // Size of cartesian grid (dimensions)
    dims[0] = m, dims[1] = n; // rows x cols
    MPI_Dims_create(csize, ndims, dims);

    // Create the cartesian grid
	ierr = MPI_Cart_create(comm, ndims, dims, periods, reorder, &comm2D);
	if(ierr != 0){
		printf("ERROR[%d] creating CART\n",ierr);
	}

	// Store rank for comm
	MPI_Comm_rank(masterComm, &mrank);
	MPI_Comm_rank(comm, &crank);

	MPI_Cart_coords(comm2D, crank, ndims, coords);
	MPI_Cart_rank(comm2D, coords, &cartrank);

	// Create node's IP
	char nodeIP[16];
	sprintf(nodeIP, "116.182.167.1%d", crank);

	// Get neighbour nodes
	int above, below, left, right;
	MPI_Cart_shift(comm2D, SHIFT_ROW, DISP, &above, &below);
	MPI_Cart_shift(comm2D, SHIFT_COL, DISP, &left, &right);
	int neighbours[] = {left, above, right, below};

	MPI_Request send_request[4];
    MPI_Request receive_request[4];
    MPI_Status send_status[4];
    MPI_Status receive_status[4];
    

	while(!stop){
		sleep(SLEEP_TIME);

		int numOfMovings = 0, reqNeighbours = 0, alert = 0, count = 0;
		double start = MPI_Wtime();
		iter++; // increment iteration
		float sum = 0, seaLevel = generateFloatValue(MIN_SEA_LEVEL, MAX_SEA_LEVEL);
		

		// Calculate current moving average by FIFO
		for(i = 2; i > 0; i--)
		{
			movings[i] = movings[i-1];
		}
		movings[0] = seaLevel;
		
		for(i = 0; i < 3; i++)
		{
			sum += movings[0];
			if(movings[0] > 0) numOfMovings += 1;
		}
		
		movingAverage = sum / numOfMovings;

		// sea level exceeded threshold
		if(movingAverage > threshold) reqNeighbours = 1; // request neighbours

		// Send a request to all neighbors for their readings
		for(i = 0; i < 4; i++){
			MPI_Isend(&reqNeighbours, 1, MPI_INT, neighbours[i], RQSTAVG, comm2D, &send_request[i]);
		}
		MPI_Waitall(4, send_request, send_status); // wait for all requests to be sent

		// Receive whether neighbours requiring readings or not
        for(i = 0; i < 4; i++){
            MPI_Irecv(&recvVals[i], 1, MPI_INT, neighbours[i], RQSTAVG, comm2D, &receive_request[i]);
        }
        MPI_Waitall(4, receive_request, receive_status); // wait for all requests to be received


        // sends own movings avg to the neighbours who has required for it
        for(i = 0; i < 4; i++){
            if(recvVals[i] == 1) MPI_Isend(&movingAverage, 1, MPI_FLOAT, neighbours[i], SENDAVG, comm2D, &send_request[i]);
        }
        MPI_Waitall(4, send_request, send_status);

        // receives the moving avgs from neighbours
        if(movingAverage > threshold){
            for(i = 0; i < 4; i++){
                if(neighbours[i] >= 0) MPI_Irecv(&neighbourAverages[i], 1, MPI_FLOAT, neighbours[i], SENDAVG, comm2D, &receive_request[i]);
            }
        }
        MPI_Waitall(4, receive_request, receive_status);

        // sends own coords avg to the neighbours who has required for it
        for(i = 0; i < 4; i++){
            if(recvVals[i] == 1) MPI_Isend(&coords, 2, MPI_INT, neighbours[i], SENDCOORDS, comm2D, &send_request[i]);
        }
        MPI_Waitall(4, send_request, send_status);

        // receives the coords from neighbours
        if(movingAverage > threshold){
            for(i = 0; i < 4; i++){
                if(neighbours[i] >= 0) MPI_Irecv(&neighbourCoords[i], 2, MPI_INT, neighbours[i], SENDCOORDS, comm2D, &receive_request[i]);
            }
        }
        MPI_Waitall(4, receive_request, receive_status);

        // sends own processor names to host of myself (my neighbour that wanted my reading)
        char procNameNeig[MPI_MAX_PROCESSOR_NAME] = {'\0'};
    	int nameLenNeig;
    	MPI_Get_processor_name(procNameNeig, &nameLenNeig);
        for(i = 0; i < 4; i++){
            if(recvVals[i] == 1) MPI_Isend(&procNameNeig, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, neighbours[i], SENDPROCNAME, comm2D, &send_request[i]);
        }
        MPI_Waitall(4, send_request, send_status);

        // receives the processor names from neighbours
        char neighbourProcNames[4][MPI_MAX_PROCESSOR_NAME];
        if(movingAverage > threshold){
            for(i = 0; i < 4; i++){
                if(neighbours[i] >= 0) MPI_Irecv(&neighbourProcNames[i], MPI_MAX_PROCESSOR_NAME, MPI_FLOAT, neighbours[i], SENDPROCNAME, comm2D, &receive_request[i]);
            }
        }
        MPI_Waitall(4, receive_request, receive_status);

        // check if at least two neighbour nodes are within the tolerance (to send alert to base station)
        
        if(movingAverage > threshold){
        	// get the number of neighbours who has similar moving avg
            for(int i = 0; i < 4; i++)
            {
				// if the neighbour average is between SEALTOLERANCE above or SEALTOLERANCE below
                if(neighbours[i] >= 0 && (abs(neighbourAverages[i] - movingAverage) <= SEALTOLERANCE)) count += 1;
            }

			// 2 or more neighbours had similar reading
            if(count >= 2) alert = 1;
        }
		AlertReport report;

        if(alert){ // We need to send an alert to base station
        	// Create structure, and populate with data
        	report.nodeRankMaster = mrank;
        	report.nodeRankCart = crank;
        	report.numberNodesCompared = count;
        	report.nodeSeaLevel = movingAverage;
        	for(i = 0; i < 16; i++){ // Store node IP
        		report.nodeIP[i] = nodeIP[i];
        	}
        	for(i = 0; i < 4; i++){ // Store all neighbour ranks
        		report.neighbourRanks[i] = neighbours[i];
        	}
        	for(i = 0; i < 4; i++){ // Store all neighbour sea levels
        		report.neighbourNodeSeaLevel[i] = neighbourAverages[i];
        	}
        	report.alertTimeTaken = MPI_Wtime() - start;
        	report.alertTimestamp = (int)time(NULL);
        	report.nodecoords[0] = coords[0];
        	report.nodecoords[1] = coords[1];
        	char procName[MPI_MAX_PROCESSOR_NAME] = {'\0'};
        	int nameLen;
        	MPI_Get_processor_name(procName, &nameLen);
        	for(i = 0; i < MPI_MAX_PROCESSOR_NAME; i++){
        		report.processName[i] = procName[i];
        	}
        	for(i = 0; i < 4; i++){
        		int y = 0;
        		for(y = 0; y < MPI_MAX_PROCESSOR_NAME; y++){
        			report.neighbourProcessNames[i][y] = neighbourProcNames[i][y];
        		}
        	}
        	for(i = 0; i < 4; i++){
        		report.neighbourCoords[i][0] = neighbourCoords[i][0];
        		report.neighbourCoords[i][1] = neighbourCoords[i][1];
        	}
        	MPI_Send(&report, 1, AlertReportType, 0, REPORTTAG, masterComm);
        } else { // Dont need to send report... send message indicating that (node rank -1)
        	report.nodeRankMaster = -10;
        	MPI_Send(&report, 1, AlertReportType, 0, REPORTTAG, masterComm);
        }

        // Check if stop message has been sent
        MPI_Recv(&stop, 1, MPI_INT, 0, STOPTAG, masterComm, MPI_STATUS_IGNORE);

        if(stop){ // If termination signal was found
        	printf("TDS // N %d: received termination signal.. gracefully shutting down..\n", crank);
        	MPI_Comm_free(&comm2D);
        	return 0;
        }
	}
	return 0;
}
