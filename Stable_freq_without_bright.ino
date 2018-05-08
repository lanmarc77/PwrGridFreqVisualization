#include <avr/sleep.h>
#include <PinChangeInterrupt.h>
#include <Adafruit_NeoPixel.h>
#define PIN 2

int bright = 20;
int prev_freq = 0;
int prev_led = 0;
boolean change = true; // true : need change
boolean low; // true : low freq // false : high freq
long ds_tics = -10; // External clock tics
volatile long timer_tic = 0;  // Internal Timer tics
volatile long tmp_tics = 0; // Tmp variable to store timer_tic
volatile int period = -10;  // Period counter
double freq = 0;  // Frequency value
long correction = 0;  // Correction factor for the internal Timer
boolean correct = false;  // If correction is calculated
boolean first_period = true;
unsigned char send_flag = 0;

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(24, PIN, NEO_RGB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);

  // Initialize NeoPixel
  strip.begin();
  strip.setBrightness(bright); //adjust brightness here
  strip.show();

  // Initialize timer1
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0; // Normal mode

  // Timer stop
  TCNT1 = 0;   // preload timer
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt

  //  Setup PC interrupt
  pinMode(7, INPUT);
  pinMode(11, INPUT);
  digitalWrite(7, HIGH);
  digitalWrite(11, LOW);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(7), pc7_int, RISING); // Enable PCInterrrupt on PIN 7 with rising edge
  interrupts();             // enable all interrupts
  set_sleep_mode(SLEEP_MODE_IDLE);
}

ISR(TIMER1_OVF_vect)        // interrupt service routine
{
  timer_tic += 65536;
}

void pc11_int() {
  if (first_period && (period == 0)) {
    TCCR1B |= (1 << CS10);  // Start internal Timer with no prescale
    first_period = false;
  }
  period++;
  if (period == 100) {
    TCCR1B = 0; // Stop internal Timer
    timer_tic = timer_tic + (TCNT1L + (TCNT1H << 8)) - correction;
    TCNT1 = 0;
    tmp_tics = timer_tic;
    timer_tic = 0;
    period = 0;
    send_flag = 1;
    TCCR1B |= (1 << CS10);
  }
}

void pc7_int() {
  if (!correct) {
    if (ds_tics == 0) TCCR1B |= (1 << CS10);  // Start internal Timer with no prescale
    ds_tics++;
    if (ds_tics == 32768) {
      TCCR1B = 0; // Stop internal Timer
      timer_tic += TCNT1;
      TCNT1 = 0;
      correction = timer_tic - 16000000;
      TCNT1 = 0;
      timer_tic = 0;
      ds_tics = 0;
      correct = true;
      attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(11), pc11_int, RISING);  // Enable PCInterrrupt on PIN 11 with rising edge
      detachPinChangeInterrupt(digitalPinToPinChangeInterrupt(7));  // Disable PCInterrupt on PIN 7
    }
  }
}

void loop() {
  boolean first_value = true;  // First freq value isn't good so just skip it
  float decimal = 0;
  while (1) {
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
    if (send_flag) {
      send_flag = 0;
      Serial.print("Frequency : ");
      if (tmp_tics <= 16000000) {
        decimal = ((16000000L - tmp_tics) / 320) * 0.001;
        freq = 50.0 + decimal;
        Serial.println(freq, 3);
      } else {
        decimal = ((1000 - (tmp_tics - 16000000L) / 320) % 1000) * 0.001;
        if (decimal == 0) decimal = 0.999;
        freq = 49.0 + decimal;
        Serial.println(freq, 3);
      }
      if (!first_value) {
        show_freq();
      }
      first_value = false;
    }
  }
}

