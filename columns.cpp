#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SD.h>

// colour definitions
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0
#define WHITE    0xFFFF
#define ORANGE   0xFA00
#define BROWN    0x99E0 //got this by experimenting

#define TFT_CS   6  // Chip select line for TFT display
#define TFT_DC   7  // Data/command line for TFT
#define TFT_RST  8  // Reset line for TFT (or connect to +5V)

#define JOY_SEL 9 //the joystick button pin
#define JOY_VERT_ANALOG 0 // pins connected to vertical and horizontal joystick
#define JOY_HORIZ_ANALOG 1

#define JOY_DEADZONE 64 //the deadzone of the joystick

#define SCREEN_SIZE_X 128 //horizontal size of screen
#define SCREEN_SIZE_Y 160 //vertical size of screen

#define NUM_COLS 6
#define COL_WIDTH 11 // = each column is a 10x10 pixel coloured block with a white border on the top and side right
#define BLOCK_HEIGHT 11

// 1st column x coordinate: 0
// 2nd : 10
// 3rd : 20
// 4th : 30
// 5th : 40
// 6th : 50
#define ENTER_COL 20 // the third column, where each block starts by default


const int colourPin = 7; //the pin used to assign random colours to the blocks
const int colChangePin = 2; //the pin attached to the button that changes the order of the colours

int location_x = ENTER_COL;
int new_location_x = location_x;
int fallDelay = 100; //larger delay = block falls more slowly, smaller delay = block falls more quickly

bool isPressed = false;

int JOY_V_CENTRE = analogRead(JOY_VERT_ANALOG); //calibrates the joystick, assumes the user is not touching it as the program starts
int JOY_H_CENTRE = analogRead(JOY_HORIZ_ANALOG);

int level = 1; // there are 10 before a the max speed is reached

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// a variable type that indicates a colour
enum Shade {Black = BLACK, Green = GREEN, Blue = BLUE, Orange = ORANGE, Magenta = MAGENTA, Yellow = YELLOW, Cyan = CYAN};
int difficulty; // can range from 1 to 6, indicates the number of different colours of blocks

Shade BlkMap [6][15] = {Black};
bool ColCode[6][15] = {0};

int score = 0; // the score is proportional to the number of blocks removed

/*Generates a random colour. Used to assign colours to new blocks
The number of colours generated is based on the difficulty.*/
Shade randomColour() {
    Shade colour;
    int reading = analogRead(colourPin);

    // creates a random number from the reading and stores it in colour
    int number = ((reading % difficulty) + 1); // +1 is added to avoid getting Black
    if (number == 1) {colour = Green;}
    if (number == 2) {colour = Blue;}
    if (number == 3) {colour = Orange;}
    if (number == 4) {colour = Magenta;}
    if (number == 5) {colour = Yellow;}
    if (number == 6) {colour = Cyan;}
    return colour;
}

/*Displays an introductory screen to the user upon starting the game.
The user must press and release the joystick to begin playing.*/
void displayMenu() {
    bool sel = digitalRead(JOY_SEL);
    tft.fillScreen(0);
    tft.setCursor(5,30);
    tft.setTextColor(GREEN);
    tft.setTextWrap(false);
    tft.setTextSize(5);
    tft.print("M");
    tft.setTextColor(BLUE);
    tft.print("E");
    tft.setTextColor(ORANGE);
    tft.print("G");
    tft.setTextColor(MAGENTA);
    tft.println("A");
    tft.setTextColor(CYAN);
    tft.setTextSize(3);
    tft.println("COLUMNS");
    tft.setTextSize(1);
    tft.setCursor(0, 120);
    tft.setTextColor(WHITE);
    //The user must press and release the button to start the program
    tft.println("Push joystick to play");

    // wait for the joystick to be pressed and then released
    while(sel) {
        sel = digitalRead(JOY_SEL);
    }
    while(!sel) {
        sel = digitalRead(JOY_SEL);
    }
}

/* Scans the joystick to allow the user to highlight different difficulties
-it is denoted as 2 because it was incorporated after the original */
void scanJoystick2(int* highlight, bool* update) {
    int v = analogRead(JOY_VERT_ANALOG);
    int delta = v - JOY_V_CENTRE;

    //if the joystick is outside the deadzone
    if (abs(delta) > JOY_DEADZONE) {
        //the highlight must change
        *update = true;

        if (delta > 0) { //if delta is positive, the joystick was moved down
            *highlight = (*highlight + 1) % 4;
        }
        //otherwise, the joystick moved up the list
        else if (*highlight > 0) {
            *highlight = (*highlight - 1);
        }
        else { // difficulty is easy so wrap to the bottom
            *highlight = 3;
        }
    }
}

