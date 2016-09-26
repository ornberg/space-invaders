#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h> //for srand() and rand()
#include <unistd.h> //for usleep() available
#include <time.h> //seed for random


///////////////////////////////////////////////////////////////////////////////////////// VARIABLES & CONSTANTS

//General game specific variables
#define UPDATEINTERVAL 25000
short int screenWidth, screenHeight;
short int gameScore;
short int shooterLives;
int invaderUpdateInterval;
short int animationFlag = FALSE;


//Shooter
#define SHOOTERTOP "  A"
#define SHOOTERBOTTOM "_/A\\_"
#define FLASH "."
#define SHOOTERWIDTH 6
#define SHOOTERHEIGHT 2
#define SHOOTERSPEED 2
short int shooterXPosition;
short int shooterHitFlag;

//Shot
#define SHOT "|"
#define SHOTSPEED 1
short int shotXPosition, shotYPosition;
short int shotFired = FALSE;


//Bomb
#define BOMB "0"
#define BOMBSPEED 1
short int bombXPosition, bombYPosition;
short int bombDropped = FALSE;


//Invaders
#define INVADERWIDTH 6
#define INVADERHEIGHT 3
#define INVADERINCREASESPEEDRATE 25000
#define INVADERDESCENDRATE 2
short int invaders[55][3];
const char *invaderOne[] = {" o  o", "d0000b", " {  }", " [  ]"};
const char *invaderTwo[] = {" /00\\", "[WWWW]", "^    ^", " ^  ^"};
const char *invaderThree[] = {"  @@", "{0000}", "\\ \"\" /", "/ \"\" \\"};
short int invaderDirection = 1; //start by moving to the right
short int numberOfInvaders, invaderRows, invaderColumns;


//Blockades
#define BLOCKADEWIDTH 16
const char *shield[] = {"m", "M"};
short int shields[144][3];



///////////////////////////////////////////////////////////////////////////////////////// GAME SETUP/RESET FUNCTION

void gameSetup(int initialSetup) {
  int i, x, row, column;
  
  //Calculate  depending on screen dimensions
  int blockadeSectionWidth, invaderMargin, numberOfBlockades;
  
  //Set invader rows based on screen height
  if (screenHeight < 50)
    invaderRows = screenHeight/12;
  else
    invaderRows = 5;
  
  //Set invader columns and number of blockades based on screen width
  if (screenWidth < 160) {
    invaderColumns =  screenWidth/14;
    numberOfBlockades = screenWidth/40;
  }
  else {
    invaderColumns = 11;
    numberOfBlockades = 4;
  }
  
  //Work out number of invaders, and spacing for invaders and blockades
  numberOfInvaders = invaderRows * invaderColumns;
  invaderMargin = (screenWidth-(invaderColumns*INVADERWIDTH*2))/2;
  blockadeSectionWidth = screenWidth/((numberOfBlockades*2-1)+2);
  
  
  //Initialize invaders
  x = 0;
  for (row = 0; row < invaderRows; row++) {
    for (column = 0; column < invaderColumns; column++) {
      invaders[x][0] = invaderMargin + column*INVADERWIDTH*2; //x position
      invaders[x][1] = row*INVADERHEIGHT*2 + INVADERHEIGHT; //y position
      invaders[x][2] = TRUE; //set visible to true
      x++;
    }
  }
  
  //If first game setup, setup game variables, player and blockades
  if (initialSetup == TRUE) {
    gameScore = 0;
    shooterLives = 3;
    shooterHitFlag = FALSE;
    invaderUpdateInterval = 250000;
    shotFired = FALSE;
    bombDropped = FALSE;
    
    //Initialize shooter
    shooterXPosition = screenWidth/2-SHOOTERWIDTH/2;
    
    //Initialize blockades
    x = 0;
    for (i = 0; i < numberOfBlockades; i++) { //For every blockade
      for (row = 0; row < 3; row++) { //For every lines of current blockade
        for (column = 0; column < BLOCKADEWIDTH; column++) { //For every block of shield
          if (row == 2 && column == 2) //Only four shields on bottom row
            column = BLOCKADEWIDTH-2;
          shields[x][0] = blockadeSectionWidth + i*blockadeSectionWidth*2 + column; //set X
          shields[x][1] = screenHeight-SHOOTERHEIGHT-4+row; //set Y
          shields[x][2] = 2; //set state of shield block
          x++;
        }
      }
    }
  }
  
  //Advancing to next level
  if (initialSetup == FALSE) {
    invaderUpdateInterval -= INVADERINCREASESPEEDRATE; //increase speed by 1/20 of original
    shooterLives++;
  }
  
}




