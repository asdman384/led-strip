#include <Arduino.h>
#include <FastLED.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// rainbow color array
uint8_t hues[] = {
    0, 5, 11, 17, 23, 28, 34, 40, 46, 52, 57, 63, 69, 75,
    81, 86, 92, 98, 104, 110, 115, 121, 127, 133, 139,
    144, 150, 156, 162, 168, 173, 179, 185, 191, 197,
    202, 208, 214, 220, 226, 231, 237, 243, 249, 255};

// starCanvas
uint8_t starCanvas[16][9] = {
    {44, 99, 99, 99, 99, 99, 99, 99, 99},
    {43, 0, 99, 99, 99, 99, 99, 99, 99},
    {42, 1, 99, 99, 99, 99, 99, 99, 99},
    {41, 2, 99, 99, 99, 99, 99, 99, 99},
    {40, 3, 99, 99, 99, 99, 99, 99, 99},
    {35, 36, 37, 38, 39, 4, 5, 6, 7},
    {34, 8, 99, 99, 99, 99, 99, 99, 99},
    {33, 9, 99, 99, 99, 99, 99, 99, 99},
    {32, 10, 99, 99, 99, 99, 99, 99, 99},
    {31, 11, 99, 99, 99, 99, 99, 99, 99},
    {30, 12, 99, 99, 99, 99, 99, 99, 99},
    {29, 13, 99, 99, 99, 99, 99, 99, 99},
    {28, 22, 21, 14, 99, 99, 99, 99, 99},
    {27, 24, 23, 20, 19, 15, 99, 99, 99},
    {26, 25, 18, 16, 99, 99, 99, 99, 99},
    {17, 99, 99, 99, 99, 99, 99, 99, 99},
};
// /*                                */ {44},
// /*                            */ {43, /**/ 0},
// /*                         */ {42, /*     */ 1},
// /*                      */ {41, /*          */ 2},
// /*                   */ {40, /*               */ 3},
// /**/ {35, 36, 37, 38, 39, /*                     */ 4, 5, 6, 7},
// /*  */ {34, /*                                         */ 8},
// /*    */ {33, /*                                    */ 9},
// /*      */ {32, /*                              */ 10},
// /*        */ {31, /*                         */ 11},
// /*        */ {30, /*                     */ 12},
// /*      */ {29, /*                      */ 13},
// /*    */ {28, /*     */ 22, 21, /*       */ 14},
// /*  */ {27, /**/ 24, 23, /**/ 20, 19, /*  */ 15},
// /**/ {26, 25, /*                 */ 18, /*  */ 16},
// /*                                     */ {17}};

// strip
#define NUM_LEDS 45
#define DATA_PIN D7
CRGB leds[NUM_LEDS];
CRGB state_led[1];
CLEDController *strip;
CLEDController *state;

// button
#define BOOT_BUTTON_PIN 9 // Boot is usually GPIO 9 on the ESP32C3 boards.
volatile bool buttonPressed = false;

// BLE
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "a6237aee-71d3-4bb2-be69-07d81b255115"

static uint8_t currentState = 0;

void IRAM_ATTR handleButtonPress()
{
  buttonPressed = true;
}

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    Serial.println("BLEServer onConnect");
  };

  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("BLEServer onDisconnect");
    delay(500); // give the bluetooth stack the chance to get things ready
    BLEDevice::startAdvertising();
    Serial.println("BLEDevice::startAdvertising(); done");
  }
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {

    uint8_t *data = pCharacteristic->getData();
    for (size_t i = 0; i < pCharacteristic->getLength(); i++)
    {
      uint8_t d = data[i];
      Serial.print(d + ' ');
    }
    state_led[0] = CHSV(data[0], data[1], data[2]);
    Serial.println();

    // std::string value = pCharacteristic->getValue();
    // Serial.print("Received Value: ");
    // for (int i = 0; i < value.length(); i++)
    // {
    //   Serial.print(value[i]);
    // }
  }
};

void setupBLE()
{
  Serial.println("init LED-ESP32");
  BLEDevice::init("LED-ESP32"); // set the device name

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic =
      pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->setValue("WNR");
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("init LED-ESP32 done");
}

void clearAll()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Black;
  }
}

