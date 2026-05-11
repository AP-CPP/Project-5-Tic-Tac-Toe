#define GAME_H
#define BOARD_SIZE 9
#define EMPTY ' '

typedef enum {
    RESULT_NONE, RESULT_X_WIN, RESULT_O_WIN, RESULT_DRAW
} GameResult;


int  label_to_index(const char *label); // Used for converting the lables in the directions "A1" into 1 for the array index
void index_to_label(int idx, char *out); // Opposite of the previous function
void available_positions(const char *board, char *out); // Just used for seprating commas 
GameResult check_result(const char *board); // Checking the game outcome