/*Displays to the screen a selection of difficulties for the user to choose */
void displayChooseDifficulty() {
    bool sel = digitalRead(JOY_SEL);
    bool update = true;
    int highlight = 0; // indicates the highlighted difficulty
    tft.fillScreen(0);
    tft.setTextSize(1);
    while(true) {
        while(update) {
            // print the original selection menu
            tft.setCursor(0,46);
            tft.setTextColor(WHITE);
            tft.print("Choose difficulty: ");
            tft.setCursor(0,61);
            tft.setTextColor(GREEN, BLACK);
            tft.print("EASY"); // difficulty 3
            tft.setCursor(0,76);
            tft.setTextColor(BLUE, BLACK);
            tft.print("NORMAL"); // difficulty 4
            tft.setCursor(0,91);
            tft.setTextColor(MAGENTA, BLACK);
            tft.print("HARD"); // difficulty 5
            tft.setCursor(0,106);
            tft.setTextColor(ORANGE, BLACK);
            tft.print("EXTREME"); // difficulty 6
            // highlight the corresponding section of the menu
            if (highlight == 0) {
                tft.setCursor(0,61);
                tft.setTextColor(GREEN, WHITE);
                tft.print("EASY");
            }
            else if (highlight == 1) {
                tft.setCursor(0,76);
                tft.setTextColor(BLUE, WHITE);
                tft.print("NORMAL");
            }
            else if (highlight == 2) {
                tft.setCursor(0,91);
                tft.setTextColor(MAGENTA, WHITE);
                tft.print("HARD");
            }
            else {
                tft.setCursor(0,106);
                tft.setTextColor(ORANGE, WHITE);
                tft.print("EXTREME");
            }
            update = false; // set update to false
        }
        sel = digitalRead(JOY_SEL);
        delay(175);
        if (!sel) { // if the button is pressed, set the highlighted selection as the difficulty
            difficulty = highlight + 3;
            break;
        }
        scanJoystick2(&highlight, &update); // allows to determine if the joystick has moved up or down
    }
}

/*Prints the in-game display to the TFT screen, including
the title, level, score, and next block.*/
void displayGame() {
    tft.fillScreen(0);
    tft.fillRect(60,0,67,9,RED);
    tft.fillRect(61,9,128,160,BROWN);

    tft.setTextColor(WHITE);

    //print Mega Columns
    tft.setCursor(71,19);
    tft.setTextSize(2);
    tft.setTextColor(GREEN);
    tft.setTextWrap(false);
    tft.setTextSize(2);
    tft.print("M");
    tft.setTextColor(BLUE);
    tft.print("E");
    tft.setTextColor(ORANGE);
    tft.print("G");
    tft.setTextColor(MAGENTA);
    tft.println("A");
    tft.setTextColor(CYAN);
    tft.setTextSize(1);
    tft.setCursor(73,41);
    tft.println("COLUMNS");

    //print level
    tft.setTextColor(WHITE);
    tft.setTextSize(1);
    tft.setCursor(64,60);
    tft.print("LEVEL:");
    tft.println(level);

    //print score
    tft.setCursor(64,75);
    tft.print("SCORE:");
    tft.print(score);

    tft.setCursor(64,90);
    tft.print("NEXT:");
    tft.fillRect(0,0,61,9, RED);
    delay(750);
    tft.fillRect(0,0,61,9, BLACK);
    delay(750);
    tft.fillRect(0,0,61,9, RED);
    delay(500);
    tft.fillRect(0,0,61,9, BLACK);
    delay(500);
    tft.fillRect(0,0,61,9, RED);
    delay(250);
    tft.fillRect(0,0,61,9, BLACK);
    delay(250);
    tft.fillRect(0,0,61,9, RED);
    delay(100);
    tft.fillRect(0,0,61,9, BLACK);
    delay(100);
    tft.setCursor(13,60);
    tft.setTextColor(WHITE);
    tft.print("START!");
    delay(1000);
    tft.fillRect(0,60,61,9, BLACK);
}

/*Reprints the updated score to the TFT screen after block sequences have been removed.*/
void updateScore() {
    tft.setCursor(100,75);
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.fillRect(98,75,30,10,BROWN);
    tft.print(score);
}

/* Prints the level and the statement "LEVEL UP!" to the TFT screen */
void levelUp() {
    tft.setCursor(100,60);
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.fillRect(98,60,30,10,BROWN);
    tft.print(level);

    tft.setCursor(73,0);
    tft.setTextColor(WHITE);
    tft.setTextSize(1);
    tft.print("LEVEL UP!");

    delay(1000);

    tft.fillRect(61,0,67,9,RED);
}

