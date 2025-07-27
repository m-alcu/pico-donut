#include <math.h>
#include <vector>
#include <string>

#include "drivers/st7789/st7789.hpp"
#include "libraries/pico_graphics/pico_graphics.hpp"
#include "gfx3d.h"
#include "pico/multicore.h"

using namespace pimoroni;
using namespace torus3d;

const int WIDTH = 240;
const int HEIGHT = 240;

#define MAX_OBJ 12
#define NLINES 240

#define SCR_WD  240
#define SCR_HT  240
#define WD_3D   240
#define HT_3D   240
#define BUTTON_PIN 15

uint16_t frBuf[SCR_WD*NLINES];
uint16_t color565_table[256];

volatile bool core0_done = false;
volatile bool core1_done = false;

ST7789 st7789(WIDTH, HEIGHT, ROTATE_0, false, get_spi_pins(BG_SPI_FRONT));
PicoGraphics_PenRGB565 graphics(st7789.width, st7789.height, frBuf);

int buttonState;
int prevState = true;
long btDebounce    = 30;
long btMultiClick  = 600;
long btLongClick   = 500;
long btLongerClick = 2000;
long btTime = 0, btTime2 = 0;
int clickCnt = 1;

void core1_main() {

    torus3d::TorusAngles angles;

    angles.sB = 0;     
    angles.cB = 16384;
    angles.sA = 11583; 
    angles.cA = 11583;
    angles.sAsB = 0;   
    angles.cAsB = 0;
    angles.sAcB = 11583; 
    angles.cAcB = 11583;       

    while (true) {

        printf("core1\n");

        drawTorus(angles, 0, 120*240, frBuf, color565_table);
        core1_done = true;
        // Wait for core0 to finish
        while (!core0_done) tight_loop_contents();

        // (Optional) Core 1 does nothing else, or could help with next frame
    }
}

// 0=idle, 1,2,3=click, -1,-2=longclick
int checkButton( bool state)
{
  if( state == false && prevState == true ) { btTime = millis(); prevState = state; return 0; } // button just pressed
  if( state == true && prevState == false ) { // button just released
    prevState = state;
    if( millis()-btTime >= btDebounce && millis()-btTime < btLongClick ) { 
      if( millis()-btTime2<btMultiClick ) clickCnt++; else clickCnt=1;
      btTime2 = millis();
      return clickCnt; 
    } 
  }
  if( state == false && millis()-btTime >= btLongerClick ) { prevState = state; return -2; }
  if( state == false && millis()-btTime >= btLongClick ) { prevState = state; return -1; }
  return 0;
}

int prevButtonState=0;

int handleButton(bool button)
{
  prevButtonState = buttonState;
  buttonState = checkButton(button);
  return buttonState;
}

unsigned int ms, msMin=1000, msMax=0, stats=1;

void showStats() {
    Pen WHITE   = graphics.create_pen(255, 255, 255);
    Pen YELLOW  = graphics.create_pen(255, 255, 0);
    Pen GREEN   = graphics.create_pen(0, 255, 0);
    Pen MAGENTA = graphics.create_pen(255, 0, 255);
    Pen BLACK   = graphics.create_pen(0, 0, 0);

    char txt[30];
 
    if (ms < msMin) msMin = ms;
    if (ms > msMax) msMax = ms;

    snprintf(txt, 30, "%d ms     %d fps ", ms, 1000 / ms);
    graphics.set_pen(YELLOW);
    graphics.text(txt, Point(0, SCR_HT - 32), 240);

    snprintf(txt, 30, "%d-%d ms  %d-%d fps   ", msMin, msMax, 1000 / msMax, 1000 / msMin);
    graphics.set_pen(GREEN);
    graphics.text(txt, Point(0, SCR_HT - 22), 240);

    snprintf(txt, 30, "r1: %d r2: %d", iter1, iter2);
    graphics.set_pen(MAGENTA);
    graphics.text(txt, Point(0, SCR_HT - 12), 240);

}

int main() {

  stdio_init_all();
  multicore_launch_core1(core1_main);  

  st7789.set_backlight(255);

  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_pull_up(BUTTON_PIN); // Use pull-up if button connects to GND

  bool last_button = true;

  //precopumpute color565 table

  for (int c = 0; c < 256; ++c) {
      color565_table[c] = pimoroni::RGB(c, c, 0).to_rgb565();
  }

  torus3d::TorusAngles angles;

  angles.sB = 0;     
  angles.cB = 16384;
  angles.sA = 11583; 
  angles.cA = 11583;
  angles.sAsB = 0;   
  angles.cAsB = 0;
  angles.sAcB = 11583; 
  angles.cAcB = 11583;      

  while(true) {

    //printf("core0");

    core0_done = false;
    core1_done = false;    

    bool button = gpio_get(BUTTON_PIN);
    handleButton(button);
    if(buttonState<0) {
        if(buttonState==-2 && prevButtonState==-1) {
        stats=!stats; 
        }
    } else
    if(buttonState>0) {
        msMin=1000;
        msMax=0;
    }

    ms=millis();

    drawTorus(angles, 120*240, 240*240, frBuf, color565_table);
    
    ms=millis()-ms;
    if(stats) showStats();

    while (!core1_done) tight_loop_contents();

    // update screen
    st7789.update(&graphics);
    core0_done = true;

    }
    
    return 0;
}

