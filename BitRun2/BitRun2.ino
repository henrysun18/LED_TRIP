#include <Charliplexing.h>
#include <Figure.h>
#include <Font.h>

#include <avr/pgmspace.h> //the use of ROM (program memory) helps to save memory

const unsigned int inhale_time_ms  = 500;
const unsigned int hold_breath_ms  = 600;
const unsigned int exhale_time_ms  = 800;
const unsigned int pause_breath_ms = 2000;


PROGMEM const uint16_t Start[9]=
{	16383	,
	738	,
	674	,
	3822	,
	0	,
	3758	,
	2722	,
	2786	,  //All LED's off = 0
	16383	}; //All LED's on=(2^14)-1
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

const int BUTTON=A0;
int8_t px,py,dpy,maxHeight; //Player X/Y coordinate & speed
int8_t ax,ay,dax; //Obstable A: Land Mine

void setup(){
  LedSign::Init(DOUBLE_BUFFER | GRAYSCALE); //Initialize screen
  pinMode(BUTTON,INPUT); //analog input 0
  
  px=1;
  py=6;
  dpy=0;
  maxHeight=1;
  
  ax=20; //just outside the screen
  ay=7; //land mine will be two LED's tall
  dax=-1;
  
  Serial.begin(9600);
}

void loop(){
  startScreen();
  game();
}

void startScreen(){
  
  for (int8_t y=0; y<9; y++){
    //"fetch data from program memory (PROGMEM) with a pointer"
    unsigned long rowInfo=pgm_read_word_near(&Start[y]); //temp storage of the row data

    for (int8_t x=0; x<14; x++){
      if (y==0||y==8) 
        LedSign::Set(x,y,4); //each row's LED's represent 1 bit (total 14 bits/row)
      
      else {
        if (rowInfo%2==1) 
          LedSign::Set(x,y,3);
        else 
          LedSign::Set(x,y,0);
        rowInfo=rowInfo/2; //check if next LED should be on
      }
    }
  }

  while(analogRead(BUTTON)==0){
    LedSign::Flip(1);
    delay(800);
  }
}

void game(){
  
  while (1){
    // [temp] prevents player going off screen
    if (py==maxHeight) dpy=1; //gravity takes over
    if (py+2==8) dpy=0; //land on ground
    // [temp] recycles Obstacle A motion
    if (ax==-1) ax=20; 
    
    // Take PLAYER input
    if (analogRead(BUTTON)>69&&dpy==0) dpy=-1; //only take input when player is on ground
    else if (analogRead(BUTTON)>69&&py==maxHeight) dpy=0; //hold button=fly!!
   
    // Move the PLAYER & OBSTACLES
    py=py+dpy; 
    ax=ax+dax;
    
   
    // Clears inactive screen to save memory
    LedSign::Clear(0);
    
     // Collision Detection
    if (ax>=0&&ax<=2&&py+2>=ay) die();
    
    else{
      // Draw the PLAYER (3x3)
      for (int8_t y=py; y<py+3; y++)
        for (int8_t x=px; x<px+2; x++)
          LedSign::Set(x,y,3);
      //Draw OBSTACLE A: Land Mine
      for (int8_t y=ay; y<ay+2; y++)
        for (int8_t x=ax; x<ax+2; x++)
          if (x>=0&&x<=13) LedSign::Set(x,y,3); //turning on LED's past the screen is buggy
          
      // Next frame
      LedSign::Flip();
      Serial.println("drawn");
      
      delay(100);
    }
  }
}

void die(){
  delay(300);
  LedSign::Clear(0);
  LedSign::Flip(1);
  Serial.println("ded");
      delay(1000);
  for (int8_t y=0; y<9; y++){
    unsigned long rowInfo=pgm_read_word_near(&Die[y]); //temp storage of the row data
    for (int8_t x=0; x<14; x++){
        if (rowInfo%2==1) LedSign::Set(x,y,3);
        else LedSign::Set(x,y,0);
    rowInfo=rowInfo/2; //check if next LED should be on
    }
  }
  do{ //ensures X is shown at least once, even if player holds down input after death
    LedSign::Flip(1);
    delay(350);
  } while (analogRead(BUTTON)<10);
  
  //reset the Obstacles
  px=1;
  py=6;
  dpy=0;
  
  ax=20; //just outside the screen
  ay=7; //land mine will be two LED's tall
  dax=-1;
}
