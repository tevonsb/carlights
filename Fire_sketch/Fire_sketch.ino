// Fire2012: a basic fire simulation for a one-dimensional string of LEDs
// Mark Kriegsman, July 2012.
//
// Compiled size for Arduino/AVR is about 3,968 bytes.

#include <FastLED.h>
#include <math.h>

#define LED_PIN     5
#define COLOR_ORDER GRB
#define CHIPSET     APA102
#define NUM_LEDS    50

#define BRIGHTNESS  200
#define FRAMES_PER_SECOND 60


//Switch pins and setups
#define switchOnePin  1
#define switchTwoPin  2

//Rangefinder Pin
#define rangePin 3
#define LED_PER_CM 1.44 //???? measure this
#define FOLLOW_HUE 150 //defines the hue of the follow pattern
#define FOLLOW_SAT 255//defines the saturation of follow pattern
#define EXTRA_OFF 50  //LEDS on either side of pattern to check to make sure they are off if needed.

CRGB leds[NUM_LEDS];

void setup() {
  delay(3000); // sanity delay
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  pinMode(switchOnePin, INPUT);
  pinMode(switchTwoPin, INPUT);
  Serial.begin(9600);
  pinMode(rangePin, INPUT);
}

int pickPattern(){
  //Check switches
  int switchOneState = digitalRead(switchOnePin);
  int switchTwoState = digitalRead(switchTwoPin);
  if(switchOneState == LOW && switchTwoState == LOW) return 0;
  if(switchOneState == HIGH && switchTwoState == HIGH) return 1;
  if(switchOneState == LOW && switchTwoState == HIGH) return 2;
  if(switchOneState == HIGH && switchTwoState == LOW) return 3;
 }


void patternOne(){ //fire pattern
    // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random());
  
  Fire2012(); // run simulation frame
  FastLED.show(); // display this frame
}

void patternTwo(){
  //Rangefinding Algo
  int middle = calcMidLED(getRange()); //get the midLED in follow pattern;
  makeFollowPattern(middle);
}

void patternThree(){
  
}

void loop()
{
  switch(pickPattern()){
    case 1: patternOne();
    case 2: patternTwo();
    case 3: patternThree();
    default: patternOne();
  }
  
#if defined(FASTLED_VERSION) && (FASTLED_VERSION >= 2001000)
  FastLED.delay(1000 / FRAMES_PER_SECOND);
#else  
  delay(1000 / FRAMES_PER_SECOND);
#endif  ﻿
}

//Function to get the range of an object from the rangefinder (in CM)
int getRange(){
  int avgRange = 10; //number of readings to take
  int range = 0; //range in centimeters from reader
  for(int i = 0; i < avgRange; i++){
    range += (pulseIn(rangePin, HIGH)/147);
    delay(5);
  }
  range = (range/10)*2.54; //take avg and scale to cm
  return range;
}

int calcMidLED(int range){
  int offset = 0; //change to dist from sensor to 1st LED
  range -= offset;
  return range*LED_PER_CM;
}

void makeFollowPattern(int middle){
  int tailLength = 10; //Change for num trailing LEDS
  for(int i = middle - tailLength-EXTRA_OFF; i < middle + tailLength+EXTRA_OFF; i++){
    if(i < 0 || i > NUM_LEDS||i == middle) continue;
    if(i == middle){
      leds[i] = CHSV(FOLLOW_HUE, FOLLOW_SAT, dim8_video(255));
      continue;
    }
    if(i >= middle-tailLength && i <= middle+tailLength){
          calcFollowLED(i, middle, tailLength); //sets fade of LEDS
        }else{
          leds[i].setHSV(0,0,0); //set stray LEDS to black
        }
  }
  FastLED.show();
}

void calcFollowLED(int i, int middle, int tailLength){
  leds[i] = CHSV(FOLLOW_HUE, FOLLOW_SAT, dim8_video((1/((middle-i)<<1))*255));
}

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100 
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120


void Fire2012()
{
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8(heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 3; k > 0; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
        leds[j] = HeatedColor( heat[j]);
    }
}



// CRGB HeatedColor( uint8_t temperature)
// [to be included in the forthcoming FastLED v2.1]
//
// Approximates a 'black body radiation' spectrum for 
// a given 'heat' level.  This is useful for animations of 'fire'.
// Heat is specified as an arbitrary scale from 0 (cool) to 255 (hot).
// This is NOT a chromatically correct 'black body radiation' 
// spectrum, but it's surprisingly close, and it's extremely fast and small.
//
// On AVR/Arduino, this typically takes around 70 bytes of program memory, 
// versus 768 bytes for a full 256-entry RGB lookup table.

CRGB HeatedColor( uint8_t temperature)
{
  CRGB heatcolor;
  
  // Scale 'heat' down from 0-255 to 0-191,
  // which can then be easily divided into three
  // equal 'thirds' of 64 units each.
  uint8_t t192 = scale8_video( temperature, 192);

  // calculate a value that ramps up from
  // zero to 255 in each 'third' of the scale.
  uint8_t heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252
 
  // now figure out which third of the spectrum we're in:
  if( t192 & 0x80) {
    // we're in the hottest third
    heatcolor.r = 255; // full red
    heatcolor.g = 255; // full green
    heatcolor.b = heatramp; // ramp up blue
    
  } else if( t192 & 0x40 ) {
    // we're in the middle third
    heatcolor.r = 255; // full red
    heatcolor.g = heatramp; // ramp up green
    heatcolor.b = 0; // no blue
    
  } else {
    // we're in the coolest third
    heatcolor.r = heatramp; // ramp up red
    heatcolor.g = 0; // no green
    heatcolor.b = 0; // no blue
  }
  
  return heatcolor;
}