/*Initializes TFT, the joystick, and colour button and calls
functions to print the menu difficulty selection and game screens.*/
void setup () {
    // Init TFT
    tft.initR(INITR_BLACKTAB);
    Serial.println("Display initialized!");
    // Init joystick
    pinMode(JOY_SEL, INPUT);
    digitalWrite(JOY_SEL, HIGH); // enables pull-up resistor - required!
    Serial.println("Joystick initialized!");

    // Init colour button
    pinMode(colChangePin, INPUT);
    digitalWrite(colChangePin, HIGH);
    Serial.println("Colour Button initialized!");

    //read the horizontal and vertical resting states of the joystick
    JOY_V_CENTRE = analogRead(JOY_VERT_ANALOG);
    JOY_H_CENTRE = analogRead(JOY_HORIZ_ANALOG);

    displayMenu(); //display start screen

    displayChooseDifficulty(); // allows the user to choose the difficulty

    displayGame(); //print the game screen
}

/*Converts a y coordinate of a pixel on the image to a BlkMap coordinate*/
int y_to_coor(int location_y) {
    return map(location_y, 9, SCREEN_SIZE_Y - BLOCK_HEIGHT, 14, 0);
}

/*Converts a BlkMap y coordinate back to a pixel height corresponding
to the top of the border block*/
int coor_to_y(int coordinate) {
    return map(coordinate, 0, 14, SCREEN_SIZE_Y - BLOCK_HEIGHT, 9);
}

/*Converts a BlkMap x coordinate to  a pixel location*/
int coor_to_x(int coordinate) {
    return coordinate*10;
}

/*Checks to see if the joystick has been horizontally or vertically moved.*/
void scanJoystick(int BlkLocation) {
    int v = analogRead(JOY_VERT_ANALOG);
    int h = analogRead(JOY_HORIZ_ANALOG);

    if (abs(h - JOY_H_CENTRE) > JOY_DEADZONE) { //if horizontal is outside of deadzone
        if (h - JOY_H_CENTRE > 0) { //if joystick was moved right
            if (BlkMap[(location_x/10) + 1][BlkLocation-1] == Black) {
                new_location_x = constrain(location_x+(COL_WIDTH-1), 0, 5*(COL_WIDTH-1));
            }
        }
        if (h - JOY_H_CENTRE < 0) { //if joystick was moved left
            if (BlkMap[(location_x/10) - 1][BlkLocation-1] == Black) {
                new_location_x = constrain(location_x-(COL_WIDTH-1), 0, 5*(COL_WIDTH-1));
            }
        }
    }
    if ((v - JOY_V_CENTRE) > JOY_DEADZONE) { //if joystick is down
        fallDelay = 0;
    }
    //when the joystick is released, set the delay back to max delay for the level
    if ((v - JOY_V_CENTRE) < JOY_DEADZONE) {
        fallDelay = constrain((100/level) - (level), 0, 100);
    }
}

/*Checks to see if the external colour button has been pressed and
changes the order of the colours of the blocks in the stack accordingly.*/
void colourChange(Shade* Bcolour, Shade* Mcolour, Shade* Tcolour) {
    bool col = digitalRead(colChangePin);

    //when the button is pushed down
    if (!col && !isPressed) {
        //the button is now pressed until it is released (until sel is true again)
        isPressed = true;

        // bottom to top, other two colours move down
        Shade tmp = *Bcolour;
        *Bcolour = *Mcolour;
        *Mcolour = *Tcolour;
        *Tcolour = tmp;
    }
    //when the button is released, isPressed becomes false
    if (col) {
        isPressed = false;
    }
}

/*Allows the user to pause the game when the joystick button is pressed */
void pauseButton(unsigned long long *startTime) {
    bool sel = digitalRead(JOY_SEL);

    unsigned long long currTime = millis(); //the time when the game was paused

    if (!sel) {
        tft.setCursor(71,0);
        tft.setTextColor(WHITE);
        tft.setTextSize(1);
        tft.print("PAUSED");
        while(!sel) {
            sel = digitalRead(JOY_SEL);
            delay(50);
        }
        while (sel) {
            sel = digitalRead(JOY_SEL);
            delay(50);
        }
        while(!sel) {
            sel = digitalRead(JOY_SEL);
            delay(50);
        }
        tft.fillRect(60,0,67,9, RED);
    }
    // adjust startTime to account for the time the game was paused
    *startTime = *startTime + (millis() - currTime); //prevents the user from levelling up while the game is paused
}

/*Set all places in the colour code array back to
0 after blocks have been removed.*/
void resetColCode() {
    for (int j = 0; j < 15; ++j) {
        for (int i = 0; i < 6; ++i) {
            ColCode[i][j] = 0;
        }
    }
}

/*Prints the colour code array to the serial monitor.
Was used in testing to make sure the checking system worked.*/
void printColCode() {
    for (int j = 14; j >= 0; --j) {
        for (int i = 0; i < 6; ++i) {
            Serial.print(ColCode[i][j]); Serial.print(" ");
        }
        Serial.println();
    }
    Serial.println();
}

