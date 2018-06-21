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

#include <Adafruit_NeoPixel.h>

/*  Calculate RGB value according to the number of led on
    param led : LED index on the ring
    return RBG color value
*/
uint32_t setColor(int led);

/**
    Display the frequency value with RGB and faded effects, 1st LED is 50Hz, going to left is minus and going to right is plus
*/
void show_freq(double freq);

/**
   Update the NeoPixel ring after changing the mode
*/
void updateMode(int mode, double freq);

/**
   Init variables for the NeoPixel for changing the mode
*/
void setInitMode();

/**
   Init the NeoPixel strip
*/
void initStrip();