void setup()
{
  Serial.begin(9600);

  strip = &FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  state = &FastLED.addLeds<WS2812, D10, GRB>(state_led, 1);

  clearAll();
  state_led[0] = CRGB::Black;
  FastLED.show();

  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP); // Set boot button pin as input with pull-up resistor
  attachInterrupt(digitalPinToInterrupt(BOOT_BUTTON_PIN), handleButtonPress, FALLING);

  delay(1000);
  // setupBLE();
}

void snake()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {

    if (buttonPressed)
    {
      return;
    }

    clearAll();

    int tailLength = 25;
    for (int j = 0; j < tailLength; j++)
    {
      int idx = i - j;
      if (idx < 0)
      {
        idx = NUM_LEDS + idx;
      }

      leds[idx] = CHSV(hues[idx], 255, 255 - j * (255 / tailLength));
    }

    strip->showLeds(255);
    delay(38);
  }
}

void starTwinkle()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CHSV(random(0, 255), 255, random(0, 255));
  }
  strip->showLeds(255);
  delay(200);
}

void starPulse()
{
  static uint8_t brightness = 0;
  static int8_t direction = 1;

  brightness += direction * 5;
  if (brightness == 0 || brightness == 255)
  {
    direction = -direction;
  }

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CHSV(0, 0, brightness);
  }
  strip->showLeds(255);
  delay(20);
}

void starRainbow()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    if (buttonPressed)
    {
      return;
    }

    for (int j = 0; j < NUM_LEDS; j++)
    {
      int hueIndex = i - j;
      if (hueIndex < 0)
      {
        hueIndex = NUM_LEDS + hueIndex;
      }

      leds[j] = CHSV(hues[hueIndex], 255, 255);
    }

    strip->showLeds(255);
    delay(38);
  }
}

void starWave()
{
  for (int i = 0; i < 256; i++)
  {
    if (buttonPressed)
    {
      return;
    }

    for (int l = 0; l < NUM_LEDS; l++)
    {
      leds[l] = CHSV(i, 255, 255);
    }
    strip->showLeds(255);
    delay(38);
  }
}

void starChase()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    if (buttonPressed)
    {
      return;
    }

    clearAll();
    for (int j = 0; j < 5; j++)
    {
      int idx = (i + j * 9) % NUM_LEDS;
      leds[idx] = CHSV(hues[idx], 255, 255);
    }
    strip->showLeds(255);
    delay(25);
  }
}

void starRazukrashka()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    if (buttonPressed)
    {
      return;
    }

    leds[random(NUM_LEDS)] = CHSV(random(0, 255), 255, 255);
    strip->showLeds(255);
    delay(50);
  }
}

void starWaveRainbow()
{
  for (int i = 0; i < 256; i++)
  {
    if (buttonPressed)
    {
      return;
    }

    for (int j = 0; j < NUM_LEDS; j++)
    {
      leds[j] = CHSV((i + j * 10) % 256, 255, 255);
    }
    strip->showLeds(255);
    delay(50);
  }
}

void loop()
{
  if (buttonPressed)
  {
    currentState++;
    Serial.println("currentState: " + String(currentState));
    buttonPressed = false;

    clearAll();
    state_led[0] = CRGB::Black;
    FastLED.show();
  }

  switch (currentState)
  {
  case 0:
    state_led[0] = CRGB::Blue;
    state->showLeds(20);
    delay(500);
    break;

  case 1:
    state_led[0] = CRGB::Red;
    state->showLeds(20);
    delay(500);
    break;

  case 2:
    snake();
    break;

  case 3:
    starWave();
    break;

  case 4:
    starTwinkle();
    break;

  case 5:
    starPulse();
    break;

  case 6:
    starRainbow();
    break;

  case 7:
    starChase();
    break;

  case 8:
    starRazukrashka();
    break;

  case 9:
    starWaveRainbow();
    break;

  case 10:
    for (int i = 0; i < 16; i++)
    {
      for (int j = 0; j < 9; j++)
      {
        Serial.print(starCanvas[i][j]);
        Serial.print(",");
        if (starCanvas[i][j] != 99)
        {
          leds[starCanvas[i][j]] = CHSV(200, 255, 55);
        }
      }
      Serial.println();

      strip->showLeds(255);
      delay(500);
      clearAll();
    }

    break;

  default:
    currentState = 0;
    break;
  }
}
