#include "LPD8806.h"
#include "SPI.h"
#include "TimerOne.h"

// Example to control LPD8806-based RGB LED Modules in a strip

/*****************************************************************************/

// Number of RGB LEDs in strand:
const int nLEDs = 160; //160 or 256

// Chose 2 pins for output; can be any valid output pins:
int dataPin  = 2;
int clockPin = 3;

// Setup image data arrays
byte imgData[3][180];  //large enough to fit the data for the longest segment (segment leds * 3)  ``  
// 282 for long, 180 for short
int  fxVars[3][6];             // Effect instance variables 
int  startEnd[4] = {0, 50, 110, 160}; // index points for strip segment ID, hard coded for TL window segments
// {0, 82, 176, 256} for long
// {0, 50, 110, 160} for short
// Program variables

byte      renderIdx[3] = {0, 0 , 0};        // hard coded render choice start state
int       tCounter[3] = {-100, -150, -200};         // Countdowns to next transition for each segment

LPD8806 strip = LPD8806(nLEDs, dataPin, clockPin);

// You can optionally use hardware SPI for faster writes, just leave out
// the data and clock pin parameters.  But this does limit use to very
// specific pins on the Arduino.  For "classic" Arduinos (Uno, Duemilanove,
// etc.), data = pin 11, clock = pin 13.  For Arduino Mega, data = pin 51,
// clock = pin 52.  For 32u4 Breakout Board+ and Teensy, data = pin B2,
// clock = pin B1.  For Leonardo, this can ONLY be done on the ICSP pins.
//LPD8806 strip = LPD8806(nLEDs);

// function prototypes, leave these be :)
void renderEffect00(byte idx);
void renderEffect01(byte idx);
void renderEffect02(byte idx);
void renderEffect03(byte idx);
void callback();
byte gamma(byte x);
long hsv2rgb(long h, byte s, byte v);
char fixSin(int angle);
char fixCos(int angle);

// List of image effect and alpha channel rendering functions; the code for
// each of these appears later in this file.  Just a few to start with...
// simply append new ones to the appropriate list here:
void (*renderEffect[])(byte) = {
  renderEffect00/*,
  renderEffect01,
  renderEffect02,
  renderEffect03*/};


void setup() {
  // Start up the LED strip
  strip.begin();
 
 // Initialize random number generator from a floating analog input.
  randomSeed(analogRead(0));
  memset(imgData, 60, sizeof(imgData)); // Clear image data
  fxVars[0][0] = 0;           // Mark back image as initialized
  fxVars[1][0] = 0;           // Mark back image as initialized
  fxVars[2][0] = 0;           // Mark back image as initialized

  // Timer1 is used so the strip will update at a known fixed frame rate.
  // Each effect rendering function varies in processing complexity, so
  // the timer allows smooth transitions between effects (otherwise the
  // effects and transitions would jump around in speed...not attractive).
  Timer1.initialize();
  Timer1.attachInterrupt(callback, 1000000 / 60); // 60 = 60 frames/second

}

void loop() {
  // Do nothing.  All the work happens in the callback() function below,
  // but we still need loop() here to keep the compiler happy.
}

// Timer1 interrupt handler.  Called at equal intervals; 60 Hz by default.
void callback() {
  // Very first thing here is to issue the strip data generated from the
  // *previous* callback.  It's done this way on purpose because show() is
  // roughly constant-time, so the refresh will always occur on a uniform
  // beat with respect to the Timer1 interrupt.  The various effects
  // rendering and compositing code is not constant-time, and that
  // unevenness would be apparent if show() were called at the end.
  strip.show();


  byte k;

  for (k=0; k<3; k++) {
  
    byte *stripPtr    = &imgData[k][0],
         r, g, b;
    int  i;

//  delay(10000);

  // Always render strip image based on current effect index:
    (*renderEffect[renderIdx[k]])(k);

    for(i=startEnd[k]; i<startEnd[k+1]; i++) {
      // See note above re: r, g, b vars.
      r = gamma(*stripPtr++);
      g = gamma(*stripPtr++);
      b = gamma(*stripPtr++);
      strip.setPixelColor(i, r, g, b);
    }
//   fxVars[k][0] = 0; // Effect not yet initialized


      // Count up to next transition (or end of current one):
   
    tCounter[k]++;

    if(tCounter[k] >= 0) { // Transition start
    // Randomly pick next image effect and alpha effect indices:
      renderIdx[k] = random((sizeof(renderEffect) / sizeof(renderEffect[0])));
      fxVars[k][0] = 0; // Effect not yet initialized    
      tCounter[k]   = -800 - random(600); // Hold image X to Y      seconds
    }
  
  }
  
}


