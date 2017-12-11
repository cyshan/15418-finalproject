#include <stdio.h>

#include <cuda.h>
#include <cuda_runtime.h>
#include <driver_functions.h>

#include "CycleTimer.h"

#define VALUEBITS 5

bool isEmpty(int cell){
  // returns true if value is not set yet, false otherwise
  int allOnes = (1 << (VALUEBITS+1)) -1;
  return !((cell & allOnes));
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

void twinsInRow(int *board, int boardSize, bool &choicesChanged){
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


int *bruteForce(int *board, int boardSize, int n) {
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
          int *solution = bruteForce(newBoard, boardSize, n);
          if (solution) return solution; //if a solution exists, return it
          free(newBoard);
        }
      }
      return NULL; //there is no solution for the given board
    }
  }
  return board;
}

double cudaSudoku(int *board, int boardSize, int n) {
	//do initial choice elimination based on given board
	initialChoiceElm(board, boardSize, n);

	double startTime = CycleTimer::currentSeconds();

	if (!humanistic(board, boardSize, n)) {
		//no solution exists
		board = NULL;
	} else board = bruteForce(board, boardSize, n);

	double endTime = CycleTimer::currentSeconds();

	return endTime - startTime;
}



void printCudaInfo()
{
		// for fun, just print out some stats on the machine

		int deviceCount = 0;
		cudaError_t err = cudaGetDeviceCount(&deviceCount);

		printf("---------------------------------------------------------\n");
		printf("Found %d CUDA devices\n", deviceCount);

		for (int i=0; i<deviceCount; i++)
		{
				cudaDeviceProp deviceProps;
				cudaGetDeviceProperties(&deviceProps, i);
				printf("Device %d: %s\n", i, deviceProps.name);
				printf("   SMs:        %d\n", deviceProps.multiProcessorCount);
				printf("   Global mem: %.0f MB\n",
							 static_cast<float>(deviceProps.totalGlobalMem) / (1024 * 1024));
				printf("   CUDA Cap:   %d.%d\n", deviceProps.major, deviceProps.minor);
		}
		printf("---------------------------------------------------------\n"); 
}
