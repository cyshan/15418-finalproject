#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include <math.h> 
#include <string>
#include <cstring>

#include "CycleTimer.h"

#define BUFSIZE 1024
#define VALUEBITS 5

static int _argc;
static char **_argv;



double cudaSudoku(int *board, int boardSize, int n); 
void printCudaInfo();
bool isEmpty(int cell);

void writeFile(FILE *output_file, int *board, int i) {
  fprintf(output_file, "%02d ", board[i] % (1<<VALUEBITS));
}

const char *get_option_string(const char *option_name,
            const char *default_value)
{
  for (int i = _argc - 2; i >= 0; i -= 2)
    if (strcmp(_argv[i], option_name) == 0)
      return _argv[i + 1];
  return default_value;
}

void usage(const char* progname) {
    printf("Usage: %s [options]\n", progname);
    printf("Program Options:\n");
    printf("  -i  --input <NAME>     Run test on given input type. Valid inputs  are: test1, random\n");
    printf("  -?  --help             This message\n");
}

void addToBoard(int num, int i, int *board, int boardSize) {
  /*temporary storage for the bitvector specifying what number choices still open for
    that spot on the board */
  int choices;
  if (num) {
    //num != 0
    //fixed number on sudoku board, so only 1 choice for the number
    choices = 1 << num;

  } else {
    //num == 0
    //blank number on sudoku board, so can be any number
    choices = ((1 << boardSize) - 1) << 1;
  }
  board[i] = (choices << VALUEBITS) + num;
}

void checkRows(int *board, int boardSize, bool &correctness){

  for (int r = 0; r < boardSize*boardSize; r += boardSize)
  {
    // Compare every 2 distenct cells
    for (int i = 0; i < boardSize-1; i++)
    {
      int A = r+i;
      int cellA = board[A];
      if (!isEmpty(cellA)){
        for (int j = i+1; j < boardSize; j++)
        {
          int B = r+j;
          int cellB = board[B];
          if (cellB == cellA){
            correctness = false;
          }
        }
      }
    }
  }
}
void checkBoxes(int *board, int boardSize, bool &correctness){
  int n = sqrt(boardSize);
  for (int bx = 0; bx < n; bx++)
  {
    for (int by = 0; by < n; by++)
    {
      int box_index = bx*n*boardSize + by*n;
      // Compare every 2 distenct cells
      for (int i = 0; i < boardSize -1; i++)
      {
        int A = ((i/n))*boardSize + (i%n) + box_index;
        int cellA = board[A];
        if (!isEmpty(cellA)){
          for (int j = i+1; j < boardSize; j++)
          {
            int B = ((j/n))*boardSize + (j%n) + box_index;
            int cellB = board[B];
            if (cellB == cellA){
              correctness = false;
            }
          }
        }
      }
    }
  }
}

void checkColumns(int *board, int boardSize, bool &correctness){
  for (int c = 0; c < boardSize; c++)
  {
    // Compare every 2 distenct cells
    for (int i = 0; i < boardSize-1; i++)
    {
      int A = i*boardSize + c;
      int cellA = board[A];
      if (!isEmpty(cellA)){
        for (int j = i+1; j < boardSize; j++)
        {
          int B = j*boardSize + c;
          int cellB = board[B];
          if (cellB == cellA){
            correctness = false;
          }
        }
      }
    }
  }
}

void compareToOriginal(int *board, int *originalBoard, int boardSize, bool &correctness) {
  for (int i = 0; i < boardSize * boardSize; ++i)
  {
    if (originalBoard[i] % (1 << VALUEBITS)) {
      if (originalBoard[i] != board[i]) {
        correctness = false;
        return;
      }
    }
  }
}

void correctnessChecker(int *board, int *originalBoard, int boardSize){
  if (board == NULL) {
    printf("No Solution\n");
    return;
  }

  bool correctness = true;
  checkRows(board, boardSize, correctness);
  checkColumns(board, boardSize, correctness);
  checkBoxes(board, boardSize, correctness);

  compareToOriginal(board, originalBoard, boardSize, correctness);

  if (correctness){
    printf("\n\nCorrectness: True\n");
  }
  else
  {
    printf("\n\nCorrectness: False\n");
  }
}

int main(int argc, char** argv)
{
  const char *input_filename = get_option_string("-f", NULL);

  _argc = argc - 1;
  _argv = argv + 1;

  // parse commandline options ////////////////////////////////////////////
  int opt;
  static struct option long_options[] = {
      {"input",      1, 0, 'i'},
      {0 ,0, 0, 0}
  };

  while ((opt = getopt_long(argc, argv, "m:n:i:?t", long_options, NULL)) != EOF) {
      switch (opt) {
      case 'i':
          input_filename = optarg;
          break;
      case '?':
      default:
          usage(argv[0]);
          return 1;
      }
  }

  printCudaInfo();

  double cudaTime = 50000.;

  //size of sudoku board is n^2 x n^2

  FILE *input = fopen(input_filename, "r");

  if (!input) {
    printf("Unable to open file: %s.\n", input_filename);
    return -1;
  }

  int n;

  fscanf(input, "%d\n", &n);

  int boardSize = n*n;

  int *board = (int *)calloc(boardSize * boardSize, sizeof(int));
  

  //for parsing sudoku board
  int num;
 
  for (int row = 0; row < boardSize; row++) {
    for (int col = 0; col < boardSize; col++) {
      int i = row * boardSize + col;
      fscanf(input, "%d", &num);
      addToBoard(num, i, board, boardSize);
    }
    fscanf(input, "\n");
  }

  int *originalBoard = (int *)calloc(boardSize * boardSize, sizeof(int)); 
  memcpy(originalBoard, board, boardSize * boardSize * sizeof(int));



  cudaTime = std::min(cudaTime, cudaSudoku(board, boardSize, n));
  printf("GPU_time: %.3f ms\n", 1000.f * cudaTime);

  correctnessChecker(board, originalBoard, boardSize);

  /* OUTPUT YOUR RESULTS TO FILES HERE */
  char input_filename_cpy[BUFSIZE];
  strcpy(input_filename_cpy, input_filename);
  char *filename = basename(input_filename_cpy);
  filename[strlen(filename) - 4] = '\0';
  char output_filename[BUFSIZE];

  sprintf(output_filename, "outputs/output_%s.txt", filename);

  FILE *output_file = fopen(output_filename, "w");
  if (!output_file) {
    printf("Error: couldn't output file");
    return -1;
  }

  fprintf(output_file, "%d\n", n);
  
  // WRITE TO FILE HERE
  if (board != NULL) {
    for (int row = 0; row < boardSize; row++) {
      for (int col = 0; col < boardSize; col++) {
        int i = boardSize * row + col;
        writeFile(output_file, board, i);
      }
      fprintf(output_file, "\n");
    }
  }

  fclose(output_file);

  return 0;
}
