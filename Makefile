all:
	mpicc -Wall only_main.c base_station.c node.c functions.c satellite.c -o Main_Out -lm
run:
	mpirun -np $(proc) --allow-run-as-root --oversubscribe Main_Out $(m) $(n) $(t)

clean:
	/bin/rm -f Main_Out *.o
	/bin/rm -f *.txt