/*Remove consecutive block sequences and update the score.*/
void eraseBlocks() {
    for (int j = 14; j >= 0; --j) {
        for (int i = 0; i < 6; ++i) {
            if (ColCode[i][j] == 1) {
                int y = coor_to_y(j);
                int x = coor_to_x(i);

                tft.fillRect(x, y, COL_WIDTH, BLOCK_HEIGHT, BLACK);
                BlkMap[i][j] = Black;
                // if the block to the left is not black, draw the white border
                if (BlkMap[i-1][j] != Black && i != 0) {
                    tft.drawLine(x, y, x, y+BLOCK_HEIGHT-1, WHITE);
                }
                // if the block to the right is not black, draw the white border
                if (BlkMap[i+1][j] != Black && i != 5) {
                    tft.drawLine(x+COL_WIDTH-1, y, x+COL_WIDTH-1, y+BLOCK_HEIGHT-1, WHITE);
                }
                // if the block below is not black, draw the white border
                if (BlkMap[i][j-1] != Black && j != 0) {
                    tft.drawLine(x, y+BLOCK_HEIGHT-1, x+COL_WIDTH-1, y+BLOCK_HEIGHT-1, WHITE);
                }
                // if the block above is not black, draw the white border
                if (BlkMap[i][j+1] != Black) {
                    tft.drawLine(x, y, x+COL_WIDTH-1, y, WHITE);
                }
                score = score + level; // the score is incremented by the level for every block that disappears
            }
        }
    }
    updateScore(); //prints the new score to the tft display
}

/*After erasing the blocks, move any coloured blocks above
the erased ones down to fill the empty spaces.
-the check variable is set to true if any blocks are moved to
cause the program to re-check*/
void dropBlocks(bool* check) {
    int k;
    for (int j = 13; j >= 0; --j) {
        for (int i = 0; i < 6; ++i) {
            // find a block that is black and see if there is a non-black block above
            if (BlkMap[i][j] == Black && BlkMap[i][j+1] != Black) {
                *check = true; // re-check since blocks will be moved
                for (k = j; BlkMap[i][k+1] != Black; ++k) {
                    int x = coor_to_x(i);
                    int y = coor_to_y(k);

                    // draw the new block
                    tft.drawRect(x, y, COL_WIDTH, BLOCK_HEIGHT, WHITE);
                    tft.fillRect(x+1, y+1, COL_WIDTH-2, BLOCK_HEIGHT-2, BlkMap[i][k+1]);

                    // update the colour of drawn block
                    BlkMap[i][k] = BlkMap[i][k+1];
                }
                int x = coor_to_x(i);
                int y = coor_to_y(k);
                // erase the top block since it has been moved down
                tft.fillRect(x, y, COL_WIDTH, BLOCK_HEIGHT-1, BLACK);

                // if the block to the left is not black, draw the white border
                if (BlkMap[i-1][k] != Black && i != 0) {
                    tft.drawLine(x, y, x, y+BLOCK_HEIGHT-1, WHITE);
                }
                // if the block to the right is not black, draw the white border
                if (BlkMap[i+1][k] != Black && i != 5) {
                    tft.drawLine(x+COL_WIDTH-1, y, x+COL_WIDTH-1, y+BLOCK_HEIGHT-1, WHITE);
                }
                // set the top block to Black
                BlkMap[i][k] = Black;
                delay(300);
            }
        }
    }
}

/*Checks each row to determine if there are 3 or more
blocks of the same colour. Updates the ColCode
array to match*/
void rowCheck() {
    // check all 6 columns to start
    int uppLim = 5;
    int lowLim = 0;
    // iterate through the rows starting from the bottom
    for (int j= 0; j <= 14; ++j) {
        // the starting sequence length is 1
        int length = 1;
        // there is only two columns per row --> no need to continue checking
        if ((uppLim - lowLim) < 2) {
            break;
        }
        // iterate through the columns starting from the right most one
        for (int i = uppLim; i > lowLim; --i) {
            if (BlkMap[uppLim][j] == Black) {
                --uppLim;
            }
            if (BlkMap[lowLim][j] == Black) {
                ++lowLim;
            }
            if (BlkMap[i][j] == Black) {
            }
            // if the block before is the same colour as the current block
            // add 1 to the length
            else if (BlkMap[i][j] == BlkMap[i-1][j]) {
                ++length;
                if (length >= 3) {
                    for (int k = i-1; k < i - 1 + length; ++k) {
                        ColCode[k][j] = 1;
                    }
                }
            }
            // if the block before is different and the sequence is less than 3,
            // reset the length
            else {
                length = 1;
            }
        }
    }
}

