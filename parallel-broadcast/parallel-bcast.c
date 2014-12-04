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
  int i, j, k, c;
  int buffer_value;
  char buffer [max_int_size];

  //-- MPI VARS
  int MASTER = 0;
  int rank, size;
  int IS_MASTER = 0;

  //-- PARTITIONING GRID VARS
  int grid_dimensions;
  int grid_rank_i;
  int grid_rank_j;
  int bound_i_lower;
  int bound_i_higher;
  int bound_j_lower;
  int bound_j_higher;
  int *subblock;
  int subblock_dimensions;
  MPI_Comm comm_col;
  MPI_Comm comm_row;

  //-- FLOYD_WARSHALL VARS
  int kth_bcast_rank;
  int *kth_row;
  int *kth_col;

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
  matrix = (int*) malloc (matrix_dimensions * matrix_dimensions * sizeof(int));
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
  bound_i_lower = ((grid_rank_i) * matrix_dimensions) / grid_dimensions;
  bound_j_lower = ((grid_rank_j) * matrix_dimensions) / grid_dimensions;
  bound_i_higher = ((grid_rank_i + 1) * matrix_dimensions) / grid_dimensions;
  bound_j_higher = ((grid_rank_j + 1) * matrix_dimensions) / grid_dimensions;
  subblock_dimensions = matrix_dimensions / grid_dimensions;
  subblock = (int*) malloc(sizeof(int) * subblock_dimensions * subblock_dimensions);
  for (i = bound_i_lower; i < bound_i_higher; i += 1) {
    for (j = bound_j_lower; j < bound_j_higher; j += 1) {
      subblock[(i - bound_i_lower) * subblock_dimensions + (j - bound_j_lower)] = matrix[i * matrix_dimensions + j];
    }
  }

  // build communicators
  MPI_Comm_split(MPI_COMM_WORLD, grid_rank_i, grid_rank_j, &comm_col);
  MPI_Comm_split(MPI_COMM_WORLD, grid_rank_j, grid_rank_i, &comm_row);


  // ITERATE! 
  kth_row = (int*) malloc(sizeof(int) * subblock_dimensions);
  kth_col = (int*) malloc(sizeof(int) * subblock_dimensions);
  for (k = 0; k < 2; k += 1) {

    // COLLECT KTH ROWS
    kth_bcast_rank = k / subblock_dimensions;
    if (grid_rank_i == kth_bcast_rank) {
      i = k - bound_i_lower;
      for (j = 0; j < subblock_dimensions; j++) {
        kth_row[j] = subblock[i * subblock_dimensions + j];
      }

      printf("ITERATION %d\n", k);
      printf("PROCESS %d\n", rank);
      printf("SUBBLOCK ROW: ");
      for (i = 0; i < subblock_dimensions; i++) {
        printf("%d ", kth_row[i]);
      }
      printf("\n\n");
    }

    // COLLECT KTH COLS
    if (grid_rank_j == kth_bcast_rank) {
      j = k - bound_j_lower;
      for (i = 0; i < subblock_dimensions; i++) {
        kth_col[i] = subblock[i * subblock_dimensions + j];
      }

      printf("ITERATION %d\n", k);
      printf("PROCESS %d\n", rank);
      printf("SUBBLOCK COL: ");
      for (i = 0; i < subblock_dimensions; i++) {
        printf("%d ", kth_col[i]);
      }
      printf("\n\n");
    }

    // BROADCAST!!!!
    

  }


  // finalize MPI
  MPI_Finalize();
}