all:
	$(MAKE) -C sequential
	$(MAKE) -C parallel-broadcast
	$(MAKE) -C parallel-pipeline

seq:
	./sequential/sequential.o input/input.txt output/sequential.txt

bcast:
	mpirun -np 4 parallel-broadcast/parallel-bcast.o input/input.txt output/parallel-broadcast.txt

pipe:
	mpirun -np 4 parallel-pipeline/parallel-pipeline.o input/input.txt output/parallel-pipeline.txt