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

#include <Adafruit_NeoPixel.h>  //adafruit neopixel library
#include <Color.h>

#define PIXELPIN 1  // neopixel control pin
#define PIXELNUM 13 //total number of pixels
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(PIXELNUM, PIXELPIN, NEO_GRB + NEO_KHZ800);

// Landing light settings
const byte LPIXNUM = 3;  //number of landing pixels
const byte LANDPIX[LPIXNUM] = {5,6,7}; //landing pixel positions
#define LL_IRQ_NUMBER 0 // Interrupt number to use (0 = pin 2 on most boards)
#define LL_PIN_SERVO 0 // Servo input pin number - this needs to match whatever interrupt is used
#define LL_SERVO_THRESHOLD 1500 // Servo signal threshold to turn on/off landing light (pulse width in microseconds, 1000 to 2000)
#define LL_SERVO_DEAD_ZONE 100 // Servo signal dead-zone size, eliminates flicker
#define LL_SERVO_REVERSED true   // Whether or not the servo channel is reversed
uint8_t LL_BR[] ={75,75,75}; //landing light color and brightness GRB


// Strobe settings
const byte SPIXNUM = 13; //number of strobe pixels
const byte STROBEPIX[SPIXNUM] = {0,1,2,3,4,5,6,7,8,9,10,11,12}; //strobe pixel positions
#define STB_BLINK_INTERVAL 2000000 // Blink interval for strobe light in microseconds


// Anti-collision beacon settings
const byte APIXNUM = 2; //number of for anti-collision beacon 1 pixels
const byte FADEAPIX[APIXNUM] = {1,4}; //for anti-collision beacon 1 pixel positions

const byte BPIXNUM = 2; //; //number of for anti-collision beacon 2 pixels
const byte FADEBPIX[BPIXNUM] = {8,11}; //for anti-collision beacon 2 pixel positions

const byte CPIXNUM = 2; //number of for anti-collision beacon 1 pixels
const byte FADECPIX[CPIXNUM] = {9,10}; //for anti-collision beacon 1 pixel positions

const byte DPIXNUM = 2; //; //number of for anti-collision beacon 2 pixels
const byte FADEDPIX[DPIXNUM] = {2,3}; //for anti-collision beacon 2 pixel positions

#define ACB_FADE_MIN 10 // Minimum fade level for beacon (0-255)
#define ACB_FADE_MAX 100 // Maximum fade level for beacon (0-255)
#define ACB_FADE_STR 255
#define ACB_FADE_INTERVAL 15000 // Fade step interval, in microseconds (lower numbers = faster fade)

// Var declarations
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
  pixels.begin();


  for (i = 0; i < pixels.numPixels(); i++) {  //black all pixels
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  
 //If you want Static Pixels, here's a good place to put them Green, Red, Blue 
  pixels.setPixelColor(0, pixels.Color(100,100,0));
 pixels.setPixelColor(12, pixels.Color(100,0,100));
 //pixels.setPixelColor(3, pixels.Color(50,0,0));
// pixels.setPixelColor(9, pixels.Color(0,50,0));
 //pixels.setPixelColor(10, pixels.Color(0,50,0));
  pixels.show();

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
    setLandingLight(true); //return to false after debug
  }
}

// Turn on or off landing light
void setLandingLight(boolean state)
{
  uint8_t j;
  float i;
  if (state && !curLandingLight) {
    for (j = 0; j < LPIXNUM; j++) {

      pixels.setPixelColor(LANDPIX[j], pixels.Color(LL_BR[0],LL_BR[1],LL_BR[2])); //landing lights set near full power
    }
    pixels.show();
    //digitalWrite(LL_PIN_LIGHT, HIGH);
  } else if (!state && curLandingLight) {
    for (j = 0; j < LPIXNUM; j++) {

      pixels.setPixelColor(LANDPIX[j], pixels.Color(0, 0, 0));  //turn off landing lights
    }

    //digitalWrite(LL_PIN_LIGHT, LOW);

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
        pixels.setPixelColor(FADEAPIX[j], pixels.Color(ACB_FADE_STR, 0, 0));
        pixels.setPixelColor(FADECPIX[j], pixels.Color(0, ACB_FADE_STR, 0));
      }
      pixels.show();
    } else {
      for (j = 0; j < APIXNUM; j++) { //loop through all enabled pins GRB
        pixels.setPixelColor(FADEBPIX[j], pixels.Color(0, ACB_FADE_STR, 0));
        pixels.setPixelColor(FADEDPIX[j], pixels.Color(ACB_FADE_STR,0, 0));
      }
      pixels.show();
    }
    delay(50);
    fadeDirection *= -1;
  }


  for (k = 0; k < APIXNUM; k++) { //loop through all enabled pins  GRB
    pixels.setPixelColor(FADEAPIX[k], pixels.Color(currentFade, 0, 0));
    pixels.setPixelColor(FADECPIX[k], pixels.Color(0, currentFade, 0));
  }
  for (l = 0; l < APIXNUM; l++) { //loop through all enabled pins GRB
    pixels.setPixelColor(FADEBPIX[l], pixels.Color(0, ACB_FADE_MAX - currentFade + ACB_FADE_MIN, 0));
    pixels.setPixelColor(FADEDPIX[l], pixels.Color(ACB_FADE_MAX - currentFade + ACB_FADE_MIN, 0, 0));
  }
  pixels.show();
}

// Strobe double-blink
void doStrobe()
{
  uint8_t i,j;
  uint8_t g[pixels.numPixels()], r[pixels.numPixels()], b[pixels.numPixels()];

  for (i=0;i<pixels.numPixels();i++) {
   g[i]=(pixels.getPixelColor(i) >> 16);
   r[i]=(pixels.getPixelColor(i) >>  8);
   b[i]=(pixels.getPixelColor(i)      );
  }
 
  for (j = 0; j < SPIXNUM; j++) { //loop through all enabled Strobe pins on GRB
    pixels.setPixelColor(STROBEPIX[j], pixels.Color(255, 255, 255));
  }
  pixels.show();

  delay(50);
  for (j = 0; j < SPIXNUM; j++) { //loop through all enabled Strobe pins off GRB
    pixels.setPixelColor(STROBEPIX[j], pixels.Color(g[STROBEPIX[j]], r[STROBEPIX[j]], b[STROBEPIX[j]]));
  }
  pixels.show();
  delay(50);
  
  for (j = 0; j < SPIXNUM; j++) { //loop through all enabled Strobe pins on GRB
    pixels.setPixelColor(STROBEPIX[j], pixels.Color(255, 255, 255));
  }
  pixels.show();
    delay(50);
    
  for (j = 0; j < SPIXNUM; j++) { //loop through all enabled Strobe pins off GRB
   pixels.setPixelColor(STROBEPIX[j], pixels.Color(g[STROBEPIX[j]], r[STROBEPIX[j]], b[STROBEPIX[j]]));
  }
 
  pixels.show();
 
 
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