// ---------------------------------------------------------------------------
// Image effect rendering functions.  Each effect is generated parametrically
// (that is, from a set of numbers, usually randomly seeded).  Because both
// back and front images may be rendering the same effect at the same time
// (but with different parameters), a distinct block of parameter memory is
// required for each image.  The 'fxVars' array is a two-dimensional array
// of integers, where the major axis is either 0 or 1 to represent the two
// images, while the minor axis holds 50 elements -- this is working scratch
// space for the effect code to preserve its "state."  The meaning of each
// element is generally unique to each rendering effect, but the first element
// is most often used as a flag indicating whether the effect parameters have
// been initialized yet.  When the back/front image indexes swap at the end of
// each transition, the corresponding set of fxVars, being keyed to the same
// indexes, are automatically carried with them.
/*
// Simplest rendering effect: fill entire image with solid color
void renderEffect00(byte idx) {
  // Only needs to be rendered once, when effect is initialized:
  if(fxVars[idx][0] == 0) {
//  byte r, g, b;
//    byte *ptr = imgData[idx];
//      switch (idx) {
//        case 0:
//           r= 128 - fxVars[idx][0]; g=0; b=0;
//          break;
//         case 1:
//           r=0; g=128 - fxVars[idx][0]; b=0;
//           break;
//           case 2:
//           r=0; g=0, b=128 - fxVars[idx][0];
//      }
      
  byte *ptr = imgData[idx],
      r = 256, g = random(1),
      b = random(1);
    for(int i= startEnd[idx]; i< startEnd[idx+1]; i++) {
      *ptr++ = r; *ptr++ = g; *ptr++ = b;
 
      
    
    }
    fxVars[idx][0]= 1; // Effect initialized
  }
}
*/

/*
// Simplest rendering effect: fill entire image with solid color
void renderEffect03(byte idx) {
  // Only needs to be rendered once, when effect is initialized:
  if(fxVars[idx][0] == 0) {
//  byte r, g, b;
//    byte *ptr = imgData[idx];
//      switch (idx) {
//        case 0:
//           r= 128 - fxVars[idx][0]; g=0; b=0;
//          break;
//         case 1:
//           r=0; g=128 - fxVars[idx][0]; b=0;
//           break;
//           case 2:
//           r=0; g=0, b=128 - fxVars[idx][0];
//      }
  byte r, g, b;
  byte *ptr = imgData[idx];
  
  r = 1, g = 256, b = 1;
  
    for(int i= startEnd[idx]; i< startEnd[idx+1]; i++) {
      *ptr++ = r; *ptr++ = g; *ptr++ = b;
 
      
    
    }
    fxVars[idx][0]= 1; // Effect initialized
  }
}

*/
/*

// Rainbow effect (1 or more full loops of color wheel at 100% saturation).
// Not a big fan of this pattern (it's way overused with LED stuff), but it's
// practically part of the Geneva Convention by now.
void renderEffect01(byte idx) {
  if(fxVars[idx][0] == 0) { // Initialize effect?
    // Number of repetitions (complete loops around color wheel); any
    // more than 4 per meter just looks too chaotic and un-rainbow-like.
    // Store as hue 'distance' around complete belt:
    fxVars[idx][1] = (1 + random(2 * (((startEnd[idx+1] - startEnd[idx]) + 31) / 32))) * 1536;
    // Frame-to-frame hue increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring.  It's generally
    // still less than a full pixel per frame, making motion very smooth.
    fxVars[idx][2] = 2 + random(fxVars[idx][1]) / (startEnd[idx+1] - startEnd[idx]);
    // Reverse speed and hue shift direction half the time.
    if(random(2) == 0) fxVars[idx][1] = -fxVars[idx][1];
    if(random(2) == 0) fxVars[idx][2] = -fxVars[idx][2];
    fxVars[idx][3] = 0; // Current position
    fxVars[idx][4] = 254; // Saturation value
    fxVars[idx][5] = -1; // Saturation direction
 
    fxVars[idx][0] = 1; // Effect initialized
  }

  byte *ptr = imgData[idx];
  long color;
  int i;
  for(i= startEnd[idx]; i< startEnd[idx+1]; i++) {
    color = hsv2rgb(fxVars[idx][3] + fxVars[idx][1] * i / (startEnd[idx+1] - startEnd[idx]),
      fxVars[idx][4], 255);
    *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
  }
  fxVars[idx][3] += fxVars[idx][2];
  fxVars[idx][4] += fxVars[idx][5];
  if (fxVars[idx][4] <= 64) fxVars[idx][5] = -fxVars[idx][5];
  if (fxVars[idx][4] >= 255) fxVars[idx][5] = -fxVars[idx][5];
 }
 
 */

