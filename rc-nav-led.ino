/**
 * RC LED Light Controller
 *
 * Designed for an ATTiny85, but should run on any arduino. Default pin numbers are for an ATTiny85
 *
 * Features:
 * Landing light controlled via an RC channel (can use a Y-harness on flaps or landing gear channel)
 * 2 fading anti-collision beacons
 * Double flash strobe
 *
 * @author Harold Asbridge
 * @version 0.3
 * @date 2014-06-12
 *
 * brutally masacred for Neopixel use by Illuxions 2015-4-7
 * You MUST customize this for the number of pixels and pixel positions you will use
 */
// Var declarations
#include <Adafruit_NeoPixel.h>  //adafruit neopixel library
#define PIXELPIN 1  // neopixel control pin
#define PIXELNUM 13 //total number of pixels
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXELNUM, PIXELPIN, NEO_GRB + NEO_KHZ800);


uint32_t cBlack = strip.Color(0,0,0);
uint32_t cWhite = strip.Color(255,255,255);
uint32_t cRed = strip.Color(0,100,0);
uint32_t cGreen = strip.Color(100,0,0);
uint32_t cBlue = strip.Color(0,0,100);

// Landing light settings
const byte LPIXNUM = 1;  //number of landing pixels
const byte LANDPIX[LPIXNUM] = {6}; //landing pixel positions
#define LL_IRQ_NUMBER 5 // Interrupt number to use (0 = pin 2 on most boards)
#define LL_PIN_SERVO 5 // Servo input pin number - this needs to match whatever interrupt is used
#define LL_SERVO_THRESHOLD 1500 // Servo signal threshold to turn on/off landing light (pulse width in microseconds, 1000 to 2000)
#define LL_SERVO_DEAD_ZONE 100 // Servo signal dead-zone size, eliminates flicker
#define LL_SERVO_REVERSED true   // Whether or not the servo channel is reversed
uint32_t LL_COLOR = strip.Color(175,175,175); //landing light color and brightness GRB


// Strobe settings 
const byte SPIXNUM = 13; //number of strobe pixels
const byte STROBEPIX[SPIXNUM] = {0, 1, 2, 3, 4,5,6,7, 8, 9, 10, 11, 12}; //strobe pixel positions
#define STB_BLINK_INTERVAL 4000000 // Blink interval for strobe light in microseconds
uint32_t STB_COLOR = strip.Color(100,100,100); //strobe color
uint8_t STB_TIMES = 3; //number of flashes per strobe cycle
uint8_t STB_DEL = 25; //ms pause for strobe


// Anti-collision beacon settings
const byte APIXNUM = 3; //number of for anti-collision beacon 1 pixels.  *!!  Must have samequantity in each of 4 sets
const byte FADEAPIX[APIXNUM] = {0,1, 4}; //for anti-collision beacon 1 pixel positions
const byte FADEBPIX[APIXNUM] = {7, 8, 11}; //for anti-collision beacon 2 pixel positions
const byte FADECPIX[APIXNUM] = {9, 10,12}; //for anti-collision beacon 1 pixel positions
const byte FADEDPIX[APIXNUM] = {2, 3,5}; //for anti-collision beacon 2 pixel positions

#define ACB_FADE_MIN 10 // Minimum fade level for beacon (0-255)
#define ACB_FADE_MAX 75 // Maximum fade level for beacon (0-255)
#define ACB_FADE_STR 255
#define ACB_FADE_INTERVAL 18000 // Fade step interval, in microseconds (lower numbers = faster fade)

volatile unsigned long servoPulseStartTime;
volatile int servoPulseWidth = 0;
boolean curLandingLight = false;
unsigned long lastFadeTime = 0;
unsigned long lastStrobeTime = 0;
int currentFade = ACB_FADE_MIN;
int fadeDirection = 1;


// Called on power on or reset
void setup()
{
  uint8_t i;
  strip.begin();

  for (i = 0; i < strip.numPixels(); i++) {  //black all pixels
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }

  //If you want Static Pixels, here's a good place to put them Green, Red, Blue
//  strip.setPixelColor(0, pixels.Color(100, 100, 0));
//  strip.PixelColor(12, pixels.Color(100, 0, 100));
  strip.show();

  // Set up interrupt handler
  attachInterrupt(LL_IRQ_NUMBER, measureServoSignal, CHANGE);

}

// Called continuouly
void loop()
{
  unsigned long currentTime = micros();

  // Check if the landing light should be turned on
  checkLandingLight();

  // Check if it's time to fade the anti-collision lights
  if ((currentTime - lastFadeTime) > ACB_FADE_INTERVAL) {
    doFade();
    lastFadeTime = currentTime;
  }

  // Check if it's time to blink the strobes
  if ((currentTime - lastStrobeTime) > STB_BLINK_INTERVAL) {
    doStrobe();
    lastStrobeTime = currentTime;
  }
}

