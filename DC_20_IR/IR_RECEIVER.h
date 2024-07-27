// SET 74 Mode on remote 
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
 
const uint16_t kRecvPin = 27;
IRrecv irrecv(kRecvPin);
decode_results results;
bool acPower = false;
int temperatureSetpoint = 0;  // Default temperature
String acMode = "";
String fanSpeed = "";
bool swing = false;

void Decoder_Remote(uint64_t rawData) {
  acPower = (rawData & 0x080000000000) != 0;
  uint8_t tempLSB = (rawData >> 8) & 0xFF;   
  temperatureSetpoint = tempLSB - 92;  

  uint8_t Mode = (rawData >> 36) & 0xF;

  switch (Mode) {
    case 0:
      acMode = "DRY";
      break;
    case 1:
      acMode = "COOL";
      break;
    case 2:
      acMode = "AUTO";
      break;
    case 3:
      acMode = "HEAT";
      break;
    case 4:
      acMode = "DROP";
      break;
    default:
      acMode = "Unknown";
      break;
  }

  switch ((rawData >> 32) & 0x0F) {  // 2 bits for fan speed
    case 0b00:
      fanSpeed = "Auto";  
      break;
    case 0b01:
      fanSpeed = "High"; 
      break;
    case 0b10:
      fanSpeed = "Medium";
      break;
    case 0b11:
      fanSpeed = "Low";
      break;
    default:
      fanSpeed = "Unknown";
      break;
  }

}
