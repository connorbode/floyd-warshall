#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *file;

int main (int argc, const char *argv[]) {

  int max_int_size = 20;
  int matrix_dimensions;
  int * matrix, * next_matrix;
  int i, j, k, c;
  int ij, ik, kj;
  int buffer_value;
  char buffer [max_int_size];

  // display banner
  printf("\n");
  printf("----------------------------------\n");
  printf("   Sequential Floyd-Warshall.\n");
  printf("      by Connor Bode\n");
  printf("----------------------------------\n");
  printf("\n");

  // check the parameters
  if(argc < 3) {
    printf("\n");
    printf("Usage:\n");
    printf("  ./sequential.o <input_file> <output_file>\n");
    printf("\n");
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
  printf("INPUT MATRIX \n\n");
  for (i = 0; i < matrix_dimensions; i += 1) {
    for (j = 0; j < matrix_dimensions; j += 1) {
      printf("%d ", matrix[i * matrix_dimensions + j]);
    }
    printf("\n");
  }

  printf("\n");

  // close the input file
  fclose(file);

  // run the algorithm
  for (k = 0; k < matrix_dimensions; k += 1) {
    next_matrix = (int*) malloc (matrix_dimensions * matrix_dimensions * sizeof(int));
    for (i = 0; i < matrix_dimensions; i += 1) {
      for (j = 0; j < matrix_dimensions; j += 1) {
        ij = i * matrix_dimensions + j;
        ik = i * matrix_dimensions + k;
        kj = k * matrix_dimensions + j;
        next_matrix[ij] = matrix[ij] < matrix[ik] + matrix[kj] ? matrix[ij] : matrix[ik] + matrix[kj];
      }
    }
    free(matrix);
    matrix = next_matrix;
  }

  // print output matrix
  printf("OUTPUT MATRIX \n\n");
  for (i = 0; i < matrix_dimensions; i += 1) {
    for (j = 0; j < matrix_dimensions; j += 1) {
      printf("%d ", next_matrix[i * matrix_dimensions + j]);
    }
    printf("\n");
  }

  // write the new matrix to a file
  file = fopen(argv[2], "w");
  for (i = 0; i < matrix_dimensions; i += 1) {
    for (j = 0; j < matrix_dimensions; j += 1) {
      fprintf(file, "%d ", matrix[i * matrix_dimensions + j]);
    }
    fprintf(file, "\n");
  }

  // clean up
  fclose(file);
  free(matrix);
}