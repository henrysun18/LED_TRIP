#include <Charliplexing.h>
#include <Font.h>
#include <TrueRandom.h> //we find this works better than randomSeed()
#include <avr/pgmspace.h> //the use of ROM (program memory) helps to save memory

//This is for the side-scrolling text
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

PROGMEM const uint16_t Start[9]=
{	1762	,
	2658	,
	1774	,
	0	,
	3383	,
	5458	,
	3378	,
	1362	,
	1362	};
PROGMEM const uint16_t Die[9]=
{	0	,
	1560	,
	816	,
	480	,
	192	,
	480	,
	816	,
	1560	,
	0	};
char Ready[]="READY!    ";
char Run[]="RUN       ";
char Tilt[]="TILT       ";
char YouLose[]="GG  YOUR  ";
char ScoreMessage[]="SCORE IS  ";
char Score[]="         ";
const int8_t BUTTON = A0;
const int8_t PRESSURE = A1;
const int8_t LDR = A2;
const int8_t TILT1 = A4;
const int8_t TILT2 = A5;
int8_t frameDelay; //time between each frame, basically a difficulty bar
int8_t tilt1,tilt2;
int8_t px,py,dpy,maxHeight; //Player X/Y coordinate & speed
int8_t score,lives; //start with as many lives as you want, we chose 5
int8_t ax,ay,dax; //obstacles
int8_t bx,by,dbx;
int8_t cx,cy,dcx;
int16_t threshold;
boolean warn,falling; //false=running game; true=tilt-sensing game
int8_t currentObstacle; //Provides the sequence of random obstacles

void setup(){
  LedSign::Init(DOUBLE_BUFFER | GRAYSCALE); //Initialize screen
  pinMode(BUTTON,INPUT);
  pinMode(PRESSURE,INPUT);
  pinMode(LDR,INPUT);
  pinMode(TILT1,INPUT);
  pinMode(TILT2,INPUT);
  
  
  Serial.begin(9600);
}

void loop(){
  resetVariables();
  startScreen();
  displayText(Ready);
  game();
  delay(2000);
  LedSign::Flip(1);
  LedSign::Flip(1);
  LedSign::Clear();
  LedSign::Flip(1);
  LedSign::Clear();
}

void startScreen(){
  for (int8_t y=0; y<9; y++){
    //"fetch data from program memory (PROGMEM) with a pointer"
    unsigned long rowInfo=pgm_read_word_near(&Start[y]); //temp storage of the row data
    for (int8_t x=0; x<14; x++){ //each row's LED's represent 1 bit (total 14 bits/row)
        if (rowInfo%2==1) LedSign::Set(x,y,2);
        else LedSign::Set(x,y,0);
        rowInfo=rowInfo/2; //check if next LED should be on
    }
  }
  while(analogRead(BUTTON)==0){
    LedSign::Flip(1);
    delay(800);
  }
}

void game(){
  while (1==1){
    // Making sure everything stays within boundaries
    if (py<=maxHeight) dpy=1; //gravity takes over
    if (py+2==8) dpy=0; //land on ground
    if (py>6) dpy=-1; //recover from being inside ground due to LDR
    if (ax==-3||bx==-4||cx==-7){ //recycles Obstacle motion
      ax=20;
      bx=20;
      cx=20;
      currentObstacle=TrueRandom.random(3); //random 1 out of 3 obstacles
      if (falling) ay=TrueRandom.random(6);
      score++;
    }
    Serial.print(analogRead(LDR));
    Serial.print(" ");
    Serial.println(threshold);
    // Take PLAYER input
    if (analogRead(LDR)<threshold){ //LDR has least priority since you might accidentally cover it when pressing something else
      dpy=1;
      if (py==8) dpy=0; //won't go farther down
    }
    if (analogRead(PRESSURE)>2){
      maxHeight=5;
      if (py>maxHeight) dpy=-1;
        if (py==maxHeight) dpy=0;
    }
    if (analogRead(BUTTON)>69){
      maxHeight=1;
      dpy=-1; //press button=jump!!
      if (py==maxHeight) 
        dpy=0; //hold button=fly!!
    }

    // Switching between Game 1 (run) and Game 2 (tilt)
    if (falling){
      currentObstacle=0;
      detectTilt(); //override all controls w/ tilt IF game switches to tilt game
    }
    if (!warn&&score%5!=0) warn=true;
    if (warn&&score%5==0){
      if (score%10==0){ //Run[] given priority when score=0
        py=6;
        ay=5;
        falling=false;
        displayText(Run); //after 5 more levels, back to running
        frameDelay-=8;
      }
      else if (score%5==0){
        py=3; 
        falling=true;
        displayText(Tilt); //after 10 levels, begin tilt game
        frameDelay-=8;
      }
      warn=false; //only shows warning every 5 levels
    }
    
    // Move the PLAYER & OBSTACLES
    py=py+dpy; 
    if (currentObstacle==0) ax=ax+dax;
    else if (currentObstacle==1) bx=bx+dax;
    else cx=cx+dcx;
   
    // Clears inactive screen to save memory
    LedSign::Clear(0);
 
    // Draw the PLAYER (3x3)
    for (int8_t y=py; y<py+3; y++)
      for (int8_t x=px; x<px+2; x++)
        if (y>=0&&y<9) LedSign::Set(x,y,3); //(because the LDR makes the player go out of bounds)
        
    // Draw Generated Obstacle   
    drawObstacle();
  
    // Next frame
    LedSign::Flip();
    
    // Detect collision for the obstacle onscreen
    detectCollision();
    delay(frameDelay);
    if (lives==0){
      delay(2000);
      String sc=String(score); //making sure the char[] is 10 elements long to send into displayText()
      if (sc.length()==1) sc+="         ";
      else if (sc.length()==2) sc+="        ";
      sc.toCharArray(Score, 10) ;
      displayText(YouLose);
      displayText(ScoreMessage);
      displayText(Score);
      break;
    }
  }
}

