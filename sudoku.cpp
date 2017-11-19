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
#include <math.h> 

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
  board[i] = (choices << VALUEBITS) + num; // oshafie: I think we set value only in ellimination, setting the value would require that you remove it as option from box,row and column
}

void writeFile(FILE *output_file, int *board, int i) {
  fprintf(output_file, "%d ", board[i] % (1<<VALUEBITS));
}

void ellimination(int *board, int boardSize, bool &cellChanged) {

}

void loneRanger(int *board, int boardSize, bool &cellChanged) {

}
/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 * Fast bitcount oshafie 15213's datalab
 */
int bitCount(int x) {
  int everyOtherTemp = (0x55 << 8) +(0x55);
  int everyOther = (everyOtherTemp << 16) +(everyOtherTemp);//6
  int every2Temp = (0x33 << 8) +(0x33);
  int every2 = (every2Temp << 16) + every2Temp;
  int every4Temp = (0xf << 8) +(0xf);
  int every4 = (every4Temp << 16) + every4Temp;//6
  int every8 = (0xff << 16) +(0xff);//2
  int every16 = (0xff << 8) +(0xff);//2
  x = (x & everyOther) + ((x >> 1) & everyOther);//4
  x = (x & every2) + ((x >> 2) & every2);//4
  x = (x & every4) + ((x >> 4) & every4);//4
  x = (x & every8) + ((x >> 8) & every8);//4
  return (x & every16) + ((x >> 16)& every16);//4
}

bool isEmpty(int cell){
  // returns true if value is not set yet, false otherwise
  int allOnes = (1 << (VALUEBITS+1)) -1;
  return !((cell & allOnes));
}
int getCellOptions(int cellA){
  //returns the options part only 
  return cellA >> VALUEBITS;
}

void setOptions(int *board, int index, int options){
  board[index] = options << VALUEBITS;
}
void twinsInRow(int *board, int boardSize, bool &choicesChanged){
  // We could assume that every empty cell has at least 2 options, therefore no need for filtering (2+)-option cells
  int A;
  int cellA;
  int B;
  int cellB;
  int totalSize = boardSize*boardSize;
  // Find twins per row
  for (int r = 0; r < totalSize; r += boardSize)
  {
    // Compare every 2 distenct cells
    for (int i = 0; i < boardSize-1; i++)
    {
      A = r+i;
      cellA = board[A];
      if (isEmpty(cellA)){ //avoid already set cells
        cellA = getCellOptions(cellA);
        for (int j = i+1; j < boardSize; j++)
        {
          B = r+j;
          cellB = board[B];
          if (isEmpty(cellB)){ //avoid already set cells
            cellB = getCellOptions(cellB);
            // cellA and cellB are twins if cellA & cellB has exactly 2 options
            int options = cellA & cellB;
            if (bitCount(options)== 2){
              //Check that no other cell have the any of the 2 options available
              bool found = false;
              for (int t = 0; t < boardSize; t++)
              {
                if (t != i && t != j){
                  if (options & getCellOptions(board[r+t]) != 0){
                    found = true;
                    break;
                  }
                }
              }
              if !(found){
                setOptions(board, A, options);
                setOptions(board, B, options);
                choicesChanged = true;
              }
            }
          }
        }
      }
    }
  }
}

void twinsInBox(int *board, int boardSize, bool &choicesChanged){
  // We could assume that every empty cell has at least 2 options, therefore no need for filtering (2+)-option cells
  int A;
  int cellA;
  int B;
  int cellB;
  int n = sqrt(boardSize);
  // Find twins per box
  for (int bx = 0; bx < n; bx++)
  {
    for (int by = 0; by < n; by++)
    {
      int box_index = bx*n*boardSize + by*n;
      // Compare every 2 distenct cells
      for (int i = 0; i < boardSize -1; i++)
      {
        A = ((i/n))*boardSize + (i%n) + box_index;
        cellA = board[A];
        if (isEmpty(cellA)){ //avoid already set cells
          cellA = getCellOptions(cellA);
          for (int j = i+1; j < boardSize; j++)
          {
            B = ((j/n))*boardSize + (j%n) + box_index;
            cellB = board[B];
            if (isEmpty(cellB)){ //avoid already set cells
              cellB = getCellOptions(cellB);
              // cellA and cellB are twins if cellA & cellB has exactly 2 options
              int options = cellA & cellB;
              if (bitCount(options)== 2){
                //Check that no other cell have the any of the 2 options available
                bool found = false;
                for (int t = 0; t < boardSize; t++)
                {
                  if (t != i && t != j){
                    if (options & getCellOptions(board[((t/n))*boardSize + (t%n) + box_index]) != 0){
                      found = true;
                      break;
                    }
                  }
                }
                if !(found){
                  setOptions(board, A, options);
                  setOptions(board, B, options);
                  choicesChanged = true;
                }
              }
            }
          }
        }
      }
    }
  }
}

