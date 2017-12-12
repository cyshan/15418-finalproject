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
#include <string> 

#define BUFSIZE 1024
//number of bits (least significant) used to store the real value of the board cell
#define VALUEBITS 5

//maximum depth to do brute force before using serial alg
#define DEPTH_THRESHOLD 2

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

std::string cellToString(int cell, int boardSize){
  cell = cell >> VALUEBITS;
  std::string s = "";
  for (int i = 0; i<= boardSize; i++){
    if (cell & 1){
      s += std::to_string(i)+"";
    }
    else{
      s += " ";
    }
    cell = cell >>1;
  }
  return s;
}
void printBoard(int *board, int boardSize) {
  printf("\n\n\n");
  for (int r = 0; r < boardSize; r++) {
    if ((r % (int)sqrt(boardSize)) == 0){
      for (int i = 0; i < boardSize; i++) printf(" ____%d_____ ",r*boardSize +i);
      printf("\n");
    }
    for (int c = 0; c < boardSize; c++)
    {
      std::string str = cellToString(board[r*boardSize + c],boardSize);
      printf("%s | ", str.c_str());
    }
    printf("\n");
  }
}


int maxInt(int a, int b) { return (a > b)? a : b; }
int minInt(int a, int b) { return (a < b)? a : b; }

bool isEmpty(int cell){
  // returns true if value is not set yet, false otherwise
  int allOnes = (1 << (VALUEBITS+1)) -1;
  return !((cell & allOnes));
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
  fprintf(output_file, "%02d ", board[i] % (1<<VALUEBITS));
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
int log2(int n) {
  //REQUIRES: n is a power of 2, n != 0
  int log = 0;
  while (n >>= 1) ++log;
  return log;
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
        board[i] += log2(value);
        eliminateChoices(board, boardSize, i / boardSize, i % boardSize, n);
      }
    }
  }
  return true;
}


void loneRanger(int *board, int boardSize, bool &cellChanged, int n) {
  for (int i = 0; i < boardSize * boardSize; i++) {
    int value = board[i];
    if (!(value % (1<<VALUEBITS))) {

      //cell is currently empty
      int row = i / boardSize;
      int col = i % boardSize;

      //create a mask to find which choices the other cells in col have
      int mask = 0;
      for (int rowI = 0; rowI < boardSize; rowI++) {
        if (rowI != row) mask = mask | board[rowI * boardSize + col];
      }

      //mask out the choices of the other cells in col
      value = value & (~mask);

      //if value still has 1 choice
      if (value && !(value & (value - 1))) {
        cellChanged = true;
        //write choice to cell and eliminate choices from relevant cells
        board[i] = value + log2(value) - VALUEBITS;
        eliminateChoices(board, boardSize, row, col, n);
        continue;
      }

      //do the same for rows and blocks
      value = board[i];
      mask = 0;
      for (int colI = 0; colI < boardSize; colI++) {
        if (colI != col) {
          mask = mask | board[row * boardSize + colI];
        }
      }

      value = value & (~mask);

      if (value && !(value & (value - 1))) {
        cellChanged = true;
        board[i] = value + log2(value) - VALUEBITS;
        eliminateChoices(board, boardSize, row, col, n);
        continue;
      }

      /*base row and col for the square the cell is located in
       (the index of upper-right corner of the square) */
      int baseRow = row / n * n;
      int baseCol = col / n * n;

      value = board[i];
      mask = 0;
      for (int squareI = 0; squareI < boardSize; squareI++){
        int squareRow = baseRow + squareI / n;
        int squareCol = baseCol + squareI % n;
        if (squareCol != col || squareRow != row) {
          mask = mask | board[squareRow * boardSize + squareCol];
        }
      }
      
      value = value & (~mask);

      if (value && !(value & (value - 1))) {
        cellChanged = true;
        board[i] = value + log2(value) - VALUEBITS;
        eliminateChoices(board, boardSize, row, col, n);
      }
    }
  }
}

int getCellOptions(int cellA){
  //returns the options part only 
  return cellA >> VALUEBITS;
}

void setOptions(int *board, int index, int options){
  board[index] = options << VALUEBITS;
}

void removeOption(int *board, int index, int option){
  int mask = ~(1 << (option + VALUEBITS));
  board[index] = board[index] & mask;
}

int hasOption(int cell, int option){
  return cell & (1 << (option + VALUEBITS));
}

