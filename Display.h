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
