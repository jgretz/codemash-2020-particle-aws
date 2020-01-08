#include "Grove_Temperature_And_Humidity_Sensor.h"
#include "Grove_ChainableLED.h"
#include "DiagnosticsHelperRK.h"

//////////
DHT dht(D2);
ChainableLED leds(A4, A5, 1);

SYSTEM_THREAD(ENABLED);

//////////
float temp, humidity;
double temp_dbl, humidity_dbl;

double currentLightLevel;

const unsigned long UPDATE_INTERVAL = 2000;
unsigned long lastUpdate = 0;

String lightpower;

// Private battery and power service UUID
const BleUuid serviceUuid("969ab5bd-3303-42a0-813e-7c350c74e06e"); // CHANGE ME

BleCharacteristic uptimeCharacteristic("uptime", BleCharacteristicProperty::NOTIFY, BleUuid("fdcf4a3f-3fed-4ed2-84e6-04bbb9ae04d4"), serviceUuid);
BleCharacteristic signalStrengthCharacteristic("strength", BleCharacteristicProperty::NOTIFY, BleUuid("cc97c20c-5822-4800-ade5-1f661d2133ee"), serviceUuid);
BleCharacteristic freeMemoryCharacteristic("freeMemory", BleCharacteristicProperty::NOTIFY, BleUuid("d2b26bf3-9792-42fc-9e8a-41f6107df04c"), serviceUuid);

//////////
void setup()
{
  Serial.begin(9600);
  
  // temp sensor
  dht.begin();

  Particle.variable("temp", temp_dbl);
  Particle.variable("humidity", humidity_dbl);

  // led
  leds.init();
  leds.setColorHSB(0, 0.0, 0.0, 0.0);

  Particle.function("toggleLed", toggleLed);

  // light sensor
  pinMode(A0, INPUT);

  // BLE
  configureBLE();
  
  // Alexa
  Particle.variable("lightpower", lightpower);
  Particle.function("toggleLightPower", toggleLightPower);
  Particle.function("toggleColor", toggleColor);
}

void loop()
{
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdate >= UPDATE_INTERVAL)
  {
    lastUpdate = millis();

    getTempStatus();
    getLightStatus();
  }
}

void getLightStatus() {
  double lightAnalogVal = analogRead(A0);
  currentLightLevel = map(lightAnalogVal, 0.0, 4095.0, 0.0, 100.0);

  if (currentLightLevel > 50) {
    Particle.publish("light-meter/level", String(currentLightLevel), PRIVATE);
  }
}

void getTempStatus() {
  float temp, humidity;

  temp = dht.getTempFarenheit();
  humidity = dht.getHumidity();

  Serial.printlnf("Temp: %f", temp);
  Serial.printlnf("Humidity: %f", humidity);

  temp_dbl = temp;
  humidity_dbl = humidity;
}

int toggleLed(String args) {
  leds.setColorHSB(0, 0.0, 1.0, 0.5);

  delay(500);

  leds.setColorHSB(0, 0.0, 0.0, 0.0);

  delay(500);

  return 1;
}

int toggleLightPower(String command) {
    if (command == "on") {
        leds.setColorHSB(0, 0.0, 1.0, 1.0);
        Particle.publish("[ON] Alexa updated lightpower state to ON.");
        lightpower = command;
        return 1;
    }
    else if (command == "off") {
        leds.setColorHSB(0, 0.0, 0.0, 0.0);
        Particle.publish("[OFF] Alexa updated lightpower state to OFF.");
        lightpower = command;
        return 1;
    }
    else {
        if (lightpower == "on") toggleLightPower("off");
        else if (lightpower == "off") toggleLightPower("on");
    }
}

int toggleColor(String command) {
    if (command == "red")         leds.setColorRGB(0, 255, 0, 0);
    else if (command == "orange") leds.setColorRGB(0, 255, 165, 0);
    else if (command == "yellow") leds.setColorRGB(0, 255, 255, 0);
    else if (command == "green")  leds.setColorRGB(0, 0, 255, 0);
    else if (command == "blue")   leds.setColorRGB(0, 0, 0, 255);
    else if (command == "purple") leds.setColorRGB(0, 128, 0, 128);
    else leds.setColorHSB(0, 255, 255, 255);

    Particle.publish("[" + command.toUpperCase() + "] Alexa updated LED color to " + command + ".");
    return 1;
}

void configureBLE()
{
  BLE.addCharacteristic(uptimeCharacteristic);
  BLE.addCharacteristic(signalStrengthCharacteristic);
  BLE.addCharacteristic(freeMemoryCharacteristic);

  BleAdvertisingData advData;

  // Advertise our private service only
  advData.appendServiceUUID(serviceUuid);

  // Continuously advertise when not connected
  BLE.advertise(&advData);
}

void writeBLE() {
  if (BLE.connected())
  {
    uint8_t uptime = (uint8_t)DiagnosticsHelper::getValue(DIAG_ID_SYSTEM_UPTIME);
    uptimeCharacteristic.setValue(uptime);

    uint8_t signalStrength = (uint8_t)(DiagnosticsHelper::getValue(DIAG_ID_NETWORK_SIGNAL_STRENGTH) >> 8);
    signalStrengthCharacteristic.setValue(signalStrength);

    int32_t usedRAM = DiagnosticsHelper::getValue(DIAG_ID_SYSTEM_USED_RAM);
    int32_t totalRAM = DiagnosticsHelper::getValue(DIAG_ID_SYSTEM_TOTAL_RAM);
    int32_t freeMem = (totalRAM - usedRAM);
    freeMemoryCharacteristic.setValue(freeMem);
  }
}