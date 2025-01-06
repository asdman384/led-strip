#include <Arduino.h>
#include <FastLED.h>

#define NUM_LEDS 299
#define DATA_PIN D7
#define BOOT_BUTTON_PIN 9 // Boot is usually GPIO 9 on the ESP32C3 boards.

CRGB leds[NUM_LEDS];
CRGB state_led[1];
volatile bool buttonPressed = false;

CLEDController *stripController;
CLEDController *stateController;

void IRAM_ATTR handleButtonPress()
{
  buttonPressed = true;
}

void setup()
{
  // Serial.begin(9600);

  stripController = &FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  stateController = &FastLED.addLeds<WS2812, D10, GRB>(state_led, 1);

  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP); // Set boot button pin as input with pull-up resistor
  attachInterrupt(digitalPinToInterrupt(BOOT_BUTTON_PIN), handleButtonPress, FALLING);
}

void clearAll()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Black;
  }
}

void snake()
{
  static uint8_t hue = 0;

  for (int i = 0; i < NUM_LEDS; i++)
  {

    if (buttonPressed)
    {
      return;
    }

    clearAll();

    int tailLength = 10;
    for (int j = 0; j < tailLength; j++)
    {
      if (i - j >= 0)
      {
        leds[i - j] = CHSV((hue + (i - j) * 10) % 255, 255, 255 - j * (255 / tailLength));
      }
    }

    stripController->showLeds(255);
    delay(20);
  }
}

void loop()
{
  static uint8_t state = 0;
  if (buttonPressed)
  {
    state++;
    buttonPressed = false;
  }

  switch (state)
  {
  case 0:
    clearAll();
    stripController->showLeds(255);

    state_led[0] = CRGB::Blue;
    stateController->showLeds(20);
    delay(200);
    break;

  case 1:
    state_led[0] = CRGB::Red;
    stateController->showLeds(20);
    delay(200);
    break;

  case 2:
    state_led[0] = CRGB::Black;
    stateController->showLeds(20);
    delay(200);
    break;

  case 3:
    state_led[0] = CRGB::Black;
    stateController->showLeds(20);
    snake();
    break;

  case 4:
    clearAll();
    state_led[0] = CRGB::DeepPink;
    leds[0] = CRGB::DeepPink;
    FastLED.show();
    delay(500);

    break;

  default:
    state = 0;
    break;
  }
}
