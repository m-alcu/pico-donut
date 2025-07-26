#include "libraries/pico_graphics/pico_graphics.hpp"

uint16_t BLUE = pimoroni::RGB(0,86,253).to_rgb565(); // BLACK
uint16_t GREY = pimoroni::RGB(200,200,200).to_rgb565(); // BLACK


const int dz = 5, r1 = 1, r2 = 2;
#define R(s,x,y) x-=(y>>s); y+=(x>>s)

// SDL screen dimensions
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 240;
const int SCREEN_SIZE = SCREEN_WIDTH * SCREEN_HEIGHT;

int16_t sB = 0, cB = 16384;
int16_t sA = 11583, cA = 11583;
int16_t sAsB = 0, cAsB = 0;
int16_t sAcB = 11583, cAcB = 11583;

int iter1 = 3, iter2 = 2;

// CORDIC algorithm (calculate the length of a vector https://en.wikipedia.org/wiki/CORDIC)
inline int length_cordic(int16_t x, int16_t y, int16_t *x2_, int16_t y2, int16_t iterations = 8) {
    int x2 = *x2_;
    if (x < 0) {
        x = -x;
        x2 = -x2;
    }
    for (int i = 0; i < iterations; i++) {
        int t = x;
        int t2 = x2;
        if (y < 0) {
            x -= y >> i;
            y += t >> i;
            x2 -= y2 >> i;
            y2 += t2 >> i;
        } else {
            x += y >> i;
            y -= t >> i;
            x2 += y2 >> i;
            y2 -= t2 >> i;
        }
    }
    *x2_ = (x2 >> 1) + (x2 >> 3);
    return (x >> 1) + (x >> 3);
}

// animated checkerboard pattern
void backgroundChecker(int sA, int sB, uint16_t* frBuf)
{
  int x,y,xx,yy, xo,yo;
  xo = (sB >> 9)+50;
  yo = (sA >> 9)+50;

  int hy = 0;

  for(y=0;y<SCREEN_HEIGHT;y++, hy+= SCREEN_WIDTH) {
    yy = (y+yo) % 64;
    for(x=0;x<SCREEN_WIDTH;x++) {
      xx = (x+xo) % 64;
      frBuf[hy+x] = ((xx<32 && yy<32) || (xx>32 && yy>32)) ? BLUE : GREY;
    }
  }
}

void drawTorus(uint16_t* frBuf, uint16_t* color565_table) {

    backgroundChecker(sA, sB, frBuf);

    // Don't worry, even though this looks like a multiplication, 
    // the compiler will probably optimize it into shifts 
    // and adds because dz is a known constant (5). 
    int p0x = dz * sB >> 6;
    int p0y = dz * sAcB >> 6;
    int p0z = -dz * cAcB >> 6;

    const int r1i = r1 * 256;
    const int r2i = r2 * 256;

    int niters = 0;
    int nnormals = 0;
    int16_t yincC = (cA >> 6);
    int16_t yincS = (sA >> 6);

    int16_t xincX = (cB >> 6);
    int16_t xincY = (sAsB >> 6);
    int16_t xincZ = (cAsB >> 6);
        
    int16_t ycA = -cA;
    int16_t ysA = -sA;

    for (int j = 0; j < SCREEN_SIZE; j +=(SCREEN_WIDTH << 1), ycA += yincC, ysA += yincS) {
        int xsAsB = (sAsB >> 4) - sAsB;
        int xcAsB = (cAsB >> 4) - cAsB;

        int16_t vxi14 = (cB >> 4) - cB - sB;
        int16_t vyi14 = ycA - xsAsB - sAcB;
        int16_t vzi14 = ysA + xcAsB + cAcB;

        for (int i = 0; i < SCREEN_WIDTH; i +=2, vxi14 += xincX, vyi14 -= xincY, vzi14 += xincZ) {
            int t = 512; // (256 * dz) - r2i - r1i;

            int16_t px = p0x + (vxi14 >> 5); // assuming t = 512, t*vxi>>8 == vxi<<1
            int16_t py = p0y + (vyi14 >> 5);
            int16_t pz = p0z + (vzi14 >> 5);

            int16_t lx0 = sB >> 2;
            int16_t ly0 = sAcB - cA >> 2;
            int16_t lz0 = -cAcB - sA >> 2;

            for (;;) {
                int t0, t1, t2, d;
                int16_t lx = lx0, ly = ly0, lz = lz0;
                t0 = length_cordic(px, py, &lx, ly, iter1);
                t1 = t0 - r2i;
                t2 = length_cordic(pz, t1, &lz, lx, iter2);
                d = t2 - r1i;
                t += d;

                if (t > 8 * 256) {
                    break;
                } else if (d < 3) {
                    int N = lz >> 5;
                    uint16_t color = color565_table[(N > 0 ? N < 256 ? N : 255 : 0)];
                    int index = j + i;
                    frBuf[index] = color;
                    frBuf[index+1] = color;
                    frBuf[index+SCREEN_WIDTH] = color;
                    frBuf[index+SCREEN_WIDTH+1] = color;
                    nnormals++;
                    break;
                }

                px += d * vxi14 >> 14;
                py += d * vyi14 >> 14;
                pz += d * vzi14 >> 14;
                niters++;
            }
        }
    }

    // rotate sines, cosines, and products thereof
    // this animates the torus rotation about two axes
    R(5, cA, sA);
    R(5, cAsB, sAsB);
    R(5, cAcB, sAcB);
    R(6, cB, sB);
    R(6, cAcB, cAsB);
    R(6, sAcB, sAsB);

}
