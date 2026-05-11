#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

static const int WIN_CONDITIONS[8][3]{
  {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
  {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
  {0, 4, 8}, {2, 4, 6}
}; /* Total of 8 possible ways to win tic-tac-toe
      first set are the rows then columns and the last two are the diagonals*/

// Function that changes "A1" to row 0 column 0 and etc for the rest of the table
int label_to_index(const char *label) {
    if (label == NULL || strlen(label) < 2) return -1; // Checking to see if two chars where inputed while also seeing if anything was passed
    char col = (label[0] >= 'a' && label[0] <= 'z') ? (label[0] - 32) : label[0]; /* So here its taking in the letters and comparing it to the ascii table meaning if 'A' 
    is inputed there is no change but if there is lower case it changes it to upper case */ 
    char row = label[1]; // Takes second input and does no "converison since its a digit
    // Validations 
    if (col < 'A' || col > 'C') return -1;
    if (row < '1' || row > '3') return -1;
    // Returning the columns and rows
    return (row - '1') * 3 + (col - 'A');
} 

// Just the opposite of the function above
void index_to_label(int idx, char *out) {
    out[0] = 'A' + (idx % 3); // Creates the letter part
    out[1] = '1' + (idx / 3); // Creates number part
    out[2] = '\0';
}

// Checking to see what part of the board has been filled
void available_positions(const char *board, char *out) {
    // Setting null 
    out[0] = '\0';
    // Creating buffer for A1-C3
    char label[3];
    int first = 1;
    for (int i = 0; i < BOARD_SIZE; i++) { // Checking everysquare
        if (board[i] == EMPTY) {
            index_to_label(i, label);
            // Adding commas
            if (!first) strcat(out, ",");
            strcat(out, label);
            first = 0;
        }
    }
}

// Used to see if there is a winner
GameResult check_result(const char *board) {
    
    for (int i = 0; i < 8; i++) { // Checking every square
      // Checks all types of possible win outcomes
        char a = board[WIN_CONDITIONS[i][0]]; 
        char b = board[WIN_CONDITIONS[i][1]];
        char c = board[WIN_CONDITIONS[i][2]];
        if (a != EMPTY && a == b && b == c) {
            return (a == 'X') ? RESULT_X_WIN : RESULT_O_WIN;
        }
    }
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i] == EMPTY) return RESULT_NONE;
    }
    return RESULT_DRAW;
}
