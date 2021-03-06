/*
   This file is part of PwrGridFreqVisualization.

    PwrGridFreqVisualization is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PwrGridFreqVisualization is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "Display.h"
#include <Adafruit_NeoPixel.h>
#define PIN_NEO 2

/**
   NeoPixel variables
*/
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
boolean modeChange_D = false; // If mode changed i.e. button is pressed

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number
Adafruit_NeoPixel strip = Adafruit_NeoPixel(24, PIN_NEO, NEO_RGB + NEO_KHZ800);

void initStrip() {
  // Initialize NeoPixel
  strip.begin();
  strip.setBrightness(42); //adjust brightness here
  strip.show();
}

uint32_t setColor(int led) {
  if (led < 6) return strip.Color(255 - (led % 6) * 20, 0 + (led % 6) * 30, 0);
  else if (led < 9) return strip.Color(155 - (led % 3) * 15, 150, 0);
  else if (led < 12) return strip.Color(110 - (led % 3) * 15, 150 + (led % 3) * 15, 0);
  else if (led < 15) return strip.Color(80 - (led % 3) * 15, 195 + (led % 3) * 15, 0);
  else if (led < 18) return strip.Color(35 - (led % 3) * 10, 240 + (led % 3) * 5, 0);
  else if (led < 24) return strip.Color(0, 255, 0);
}

void setInitMode() {
  idx = 0;
  change = false;
  prev_led = 0;
  prev_freq = 0;
  nbLed = 0;
  modeChange_D = true;
}

void updateMode(int mode, double freq) {
  if (mode == 2) {
    range = 0.0043;
    for (int i = 0; i < 24; i++) {
      strip.setPixelColor(i, 255, 255, 255);
      strip.show();
    }
  }
  if (mode == 1) {
    range = 0.00109;
    for (int i = 0; i < 24; i++) {
      strip.setPixelColor(i, 255, 255, 0);
      strip.show();
    }
  }
  delay(300);
  for (int i = 0; i < 24; i++) {
    strip.setPixelColor(i, 0, 0, 0);
    strip.show();
  }
  if (freq >= 50.0) idx = 1;
  else idx = 23;
  nbLed = abs(50.0 - freq) / range;
  if (nbLed > 23) nbLed = 23;
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
  modeChange_D = false;
}

void show_freq(double freq) {
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
          if (modeChange_D) return;
          for (int l = bright; l > 0; l -= 17) {
            strip.setPixelColor(i, setColor(24 - i), l);
            strip.show();
          }
          strip.setPixelColor(i, 0, 0, 0);
          strip.show();
          for (int j = i + 1; j < 24 ; j++) { // Update all previous led with the right color
            if (modeChange_D) return;
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
              if (modeChange_D) return;
              strip.setPixelColor(j, setColor(24 - j), l);
              strip.show();
              delay(delay_t);
            }
            strip.setPixelColor(j, 0, 0, 0);
            strip.show();
            for (int k = j + 1; k < 24 ; k++) { //  Update all previous led with the right color
              if (modeChange_D) return;
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
          if (modeChange_D) return;
          for (int l = bright; l > 0; l -= 8) {
            strip.setPixelColor(i, setColor(i), l);
            strip.show();
          }
          strip.setPixelColor(i, 0, 0, 0);
          strip.show();
          for (int j = i - 1; j > 0 ; j--) { // Update all previous led with the right color
            if (modeChange_D) return;
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
            if (modeChange_D) return;
            for (int l = bright; l > 0; l -= 8) {
              strip.setPixelColor(j, setColor(j), l);
              strip.show();
              delay(delay_t);
            }
            strip.setPixelColor(j, 0, 0, 0);
            strip.show();
            for (int k = j - 1; k >= 0 ; k--) { // Update all previous led with the right color
              if (modeChange_D) return;
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
          if (modeChange_D) return;
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
          if (modeChange_D) return;
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