//
//
//
//// Saturation effect .
//void renderEffect02(byte idx) {
//  if(fxVars[idx][0] == 0) { // Initialize effect?
//    // Number of repetitions (complete loops around color wheel); any
//    // more than 4 per meter just looks too chaotic and un-rainbow-like.
//    // Store as hue 'distance' around complete belt:
////    fxVars[idx][1] = (1 + random(2 * (((startEnd[idx+1] - startEnd[idx]) + 31) / 32))) * 1536;
//    fxVars[idx][1] = 2;
//    // Frame-to-frame hue increment (speed) -- may be positive or negative,
//    // but magnitude shouldn't be so small as to be boring.  It's generally
//    // still less than a full pixel per frame, making motion very smooth.
////    fxVars[idx][2] = 8 + random(fxVars[idx][1]) / (startEnd[idx+1] - startEnd[idx]);
//    fxVars[idx][2] = 1 ;
//    // Reverse speed and hue shift direction half the time.
// //   if(random(2) == 0) fxVars[idx][1] = -fxVars[idx][1];
// //   if(random(2) == 0) fxVars[idx][2] = -fxVars[idx][2];
//    fxVars[idx][3] = 0; // Current position
//    fxVars[idx][4] = 255; 
// 
//    fxVars[idx][0] = 1; // Effect initialized
//  }
//
//  byte *ptr = imgData[idx];
//  long color;
//  int i;
//  for(i= startEnd[idx]; i< startEnd[idx+1]; i++) {
//    color = hsv2rgb(150,                                                                //hue
//              fxVars[idx][3] + / (startEnd[idx+1] - startEnd[idx]),  //saturation
//               255);                                                                    //value
//    *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
//  }
//  fxVars[idx][3] += fxVars[idx][2];
////  fxVars[idx][4] --;
//}
//
//

 
/*

// Sine wave chase effect
void renderEffect01(byte idx) {
  if(fxVars[idx][0] == 0) { // Initialize effect?
    fxVars[idx][1] = 1; // Random hue
    // Number of repetitions (complete loops around color wheel);
    // any more than 4 per meter just looks too chaotic.
    // Store as distance around complete belt in half-degree units:
  //  fxVars[idx][2] = (1 + random(4 * (((startEnd[idx+1] - startEnd[idx]) + 31) / 32))) * 720; //32=led's per meter
    fxVars[idx][2] = 720;
    // Frame-to-frame increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring.  It's generally
    // still less than a full pixel per frame, making motion very smooth.
    fxVars[idx][3] = 4 + random(fxVars[idx][1]) / (startEnd[idx+1] - startEnd[idx]);
    // Reverse direction half the time.
    if(random(2) == 0) fxVars[idx][3] = -fxVars[idx][3];
    fxVars[idx][4] = 0; // Current position
    fxVars[idx][0] = 1; // Effect initialized
  }

  byte *ptr = imgData[idx];
  int  foo;
  long color, i;
  for(long i=0; i<(startEnd[idx+1] - startEnd[idx]); i++) {
    foo = fixSin(fxVars[idx][4] + fxVars[idx][2] * i / (startEnd[idx+1] - startEnd[idx]));
    // Peaks of sine wave are white, troughs are black, mid-range
    // values are pure hue (100% saturated).
    color = (foo >= 0) ?
       hsv2rgb(fxVars[idx][1], 254 - (foo * 2), 255) :
       hsv2rgb(fxVars[idx][1], 255, 254 + foo * 2);
    *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
  }
  fxVars[idx][4] += fxVars[idx][3];
}


*/

