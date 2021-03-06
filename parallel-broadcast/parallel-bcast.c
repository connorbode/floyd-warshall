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
  int *subblock, *next_subblock;
  int subblock_dimensions;
  MPI_Comm comm_col;
  MPI_Comm comm_row;

  //-- FLOYD_WARSHALL VARS
  int kth_bcast_rank;
  int *kth_row;
  int *kth_col;
  int curr_cost;
  int alt_cost;

  //-- GATHERING THE FINAL MATRIX
  int *out_of_order_subblocks;
  int *gather_counts;
  int *gather_offsets;
  int written;
  int bump;

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
      printf("  ./parallel.o <input_file> <output_file>\n");
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
    if (c == '\n' || c == '\r') { } 
    else if (c == ' ') {
      buffer_value = atoi(buffer);
      memset(buffer, 0, sizeof(buffer));
      matrix[i] = buffer_value;
      j = 0;
      i += 1;
    } 
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
  MPI_Comm_split(MPI_COMM_WORLD, grid_rank_j, grid_rank_i, &comm_col);
  MPI_Comm_split(MPI_COMM_WORLD, grid_rank_i, grid_rank_j, &comm_row);


  // ITERATE! 
  kth_row = (int*) malloc(sizeof(int) * subblock_dimensions);
  kth_col = (int*) malloc(sizeof(int) * subblock_dimensions);
  for (k = 0; k < matrix_dimensions; k += 1) {

    next_subblock = (int*) malloc(sizeof(int) * subblock_dimensions * subblock_dimensions);

    // COLLECT KTH ROWS
    kth_bcast_rank = k / subblock_dimensions;
    if (grid_rank_i == kth_bcast_rank) {
      i = k - bound_i_lower;
      for (j = 0; j < subblock_dimensions; j++) {
        kth_row[j] = subblock[i * subblock_dimensions + j];
      }
    }

    // COLLECT KTH COLS
    if (grid_rank_j == kth_bcast_rank) {
      j = k - bound_j_lower;
      for (i = 0; i < subblock_dimensions; i++) {
        kth_col[i] = subblock[i * subblock_dimensions + j];
      }
    }

    // BROADCAST!!!!
    MPI_Bcast(kth_row, subblock_dimensions, MPI_INT, kth_bcast_rank, comm_col);
    MPI_Bcast(kth_col, subblock_dimensions, MPI_INT, kth_bcast_rank, comm_row);

    // Build next subblock
    for (i = 0; i < subblock_dimensions; i += 1) {
      for (j = 0; j < subblock_dimensions; j += 1) {
        curr_cost = subblock[i * subblock_dimensions + j];
        alt_cost = kth_row[j] + kth_col[i];
        next_subblock[i * subblock_dimensions + j] = curr_cost < alt_cost ? curr_cost : alt_cost;
      }
    }

    free(subblock);
    subblock = next_subblock;
  }
  free(kth_row);
  free(kth_col);

  // GATHER
  if (IS_MASTER) {
    out_of_order_subblocks = (int*) malloc(sizeof(int) * size * subblock_dimensions * subblock_dimensions);
    gather_counts = (int*) malloc(sizeof(int) * size);
    gather_offsets = (int*) malloc(sizeof(int) * size);
    for (i = 0; i < size; i += 1) {
      gather_counts[i] = subblock_dimensions * subblock_dimensions;
      gather_offsets[i] = gather_counts[i] * i;
    }
  }
  MPI_Gatherv(subblock, subblock_dimensions * subblock_dimensions, MPI_INT, out_of_order_subblocks, gather_counts, gather_offsets, MPI_INT, MASTER, MPI_COMM_WORLD);

  // Reorder the subblocks
  if (IS_MASTER) {
    i = 0;
    j = 0;
    k = 0;
    bump = 0;
    for (written = 0; written < matrix_dimensions * matrix_dimensions; written += 1) {
      matrix[written] = out_of_order_subblocks[k * matrix_dimensions * subblock_dimensions + j * subblock_dimensions * subblock_dimensions + i + bump];
      i += 1;
      if (i % subblock_dimensions == 0) {
        i = 0;
        j += 1;
        if (j % grid_dimensions == 0) {
          j = 0;
          bump += subblock_dimensions;
          if (bump == subblock_dimensions * subblock_dimensions) {
            bump = 0;
            k += 1;
          }
        }
      }
    }
  }

  // print output 
  if (IS_MASTER) {
    printf("OUTPUT MATRIX\n\n");
    for (i = 0; i < matrix_dimensions; i += 1) {
      for (j = 0; j < matrix_dimensions; j += 1) {
        printf("%d ", matrix[i * matrix_dimensions + j]);
      }
      printf("\n");
    }
    printf("\n");
  }

  // print output to file
  if (IS_MASTER) {
    file = fopen(argv[2], "w");
    for (i = 0; i < matrix_dimensions; i += 1) {
      for (j = 0; j < matrix_dimensions; j += 1) {
        fprintf(file, "%d ", matrix[i * matrix_dimensions + j]);
      }
      fprintf(file, "\n");
    }
  }

  // finalize 
  free(matrix);
  free(subblock);
  if (IS_MASTER) { free(out_of_order_subblocks); }
  MPI_Finalize();
}