void twinsInColumn(int *board, int boardSize, bool &choicesChanged){
  // We could assume that every empty cell has at least 2 options, therefore no need for filtering (2+)-option cells
  int A;
  int cellA;
  int B;
  int cellB;
  // Find twins per column
  for (int c = 0; c < boardSize; c++)
  {
    // Compare every 2 distenct cells
    for (int i = 0; i < boardSize-1; i++)
    {
      A = i*boardSize + c;
      cellA = board[A];
      if (isEmpty(cellA)){ //avoid already set cells
        cellA = getCellOptions(cellA);
        for (int j = i+1; j < boardSize; j++)
        {
          B = j*boardSize + c;
          cellB = board[B];
          if (isEmpty(cellB)){ //avoid already set cells
            cellB = getCellOptions(cellB);
            // cellA and cellB are twins if cellA & cellB has exactly 2 options
            int options = cellA & cellB;
            if (bitCount(options)== 2){
              //Check that no other cell have the any of the 2 options available
              bool found = false;
              for (int t = 0; t < boardSize; t++)
              {
                if (t != i && t != j){
                  if (options & getCellOptions(board[t*boardSize + c]) != 0){
                    found = true;
                    break;
                  }
                }
              }
              if !(found){
                setOptions(board, A, options);
                setOptions(board, B, options);
                choicesChanged = true;
              }
            }
          }
        }
      }
    }
  }
}

