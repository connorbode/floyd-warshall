#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *file;

int main (int argc, const char *argv[]) {

  int max_int_size = 20;
  int matrix_dimensions;
  int * matrix;
  int i, j, c;
  int buffer_value;
  char buffer [max_int_size];

  // check the parameters
  if(argc < 2) {
    printf("\n");
    printf("Usage:\n");
    printf("  ./sequential <input_matrix>\n");
    printf("\n");
    exit(0);
  }

  // open the file & get the matrix dimensions
  file = fopen(argv[1], "rt");
  fgets(buffer, max_int_size, file);
  matrix_dimensions = atoi(buffer);
  matrix = (int*) malloc (matrix_dimensions * matrix_dimensions);

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
}