/*Checks each column to determine if there are 3 or more
blocks in a row of the same colour. Updates the ColCode
array to match*/
void columnCheck() {
    int length;
    int uppLim = 14;

    // check each column
    for (int i = 0; i <= 5; ++i) {
        // the starting length of a sequence is 1
        length = 1;
        // iterate through the rows starting from the top most one
        for (int j = uppLim; j >= 0; --j) {
            if (BlkMap[i][j] == Black) {
            }
            // if the block above is the same colour as the current block
            // add 1 to the length
            else if (BlkMap[i][j] == BlkMap[i][j-1]) {
                ++length;
                if (length >= 3) {
                    for (int k = j-1; k < j-1 + length; ++k) {
                        ColCode[i][k] = 1;
                    }
                }
            }
            // if the block before is different and the sequence is less than 3,
            // reset the length
            else {
                length = 1;
            }
        }
    }
}

/*Determines the number of times the right diagonal
checker must iterate by returning the smaller
of two distances.*/
int numIterationsRight(int i, int j) {
    // calculate the smaller of 14-j and 5-i
    // return the value
    if ((14-j) < (5-i)) {
        return 14-j; //the distance from j to the top of the grid
    }
    else {
        return 5-i; //the distance from i to the right side of the grid
    }
}

/*Determines the number of times the left diagonal
checker must iterate by returning the smaller
of two distances*/
int numIterationsLeft(int i, int j) {
    // calculate the smaller of 14-j and i-0
    if ((14-j) < (i)) {
        return 14-j; //the distance from j to the top of the grid
    }
    else {
        return i; //the distance from i to the left of the grid
    }
}

/*Checks each diagonal to determine if there are 3 or more
blocks in a row of the same colour. Starts in the bottom
left hand corner and moves up and right to check each
diagonal. Updates the ColCode array to match*/
void rDiagonalCheck() {
    // start from bottom left and move up
    int x = 0;
    int length;
    int iterations;
    for (int y = 0; y <= 12; ++y) {
        length = 1;
        iterations = numIterationsRight(x,y);
        int j = y;

        for (int i = 0; i < iterations; ++i) {
            if (BlkMap[i][j] == Black) {
            }
            else if (BlkMap[i][j] == BlkMap[i+1][j+1]) {
                ++length;
                if (length >= 3) {
                    int h = j + 1;
                    for (int k = i + 1; k > (i+1) - length; --k) {
                        ColCode[k][h] = 1;
                        --h;
                    }
                }
            }
            else {
                length = 1;
            }
            ++j;
        }
    }

    // start from 1 right of bottom left and move right
    int y = 0;
    for (int x = 1; x <= 3; ++x) {
        length = 1;
        iterations = numIterationsRight(x,y);
        int j = 0;

        for (int i = x; i < x+iterations; ++i) {
            if (BlkMap[i][j] == Black) {
            }
            else if (BlkMap[i][j] == BlkMap[i+1][j+1]) {
                ++length;
                if (length >= 3) {
                    int h = j + 1;
                    for (int k = i + 1; k > (i+1) - length; --k) {
                        ColCode[k][h] = 1;
                        --h;
                    }
                }
            }
            else {
                length = 1;
            }
            ++j;
        }
    }
}

/*Checks each diagonal to determine if there are 3 or more
blocks in a row of the same colour. Starts in the bottom
right hand corner and moves up and left to check each
diagonal. Updates the ColCode array to match*/
void lDiagonalCheck() {
    // start from bottom right and move up
    int x = 5;
    int length;
    int iterations;
    for (int y = 0; y <= 12; ++y) {
        length = 1;
        iterations = numIterationsLeft(x,y);
        int j = y;
        for (int i = 5; i > 5-iterations; --i) {
            if (BlkMap[i][j] == Black) {
            }
            else if (BlkMap[i][j] == BlkMap[i-1][j+1]) {
                ++length;
                if (length >= 3) {
                    int h = j + 1;
                    for (int k = i - 1; k < (i-1) + length; ++k) {
                        ColCode[k][h] = 1;
                        --h;
                    }
                }
            }
            else {
                length = 1;
            }
            ++j;
        }
    }

    // start from one to the left of bottom right and move left
    int y = 0;
    for (int x = 4; x >= 2; --x) {
        length = 1;
        int j = 0;
        iterations = numIterationsLeft(x,y);

        for (int i = x; i > x-iterations; --i) {
            if (BlkMap[i][j] == Black) {
            }
            else if (BlkMap[i][j] == BlkMap[i-1][j+1]) {
                ++length;
                if (length >= 3) {
                    int h = j + 1;
                    for (int k = i - 1; k < (i-1) + length; ++k) {
                        ColCode[k][h] = 1;
                        --h;
                    }
                }
            }
            else {
                length = 1;
            }
            ++j;
        }
    }
}

