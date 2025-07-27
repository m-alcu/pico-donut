#include "libraries/pico_graphics/pico_graphics.hpp"

uint16_t BLUE = pimoroni::RGB(0,86,253).to_rgb565(); // BLACK
uint16_t GREY = pimoroni::RGB(200,200,200).to_rgb565(); // BLACK


const int dz = 5, r1 = 1, r2 = 2;
#define R(s,x,y) x-=(y>>s); y+=(x>>s)

const int WIDTH = 240;
const int HEIGHT = 240;
const int HALF_HEIGHT = HEIGHT >> 1;
const int SCREEN_SIZE = WIDTH * HEIGHT;

namespace torus3d {

    struct TorusAngles {
        int16_t sB;
        int16_t cB;
        int16_t sA;
        int16_t cA;
        int16_t sAsB;
        int16_t cAsB;
        int16_t sAcB; 
        int16_t cAcB;
    };

}

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

void drawTorus(torus3d::TorusAngles& angles, int fromLine, int toLine, uint16_t* frBuf, uint16_t* color565_table, int iter1, int iter2) {

    //backgroundChecker(angles.sA, angles.sB, frBuf);

    // Don't worry, even though this looks like a multiplication, 
    // the compiler will probably optimize it into shifts 
    // and adds because dz is a known constant (5). 
    int p0x = dz * angles.sB >> 6;
    int p0y = dz * angles.sAcB >> 6;
    int p0z = -dz * angles.cAcB >> 6;

    const int r1i = r1 * 256;
    const int r2i = r2 * 256;

    int16_t yincC = (angles.cA >> 6);
    int16_t yincS = (angles.sA >> 6);

    int16_t xincX = (angles.cB >> 6);
    int16_t xincY = (angles.sAsB >> 6);
    int16_t xincZ = (angles.cAsB >> 6);
        
    int16_t ycA = -angles.cA;
    int16_t ysA = -angles.sA;

    int jpixel = 0;

    int xsAsB = (angles.sAsB >> 4) - angles.sAsB;
    int xcAsB = (angles.cAsB >> 4) - angles.cAsB;

    for (int j = 0; j < SCREEN_SIZE; j +=(WIDTH << 1), ycA += yincC, ysA += yincS, jpixel +=2) {

        int16_t vxi14 = (angles.cB >> 4) - angles.cB - angles.sB;
        int16_t vyi14 = ycA - xsAsB - angles.sAcB;
        int16_t vzi14 = ysA + xcAsB + angles.cAcB;

        int jpixelx = jpixel % 64;

        for (int i = 0; i < WIDTH; i +=2, vxi14 += xincX, vyi14 -= xincY, vzi14 += xincZ) {
            int t = 512; // (256 * dz) - r2i - r1i;

            int ix = i % 64;

            int16_t px = p0x + (vxi14 >> 5); // assuming t = 512, t*vxi>>8 == vxi<<1
            int16_t py = p0y + (vyi14 >> 5);
            int16_t pz = p0z + (vzi14 >> 5);

            int16_t lx0 = angles.sB >> 2;
            int16_t ly0 = angles.sAcB - angles.cA >> 2;
            int16_t lz0 = -angles.cAcB - angles.sA >> 2;

            if (j >= fromLine && j < toLine) {
                for (;;) {
                    int t0, t1, t2, d;
                    int16_t lx = lx0, ly = ly0, lz = lz0;
                    t0 = length_cordic(px, py, &lx, ly, iter1);
                    t1 = t0 - r2i;
                    t2 = length_cordic(pz, t1, &lz, lx, iter2);
                    d = t2 - r1i;
                    t += d;

                    int index = j + i;
                    if (t > 8 * 256) {
                        // if the distance is too large, draw the background, a blue grey chess pattern
                        uint16_t color = ((jpixelx<32 && ix<32) || (jpixelx>32 && ix>32)) ? BLUE : GREY;
                        frBuf[index] = color;
                        frBuf[index+1] = color;
                        frBuf[index+WIDTH] = color;
                        frBuf[index+WIDTH+1] = color;
                        break;
                    } else if (d < 3) {
                        int N = lz >> 5;
                        uint16_t color = color565_table[(N > 0 ? N < 256 ? N : 255 : 0)];
                        int index = j + i;
                        frBuf[index] = color;
                        frBuf[index+1] = color;
                        frBuf[index+WIDTH] = color;
                        frBuf[index+WIDTH+1] = color;
                        break;
                    }

                    px += d * vxi14 >> 14;
                    py += d * vyi14 >> 14;
                    pz += d * vzi14 >> 14;
                }
            }

        }
    }

    // rotate sines, cosines, and products thereof
    // this animates the torus rotation about two axes
    R(5, angles.cA, angles.sA);
    R(5, angles.cAsB, angles.sAsB);
    R(5, angles.cAcB, angles.sAcB);
    R(6, angles.cB, angles.sB);
    R(6, angles.cAcB, angles.cAsB);
    R(6, angles.sAcB, angles.sAsB);
    //printf("cA: %d, sA: %d\n", angles.cA, angles.sA);

}