void eliminateFromBoxRow(int *board, int boxSize ,int index, int option, bool &choicesChanged){
  for (int col = 0; col < boxSize; col++)
  {
    if (hasOption(board[index + col],option))
    { 
      choicesChanged = true;
      removeOption(board, index + col, option);

    }
  }
}
void eliminateFromBoxCol(int *board, int boardsize, int boxSize ,int index, int option, bool &choicesChanged){
  for (int row = 0; row < boxSize; row++)
  {
    if (hasOption(board[index + row*boardsize],option))
    { 
      choicesChanged = true;
      removeOption(board, index + row*boardsize, option);
    }
  }
}

void boxElimination(int *board, int boardSize, bool &choicesChanged, int n){
  for (int bx = 0; bx < n; bx++)
  {
    for (int by = 0; by < n; by++)
    {
      int box_index = bx*n*boardSize + by*n;
      // Compare every 2 distenct cells
      for (int option = 1; option <= boardSize; option++){
        int row = -1; //uninitialized
        int col = -1; //uninitialized
        bool optionOnRow = true;
        bool optionOnCol = true;
        for (int i = 0; i < boardSize; i++){
          int A = ((i/n))*boardSize + (i%n) + box_index;
          int cellA = board[A];
          if (isEmpty(cellA)){ //avoid already set cells
            if (hasOption(cellA, option)){
              if (row == -1) {
                row = i/n;
                col = i%n;
              } 
              if (row != i/n) optionOnRow = false;
              if (col != i%n) optionOnCol = false;
            }
          }
        }
        if (row != -1 && optionOnRow){
          for (int j = 0; j < n; j++)
          {
            if (j != by) eliminateFromBoxRow(board, n, bx*n*boardSize + row*boardSize + j*n,option, choicesChanged);
          }
        }
        if (col != -1 && optionOnCol){
          for (int j = 0; j < n; j++)
          {
            if (j != bx) eliminateFromBoxCol(board, boardSize, n, j*n*boardSize + by*n + col,option, choicesChanged);
          }
        }
      }
    }
  }
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

void twinsInRow(int *board, int boardSize, bool &choicesChanged) {
  // We could assume that every empty cell has at least 2 options, therefore no need for filtering (2+)-option cells
  int A;
  int cellA;
  int B;
  int cellB;
  // Find twins per row
  for (int r = 0; r < boardSize; r++)
  {
    // Compare every 2 distenct cells
    for (int i = 0; i < boardSize-1; i++)
    {
      A = (r*boardSize) +i;
      cellA = board[A];
      if (isEmpty(cellA)){ //avoid already set cells
        cellA = getCellOptions(cellA);
        for (int j = i+1; j < boardSize; j++)
        {
          B = (r*boardSize)+j;
          cellB = board[B];
          if (isEmpty(cellB)){ //avoid already set cells
            cellB = getCellOptions(cellB);
            // cellA and cellB are twins if cellA & cellB has exactly 2 options
            int options = cellA & cellB;
            if (bitCount(options) >= 2){
              //Check that no other cell have the any of the 2 options available
              int optionsUnion = 0; // get all options for all left cells
              for (int t = 0; t < boardSize; t++)
              {
                if (t != i && t != j){
                  int tempCell = board[(r*boardSize)+t];
                  optionsUnion = optionsUnion | getCellOptions(tempCell);
                }
              }
              options = options & ~optionsUnion; // remove options that are in the union
              if (bitCount(options) == 2){
                if (options != cellA){
                  setOptions(board, A, options);
                  choicesChanged = true;
                }
                if (options != cellB){
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

void twinsInBox(int *board, int boardSize, bool &choicesChanged) {
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
              if (bitCount(options) >= 2){
                //Check that no other cell have the any of the 2 options available
                int optionsUnion = 0; // get all options for all left cells
                for (int t = 0; t < boardSize; t++)
                {
                  if (t != i && t != j){
                    int tempCell = board[((t/n))*boardSize + (t%n) + box_index];
                    optionsUnion = optionsUnion | getCellOptions(tempCell);
                  }
                }
                options = options & ~optionsUnion; // remove options that are in the union
                if (bitCount(options) == 2){
                  if (options != cellA){
                  setOptions(board, A, options);
                  choicesChanged = true;
                  }
                  if (options != cellB){
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
}

void twinsInColumn(int *board, int boardSize, bool &choicesChanged) {
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
            if (bitCount(options) >= 2){
              //Check that no other cell have the any of the 2 options available
              int optionsUnion = 0; // get all options for all left cells
              for (int t = 0; t < boardSize; t++)
              {
                if (t != i && t != j){
                  int tempCell = board[t*boardSize + c];
                  optionsUnion = optionsUnion | getCellOptions(tempCell);
                }
              }
              options = options & ~optionsUnion; // remove options that are in the union
              if (bitCount(options) == 2){
                if (options != cellA){
                  setOptions(board, A, options);
                  choicesChanged = true;
                }
                if (options != cellB){
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

void tripletsInRow(int *board, int boardSize, bool &choicesChanged){
  // We could assume that every empty cell has at least 2 options, therefore no need for filtering (2+)-option cells
  int A;
  int cellA;
  int B;
  int cellB;
  int C;
  int cellC;
  // Find twins per row
  for (int r = 0; r < boardSize; r ++)
  {
    // Compare every 2 distenct cells
    for (int i = 0; i < boardSize-2; i++)
    {
      A = r*boardSize+i;
      cellA = board[A];
      if (isEmpty(cellA)){ //avoid already set cells
        cellA = getCellOptions(cellA);
        for (int j = i+1; j < boardSize-1; j++)
        {
          B = r*boardSize+j;
          cellB = board[B];
          if (isEmpty(cellB)){ //avoid already set cells
            cellB = getCellOptions(cellB);
            for (int k = j+1; k < boardSize; k++)
            {
              C = r*boardSize+k;
              cellC = board[C];
              if (isEmpty(cellC)){ //avoid already set cells
                cellC = getCellOptions(cellC);
                // cellA, cellB and cellC are triplets if (cellA & cellB & cellC) has exactly 3 options
                int options = cellA & cellB & cellC;
                if (bitCount(options) >= 3){
                  int optionsUnion = 0; // get all options for all left cells
                  for (int t = 0; t < boardSize; t++)
                  {
                    if (t != i && t != j){
                      int tempCell = board[r*boardSize+t];
                      optionsUnion = optionsUnion | getCellOptions(tempCell);
                    }
                  }
                  options = options & ~optionsUnion; // remove options that are in the union
                  if (bitCount(options) == 3){
                    if (options != cellA){
                      setOptions(board, A, options);
                      choicesChanged = true;
                    }
                    if (options != cellB){
                      setOptions(board, B, options);
                      choicesChanged = true;
                    }
                    if (options != cellC){
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
                  if (bitCount(options) >= 3){
                    int optionsUnion = 0; // get all options for all left cells
                    for (int t = 0; t < boardSize; t++)
                    {
                      if (t != i && t != j){
                        int tempCell = board[((t/n))*boardSize + (t%n) + box_index];
                        optionsUnion = optionsUnion | getCellOptions(tempCell);
                      }
                    }
                    options = options & ~optionsUnion; // remove options that are in the union
                    if (bitCount(options) == 3){
                      if (options != cellA){
                        setOptions(board, A, options);
                        choicesChanged = true;
                      }
                      if (options != cellB){
                        setOptions(board, B, options);
                        choicesChanged = true;
                      }
                      if (options != cellC){
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
                if (bitCount(options) >= 3){
                  int optionsUnion = 0; // get all options for all left cells
                  for (int t = 0; t < boardSize; t++)
                  {
                    if (t != i && t != j){
                      int tempCell = board[t*boardSize + c];
                      optionsUnion = optionsUnion | getCellOptions(tempCell);
                    }
                  }
                  options = options & ~optionsUnion; // remove options that are in the union
                  if (bitCount(options) == 3){
                    if (options != cellA){
                      setOptions(board, A, options);
                      choicesChanged = true;
                    }
                    if (options != cellB){
                      setOptions(board, B, options);
                      choicesChanged = true;
                    }
                    if (options != cellC){
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
    }
    if (choicesChanged) {
      loneRanger(board, boardSize, cellChanged, n);
      if (cellChanged) continue;
      choicesChanged = false;
      boxElimination(board, boardSize, choicesChanged, n);
      if (choicesChanged) continue;
      twins(board, boardSize, choicesChanged);
      if (choicesChanged) continue;
      triplets(board, boardSize, choicesChanged);
    }
  }
  return true;
}

int *bruteForceSeq(int *board, int boardSize, int n) {
  int totalSquares = boardSize * boardSize;
  for (int i=0; i < totalSquares; i++) {
    int value = board[i];
    if (!(value % (1<<VALUEBITS))) { //cell is empty
      value = value >> VALUEBITS;
      int choice = 0;
      //printBoard(board, boardSize);
      //printf("row: %d, col: %d\n", i/boardSize, i%boardSize);
      while (value) {
        value = value>>1;
        choice++;
        if (value % 2) {
          //printf("choice: %d\n", choice);
          int *newBoard = (int *)calloc(totalSquares, sizeof(int));
          memcpy(newBoard, board, totalSquares * sizeof(int));
          newBoard[i] = (1 << (VALUEBITS + choice)) + choice;
          //printBoard(newBoard, boardSize);
          eliminateChoices(newBoard, boardSize, i / boardSize, i % boardSize, n);
          if (!humanistic(board, boardSize, n)){
            //no solution exists
            return NULL;
          } 
          int *solution = bruteForceSeq(newBoard, boardSize, n);
          if (solution) return solution; //if a solution exists, return it
          free(newBoard);
        }
      }
      return NULL; //there is no solution for the given board
    }
  }
  return board;
}

int *bruteForce(int *board, int boardSize, int n, int depth) {
  int totalSquares = boardSize * boardSize;
  for (int i=0; i < totalSquares; i++) {
    int value = board[i];
    if (!(value % (1<<VALUEBITS))) { //cell is empty
      value = value >> VALUEBITS;
      int choice = 0;
      //printBoard(board, boardSize);
      //printf("row: %d, col: %d\n", i/boardSize, i%boardSize);
      while (value) {
        value = value>>1;
        choice++;
        if (value % 2) {
          //printf("choice: %d\n", choice);
          int *newBoard = (int *)calloc(totalSquares, sizeof(int));
          memcpy(newBoard, board, totalSquares * sizeof(int));
          newBoard[i] = (1 << (VALUEBITS + choice)) + choice;
          //printBoard(newBoard, boardSize);
          eliminateChoices(newBoard, boardSize, i / boardSize, i % boardSize, n);
          if (!humanistic(newBoard, boardSize, n)){
            //no solution exists
            continue;
          }
          int *solution;

          //#pragma omp task {
            if (depth < DEPTH_THRESHOLD) {
              solution = bruteForce(newBoard, boardSize, n, depth + 1);
            } else {
                solution = bruteForceSeq(newBoard, boardSize, n);
            }
            //}
          //#pragma omp taskwait
          if (solution) return solution; //if a solution exists, return it
          //free(newBoard);
        }
      }
      return NULL; //there is no solution for the given board
    }
  }
  return board;
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

  int *originalBoard = (int *)calloc(boardSize * boardSize, sizeof(int)); 
  memcpy(originalBoard, board, boardSize * boardSize * sizeof(int));


  /* Initialize additional data structures needed in the algorithm 
   * here if you feel it's needed. */

  //do initial choice elimination based on given board
  initialChoiceElm(board, boardSize, n);

  error = 0;

  init_time += duration_cast<dsec>(Clock::now() - init_start).count();
  printf("Initialization Time: %lf.\n", init_time);

  auto compute_start = Clock::now();
  double compute_time = 0;

  //store whether the sudoku has a solution or not
  bool solution = true;

#ifdef RUN_MIC /* Use RUN_MIC to distinguish between the target of compilation */

  /* This pragma means we want the code in the following block be executed in 
   * Xeon Phi.
   */

#pragma offload target(mic)  \
  inout(board: length(boardSize * boardSize) INOUT) 
#endif
  {
    //keep memory location so that memory can be transfered out properly
    int *temp = board;

    //Humanistic algorithm
    if (!humanistic(board, boardSize, n)){
      //no solution exists
      board = NULL;
    } else { 
      #pragma omp parallel 
      #pragma omp single
      board = bruteForce(board, boardSize, n, 0);
      
    }
    if (board != NULL) {
      memcpy(temp, board, boardSize * boardSize * sizeof(int));
    } else solution = false;
  }

  //printBoard(board, boardSize);

  compute_time += duration_cast<dsec>(Clock::now() - compute_start).count();
  printf("Computation Time: %lf.\n", compute_time);

  if (!solution) {
    board = NULL;
  }
  
  correctnessChecker(board, originalBoard, boardSize);

  /* OUTPUT YOUR RESULTS TO FILES HERE */
  char input_filename_cpy[BUFSIZE];
  strcpy(input_filename_cpy, input_filename);
  char *filename = basename(input_filename_cpy);
  filename[strlen(filename) - 4] = '\0';
  char output_filename[BUFSIZE];

  sprintf(output_filename, "file_outputs/output_%s_%d.txt", filename, num_of_threads);

  #ifdef RUN_MIC 
    sprintf(output_filename, "output_%s_%d.txt", filename, num_of_threads);
  #endif

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