void show_freq() {
  bright = 255;
  change = true;  // true: execute the main loop to turn on led
  int idx;
  //Index starts at 1 or 23 accorting to the value of frequency
  if (freq >= 50.0) idx = 1;
  else idx = 23;
  int nbLed = abs(50.0 - freq) / 0.002;
  Serial.print("Nb Leds: ");
  Serial.println(nbLed);
  if (prev_freq != 0) { // Don't check at the first call
    if (low) { // If previous freq was low
      if (freq > 50.0) { // If new freq is high  -> Normal clean
        for (int i = 24 - prev_led; i < 24; i++) {
          strip.setPixelColor(i, 0, 0, 0);
          strip.show();
          for (int j = i + 1; j < 24 ; j++) { // Update all previous led with the right color
            strip.setPixelColor(0, setColor(24 - (i + 1)), bright);
            strip.setPixelColor(j, setColor(24 - (i + 1)), bright);
            strip.show();
          }
          delay(100);
        }
      }
      else { // If freq is still low
        if (prev_led > nbLed) { // If new freq < prev freq
          change = false; // Do not execute the main loop, led are updated here
          for (int j = 24 - prev_led; j < 24 - nbLed; j++) {
            strip.setPixelColor(j, 0, 0, 0);
            strip.show();
            for (int k = j + 1; k < 24 ; k++) { //  Update all previous led with the right color
              strip.setPixelColor(0, setColor(24 - (j + 1)), bright);
              strip.setPixelColor(k, setColor(24 - (j + 1)), bright);
              strip.show();
            }
            delay(100);
          }
        }
        else { // If new freq > prev freq
          idx = 24 - prev_led;
        }
      }
    }
    else { // If previous freq was high
      if (freq < 50.0) { // If new freq is low -> Normal clean
        for (int i = prev_led; i > 0; i--) {
          strip.setPixelColor(i, 0, 0, 0);
          strip.show();
          for (int j = i - 1; j > 0 ; j--) { // Update all previous led with the right color
            strip.setPixelColor(0, setColor(i - 1), bright);
            strip.setPixelColor(j, setColor(i - 1), bright);
            strip.show();
          }
          delay(100);
        }
      }
      else { // If freq is still high
        if (prev_led > nbLed) { // If new freq < prev freq
          change = false; // Do not execute the main loop, led are updated here
          for (int j = prev_led; j > nbLed; j--) {
            strip.setPixelColor(j, 0, 0, 0);
            strip.show();
            for (int k = j - 1; k >= 0 ; k--) { // Update all previous led with the right color
              strip.setPixelColor(0, setColor(j - 1), bright);
              strip.setPixelColor(k, setColor(j - 1), bright);
              strip.show();
            }
            delay(100);
          }
        }
        else { // If new freq > prev freq
          idx = prev_led;
        }
      }
    }
  }
  if (change) { // If led are not already updated
    if (freq > 50.0) {
      low = false;
      for (idx; idx < nbLed; idx++) {
        for (int j = 0; j < idx; j++) {
          strip.setPixelColor(j, setColor(idx), bright);
          strip.show();
        }
        strip.setPixelColor(idx, setColor(idx), bright);
        strip.show();
        delay(100);
      }
    }
    else {
      low = true;
      for (idx ; idx > 24 - nbLed; idx--) {
        for (int j = 23; j > idx; j--) {
          strip.setPixelColor(0, setColor(24 - idx), bright);
          strip.setPixelColor(j, setColor(24 - idx), bright);
          strip.show();
        }
        strip.setPixelColor(idx, setColor(24 - idx), bright);
        strip.show();
        delay(100);
      }
    }
  }

  prev_freq = freq;
  prev_led = nbLed;
}

uint32_t setColor(int led) { // Calculate RGB value according to the number of led on
  if (led < 6) return strip.Color(255 - (led % 6) * 20, 0 + (led % 6) * 30, 0);
  else if (led < 9) return strip.Color(155 - (led % 3) * 15, 150, 0);
  else if (led < 12) return strip.Color(110 - (led % 3) * 15, 150 + (led % 3) * 15, 0);
  else if (led < 15) return strip.Color(80 - (led % 3) * 15, 195 + (led % 3) * 15, 0);
  else if (led < 18) return strip.Color(35 - (led % 3) * 10, 240 + (led % 3) * 5, 0);
  else if (led < 23) return strip.Color(0, 255, 0);
}
