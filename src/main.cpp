#include <Arduino.h>
#include <FastLED.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// strip
#define NUM_LEDS 299
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
      Serial.print(d);
    }
    state_led[0] = CRGB(data[0], data[1], data[2]);
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
  delay(5000);
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

  setupBLE();
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

    strip->showLeds(255);
    delay(20);
  }
}

void loop()
{
  if (buttonPressed)
  {
    currentState++;
    Serial.println("currentState: " + String(currentState));
    buttonPressed = false;
  }

  switch (currentState)
  {
  case 0:
    clearAll();
    strip->showLeds(255);

    state_led[0] = CRGB::Blue;
    state->showLeds(20);
    delay(1000);
    break;

  case 1:
    state_led[0] = CRGB::Red;
    state->showLeds(20);
    delay(1000);
    break;

  case 2:
    state_led[0] = CRGB::Green;
    state->showLeds(20);
    delay(1000);
    break;

  case 3:
    state_led[0] = CRGB::GreenYellow;
    state->showLeds(10);
    snake();
    break;

  case 4:
    clearAll();
    state_led[0] = CRGB::DeepPink;
    leds[0] = CRGB::DeepPink;
    FastLED.show();
    delay(1000);

    break;

  case 5:
    delay(50);
    state->showLeds(255);
    break;

  default:
    currentState = 0;
    break;
  }
}
