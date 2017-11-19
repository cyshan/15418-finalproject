/**
 * Parallel Sudoku solver
 * Christopher Shan(cshan1), Omar Shafie(oshafie)
 */

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <assert.h>
#include <omp.h>
#include "mic.h"

#define BUFSIZE 1024
//number of bits (least significant) used to store the real value of the board cell
#define VALUEBITS 5

static int _argc;
static const char **_argv;

/* Starter code function, don't touch */
const char *get_option_string(const char *option_name,
            const char *default_value)
{
  for (int i = _argc - 2; i >= 0; i -= 2)
    if (strcmp(_argv[i], option_name) == 0)
      return _argv[i + 1];
  return default_value;
}

/* Starter code function, do not touch */
int get_option_int(const char *option_name, int default_value)
{
  for (int i = _argc - 2; i >= 0; i -= 2)
    if (strcmp(_argv[i], option_name) == 0)
      return atoi(_argv[i + 1]);
  return default_value;
}

/* Starter code function, do not touch */
float get_option_float(const char *option_name, float default_value)
{
  for (int i = _argc - 2; i >= 0; i -= 2)
    if (strcmp(_argv[i], option_name) == 0)
      return (float)atof(_argv[i + 1]);
  return default_value;
}

/* Starter code function, do not touch */
static void show_help(const char *program_path)
{
    printf("Usage: %s OPTIONS\n", program_path);
    printf("\n");
    printf("OPTIONS:\n");
    printf("\t-f <input_filename> (required)\n");
    printf("\t-n <num_of_threads> (required)\n");
}

void printBoard(int *board, int boardSize) {
  for (int i = 0; i < boardSize * boardSize; i++) {
    printf("%d\n", board[i]);
  }
}


int maxInt(int a, int b) { return (a > b)? a : b; }
int minInt(int a, int b) { return (a < b)? a : b; }

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

void writeFile(FILE *output_file, int *board, int i) {
  fprintf(output_file, "%d ", board[i] % (1<<VALUEBITS));
}

void eliminateChoices(int *board, int boardSize, int row, int col, int n) {
  //number to eliminate as option from relevant cells
  int num = board[row * boardSize + col] % (1<<VALUEBITS);
  //filter for removing options from cells
  int filter = ~(1 << (VALUEBITS + num));
  for (int rowI = 0; rowI < boardSize; rowI++) {
    //eliminate choices for the column
    if (rowI != row) {
      int i = rowI * boardSize + col;
      board[i] = board[i] & filter; 
    }
  }

  for (int colI = 0; colI < boardSize; colI++) {
    //eliminate choices for the row
    if (colI != col) {
      int i = row * boardSize + colI;
      board[i] = board[i] & filter; 
    }
  }

  /*base row and col for the square the cell is located in
   (the index of upper-right corner of the square) */
  int baseRow = row / n * n;
  int baseCol = col / n * n;
  for (int squareI = 0; squareI < boardSize; squareI++){
    //eliminate choices for the square
    int squareRow = baseRow + squareI / n;
    int squareCol = baseCol + squareI % n;
    if (squareCol != col || squareRow != row) {
      int i = squareRow * boardSize + squareCol;
      board[i] = board[i] & filter;
    }
  }
}

bool elimination(int *board, int boardSize, bool &cellChanged, int n) {
  //return false iff the board given has no valid solution
  for (int i = 0; i < boardSize * boardSize; i++) {
    int value = board[i];
    if (!(value % (1<<VALUEBITS))) {
      //cell is currently empty
      value = value >> VALUEBITS;
      //all the choice bits are 0, so no solution to board
      if (!value) return false;

      if (!(value & (value - 1))) {
        //value is a power of 2, aka there is only one value this cell can take
        cellChanged = true;
        //find position of the single choice
        int pos = 0;
        while (value >>= 1) ++pos;
        board[i] += pos;
        eliminateChoices(board, boardSize, i / boardSize, i % boardSize, n);
      }
    }
  }
  return true;
}

void loneRanger(int *board, int boardSize, bool &cellChanged, int n) {
           
}

void twins(int *board, int boardSize, bool &choicesChanged) {

}

void triplets(int *board, int boardSize, bool &choicesChanged) {

}