// Check servo signal, and decide to turn on/off the landing light
void checkLandingLight()
{
  // Modify threshold to prevent flicker
  int threshold = LL_SERVO_THRESHOLD;
  if (!curLandingLight) {
    // Light is not on, adjust threshold up
    threshold += LL_SERVO_DEAD_ZONE;
  } else {
    // Light is on, adjust threshold down
    threshold -= LL_SERVO_DEAD_ZONE;
  }

  // Check servo position
  if (servoPulseWidth >= threshold) {
    setLandingLight(true);
  } else {
    setLandingLight(false); //return to false after debug
  }
}

// Turn on or off landing light
void setLandingLight(boolean state)
{
  uint8_t j;
  if (state && !curLandingLight) {
    for (j = 0; j < LPIXNUM; j++) {

      strip.setPixelColor(LANDPIX[j], LL_COLOR); //landing lights by LL_C
    }
    strip.show();
    
  } else if (!state && curLandingLight) {
    for (j = 0; j < LPIXNUM; j++) {

      strip.setPixelColor(LANDPIX[j], cBlack);  //turn off landing lights
    }


  }
  curLandingLight = state;
}

// Fade anti-collision LEDs
void doFade()
{
  uint8_t j, k, l;
  currentFade += fadeDirection;
  if (currentFade == ACB_FADE_MAX || currentFade == ACB_FADE_MIN) {
    // If we hit the fade limit, flash the high beacon, and flip the fade direction
    if (fadeDirection == 1) {
      for (j = 0; j < APIXNUM; j++) {  //loop through all enabled pins GRB
        strip.setPixelColor(FADEAPIX[j], strip.Color(ACB_FADE_STR, 0, 0));
        strip.setPixelColor(FADECPIX[j], strip.Color(0, ACB_FADE_STR, 0));
      }
      strip.show();
    } else {
      for (j = 0; j < APIXNUM; j++) { //loop through all enabled pins GRB
        strip.setPixelColor(FADEBPIX[j], strip.Color(0, ACB_FADE_STR, 0));
        strip.setPixelColor(FADEDPIX[j], strip.Color(ACB_FADE_STR, 0, 0));
      }
      strip.show();
    }
    delay(50);
    fadeDirection *= -1;
  }


  for (k = 0; k < APIXNUM; k++) { //loop through all enabled pins  GRB
    strip.setPixelColor(FADEAPIX[k], strip.Color(currentFade, 0, 0));
    strip.setPixelColor(FADECPIX[k], strip.Color(0, currentFade, 0));
  }
  for (l = 0; l < APIXNUM; l++) { //loop through all enabled pins GRB
    strip.setPixelColor(FADEBPIX[l], strip.Color(0, ACB_FADE_MAX - currentFade + ACB_FADE_MIN, 0));
    strip.setPixelColor(FADEDPIX[l], strip.Color(ACB_FADE_MAX - currentFade + ACB_FADE_MIN, 0, 0));
  }
  strip.show();
}

// Strobe double-blink
void doStrobe()
{
  uint8_t i, j, k;
  
  uint8_t g[strip.numPixels()], r[strip.numPixels()], b[strip.numPixels()];


  for (i = 0; i < STB_TIMES; i++) { 
    for (k = 0; k < strip.numPixels(); k++) { //read existing pixel color
    g[k] = (strip.getPixelColor(k) >> 16);
    r[k] = (strip.getPixelColor(k) >>  8);
    b[k] = (strip.getPixelColor(k)      );
    }
    for (j = 0; j < SPIXNUM; j++) { //loop through all enabled Strobe pins on GRB
      strip.setPixelColor(STROBEPIX[j], STB_COLOR);
    }
    strip.show();
    delay(STB_DEL);
    
    for (j = 0; j < SPIXNUM; j++) { // loop through all strobed pins and return to original color
      strip.setPixelColor(STROBEPIX[j], strip.Color(g[STROBEPIX[j]], r[STROBEPIX[j]], b[STROBEPIX[j]]));
    }
    strip.show();
    delay(STB_DEL);
  }


}

// Measure servo PWM signal
void measureServoSignal()
{
  int pinState = digitalRead(LL_PIN_SERVO);
  if (pinState == HIGH) {
    // Beginning of PWM pulse, mark time
    servoPulseStartTime = micros();
  } else {
    // End of PWM pulse, calculate pulse duration in mcs
    servoPulseWidth = (int)(micros() - servoPulseStartTime);

    // If servo channel is reversed, use the inverse
    if (LL_SERVO_REVERSED) {
      servoPulseWidth = (1000 - (servoPulseWidth - 1000)) + 1000;
    }
  }
}