/*Checks the blocks, erases any consecutive sequences
and moves blocks down to fill the empty spaces.*/
void checkBlocks(bool* check) {
    rowCheck(); //check for 3 blocks of the same colour in a row, column and diagonal
    columnCheck();
    rDiagonalCheck();
    lDiagonalCheck();
    //printColCode(); //prints the colour code to the serial monitor - was used to check that it worked correctly

    // Delete blocks and move any blocks above the erased ones down
    // into the empty spaces
    eraseBlocks();
    resetColCode(); //reset the colour code in between checks
    dropBlocks(check);
}

/*Draws one stack of three blocks to the TFT screen at a given pixel coordinate
-the horizontal value of the coordinate is set globally prior to entering the function */
void drawStack(int location_y, Shade Bcolour, Shade Mcolour, Shade Tcolour) {
    //bottom block
    tft.drawRect(location_x, location_y, COL_WIDTH, BLOCK_HEIGHT, WHITE);
    tft.fillRect(location_x+1, location_y+1, COL_WIDTH-2, BLOCK_HEIGHT-2, Bcolour);

    //middle block of stack
    tft.drawRect(location_x, location_y - (BLOCK_HEIGHT)+1, COL_WIDTH, BLOCK_HEIGHT, WHITE);
    tft.fillRect(location_x+1, location_y - (BLOCK_HEIGHT)+2, COL_WIDTH-2, BLOCK_HEIGHT-2, Mcolour);

    //top block of stack
    tft.drawRect(location_x, location_y - (2*BLOCK_HEIGHT)+2, COL_WIDTH, BLOCK_HEIGHT, WHITE);
    tft.fillRect(location_x+1, location_y - (2*BLOCK_HEIGHT)+3, COL_WIDTH-2, BLOCK_HEIGHT-2, Tcolour);
}

/*Prints a new vertical stack of 3 blocks*/
void newBlockStack(int* location_y, Shade* nextBcolour, Shade* nextMcolour, Shade* nextTcolour, int* BlkLocation) {
    *location_y = 0;
    *nextBcolour = randomColour();
    delay(50);
    *nextMcolour = randomColour();
    delay(50);
    *nextTcolour = randomColour();
    *BlkLocation = 14;
    location_x = 88;
    drawStack(130, *nextBcolour, *nextMcolour, *nextTcolour);
    location_x = ENTER_COL; // the top right corner of the border block
    new_location_x = location_x;
}

/*Prints game over to the tft screen to end the game.*/
void gameOver() {
    tft.setCursor(15,25);
    tft.setTextSize(4);
    tft.setTextColor(RED);
    tft.println("GAME");
    tft.setCursor(10,90);
    tft.println("OVER!");
    tft.fillRect(0,0,61,9, RED);
}

//Returns the y coordinate of the nearest block below location_y
int blkbelow (int location_y) {
    for (int i = 1; i < 15; ++i) {
        if ((9 + i*10) > location_y) {
            return 9+i*10;
        }
    }
}