// Sine wave chase effect
void renderEffect00(byte idx) {
  if(fxVars[idx][0] == 0) { // Initialize effect?
    fxVars[idx][1] = random(720); // Random hue
    // Number of repetitions (complete loops around color wheel);
    // any more than 4 per meter just looks too chaotic.
    // Store as distance around complete belt in half-degree units:
  //  fxVars[idx][2] = (1 + random(4 * (((startEnd[idx+1] - startEnd[idx]) + 31) / 32))) * 720; //32=led's per meter
    fxVars[idx][2] = 720;
    // Frame-to-frame increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring.  It's generally
    // still less than a full pixel per frame, making motion very smooth.
    fxVars[idx][3] = 4 + random(fxVars[idx][1]) / (startEnd[idx+1] - startEnd[idx]);
    // Reverse direction half the time.
    if(random(2) == 0) fxVars[idx][3] = -fxVars[idx][3];
    fxVars[idx][4] = 0; // Current position
    fxVars[idx][0] = 1; // Effect initialized
  }

  byte *ptr = imgData[idx];
  int  foo;
  long color, i;
  for(long i=0; i<(startEnd[idx+1] - startEnd[idx]); i++) {
    foo = fixSin(fxVars[idx][4] + fxVars[idx][2] * i / (startEnd[idx+1] - startEnd[idx]));
    // Peaks of sine wave are white, troughs are black, mid-range
    // values are pure hue (100% saturated).
    color = (foo >= 0) ?
       hsv2rgb(fxVars[idx][1], 254 - (foo * 2), 255) :
       hsv2rgb(fxVars[idx][1], 255, 254 + foo * 2);
    *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
  }
  fxVars[idx][4] += fxVars[idx][3];
}


/*

// Data for American-flag-like colors (20 pixels representing
// blue field, stars and stripes).  This gets "stretched" as needed
// to the full LED strip length in the flag effect code, below.
// Can change this data to the colors of your own national flag,
// favorite sports team colors, etc.  OK to change number of elements.
#define C_RED   160,   100,   0
#define C_WHITE 10, 200, 0
#define C_BLUE    64, 0, 110
//#define C_BLUE 110, 0, 64
PROGMEM prog_uchar flagTable[]  = {
  C_BLUE , C_WHITE, C_BLUE , C_WHITE, C_BLUE , C_WHITE, C_BLUE,
  C_RED  , C_WHITE, C_RED  , C_WHITE, C_RED  , C_WHITE, C_RED ,
  C_WHITE, C_RED  , C_WHITE, C_RED  , C_WHITE, C_RED };

// Wavy flag effect
void renderEffect03() {
  long i, sum, s, x;
  int  idx1, idx2, a, b;
  if(fxVars[0] == 0) { // Initialize effect?
    fxVars[1] = 720 + random(720); // Wavyness
    fxVars[2] = 4 + random(10);    // Wave speed
    fxVars[3] = 200 + random(200); // Wave 'puckeryness'
    fxVars[4] = 0;                 // Current  position
    fxVars[0] = 1;                 // Effect initialized
  }
  for(sum=0, i=0; i<numPixels-1; i++) {
    sum += fxVars[3] + fixCos(fxVars[4] + fxVars[1] *
      i / numPixels);
  }

  byte *ptr = &imgData[0];
  for(s=0, i=0; i<numPixels; i++) {
    x = 256L * ((sizeof(flagTable) / 3) - 1) * s / sum;
    idx1 =  (x >> 8)      * 3;
    idx2 = ((x >> 8) + 1) * 3;
    b    = (x & 255) + 1;
    a    = 257 - b;
    *ptr++ = ((pgm_read_byte(&flagTable[idx1    ]) * a) +
              (pgm_read_byte(&flagTable[idx2    ]) * b)) >> 8;
    *ptr++ = ((pgm_read_byte(&flagTable[idx1 + 1]) * a) +
              (pgm_read_byte(&flagTable[idx2 + 1]) * b)) >> 8;
    *ptr++ = ((pgm_read_byte(&flagTable[idx1 + 2]) * a) +
              (pgm_read_byte(&flagTable[idx2 + 2]) * b)) >> 8;
    s += fxVars[3] + fixCos(fxVars[4] + fxVars[1] *
      i / numPixels);
  }

  fxVars[4] += fxVars[2];
  if(fxVars[4] >= 720) fxVars[4] -= 720;
}

// TO DO: Add more effects here...Larson scanner, etc.

*/
// ---------------------------------------------------------------------------
// Assorted fixed-point utilities below this line.  Not real interesting.

