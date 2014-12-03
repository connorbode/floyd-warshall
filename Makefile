all:
	$(MAKE) -C sequential
	$(MAKE) -C parallel-broadcast

seq:
	./sequential/sequential.o input.txt output.txt

bcast:
	mpirun -np 4 parallel-broadcast/parallel-bcast.o input.txt output.txt