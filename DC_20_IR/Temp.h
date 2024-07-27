#include <OneWire.h>

#define DS18B20_PIN 4  // Pin where the DS18B20 is connected

OneWire ds(DS18B20_PIN);  // Create a OneWire instance

float readDS18B20Temperature() {
  byte data[9];
  byte addr[8];

  ds.reset_search();  // Reset the search
  if (!ds.search(addr)) {
    ds.reset_search();
    return -999;  // No device found
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
    return -999;  // CRC is not valid
  }

  if (addr[0] != 0x28) {
    return -999;  // Device is not a DS18B20
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);  // Start conversion, with parasite power on at the end

  delay(500);  // Wait for conversion to complete

  // unsigned long last_conversion_time = millis();
  // while(millis() - last_conversion_time <= 500){}

  ds.reset();
  ds.select(addr);
  ds.write(0xBE);  // Read Scratchpad

  for (int i = 0; i < 9; i++) {  // We need 9 bytes
    data[i] = ds.read();
  }

  if (OneWire::crc8(data, 8) != data[8]) {
    return -999;  // CRC is not valid
  }

  int16_t raw = (data[1] << 8) | data[0];

  if (addr[0] == 0x10) {  // Model is DS18S20
    raw = raw << 3;       // 9-bit resolution default
    if (data[7] == 0x10) {
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw & ~7;       // 9-bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3;  // 10-bit resolution, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1;  // 11-bit resolution, 375 ms
  }

  float celsius = (float)raw / 16.0;
  return celsius;
}