///////////////////////////////////////////////////////////////////////////////////////// INVADERS HANDLER

void invadersHandler() {
  static short int invertDirectionFlag;
  int i, j;
  
  
  //In 1 out 3, drop a bomb
  if (!(rand()%3)) {
    i = rand() % invaderColumns + 1;
    
    //Find a random invader with no other invader below, and drop bomb from there
    while (!bombDropped) {
      if (i >= invaderColumns)
        i = 0;
      
      for (j = invaderRows-1; j >= 0; j--) {
        if (invaders[j*invaderColumns+i][2] != 0) {
          bombDropped = TRUE;
          bombXPosition = invaders[j*invaderColumns+i][0]+INVADERWIDTH/2;
          bombYPosition = invaders[j*invaderColumns+i][1]+INVADERHEIGHT;
          break;
        }
      }
      i++;
    }
  }

  //If hit edge of screen, flag direction change
  for (i = 0; i < 55; i++) {
    if (invaders[i][2]) {
      invaders[i][0] += invaderDirection;
      
      if (invaders[i][0] + INVADERWIDTH + 1 >= screenWidth || invaders[i][0] - 1 <= 0)
        invertDirectionFlag = TRUE;
    }
  }
  
  //If flagged; change direction, move all invaders down and increase update interval
  if (invertDirectionFlag == TRUE) {
    invertDirectionFlag = FALSE;
  
    for (i = 0; i < 55; i++) {
      invaders[i][1] += INVADERDESCENDRATE;
      
      
      //check if invaders have hit bottom of screen (game end)
      if (invaders[i][2] != 0 && invaders[i][1]+INVADERHEIGHT >= screenHeight-SHOOTERHEIGHT)
        shooterLives = 0;
    }
    
    //Invert direction
    if (invaderDirection > 0)
      invaderDirection = -1;
    else
      invaderDirection = 1;
  }
  
  if (animationFlag == TRUE)
    animationFlag = FALSE;
  else
    animationFlag = TRUE;
}



///////////////////////////////////////////////////////////////////////////////////////// PROJECTILES HANDLER

void projectilesHandler() {
  short int i;
  
  //Shot has moved off screen
  if (shotYPosition <= 0)
    shotFired = FALSE;
    
  if (bombYPosition > screenHeight)
    bombDropped = FALSE;
  
  //1: Check if shot has hit invader
  if (shotFired) {
    for (i = 0; i < 55; i++) {
      if (invaders[i][2]) { //if invader is not already shot
        if (invaders[i][0] <= shotXPosition && invaders[i][0]+INVADERWIDTH >= shotXPosition && invaders[i][1] <= shotYPosition && invaders[i][1]+INVADERHEIGHT >= shotYPosition) {
          invaders[i][2] = FALSE;
          numberOfInvaders--;
          shotFired = FALSE;
          
          //Give score based on row position of shot invader
          if (i/invaderColumns < 1)
            gameScore += 30;
          else if (i/invaderColumns < 3)
            gameScore += 20;
          else
            gameScore += 10;
          
          //If wave of invaders are killed, advance to next level
          if (numberOfInvaders == 0)
            gameSetup(FALSE);
        }
      }
    }
  }
  
  
  //2: Check if bomb has hit player
  if (bombDropped) {
    if (bombXPosition >= shooterXPosition && bombXPosition <= shooterXPosition+SHOOTERWIDTH && bombYPosition >= screenHeight-3) {
      shooterLives--;
      shooterHitFlag = TRUE;
      bombDropped = FALSE;
    }
  }
  
  
  //3 & 4: Check if shot or bomb has hit shield; damage shield
  if (shotYPosition >= screenHeight-6 || bombYPosition >= screenHeight-6) {
    for (i = 0; i < 144; i++) {
      if (shields[i][2]) { //if shield is up
        if (shotFired && shotXPosition == shields[i][0] && shotYPosition == shields[i][1]) {
          shields[i][2]--;
          shotFired = FALSE;
        }
        if (bombDropped && bombXPosition == shields[i][0] && bombYPosition == shields[i][1]) {
          shields[i][2]--;
          bombDropped = FALSE;
        }
      }
    }
  }
  
  //5: And finally we move the shot and the bomb
  shotYPosition-=SHOTSPEED;
  bombYPosition+=BOMBSPEED;
}