/*When a stack is moved horizontally to a new column, erase it
before redrawing it in its new location. Check if there is an
existing column to the left or right - if so, do not erase the
white border on that stack.*/
void EraseStack(int* location_y) {
    //old block is replaced with a black rectangle
    //however, must also check if there is already a column to the left or right, depending on the direction moved
    //in the case of a column already existing, the white border should NOT be erased

    //if at edges of grid, just redraw full rectangle
    if (location_x == 0 || location_x == 50) {
        tft.fillRect(location_x, *location_y - (BLOCK_HEIGHT*2)-1, COL_WIDTH, BLOCK_HEIGHT*3, BLACK);
    }

    //if block moved right
    else if (new_location_x > location_x) {

        //we want the coordinate of the nearest block below a y coordinate
        //this is necessary because a block could be moved left or right at any point during its fall
        int blk = blkbelow(*location_y-2*BLOCK_HEIGHT-1); //the nearest block below a coordinate (19-149)
        int blkmap_y = y_to_coor(blk); //gives the block map coordinate of the nearest block below location_y (0-13)

        //if the nearest block below and to the left of the top block is not black
        if (BlkMap[location_x/10-1][blkmap_y] != Black && location_x != 0) { //block must also not be at edge

            if (BlkMap[location_x/10-1][blkmap_y+1] != Black) { //if all blocks to the left are not black
                tft.fillRect(location_x+1, *location_y - (BLOCK_HEIGHT*2)-1, COL_WIDTH, BLOCK_HEIGHT*3, BLACK); //do not erase white line
            }

            else {
                //print black rectangle in old location of block where no colour exists to the left
                tft.fillRect(location_x, *location_y - (BLOCK_HEIGHT*2)-1, COL_WIDTH, blk-(*location_y-2*BLOCK_HEIGHT-1), BLACK);
                //do not erase white line where a column exists
                tft.fillRect(location_x+1, blk, COL_WIDTH, BLOCK_HEIGHT*3-(blk-(*location_y-2*BLOCK_HEIGHT-1)), BLACK);
            }
        }

        else { //now check middle block of stack
            blk = blkbelow(*location_y-BLOCK_HEIGHT-1); //the nearest block below the middle block of the stack
            blkmap_y = y_to_coor(blk); //gives the block map location of the nearest block below location_y

            if (BlkMap[location_x/10-1][blkmap_y] != Black) {
                //print in black
                tft.fillRect(location_x, *location_y - (BLOCK_HEIGHT*2)-1, COL_WIDTH, blk-(*location_y-2*BLOCK_HEIGHT-1), BLACK);
                //leave white line
                tft.fillRect(location_x+1, blk, COL_WIDTH, BLOCK_HEIGHT*3-(blk-(*location_y-2*BLOCK_HEIGHT-1)), BLACK);
            }

            else { //now check bottom block of stack
                blk = blkbelow(*location_y-1); //the nearest block below a the bottom block of the stack
                blkmap_y = y_to_coor(blk); //gives the block map location of the nearest block below location_y

                if (BlkMap[location_x/10-1][blkmap_y] != Black) {
                    //print in black
                    tft.fillRect(location_x, *location_y - (BLOCK_HEIGHT*2)-1, COL_WIDTH, blk-(*location_y-2*BLOCK_HEIGHT-1), BLACK);
                    //leave white line
                    tft.fillRect(location_x+1, blk, COL_WIDTH, BLOCK_HEIGHT*3-(blk-(*location_y-2*BLOCK_HEIGHT-1)), BLACK);
                }

                else { //no coloured blocks to the left, just print a black rectangle
                    tft.fillRect(location_x, *location_y - (BLOCK_HEIGHT*2)-1, COL_WIDTH, BLOCK_HEIGHT*3, BLACK);
                }
            }
        }
    }

    //if block has moved left
    else if (new_location_x < location_x) {

        //we want the coordinate of the nearest block below a y coordinate
        //this is necessary because a block could be moved left or right at any point during its fall
        int blk = blkbelow(*location_y-2*BLOCK_HEIGHT-1); //the nearest block below a coordinate
        int blkmap_y = y_to_coor(blk); //gives the block map location of the nearest block below location_y

        //check top block
        if (BlkMap[location_x/10+1][blkmap_y] != Black && location_x != 50) { //if the nearest block below and to the right is not black

            if (BlkMap[location_x/10+1][blkmap_y+1] != Black) { //if all blocks to the left are coloured
                tft.fillRect(location_x, *location_y - (BLOCK_HEIGHT*2)-1, COL_WIDTH-1, BLOCK_HEIGHT*3, BLACK);
            }

            else {
                //print in black
                tft.fillRect(location_x, *location_y - (BLOCK_HEIGHT*2)-1, COL_WIDTH, blk-(*location_y-2*BLOCK_HEIGHT-1), BLACK);
                //leave white line
                tft.fillRect(location_x, blk, COL_WIDTH-1, BLOCK_HEIGHT*3-(blk-(*location_y-2*BLOCK_HEIGHT-1)), BLACK);
            }
        }
        else { //check middle block
            blk = blkbelow(*location_y-BLOCK_HEIGHT-1);
            blkmap_y = y_to_coor(blk);

            if (BlkMap[location_x/10+1][blkmap_y] != Black) {
                //print in black
                tft.fillRect(location_x, *location_y - (BLOCK_HEIGHT*2)-1, COL_WIDTH, blk-(*location_y-2*BLOCK_HEIGHT-1), BLACK);
                //leave white line
                tft.fillRect(location_x, blk, COL_WIDTH-1, BLOCK_HEIGHT*3-(blk-(*location_y-2*BLOCK_HEIGHT-1)), BLACK);
            }

            else { //check bottom block
                blk = blkbelow(*location_y-1);
                blkmap_y = y_to_coor(blk);

                if (BlkMap[location_x/10+1][blkmap_y] != Black) {
                    //print in black
                    tft.fillRect(location_x, *location_y - (BLOCK_HEIGHT*2)-1, COL_WIDTH, blk-(*location_y-2*BLOCK_HEIGHT-1), BLACK);
                    //leave white line
                    tft.fillRect(location_x, blk, COL_WIDTH-1, BLOCK_HEIGHT*3-(blk-(*location_y-2*BLOCK_HEIGHT-1)), BLACK);
                }

                else { //no coloured blocks to the right, just print a black rectangle
                    tft.fillRect(location_x, *location_y - (BLOCK_HEIGHT*2)-1, COL_WIDTH, BLOCK_HEIGHT*3, BLACK);
                }
            }
        }
    }

    else { //if there is no column to the left or right, erase the full stack
        tft.fillRect(location_x, *location_y - (BLOCK_HEIGHT*2)-1, COL_WIDTH, BLOCK_HEIGHT*3, BLACK);
    }
}

