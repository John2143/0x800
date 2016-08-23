#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

int screenSizeX, screenSizeY;
int numBoardColors;

enum boardMoveDirection{
    DIR_LEFT  = 0,
    DIR_UP    = 1,
    DIR_RIGHT = 2,
    DIR_DOWN  = 3,
};

/*typedef enum bool{false, true} bool;*/
typedef uint8_t square;

typedef struct board{
    int size;
    bool alive : 1;
    bool won : 1;
    uint64_t score;
    int turn;
    int moves;
    square *board;
} *pboard;

#define boardAt(b, x, y) (b)->board[(b)->size * (y) + (x)]
#define boardLength(b) ((b)->size * (b)->size)
#define boardBytes(b) (boardLength(b) * sizeof(square))

pboard createBoard(int size){
    pboard board = malloc(sizeof(*board));
    board->turn = 0;
    board->size = size;
    board->alive = true;
    board->score = 0;
    board->board = calloc(boardLength(board), sizeof(square));
    return board;
}

void freeBoard(pboard b){
    free(b->board);
    free(b);
}

const int blockWidth = 10;
const int blockHeight = 3;
const int blockSpacing = 1;

//Translate value into char * ascii
int writeBoardValue(char *mem, square val){
    const char boardPrint[] = {'1', '2', '4', '8'};
    int len = 0;
    if(val-- > 0){
        if(val / 4 < blockWidth - 4){
            *mem++ = '0';
            *mem++ = 'x';
            *mem++ = boardPrint[val % 4];
            len += 3;
            while(val >= 4){
                *mem++ = '0';
                len++;
                val -= 4;
            }
        }else{
            return sprintf(mem, "2 ^ 0x%x", val);
        }
    }
    *mem = '\0';
    return len;
}


void displayBoard(pboard b, int baseXOffset, int baseYOffset){
    const char background[] = {' ', '-', '=', '#', 'X'};
    char mem[blockWidth + 1];
    for(int y = 0; y < b->size; y++){
        for(int x = 0; x < b->size; x++){
            square val = boardAt(b, x, y);
            int writeLength = writeBoardValue(mem, val);
            int color = val % numBoardColors;
            attron(COLOR_PAIR(color));
            int offsety = y * (blockHeight + blockSpacing) + 5 + baseYOffset;
            int offsetx = x * (blockWidth + blockSpacing) + baseXOffset;

            //Create a background box
            int backgroundChar = val/numBoardColors;
            for(int i = 0; i < blockHeight; i++){
                for(int k = 0; k < blockWidth; k++){
                    mvaddch(offsety + i, offsetx + k, background[backgroundChar]);
                }
            }
            //Draw number
            mvaddstr(offsety + blockHeight/2, offsetx + (blockWidth - writeLength) / 2, mem);

            attroff(COLOR_PAIR(color));
        }
    }
    mvprintw(0, 0, "Turn: 0x%x Score: 0x%.8x", b->turn, b->score);
    mvprintw(1, 0, "Empty Squares: %i", b->moves);
}

//Spawn a random number somewhere free on the board
//TODO make constant space, not constant time?
int boardSpawn(pboard b){
    int possibleMoves = 0;
    int moveList[boardLength(b)];
    for(int i = 0; i < boardLength(b); i++){
        if(b->board[i] == 0){
            //Add the board index to the array
            moveList[possibleMoves++] = i;
        }
    }
    if(!possibleMoves) return 0;
    b->board[moveList[rand() % possibleMoves]] = rand() % 10 == 0 ? 2 : 1;
    return possibleMoves;
}

//Swap two spaces on the board
void boardSwap(pboard b, int x1, int y1, int x2, int y2){
    square val = boardAt(b, x1, y1);
    boardAt(b, x1, y1) = boardAt(b, x2, y2);
    boardAt(b, x2, y2) = val;
}

//Rotate counter clockwise (TODO non-constant space)
void rotateBoard(pboard b){
    square *newmem = malloc(boardBytes(b));
    for(int y = 0; y < b->size; y++){
        for(int x = 0; x < b->size; x++){
            newmem[x + b->size * y] = boardAt(b, (b->size - y) - 1, x);
        }
    }
    free(b->board);
    b->board = newmem;
}

