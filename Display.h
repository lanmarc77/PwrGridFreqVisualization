#include <Adafruit_NeoPixel.h>
#define PIN_NEO 2
/**
   NeoPixel variables
*/
double freq = 0;  // Frequency value
boolean first_freq = true;  // First freq has to de displayed in one time
float range = 0.00109; // Default : 50Mhz
int bright = 42;  // NeoPixel brightness
float prev_freq = 0;  // Value of the previous frequency when a new one is calculated
int prev_led = 0; // Value of the previous variable nbLed when a new frequency is calculated
int idx = 0;  // LED index by traversing the NeoPixel
int nbLed = 0;  // Number of LEDs to turn on according to the frequency value
int delay_t = 0;  // Value of delay between 2 LEDs
boolean low; // true : low freq // false : high freq
boolean change = true; // true : need change
boolean modeChange = false; // If mode changed i.e. button is pressed

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number
Adafruit_NeoPixel strip = Adafruit_NeoPixel(24, PIN_NEO, NEO_RGB + NEO_KHZ800);

/**
  Button interrupt variables
*/
int buttonTic = 0;  // 1 press on button = 1 tic
boolean buttonPressed = false;  // If button is pressed
int tmpButtonTic = 0; // Tmp cariable to store buttonTic

/**
   Freq interrupt variables
*/
volatile long timer_tic = 0;  // Internal Timer tics
volatile long tmp_tics = 0; // Tmp variable to store timer_tic
volatile int period = -10;  // Period counter
unsigned char send_flag = 0;  // True when a new frequency is calculated

/**
   External clock interrupt variables
*/
long ds_tics = -10; // External clock tics
long correction = 0;  // Correction factor for the internal Timer

/**
   Others variables
*/
volatile char program_state = 0; // 0 = waiting for precision correction , 1 = done with precision  , 2 = mesuring 1st 100 swings , 3 =  normal operating state


/*  Calculate RGB value according to the number of led on
    param led : LED index on the ring
    return RBG color value
*/
uint32_t setColor(int led) {
  if (led < 6) return strip.Color(255 - (led % 6) * 20, 0 + (led % 6) * 30, 0);
  else if (led < 9) return strip.Color(155 - (led % 3) * 15, 150, 0);
  else if (led < 12) return strip.Color(110 - (led % 3) * 15, 150 + (led % 3) * 15, 0);
  else if (led < 15) return strip.Color(80 - (led % 3) * 15, 195 + (led % 3) * 15, 0);
  else if (led < 18) return strip.Color(35 - (led % 3) * 10, 240 + (led % 3) * 5, 0);
  else if (led < 24) return strip.Color(0, 255, 0);
}