///////////////////////////////////////////////////////////////////////////////////////// DISPLAY OBJECTS

//Run through all objects and print them to screen
void displayObjects() {
  int i, j;
  
  //Flash screen if shooter has been hit
  if (shooterHitFlag) {
    clear();
    for (i = 0; i < screenHeight; i++) {
      for (j = 0; j < screenWidth; j++) {
        mvprintw(i,j, FLASH);
      }
    }
    refresh();
    shooterHitFlag = FALSE;
    usleep(UPDATEINTERVAL);
  }
  
  //Display invaders
  for (i = 0; i < invaderRows; i++) {
    const char *(*currentInvader)[4];
    
    if (i == 0)
      currentInvader = &invaderOne;
    else if (i > 2)
      currentInvader = &invaderThree;
    else
      currentInvader = &invaderTwo;
    
    //If invader has not been shot, display
    for (j = 0; j < invaderColumns; j++) {
      if (invaders[i*invaderColumns+j][2]) {
        mvprintw(invaders[i*invaderColumns+j][1], invaders[i*invaderColumns+j][0], (*currentInvader)[0]);
        mvprintw(invaders[i*invaderColumns+j][1]+1, invaders[i*invaderColumns+j][0], (*currentInvader)[1]);
        mvprintw(invaders[i*invaderColumns+j][1]+2, invaders[i*invaderColumns+j][0], (*currentInvader)[2+animationFlag]);
      }
    }
  }
  
  //Display shields if not destroyed
  for (i = 0; i < 144; i++) {
    if (shields[i][2])
      mvprintw(shields[i][1], shields[i][0], shield[shields[i][2]-1]);
  }
  
  //Display shot if fired
  if (shotFired)
    mvprintw(shotYPosition, shotXPosition, SHOT);
  
  //Display bomb if dropped
  if (bombDropped)
    mvprintw(bombYPosition, bombXPosition, BOMB);
  
  //Display shooter
  mvprintw(screenHeight-3, shooterXPosition, SHOOTERTOP);
  mvprintw(screenHeight-2, shooterXPosition, SHOOTERBOTTOM);
  
  //Print game status lines at bottom of screen
  mvprintw(screenHeight-1, 0, "Score:%d", gameScore);
  mvprintw(screenHeight-1, screenWidth-7, "Lives:%d", shooterLives);
}



///////////////////////////////////////////////////////////////////////////////////////// GAME LOOP

void loop() {
  int invaderTimer = 0;
  short int keyPressed;
  
  while (shooterLives) {
    usleep(UPDATEINTERVAL); //continue screen update after UPDATEINTERVAL milliseconds
    clear(); //clear screen
    displayObjects(); //place all objects accordingly on screen, and update
    refresh();
    
    
    keyPressed = getch(); //get current pressed key
    
    //Handle keypresses
    switch(keyPressed) {
      case KEY_LEFT: //Move left
        if (shooterXPosition - SHOOTERSPEED > 0)
          shooterXPosition-=SHOOTERSPEED;
        break;
      
      case KEY_RIGHT: //Move right
        if (shooterXPosition + SHOOTERWIDTH <= screenWidth)
          shooterXPosition+=SHOOTERSPEED;
        break;
      
      case KEY_UP: //Fire shot
        if (!shotFired) {
          shotXPosition = shooterXPosition+2;
          shotYPosition = screenHeight-3;
          shotFired = TRUE;
        }
        break;
      
      case 113: //Key Q to kill program
        shooterLives = 0;
        break;
    }
    
    //Update timers
    invaderTimer += UPDATEINTERVAL;
    
    //Handle screen action
    if (shotFired || bombDropped)
      projectilesHandler();
    
    //If enought time has passed, move invaders
    if (invaderTimer >= invaderUpdateInterval) {
      invaderTimer = 0;
      invadersHandler();
    }
  }
}