/*void boardSet(pboard b, int x, int y, square val){*/
    /*boardAt(b, x, y) = val;*/
/*}*/
/*square boardGet(pboard b, int x, int y){*/
    /*return boardAt(b, x, y);*/
/*}*/

//Shift all blocks left, combining if possible
//Returns true if any movement was made
bool dropLeft(pboard b){
    bool hasMoved = false;
    for(int y = 0; y < b->size; y++){
        for(int x = 0; x < b->size; x++){
            int ind = x-1;
            square val = boardAt(b, x, y);
            if(val == 0) continue;
            int lastHighestInd = -1;
            while(ind >= 0){
                square indval = boardAt(b, ind, y);
                //combine values
                if(indval == val){
                    boardAt(b, x, y) = 0;
                    boardAt(b, ind, y)++;
                    hasMoved = true;
                }else if(indval == 0){
                    lastHighestInd = ind;
                }
                ind--;
            }
            if(lastHighestInd != -1){
                boardAt(b, x, y) = 0;
                boardAt(b, lastHighestInd, y) = val;
                hasMoved = true;
            }
        }
    }
    return hasMoved;
}

void calculateBoardScore(pboard b){
    b->score = 0;
    for(int i = 0; i < boardLength(b); i++){
        if(b->board[i]){
            b->score += (1 << b->board[i]) * b->board[i];
        }
    }
}

//Play the actual game
void boardAdvance(pboard b, enum boardMoveDirection dir){
    for(unsigned int i = 0; i < dir; i++) rotateBoard(b);
    bool hasMoved = dropLeft(b);
    for(unsigned int i = dir; i < 4; i++) rotateBoard(b);

    if(hasMoved){
        int moves = boardSpawn(b);
        b->moves = moves;
        if(!moves){
            b->alive = false;
        }else{
            calculateBoardScore(b);
            b->turn++;
        }
    }
}

#define P(x, y) init_pair(numBoardColors++, x, y);
void generateColors(){
    numBoardColors = 0;
    for(int i = COLOR_BLACK; i < COLOR_WHITE; i++){
        P(COLOR_WHITE, i);
    }
    for(int i = COLOR_BLACK + 1; i <= COLOR_WHITE; i++){
        P(COLOR_BLACK, i);
    }
}

int main(int argc, char **argv){
    initscr();/* start the curses mode */
    getmaxyx(stdscr, screenSizeY, screenSizeX);/* get the number of rows and columns */
    start_color();
    generateColors();

    int boardSize = 4;
    bool colorTest = false;
    if(argc > 1){
        int argval = atoi(argv[1]);
        if(argval){
            boardSize = argval;
        }else{
            if(strcmp(argv[1], "colortest") == 0){
                colorTest = true;
                boardSize = 7;
            }
        }
    }

    pboard gameboard = createBoard(boardSize);
    if(colorTest){
        for(int i = 0; i < boardLength(gameboard); i++){
            gameboard->board[i] = i;
        }
    }else{
        //Game always starts with 2 squares
        boardSpawn(gameboard);
        boardSpawn(gameboard);
    }
    for(;;){
        displayBoard(gameboard, 0, 0);
        //REMOVE = repeat move
REMOVE:
        switch(getch()){
            case 0x77: boardAdvance(gameboard, DIR_UP); break;
            case 0x61: boardAdvance(gameboard, DIR_LEFT); break;
            case 0x73: boardAdvance(gameboard, DIR_DOWN); break;
            case 0x64: boardAdvance(gameboard, DIR_RIGHT); break;
            case 0x1b: {
                if(getch() != 0x5b) goto REMOVE;
                switch(getch()){
                    case 0x41: boardAdvance(gameboard, DIR_UP); break;
                    case 0x44: boardAdvance(gameboard, DIR_LEFT); break;
                    case 0x42: boardAdvance(gameboard, DIR_DOWN); break;
                    case 0x43: boardAdvance(gameboard, DIR_RIGHT); break;
                    default: goto REMOVE;
                }
            } break;
            default: goto REMOVE;
        }
    }
    freeBoard(gameboard);

    endwin();
    return 0;
}