bool humanistic(int *board, int boardSize, int n) {
  /* return false if board has no solution
     otherwise (i.e. if solution found or algorithm makes no more changes) return true */

  //in some step of algorithm, a cell was written to with its final value
  bool cellChanged = true;
  //in some step of algorithm, choices were eliminated from some cell
  bool choicesChanged = true;
  while (cellChanged || choicesChanged) {
    if (cellChanged) {
      cellChanged = false;
      if (!elimination(board, boardSize, cellChanged, n)) return false;
      if (cellChanged) continue;
      loneRanger(board, boardSize, cellChanged, n);
      if (cellChanged) continue;
    }
    if (choicesChanged) {
      choicesChanged = false;
      twins(board, boardSize, choicesChanged);
      if (choicesChanged) continue;
      triplets(board, boardSize, choicesChanged);
    }
  }
  return true;
}



void initialChoiceElm(int *board, int boardSize, int n) {
  //n is square root of board size
  for (int row = 0; row < boardSize; row++) {
    for (int col = 0; col < boardSize; col++) {
      int i = row * boardSize + col;
      if (board[i] % (1<<VALUEBITS)) {
        //if the cell in the board has a value
        eliminateChoices(board, boardSize, row, col, n);
      }
    }
  }
}

int main(int argc, const char *argv[])
{
  using namespace std::chrono;
  typedef std::chrono::high_resolution_clock Clock;
  typedef std::chrono::duration<double> dsec;

  auto init_start = Clock::now();
  double init_time = 0;
 
  _argc = argc - 1;
  _argv = argv + 1;

  srand(time(NULL));

  /* You'll want to use these parameters in your algorithm */
  const char *input_filename = get_option_string("-f", NULL);
  int num_of_threads = get_option_int("-n", 1);

  int error = 0;

  if (input_filename == NULL) {
    printf("Error: You need to specify -f.\n");
    error = 1;
  }

  if (error) {
    show_help(argv[0]);
    return 1;
  }
  
  printf("Number of threads: %d\n", num_of_threads);
  printf("Input file: %s\n", input_filename);

  FILE *input = fopen(input_filename, "r");

  if (!input) {
    printf("Unable to open file: %s.\n", input_filename);
    return -1;
  }
 
  //size of sudoku board is n^2 x n^2
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


  /* Initialize additional data structures needed in the algorithm 
   * here if you feel it's needed. */

  //do initial choice elimination based on given board
  initialChoiceElm(board, boardSize, n);

  //printBoard(board, boardSize);


  error = 0;

  init_time += duration_cast<dsec>(Clock::now() - init_start).count();
  printf("Initialization Time: %lf.\n", init_time);
  auto compute_start = Clock::now();
  double compute_time = 0;


#ifdef RUN_MIC /* Use RUN_MIC to distinguish between the target of compilation */

  /* This pragma means we want the code in the following block be executed in 
   * Xeon Phi.
   */
#pragma offload target(mic)  \
  inout(board: length(boardSize * boardSize) INOUT) 
#endif
  {

    //Humanistic algorithm
    if (!humanistic(board, boardSize, n)){
      //no solution exists
    }

  }

  compute_time += duration_cast<dsec>(Clock::now() - compute_start).count();
  printf("Computation Time: %lf.\n", compute_time);
  
  /* OUTPUT YOUR RESULTS TO FILES HERE */
  char input_filename_cpy[BUFSIZE];
  strcpy(input_filename_cpy, input_filename);
  char *filename = basename(input_filename_cpy);
  filename[strlen(filename) - 4] = '\0';
  char output_filename[BUFSIZE];


  sprintf(output_filename, "file_outputs/output_%s_%d.txt", filename, num_of_threads);
  FILE *output_file = fopen(output_filename, "w");
  if (!output_file) {
    printf("Error: couldn't output file");
    return -1;
  }

  fprintf(output_file, "%d\n", n);
  
  // WRITE TO FILE HERE

  for (int row = 0; row < boardSize; row++) {
    for (int col = 0; col < boardSize; col++) {
      int i = boardSize * row + col;
      writeFile(output_file, board, i);
    }
    fprintf(output_file, "\n");
  }
  fclose(output_file);

  return 0;
}