/**
    Display the frequency value with RGB and faded effects, 1st LED is 50Hz, going to left is minus and going to right is plus
*/
void show_freq() {
  bright = 255;
  change = true;  // true: execute the main loop to turn on led
  //freq = random(49965, 50034) / 1000.0;
  //Index starts at 1 or 23 accorting to the value of frequency
  if (freq >= 50.0) idx = 1;
  else idx = 23;
  nbLed = abs(50.0 - freq) / range;
  if (nbLed > 23) nbLed = 23;
  if (nbLed == 0) nbLed = 1;
  delay_t = (23 - abs(prev_led - nbLed)) * 1.5;
#ifdef SERIAL_OUT
  Serial.print("Nb Leds: ");
  Serial.println(nbLed);
#endif
  if (first_freq) {
    if (freq > 50.0) {
      low = false;
      for (idx; idx <= nbLed; idx++) {  // Set LED from 0 to right
        for (int j = 0; j < idx; j++) { // Update alight LED with new color
          strip.setPixelColor(j, setColor(idx), bright);
          strip.show();
        }
        strip.setPixelColor(idx, setColor(idx), 255);
        strip.show();
      }
    }
    else {
      low = true;
      for (idx ; idx >= 24 - nbLed; idx--) {  // Set LED from 0 to left
        strip.setPixelColor(0, setColor(24 - idx), bright);
        for (int j = 23; j > idx; j--) {  // Update alight LED with new color
          strip.setPixelColor(0, setColor(24 - idx), bright);
          strip.setPixelColor(j, setColor(24 - idx), bright);
          strip.show();
        }
        strip.setPixelColor(idx, setColor(24 - idx), 255);
        strip.show();
      }
    }
    prev_freq = freq;
    prev_led = nbLed;
    change = false;
    first_freq = false;
  }
  if (prev_freq != 0) { // Don't check at the first call
    if (low) { // If previous freq was low (<50)
      if (freq > 50.0) { // If new freq is high (>50)  -> Normal clean
        for (int i = 24 - prev_led; i < 24; i++) {
          if (modeChange) return;
          for (int l = bright; l > 0; l -= 17) {
            strip.setPixelColor(i, setColor(24 - i), l);
            strip.show();
          }
          strip.setPixelColor(i, 0, 0, 0);
          strip.show();
          for (int j = i + 1; j < 24 ; j++) { // Update all previous led with the right color
            if (modeChange) return;
            strip.setPixelColor(0, setColor(24 - (i + 1)), bright);
            strip.setPixelColor(j, setColor(24 - (i + 1)), bright);
            strip.show();
          }
          delay(100);
        }
      }
      else { // If freq is still low
        if (prev_led == nbLed) {
          change = false;
        }
        else if (prev_led > nbLed) { // If new freq < prev freq
          change = false; // Do not execute the main loop, led are updated here
          for (int j = 24 - prev_led; j < 24 - nbLed; j++) {
            for (int l = bright; l > 0; l -= 8) {
              if (modeChange) return;
              strip.setPixelColor(j, setColor(24 - j), l);
              strip.show();
              delay(delay_t);
            }
            strip.setPixelColor(j, 0, 0, 0);
            strip.show();
            for (int k = j + 1; k < 24 ; k++) { //  Update all previous led with the right color
              if (modeChange) return;
              strip.setPixelColor(0, setColor(24 - (j + 1)), bright);
              strip.setPixelColor(k, setColor(24 - (j + 1)), bright);
              strip.show();
            }
            delay(100);
          }
        }
        else { // If new freq > prev freq
          idx = 24 - prev_led - 1;  // Set new idx to start at the previous led
        }
      }
    }
    else { // If previous freq was high
      if (freq < 50.0) { // If new freq is low -> Normal clean
        for (int i = prev_led; i > 0; i--) {
          if (modeChange) return;
          for (int l = bright; l > 0; l -= 8) {
            strip.setPixelColor(i, setColor(i), l);
            strip.show();
          }
          strip.setPixelColor(i, 0, 0, 0);
          strip.show();
          for (int j = i - 1; j > 0 ; j--) { // Update all previous led with the right color
            if (modeChange) return;
            strip.setPixelColor(0, setColor(i - 1), bright);
            strip.setPixelColor(j, setColor(i - 1), bright);
            strip.show();
          }
          delay(100);
        }
      }
      else { // If freq is still high
        if (prev_led == nbLed) {
          change = false;
        }
        else if (prev_led > nbLed) { // If new freq < prev freq
          change = false; // Do not execute the main loop, led are updated here
          for (int j = prev_led; j > nbLed; j--) {
            if (modeChange) return;
            for (int l = bright; l > 0; l -= 8) {
              strip.setPixelColor(j, setColor(j), l);
              strip.show();
              delay(delay_t);
            }
            strip.setPixelColor(j, 0, 0, 0);
            strip.show();
            for (int k = j - 1; k >= 0 ; k--) { // Update all previous led with the right color
              if (modeChange) return;
              strip.setPixelColor(0, setColor(j - 1), bright);
              strip.setPixelColor(k, setColor(j - 1), bright);
              strip.show();
            }
            delay(100);
          }
        }
        else { // If new freq > prev freq
          idx = prev_led + 1; // Set new idx to start at the previous led
        }
      }
    }
  }
  if (change) { // If led are not already updated
    if (freq > 50.0) {
      low = false;
      for (idx; idx <= nbLed; idx++) {  // Set LED from 0 to right
        for (int j = 0; j < idx; j++) { // Update alight LED with new color
          strip.setPixelColor(j, setColor(idx), bright);
          strip.show();
        }
        for (int l = 0; l < bright; l += 8) { // Turn on gradually the current LED
          if (modeChange) return;
          strip.setPixelColor(idx, setColor(idx), l);
          strip.show();
          delay(delay_t);
        }
      }
    }
    else {
      low = true;
      for (idx ; idx >= 24 - nbLed; idx--) {  // Set LED from 0 to left
        strip.setPixelColor(0, setColor(24 - idx), bright);
        for (int j = 23; j > idx; j--) {  // Update alight LED with new color
          strip.setPixelColor(0, setColor(24 - idx), bright);
          strip.setPixelColor(j, setColor(24 - idx), bright);
          strip.show();
        }
        for (int l = 0; l < bright; l += 8) { // Turn on gradually the current LED
          if (modeChange) return;
          strip.setPixelColor(idx, setColor(24 - idx), l);
          strip.show();
          delay(delay_t);
        }
      }
    }
  }
  prev_freq = freq;
  prev_led = nbLed;
}