void die(){
  lives--;
  delay(300);
  LedSign::Clear(0);
  LedSign::Flip(1);
      delay(1000);
  for (int8_t y=0; y<9; y++){
    unsigned long rowInfo=pgm_read_word_near(&Die[y]); // Temp storage of the row data
    for (int8_t x=0; x<14; x++){
      if (rowInfo%2==1) LedSign::Set(x,y,2);
      else LedSign::Set(x,y,0);
      rowInfo=rowInfo/2; //check if next LED should be on
    }
  }
  do{ // Ensures X is shown at least once, even if player holds down input after death
    LedSign::Flip(1);
    delay(350);
  } while (analogRead(BUTTON)<10);
  
  // Reset the Player
  px=1;
  py=6;
  dpy=0;
  
  // Reset the Obstacles
  ax=20; 
  if (falling) ay=TrueRandom.random(6);
  else ay=5;
  bx=20;
  cx=20;
  currentObstacle=TrueRandom.random(3);
}

void drawObstacle(){
  
  if (currentObstacle==0){ //corresponds to A0 (button)
    for (int8_t y=ay; y<ay+4; y++)
      for (int8_t x=ax; x<ax+4; x++)
        if (x>=0&&x<=13&&y>=0&&y<=8) LedSign::Set(x,y,1); //because turning on LED's past the screen is buggy
  }
  else if (currentObstacle==1){ //corresponds to A1 (pressure)
    for (int8_t y=by; y<by+9; y++)
      for (int8_t x=bx; x<bx+4; x++)
        if (y<5||y>7)
          if (x>=0&&x<=13) LedSign::Set(x,y,1); 
  }
  else { //corresponds to A2 (LDR)
    for (int8_t y=cy; y<cy+8; y++)
      for (int8_t x=cx; x<cx+8; x++)
        if (x+y<=cx+7)
          if (x>=0&&x<=13) LedSign::Set(x,y,1);
  }
}

void detectCollision(){
  if (currentObstacle==0){
    if (px+1>=ax&&px<=ax+3&&py+2>=ay&&py<=ay+3) die();
  }
  else if (currentObstacle==1){
    if (px+1>=bx&&px<=bx+3)
      if (py!=5) die();
  }
  else { //currentObstacle==2
    if ((px+1==cx||px==cx)&&py==7) die();
      else if (px+1==cx&&py<=6) die();
  }
}

void detectTilt() {
  tilt1 = analogRead(TILT1);
  tilt2 = analogRead(TILT2);
  if (tilt1<00&&tilt2>00) //tilt left
    if (py+2==8) dpy=0; else dpy=1; //prevents going too far left 
  else if (tilt1>00&&tilt2<00) 
    if (py==0) dpy=0; else dpy=-1;
  else
    dpy=0; //tilting up or down = stop
}

void displayText(char Message[10]){
  LedSign::Clear();
  int8_t limit=0;
    for (int8_t x=DISPLAY_COLS, i=0;; x--) {
      limit++;
      if (limit>46) break;
      LedSign::Clear();
      for (int8_t x2=x, i2=i; x2<DISPLAY_COLS;) {
	int8_t w = Font::Draw(Message[i2], x2, 0);
        x2 += w, i2 = (i2+1)%strlen(Message);
	if (x2 <= 0)	
	x = x2, i = i2;
      }
      LedSign::Flip();
      delay(70);
    }
}

void resetVariables(){
  //game state
  frameDelay=100;
  score=0;
  lives=3;
  warn=true;
  falling=false;
  threshold=analogRead(LDR)-100; //auto-calibrating LDR
  currentObstacle=TrueRandom.random(3);
  //player
  px=1;
  py=6;
  dpy=0;
  maxHeight=1;
  //obstacles
  ax=20; //just outside the screen
  ay=5; //land mine will be four LED's tall
  dax=-1;
  bx=20;
  by=0;
  dbx=-1;
  cx=20;
  cy=0;
  dcx=-1;
}
