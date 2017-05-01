// include PinChangeInterrupt library* BEFORE IRLremote to acces more pins if needed
//#include "PinChangeInterrupt.h"

#include "IRLremote.h"
#include "FastLED.h"
#include "EEPROM.h"                                           // This is included with base install

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

// Fixed definitions cannot change on the fly.
#define LED_DT 12                                             // Data pin to connect to the strip.
#define LED_CK 11                                             // Clock pin for WS2801 or APA102.
#define COLOR_ORDER BGR                                       // It's GRB for WS2812 and BGR for APA102.
#define LED_TYPE APA102                                       // Using APA102, WS2812, WS2801. Don't forget to modify LEDS.addLeds to suit.
#define NUM_LEDS 22                                           // Number of LED's.
#define MAX_LEDS 22                                          // Maximum number of LED's defined (at compile time).

// EEPROM location definitions.
#define STARTMODE 0


struct CRGB leds[NUM_LEDS];

// Global variables can be changed on the fly.
uint8_t max_bright = 250;                                      // Overall brightness.
static uint8_t hue = 0;


uint8_t ledMode;                                              // Starting mode is typically 0.
bool staticMode;

// Generic/shared routine variables ----------------------------------------------------------------------
uint8_t allfreq = 32;                                         // You can change the frequency, thus overall width of bars.
uint8_t bgclr = 0;                                            // Generic background colour
uint8_t bgbri = 0;                                            // Generic background brightness
bool    glitter = 0;                                          // Glitter flag
uint8_t palchg;                                               // 0=no change, 1=similar, 2=random
uint8_t startindex = 0;
uint8_t thisbeat;                                             // Standard beat
uint8_t thisbright = 0;                                       // Standard brightness
uint8_t thiscutoff = 192;                                     // You can change the cutoff value to display this wave. Lower value = longer wave.
int thisdelay = 0;                                            // Standard delay
uint8_t thisdiff = 1;                                         // Standard palette jump
bool    thisdir = 0;                                          // Standard direction
uint8_t thisfade = 224;                                       // Standard fade rate
uint8_t thishue = 0;                                          // Standard hue
uint8_t thisindex = 0;                                        // Standard palette index
uint8_t thisinc = 1;                                          // Standard incrementer
int     thisphase = 0;                                        // Standard phase change
uint8_t thisrot = 1;                                          // You can change how quickly the hue rotates for this wave. Currently 0.
uint8_t thissat = 255;                                        // Standard saturation
int8_t  thisspeed = 4;                                        // Standard speed change
uint8_t wavebright = 255;                                     // You can change the brightness of the waves/bars rolling across the screen.

uint8_t xd[MAX_LEDS];                                         // arrays for the 2d coordinates of any led
uint8_t yd[MAX_LEDS];

long summ=0;


// Display functions -----------------------------------------------------------------------

#include "rainbow_march.h"
#include "noise16_pal.h"


// Choose a valid PinInterrupt or PinChangeInterrupt* pin of your Arduino board
#define pinIR 2

// Choose the IR protocol of your remote. See the other example for this.
CNec IRLremote;

void setup()
{
  // Start serial debug output
  while (!Serial);
  Serial.begin(57600);
  Serial.println(F("Startup"));

  // Start reading the remote. PinInterrupt or PinChangeInterrupt* will automatically be selected
  if (!IRLremote.begin(pinIR))
    Serial.println(F("You did not choose a valid pin."));

  //  LEDS.addLeds<LED_TYPE, LED_DT, COLOR_ORDER>(leds, NUM_LEDS);      // For WS2812B - Does not work due to 3 pin configuration.
  LEDS.addLeds<LED_TYPE, LED_DT, LED_CK, COLOR_ORDER>(leds, NUM_LEDS);   // For APA102 or WS2801

  FastLED.setBrightness(max_bright);
  set_max_power_in_volts_and_milliamps(5, 500);               // FastLED Power management set at 5V, 500mA.

  ledMode = 6; 

  strobe_mode(ledMode, 1);
}

void loop()
{
    FastLED.setBrightness(max_bright);


      if (!IRLremote.receiving()) {  
        EVERY_N_MILLIS_I(thistimer, thisdelay) {                                    // Sets the original delay time.
          thistimer.setPeriod(thisdelay);                                           // This is how you update the delay value on the fly.
          if(!staticMode)
            strobe_mode(ledMode, 0);                                                 // Strobe to display the current sequence, but don't initialize the variables, so mc=0;
        }
        FastLED.show();
   }
  // Check if we are currently receiving data
  //Serial.println(hue);

  irrecv();
}

void irrecv() {
   // Check if new IR protocol data is available
  if (IRLremote.available())
  {
    // Get the new data from the remote
    auto data = IRLremote.read();

    // Print the protocol data
    Serial.print(F("Command: 0x"));
    Serial.println(data.command);
    Serial.println();

    switch(data.command) {

      case 92:  max_bright=min(max_bright*2,255); LEDS.setBrightness(max_bright);   break;          //a1 - Brightness up
      case 93:  max_bright=max(max_bright/2,1); LEDS.setBrightness(max_bright);   break;          //a2 - Brightness down
      case 65:  toggle_on_off();   break;          //a3 - Dunno
      case 64:  strobe_mode(ledMode,1);   break;          //a4 - Off
    
      case 12:  strobe_mode(5,1); break;//fill_solid(leds, NUM_LEDS, CHSV( 192, 255, 255));   break;          //DIY1 - 
      case 13:  strobe_mode(6,1);   break;          //DIY2 - 

      case 89: static_color(); break;
    
      default: break;                                // We could do something by default
    } // switch
  }
}

void strobe_mode(uint8_t newMode, bool mc) {
  
  if(mc) {
    staticMode = false;
    fill_solid(leds,NUM_LEDS,CRGB(0,0,0));                    // Clean up the array for the first time through. Don't show display though, so you may have a smooth transition.
    Serial.print("Mode: "); 
    ledMode = newMode;
    Serial.println(ledMode);
  }

  switch(newMode) {
    case 1: if(mc) {thisdelay=20; static_color(); } break;
    case 2: if(mc) { } break;
    case 4: if(mc) { }   break;
    case 5: if(mc) {thisdelay=20; hxyinc = random16(1,15); octaves=random16(1,3); hue_octaves=random16(1,5); hue_scale=random16(10, 50);  x=random16(); xscale=random16(); hxy= random16(); hue_time=random16(); hue_speed=random16(1,3); x_speed=random16(1,30);} noise16_pal(); break;
    case 6: if(mc) { thisdelay=10; thisdir=1; thisrot=1; thisdiff=1;} rainbow_march(); break;
  }

  //Serial.print("MC: "); 
  //Serial.println(mc);
  
} // change_mode()

void toggle_on_off() {
  if(max_bright > 0) {
    max_bright = 0;
  }else{
    max_bright = 250;
  }
}

void static_color() {
  staticMode = true;
  fill_solid(leds,NUM_LEDS,CRGB(0,0,0));
  delay(10);
  fill_solid( leds, NUM_LEDS, CHSV( 190, 187, 255) );
}

