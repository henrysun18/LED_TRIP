#include <avr/pgmspace.h> //the use of ROM (program memory) helps to save memory
#include <Charliplexing.h>



PROGMEM const uint16_t Start[9]=
{	580	,
	516	,
	1884	,
	596	,
	604	,
	0	,
	3758	,
	2722	,  //All LED's off=0
	2786	}; //All LED's on=(2^14)-1

const int BUTTON=A0;
int8_t px,py,dpy,maxHeight; //Player X/Y coordinate & speed


void setup(){
  LedSign::Init(DOUBLE_BUFFER); //Initialize screen
  pinMode(BUTTON,INPUT); //analog input 0
  
  px=1;
  py=6;
  dpy=0;
  maxHeight=1;
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
      if (rowInfo%2==1) 
        LedSign::Set(x,y,1); //each row's LED's represent 1 bit (total 14 bits/row)
      else 
        LedSign::Set(x,y,0);
      rowInfo=rowInfo/2; //check if next LED should be on
    }
  }

  while(analogRead(BUTTON)==0){
    LedSign::Flip(1);
    delay(700);
    LedSign::Flip(0);
    delay(600);
  }
}

void game(){
  
  while (1){
    
    
    // [temp] prevents player going off screen
    if(py==maxHeight) dpy=1; //gravity takes over
    if (py+2==8) dpy=0; //land on ground
    
    // Take PLAYER input
    if (analogRead(BUTTON)>69&&dpy==0) dpy=-1; //only take input when player is on ground
    else if (analogRead(BUTTON)>69&&py==maxHeight) dpy=0; //hold button=fly!!
   
    // Move the PLAYER
    py=py+dpy; 
    
    // Clears inactive screen to save memory
    LedSign::Clear(0);
    
    // Draw the PLAYER (3x3)
    for (int8_t y=py; y<py+3; y++)
      for (int8_t x=px; x<px+3; x++){
        LedSign::Set(x,y,1);
      }
      
    // Next frame
    LedSign::Flip();
    delay(100);
  }
}

void jump(){
  dpy=1;
}