// Gamma correction compensates for our eyes' nonlinear perception of
// intensity.  It's the LAST step before a pixel value is stored, and
// allows intermediate rendering/processing to occur in linear space.
// The table contains 256 elements (8 bit input), though the outputs are
// only 7 bits (0 to 127).  This is normal and intentional by design: it
// allows all the rendering code to operate in the more familiar unsigned
// 8-bit colorspace (used in a lot of existing graphics code), and better
// preserves accuracy where repeated color blending operations occur.
// Only the final end product is converted to 7 bits, the native format
// for the LPD8806 LED driver.  Gamma correction and 7-bit decimation
// thus occur in a single operation.
PROGMEM prog_uchar gammaTable[]  = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,
    2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,
    4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  7,  7,
    7,  7,  7,  8,  8,  8,  8,  9,  9,  9,  9, 10, 10, 10, 10, 11,
   11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15, 15, 16, 16,
   16, 17, 17, 17, 18, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22,
   23, 23, 24, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30,
   30, 31, 32, 32, 33, 33, 34, 34, 35, 35, 36, 37, 37, 38, 38, 39,
   40, 40, 41, 41, 42, 43, 43, 44, 45, 45, 46, 47, 47, 48, 49, 50,
   50, 51, 52, 52, 53, 54, 55, 55, 56, 57, 58, 58, 59, 60, 61, 62,
   62, 63, 64, 65, 66, 67, 67, 68, 69, 70, 71, 72, 73, 74, 74, 75,
   76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
   92, 93, 94, 95, 96, 97, 98, 99,100,101,102,104,105,106,107,108,
  109,110,111,113,114,115,116,117,118,120,121,122,123,125,126,127
};


// This function (which actually gets 'inlined' anywhere it's called)
// exists so that gammaTable can reside out of the way down here in the
// utility code...didn't want that huge table distracting or intimidating
// folks before even getting into the real substance of the program, and
// the compiler permits forward references to functions but not data.
inline byte gamma(byte x) {
  return pgm_read_byte(&gammaTable[x]);
}

