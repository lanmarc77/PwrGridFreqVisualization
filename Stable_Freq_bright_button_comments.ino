#include <avr/sleep.h>
#include <PinChangeInterrupt.h>
#include <Adafruit_NeoPixel.h>
#define PIN_NEO 2
#define PIN_BUTTON 5
#define PIN_FREQ 9
#define PIN_CLOCK 13

boolean first_freq = true;  // First freq has to de displayed in one time
int buttonTic = 0;  // 1 press on button = 1 tic
float range = 0.00109; // Default : 50Mhz
boolean buttonPressed = false;  // If button is pressed
int tmpButtonTic = 0; // Tmp cariable to store buttonTic
int bright = 42;  // NeoPixel brightness
float prev_freq = 0;  // Value of the previous frequency when a new one is calculated
int prev_led = 0; // Value of the previous variable nbLed when a new frequency is calculated
int idx = 0;  // LED index by traversing the NeoPixel
int nbLed = 0;  // Number of LEds to turn on according to the frequency value
int delay_t = 0;  // Value of delay between 2 LEDs
boolean modeChange = false; // If mode changed i.e. button is pressed
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
unsigned char send_flag = 0;  // True when a new frequency is calculated

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number
Adafruit_NeoPixel strip = Adafruit_NeoPixel(24, PIN_NEO, NEO_RGB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);

  // Initialize NeoPixel
  strip.begin();
  strip.setBrightness(bright); //adjust brightness here
  strip.show();

  pinMode(A0, OUTPUT);
  digitalWrite(A0, LOW);
  pinMode(A1, OUTPUT);
  digitalWrite(A1, LOW);
  pinMode(A2, OUTPUT);
  digitalWrite(A2, LOW);
  pinMode(A3, OUTPUT);
  digitalWrite(A3, LOW);
  pinMode(A4, OUTPUT);
  digitalWrite(A4, LOW);
  pinMode(A5, OUTPUT);
  digitalWrite(A5, LOW);

  // Initialize timer1
  noInterrupts(); // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0; // Normal mode

  // Timer stop
  TCNT1 = 0;   // preload timer
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt

  //  Setup PC interrupt
  pinMode(PIN_CLOCK, INPUT);
  pinMode(PIN_FREQ, INPUT);
  digitalWrite(PIN_CLOCK, HIGH);
  digitalWrite(PIN_FREQ, LOW);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(PIN_CLOCK), clock_int, RISING); // Enable PCInterrrupt on PIN_CLOCK with rising edge
  pinMode(PIN_BUTTON, INPUT);
  digitalWrite(PIN_BUTTON, HIGH);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(PIN_BUTTON), button_int, FALLING  );
  interrupts();             // enable all interrupts
  set_sleep_mode(SLEEP_MODE_IDLE);
}

ISR(TIMER1_OVF_vect) {  // interrupt service routine
  timer_tic += 65536;
}

void button_int() { // Button Interrupt
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 100) {
    if (!buttonPressed) {
      buttonTic++;
      buttonPressed = true;
      tmpButtonTic = buttonTic;
      modeChange = true;
      idx = 0;
      change = false;
      prev_led = 0;
      prev_freq = 0;
      nbLed = 0;
    }
    else {
      buttonPressed = false;
    }
  }
  last_interrupt_time = interrupt_time;
}

void setMode(int btTic) {
  if (buttonTic % 2 == 1) {
    Serial.println("Mode 2 ~ 200MHz");
    range = 0.0043;
    for (int i = 0; i < 24; i++) {
      strip.setPixelColor(i, 255, 255, 255);
      strip.show();
    }
  }
  else {
    Serial.println("Mode 1 ~ 50MHz");
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
}

void freq_int() {
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

void clock_int() {
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
      attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(PIN_FREQ), freq_int, RISING);  // Enable PCInterrrupt on PIN_FREQ with rising edge
      detachPinChangeInterrupt(digitalPinToPinChangeInterrupt(PIN_CLOCK));  // Disable PCInterrupt on PIN_CLOCK
    }
  }
}

void loop() {
  boolean first_value = true;  // First freq value isn't good so just skip it
  float decimal = 0;
  while (1) {
    if (modeChange) {
      setMode(tmpButtonTic);
      modeChange = false;
    }
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
  //freq = random(49965, 50034) / 1000.0;
  //Index starts at 1 or 23 accorting to the value of frequency
  if (freq >= 50.0) idx = 1;
  else idx = 23;
  nbLed = abs(50.0 - freq) / range;
  if (nbLed > 23) nbLed = 23;
  if (nbLed == 0) nbLed = 1;
  delay_t = (23 - abs(prev_led - nbLed)) * 1.5;
  Serial.print("Nb Leds: ");
  Serial.println(nbLed);
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

uint32_t setColor(int led) { // Calculate RGB value according to the number of led on
  if (led < 6) return strip.Color(255 - (led % 6) * 20, 0 + (led % 6) * 30, 0);
  else if (led < 9) return strip.Color(155 - (led % 3) * 15, 150, 0);
  else if (led < 12) return strip.Color(110 - (led % 3) * 15, 150 + (led % 3) * 15, 0);
  else if (led < 15) return strip.Color(80 - (led % 3) * 15, 195 + (led % 3) * 15, 0);
  else if (led < 18) return strip.Color(35 - (led % 3) * 10, 240 + (led % 3) * 5, 0);
  else if (led < 24) return strip.Color(0, 255, 0);
}