int main () {
    init();
    Serial.begin(9600);

    setup(); //Initializes TFT, joystick, and button and prints introductory menus as well as the game screen

    int location_y = 0; // all falling blocks are drawn with reference to this location
    Shade nextBcolour = randomColour();
    delay(50);
    Shade nextMcolour = randomColour();
    delay(50);
    Shade nextTcolour = randomColour();
    Shade Bcolour; // the colour of the bottom block
    Shade Mcolour; // the colour of the middle block
    Shade Tcolour; // the colour of the top block

    unsigned long long startTime = millis(); //used to determine level

    int BlkLocation = 14;
    bool check = false;

    while (true) {
        Bcolour = nextBcolour;
        Mcolour = nextMcolour;
        Tcolour = nextTcolour;
        // prep a new stack and determine the colours of the future stack
        newBlockStack(&location_y, &nextBcolour, &nextMcolour, &nextTcolour, &BlkLocation);
        while (true) {
            //if the stack has been moved horizontally, shift it to a new column
            if (new_location_x != location_x) {

                EraseStack(&location_y); //erase the stack in the old location

                location_x = new_location_x; //update x coordinate

                //redraw the blocks in the new column
                drawStack(location_y, Bcolour, Mcolour, Tcolour);

                //delay makes block fall more slowly when moving horizontally
                //but is necessary to have here or block would shift too fast
                delay(100);
            }
            else {
                //erases the last block as it falls
                tft.drawLine(location_x + 1, location_y - (2*BLOCK_HEIGHT)+1, location_x + COL_WIDTH-2, location_y - (2*BLOCK_HEIGHT)+1, BLACK);

                int block = constrain(BlkLocation + 2, 0, 14);

                //if the column to the left is black or if block is at left edge of screen
                if (BlkMap[(location_x/10) - 1][block] == Black || location_x == 0) {
                    tft.drawPixel(location_x, location_y - (2*BLOCK_HEIGHT)+1, BLACK);
                }
                //if the column to the right is black or if block is at right edge of screen
                if (BlkMap[(location_x/10) + 1][block] == Black || location_x == 50) {
                    tft.drawPixel(location_x + COL_WIDTH-1, location_y - (2*BLOCK_HEIGHT)+1, BLACK);
                    tft.drawPixel(location_x + COL_WIDTH-1, location_y - (2*BLOCK_HEIGHT), BLACK);
                }

                drawStack(location_y, Bcolour, Mcolour, Tcolour);
            }

            BlkLocation = y_to_coor(location_y);

            //when the blocks have reached the bottom of the screen or have landed on another stack
            if (BlkMap[location_x/10][BlkLocation - 1] != Black || location_y == SCREEN_SIZE_Y - BLOCK_HEIGHT) {
                BlkMap[location_x/10][BlkLocation] = Bcolour;
                BlkMap[location_x/10][BlkLocation+1] = Mcolour;
                BlkMap[location_x/10][BlkLocation+2] = Tcolour;

                // colour check for three or more in a row, diagonal, or column
                do {
                    check = false;
                    checkBlocks(&check);
                } while(check == true);

                //game over if any of the three blocks in the stack are at the top of the grid after checking is complete
                if (BlkMap[location_x/10][BlkLocation+1] != Black && BlkLocation+2 == 14) {
                    gameOver();
                    return 0;
                }
                else if (BlkMap[location_x/10][BlkLocation] != Black && BlkLocation+1 == 14) {
                    gameOver();
                    return 0;
                }
                else if (BlkMap[location_x/10][BlkLocation-1] != Black && BlkLocation == 14) {
                    gameOver();
                    return 0;
                }
                if ((millis() - startTime) >= 60000) {
                    level += 1;
                    levelUp();
                    startTime = millis();
                    if (level == 10) {
                        tft.setCursor(66,150);
                        tft.print("MAX SPEED!");
                    }
                }
                break;
            }

            // check to see if the joystick has been moved
            scanJoystick(BlkLocation);

            //increment y and delay
            ++location_y;
            delay(fallDelay);

            colourChange(&Bcolour, &Mcolour, &Tcolour); //check if the order of the coloured blocks has been changed
            pauseButton(&startTime); // pause the game if joystick button is pressed until re-pressed and released
        }
    }

    Serial.end();
    return 0;
}