void tripletsInRow(int *board, int boardSize, bool &choicesChanged){
  // We could assume that every empty cell has at least 2 options, therefore no need for filtering (2+)-option cells
  int A;
  int cellA;
  int B;
  int cellB;
  int C;
  int cellC;
  int totalSize = boardSize*boardSize;
  // Find twins per row
  for (int r = 0; r < totalSize; r += boardSize)
  {
    // Compare every 2 distenct cells
    for (int i = 0; i < boardSize-2; i++)
    {
      A = r+i;
      cellA = board[A];
      if (isEmpty(cellA)){ //avoid already set cells
        cellA = getCellOptions(cellA);
        for (int j = i+1; j < boardSize-1; j++)
        {
          B = r+j;
          cellB = board[B];
          if (isEmpty(cellB)){ //avoid already set cells
            cellB = getCellOptions(cellB);
            for (int k = j+1; k < boardSize; k++)
            {
              C = r+k;
              cellC = board[C];
              if (isEmpty(cellC)){ //avoid already set cells
                cellC = getCellOptions(cellC);
                // cellA, cellB and cellC are triplets if (cellA & cellB & cellC) has exactly 3 options
                int options = cellA & cellB & cellC;
                if (bitCount(options)== 3){
                  bool found = false;
                  for (int t = 0; t < boardSize; t++)
                  {
                    if (t != i && t != j && t != k){
                      if (options & getCellOptions(board[r+t]) != 0){
                        found = true;
                        break;
                      }
                    }
                  }
                  if !(found){
                    setOptions(board, A, options);
                    setOptions(board, B, options);
                    setOptions(board, C, options);
                    choicesChanged = true;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void tripletsInBox(int *board, int boardSize, bool &choicesChanged){
  // We could assume that every empty cell has at least 2 options, therefore no need for filtering (2+)-option cells
  int A;
  int cellA;
  int B;
  int cellB;
  int C;
  int cellC;
  int n = sqrt(boardSize);
  // Find twins per box
  for (int bx = 0; bx < n; bx++)
  {
    for (int by = 0; by < n; by++)
    {
      int box_index = bx*n*boardSize + by*n;
      // Compare every 2 distenct cells
      for (int i = 0; i < boardSize -2; i++)
      {
        A = ((i/n))*boardSize + (i%n) + box_index;
        cellA = board[A];
        if (isEmpty(cellA)){ //avoid already set cells
          cellA = getCellOptions(cellA);
          for (int j = i+1; j < boardSize -1; j++)
          {
            B = ((j/n))*boardSize + (j%n) + box_index;
            cellB = board[B];
            if (isEmpty(cellB)){ //avoid already set cells
              cellB = getCellOptions(cellB);
              for (int k = j+1; k < boardSize; k++)
              {
                C = ((k/n))*boardSize + (k%n) + box_index;
                cellC = board[C];
                if (isEmpty(cellC)){ //avoid already set cells
                  cellC = getCellOptions(cellC);
                  // cellA, cellB and cellC are triplets if (cellA & cellB & cellC) has exactly 3 options
                  int options = cellA & cellB & cellC;
                  if (bitCount(options)== 3){
                    bool found = false;
                    for (int t = 0; t < boardSize; t++)
                    {
                      if (t != i && t != j && t != k){
                        if (options & getCellOptions(board[((t/n))*boardSize + (t%n) + box_index]) != 0){
                          found = true;
                          break;
                        }
                      }
                    }
                    if !(found){
                      setOptions(board, A, options);
                      setOptions(board, B, options);
                      setOptions(board, C, options);
                      choicesChanged = true;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void tripletsInColumn(int *board, int boardSize, bool &choicesChanged){
  // We could assume that every empty cell has at least 2 options, therefore no need for filtering (2+)-option cells
  int A;
  int cellA;
  int B;
  int cellB;
  int C;
  int cellC;
  // Find twins per column
  for (int c = 0; c < boardSize; c++)
  {
    // Compare every 2 distenct cells
    for (int i = 0; i < boardSize-2; i++)
    {
      A = i*boardSize + c;
      cellA = board[A];
      if (isEmpty(cellA)){ //avoid already set cells
        cellA = getCellOptions(cellA);
        for (int j = i+1; j < boardSize-1; j++)
        {
          B = j*boardSize + c;
          cellB = board[B];
          if (isEmpty(cellB)){ //avoid already set cells
            cellB = getCellOptions(cellB);
            for (int k = j+1; k < boardSize; k++)
            {
              C = k*boardSize + c;
              cellC = board[C];
              if (isEmpty(cellC)){ //avoid already set cells
                cellC = getCellOptions(cellC);
                // cellA, cellB and cellC are triplets if (cellA & cellB & cellC) has exactly 3 options
                int options = cellA & cellB & cellC;
                if (bitCount(options)== 3){
                  bool found = false;
                  for (int t = 0; t < boardSize; t++)
                  {
                    if (t != i && t != j && t != k){
                      if (options & getCellOptions(board[t*boardSize + c]) != 0){
                        found = true;
                        break;
                      }
                    }
                  }
                  if !(found){
                    setOptions(board, A, options);
                    setOptions(board, B, options);
                    setOptions(board, C, options);
                    choicesChanged = true;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void twins(int *board, int boardSize, bool &choicesChanged) {
  twinsInRow(board, boardSize, choicesChanged);
  twinsInBox(board, boardSize, choicesChanged);//Some overlapping work will occur
  twinsInColumn(board, boardSize, choicesChanged);
}

void triplets(int *board, int boardSize, bool &choicesChanged) {

  tripletsInRow(board, boardSize, choicesChanged);
  tripletsInBox(board, boardSize, choicesChanged);//Some overlapping work will occur
  tripletsInColumn(board, boardSize, choicesChanged);
}

bool humanistic(int *board, int boardSize) {
  /* return false if board has no solution
     otherwise (i.e. if solution found or algorithm makes no more changes) return true */

  //in some step of algorithm, a cell was written to with its final value
  bool cellChanged = true;
  //in some step of algorithm, choices were elliminated from some cell
  bool choicesChanged = true;
  while (cellChanged || choicesChanged) {
    if (cellChanged) {
      cellChanged = false;
      ellimination(board, boardSize, cellChanged);
      if (cellChanged) continue;
      loneRanger(board, boardSize, cellChanged);
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
    if (!humanistic(board, boardSize)){
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
