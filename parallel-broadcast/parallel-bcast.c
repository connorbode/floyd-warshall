#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

FILE *file;

int main (int argc, const char *argv[]) {

  //-- INPUT COLLECTION VARS
  int max_int_size = 20;        
  int matrix_dimensions;
  int * matrix;
  int i, j, c;
  int buffer_value;
  char buffer [max_int_size];

  //-- MPI VARS
  int MASTER = 0;
  int rank, size;
  int IS_MASTER = 0;

  //-- PARALLEL ALGO VARS
  int grid_dimensions;
  int grid_rank_i;
  int grid_rank_j;

  // init MPI
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  IS_MASTER = rank == MASTER;

  // print banner
  if (IS_MASTER) {
    printf("\n");
    printf("-----------------------------------------------\n");
    printf("   Parallel Floyd-Warshall using Broadcasts.\n");
    printf("      by Connor Bode.\n");
    printf("-----------------------------------------------\n\n");
  }

  // check the parameters
  if(argc < 3) {
    if (IS_MASTER) {
      printf("\n");
      printf("Usage:\n");
      printf("  ./sequential.o <input_file> <output_file>\n");
      printf("\n");
    }
    exit(0);
  }

  // check comm size is square rootable
  if (floor(sqrt(size)) - sqrt(size) != 0.0) {
    if (IS_MASTER) {
      printf("Number of processes must be a perfect square.\n");
    }
    exit(0);
  }

  // open the file & get the matrix dimensions
  file = fopen(argv[1], "rt");
  fgets(buffer, max_int_size, file);
  matrix_dimensions = atoi(buffer);
  matrix = (int*) malloc (matrix_dimensions * matrix_dimensions);
  memset(buffer, 0, sizeof(buffer));

  // read in matrix from file
  while((c = fgetc(file)) != EOF) {

    // new line in file
    if (c == '\n' || c == '\r') { } 

    // finished buffering cell value
    else if (c == ' ') {
      buffer_value = atoi(buffer);
      memset(buffer, 0, sizeof(buffer));
      matrix[i] = buffer_value;
      j = 0;
      i += 1;
    } 

    // add character to cell value buffer
    else {
      buffer[j] = c;
      j += 1;
    }
  }

  // print input matrix
  if (IS_MASTER) {
    printf("INPUT MATRIX\n\n");
    for (i = 0; i < matrix_dimensions; i += 1) {
      for (j = 0; j < matrix_dimensions; j += 1) {
        printf("%d ", matrix[i * matrix_dimensions + j]);
      }
      printf("\n");
    }
    printf("\n");
  }

  // close the input file
  fclose(file);

  // partition matrix
  grid_dimensions = sqrt(size);
  grid_rank_i = rank / grid_dimensions;
  grid_rank_j = rank % grid_dimensions;

  printf("PROCESS %d is grid rank %d, %d\n", rank, grid_rank_i, grid_rank_j);


  // finalize MPI
  MPI_Finalize();
}