// Fixed-point colorspace conversion: HSV (hue-saturation-value) to RGB.
// This is a bit like the 'Wheel' function from the original strandtest
// code on steroids.  The angular units for the hue parameter may seem a
// bit odd: there are 1536 increments around the full color wheel here --
// not degrees, radians, gradians or any other conventional unit I'm
// aware of.  These units make the conversion code simpler/faster, because
// the wheel can be divided into six sections of 256 values each, very
// easy to handle on an 8-bit microcontroller.  Math is math, and the
// rendering code elsehwere in this file was written to be aware of these
// units.  Saturation and value (brightness) range from 0 to 255.
long hsv2rgb(long h, byte s, byte v) {
  byte r, g, b, lo;
  int  s1;
  long v1;

  // Hue
  h %= 1536;           // -1535 to +1535
  if(h < 0) h += 1536; //     0 to +1535
  lo = h & 255;        // Low byte  = primary/secondary color mix
  switch(h >> 8) {     // High byte = sextant of colorwheel
    case 0 : r = 255     ; g =  lo     ; b =   0     ; break; // R to Y
    case 1 : r = 255 - lo; g = 255     ; b =   0     ; break; // Y to G
    case 2 : r =   0     ; g = 255     ; b =  lo     ; break; // G to C
    case 3 : r =   0     ; g = 255 - lo; b = 255     ; break; // C to B
    case 4 : r =  lo     ; g =   0     ; b = 255     ; break; // B to M
    default: r = 255     ; g =   0     ; b = 255 - lo; break; // M to R
  }

  // Saturation: add 1 so range is 1 to 256, allowig a quick shift operation
  // on the result rather than a costly divide, while the type upgrade to int
  // avoids repeated type conversions in both directions.
  s1 = s + 1;
  r = 255 - (((255 - r) * s1) >> 8);
  g = 255 - (((255 - g) * s1) >> 8);
  b = 255 - (((255 - b) * s1) >> 8);

  // Value (brightness) and 24-bit color concat merged: similar to above, add
  // 1 to allow shifts, and upgrade to long makes other conversions implicit.
  v1 = v + 1;
  return (((r * v1) & 0xff00) << 8) |
          ((g * v1) & 0xff00)       |
         ( (b * v1)           >> 8);
}

// The fixed-point sine and cosine functions use marginally more
// conventional units, equal to 1/2 degree (720 units around full circle),
// chosen because this gives a reasonable resolution for the given output
// range (-127 to +127).  Sine table intentionally contains 181 (not 180)
// elements: 0 to 180 *inclusive*.  This is normal.

PROGMEM prog_char sineTable[181]  = {
    0,  1,  2,  3,  5,  6,  7,  8,  9, 10, 11, 12, 13, 15, 16, 17,
   18, 19, 20, 21, 22, 23, 24, 25, 27, 28, 29, 30, 31, 32, 33, 34,
   35, 36, 37, 38, 39, 40, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
   67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 77, 78, 79, 80, 81,
   82, 83, 83, 84, 85, 86, 87, 88, 88, 89, 90, 91, 92, 92, 93, 94,
   95, 95, 96, 97, 97, 98, 99,100,100,101,102,102,103,104,104,105,
  105,106,107,107,108,108,109,110,110,111,111,112,112,113,113,114,
  114,115,115,116,116,117,117,117,118,118,119,119,120,120,120,121,
  121,121,122,122,122,123,123,123,123,124,124,124,124,125,125,125,
  125,125,126,126,126,126,126,126,126,127,127,127,127,127,127,127,
  127,127,127,127,127
};

char fixSin(int angle) {
  angle %= 720;               // -719 to +719
  if(angle < 0) angle += 720; //    0 to +719
  return (angle <= 360) ?
     pgm_read_byte(&sineTable[(angle <= 180) ?
       angle          : // Quadrant 1
      (360 - angle)]) : // Quadrant 2
    -pgm_read_byte(&sineTable[(angle <= 540) ?
      (angle - 360)   : // Quadrant 3
      (720 - angle)]) ; // Quadrant 4
}

char fixCos(int angle) {
  angle %= 720;               // -719 to +719
  if(angle < 0) angle += 720; //    0 to +719
  return (angle <= 360) ?
    ((angle <= 180) ?  pgm_read_byte(&sineTable[180 - angle])  : // Quad 1
                      -pgm_read_byte(&sineTable[angle - 180])) : // Quad 2
    ((angle <= 540) ? -pgm_read_byte(&sineTable[540 - angle])  : // Quad 3
                       pgm_read_byte(&sineTable[angle - 540])) ; // Quad 4
}