///////////////////////////////////////////////////////////////////////////////////////// GAME INTRO

void gameIntro() {
  clear();
  mvprintw(screenHeight/2-10, screenWidth/2-35, "=======================================================================");
  mvprintw(screenHeight/2-9, screenWidth/2-35, " ___   ___   ___   ___  ___                  ___   __   ___   ___  ___");
  mvprintw(screenHeight/2-8, screenWidth/2-35, "|___  |___| |___| |    |___     | |\\ | \\  / |___| |  \\ |___  |__/ |___");
  mvprintw(screenHeight/2-7, screenWidth/2-35, " ___| |     |   | |___ |___     | | \\|  \\/  |   | |__/ |___  |  \\  ___|");
  mvprintw(screenHeight/2-5, screenWidth/2-35, "=======================================================================");
  
  mvprintw(screenHeight/2-3, screenWidth/2-35, invaderThree[0]);
  mvprintw(screenHeight/2-2, screenWidth/2-35, invaderThree[1]);
  mvprintw(screenHeight/2-1, screenWidth/2-35, invaderThree[2]);
  mvprintw(screenHeight/2+1, screenWidth/2-36, "10 points");
  
  mvprintw(screenHeight/2-3, screenWidth/2-3, invaderTwo[0]);
  mvprintw(screenHeight/2-2, screenWidth/2-3, invaderTwo[1]);
  mvprintw(screenHeight/2-1, screenWidth/2-3, invaderTwo[2]);
  mvprintw(screenHeight/2+1, screenWidth/2-4, "20 points");
  
  mvprintw(screenHeight/2-3, screenWidth/2+29, invaderOne[0]);
  mvprintw(screenHeight/2-2, screenWidth/2+29, invaderOne[1]);
  mvprintw(screenHeight/2-1, screenWidth/2+29, invaderOne[2]);
  mvprintw(screenHeight/2+1, screenWidth/2+28, "30 points");
  
  mvprintw(screenHeight/2+4, screenWidth/2-12, "Press any key to continue");
  refresh();
  nodelay(stdscr, FALSE);
  getch();
}


///////////////////////////////////////////////////////////////////////////////////////// GAME END

int gameEnd() {
  clear();
  mvprintw(screenHeight/2-3, screenWidth/2-24, "=================================================");
  mvprintw(screenHeight/2-2, screenWidth/2-24, " ___   ___   _  _  ___      ___        ___   ___");
  mvprintw(screenHeight/2-1, screenWidth/2-24, "| __  |___| | \\/ ||___     |   | \\  / |___  |__/");
  mvprintw(screenHeight/2, screenWidth/2-24, "|___| |   | |    ||___     |___|  \\/  |___  |  \\");
  mvprintw(screenHeight/2+2, screenWidth/2-24, "=================================================");
  
  mvprintw(screenHeight/2+4, screenWidth/2-5, "Your score: %d", gameScore);
  mvprintw(screenHeight/2+6, screenWidth/2-20, "Press R to restart or any other key to quit", gameScore);
  refresh();
  usleep(1000000); //Pause, so player does not accidentally hit key and quit program
  nodelay(stdscr, FALSE);
  return getch();
}


///////////////////////////////////////////////////////////////////////////////////////// MAIN

int main() {
  initscr(); //start cursors mode
  raw(); //disable line buffering
	curs_set(0);  //hide cursor
  keypad(stdscr, TRUE); //initialize keypad
  srand(time(NULL)); //seed random generator
  getmaxyx(stdscr, screenHeight, screenWidth); //get screen width and height
  
  
  
  int keyPressed;
  do {
    gameIntro();
    nodelay(stdscr, TRUE); //don't wait for getch
    gameSetup(TRUE); //game setup - initialize objects
    loop(); //start game
    keyPressed = gameEnd();
    mvprintw(0,0,"hello %d", keyPressed);
    refresh();
  } while (keyPressed == 114);

  
	endwin(); //end cursors mode
  return 0;
}










