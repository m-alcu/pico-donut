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

#define BUTTON_PIN_A 12
#define BUTTON_PIN_B 13
#define BUTTON_PIN_X 14
#define BUTTON_PIN_Y 15

uint16_t frBuf[WIDTH*HEIGHT];
uint16_t color565_table[256];

volatile bool core0_done = false;
volatile bool core1_done = false;

ST7789 st7789(WIDTH, HEIGHT, ROTATE_0, false, get_spi_pins(BG_SPI_FRONT));
PicoGraphics_PenRGB565 graphics(st7789.width, st7789.height, frBuf);

int iter1 = 3, iter2 = 2;
bool last_button_a = true;
bool last_button_b = true;
bool last_button_x = true;
bool last_button_y = true;

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

        drawTorus(angles, 0, 120*240, frBuf, color565_table, iter1, iter2);
        core1_done = true;
        // Wait for core0 to finish
        while (!core0_done) tight_loop_contents();

        //Core1 does not update the screen, it just draws the first half of the torus
    }
}

unsigned int ms, msMin=1000, msMax=0;

void showStats() {

    Pen YELLOW  = graphics.create_pen(255, 255, 0);
    Pen GREEN   = graphics.create_pen(0, 255, 0);
    Pen MAGENTA = graphics.create_pen(255, 0, 255);

    char txt[30];
 
    if (ms < msMin) msMin = ms;
    if (ms > msMax) msMax = ms;

    snprintf(txt, 30, "%d ms     %d fps ", ms, 1000 / ms);
    graphics.set_pen(YELLOW);
    graphics.text(txt, Point(0, HEIGHT - 32), 240);

    snprintf(txt, 30, "%d-%d ms  %d-%d fps   ", msMin, msMax, 1000 / msMax, 1000 / msMin);
    graphics.set_pen(GREEN);
    graphics.text(txt, Point(0, HEIGHT - 22), 240);

    snprintf(txt, 30, "r1: %d r2: %d", iter1, iter2);
    graphics.set_pen(MAGENTA);
    graphics.text(txt, Point(0, HEIGHT - 12), 240);

}

int main() {

  stdio_init_all();

  torus3d::TorusAngles angles;

  angles.sB = 0;     
  angles.cB = 16384;
  angles.sA = 11583; 
  angles.cA = 11583;
  angles.sAsB = 0;   
  angles.cAsB = 0;
  angles.sAcB = 11583; 
  angles.cAcB = 11583;      

  //precopumpute color565 table
  for (int c = 0; c < 256; ++c) {
      color565_table[c] = pimoroni::RGB(c, c, 0).to_rgb565();
  }  

  multicore_launch_core1(core1_main);  

  st7789.set_backlight(255);

  gpio_init(BUTTON_PIN_A);
  gpio_set_dir(BUTTON_PIN_A, GPIO_IN);
  gpio_pull_up(BUTTON_PIN_A); // Use pull-up if button connects to GND
  
  gpio_init(BUTTON_PIN_B);
  gpio_set_dir(BUTTON_PIN_B, GPIO_IN);
  gpio_pull_up(BUTTON_PIN_B);
  
  gpio_init(BUTTON_PIN_X);
  gpio_set_dir(BUTTON_PIN_X, GPIO_IN);
  gpio_pull_up(BUTTON_PIN_X);

  gpio_init(BUTTON_PIN_Y);
  gpio_set_dir(BUTTON_PIN_Y, GPIO_IN);
  gpio_pull_up(BUTTON_PIN_Y);

  
  while(true) {

    //printf("core0");

    core0_done = false;
    core1_done = false;    

    bool button_a = gpio_get(BUTTON_PIN_A);
    if (!button_a && last_button_a) {  // pressed (falling edge, active low)
        iter1++;
    }
    last_button_a = button_a;

    bool button_b = gpio_get(BUTTON_PIN_B);
    if (!button_b && last_button_b) {  // pressed (falling edge, active low)
        iter1--;
    }
    last_button_b = button_b;


    bool button_x = gpio_get(BUTTON_PIN_X);
    if (!button_x && last_button_x) {  // pressed (falling edge, active low)
        iter2++;
    }
    last_button_x = button_x;

    bool button_y = gpio_get(BUTTON_PIN_Y);
    if (!button_y && last_button_y) {  // pressed (falling edge, active low)
        iter2--;
    }
    last_button_y = button_y;

    ms=millis();

    drawTorus(angles, 120*240, 240*240, frBuf, color565_table, iter1, iter2);
    
    ms=millis()-ms;
    showStats();

    while (!core1_done) tight_loop_contents();

    // update screen
    st7789.update(&graphics);
    core0_done = true;

    }
    
    return 0;
}

