#define DEBUG

/*Global Variables*/
unsigned long previousMillis = 0;
const long interval = 1000;
unsigned long last_read_time = 0;

float temp_by_ds18b20;

// Servo Variables
int servo_open_pos = 135;
int servo_close_pos = 25;
int last_pos_servo;
int servo_delay = 35;



String Last_Fan_Speed = "";
int buttonpressed = 0;
bool adjust_setpoint = false;
int last_temp_setpoint = 0;

int CFM = 0;
int CFM_s;
int minval, maxval;


const int Buzzer_Pin = 23;
const int LED_Pin = 22;

/*Libraries*/
#include "Temp.h"
#include "Servo.h"
// #include "ServerForWiFiCredentials.h"
#include "TM1637_Display.h"
#include "IR_RECEIVER.h"
#include <Preferences.h>

Preferences preferences;

void Pot_Calib(int min, int max) {
  if (min == 0 || max == 0) {
    if (min == 0) {
      myservo.write(servo_close_pos);
      delay(1000);
      int potmin = analogRead(34);
      delay(1000);
      preferences.begin("Pot", false);
      preferences.putInt("Min", potmin);
      preferences.end();
#ifdef DEBUG
      Serial.print("PotMin:");
      Serial.println(potmin);
#endif
    }
    if (max == 0) {
      myservo.write(servo_open_pos);
      delay(1000);
      int potmax = analogRead(34);
      delay(1000);
      preferences.begin("Pot", false);
      preferences.putInt("Max", potmax);
      preferences.end();
#ifdef DEBUG
      Serial.print("potmax:");
      Serial.println(potmax);
#endif
    }
  }
}


int ReadPot(int potPin) {
  int minValue = minval;  // at servo_close_pos
  int maxValue = maxval;  // at servo_open_pos

  float potValue = analogRead(potPin);
  int mappedValue = map(potValue, minValue, maxValue, 0, 100);

  if (mappedValue < minValue) {
    minValue = mappedValue;
  } else if (mappedValue > maxValue) {
    maxValue = mappedValue;
  }
  return mappedValue;
}

void Power_OnOff(bool power) {
  if (power == false) {
    MoveServo(servo_close_pos, 1, servo_delay);
    show_on_led(0, 0);
  }
}

void Debug() {
  Serial.print("AC Power: ");
  Serial.println(acPower ? "On" : "Off");

  Serial.print("Temperature Setpoint: ");
  Serial.println(temperatureSetpoint);

  Serial.print("AC Mode: ");
  Serial.println(acMode);

  Serial.print("Fan Speed: ");
  Serial.println(fanSpeed);
}


void Beep(int beepDelay, int numberOfBeeps) {
  for (int i = 0; i < numberOfBeeps; i++) {
    digitalWrite(Buzzer_Pin, HIGH);
    delay(beepDelay);
    digitalWrite(Buzzer_Pin, LOW);
    delay(beepDelay);
  }
}


void Control_CFM(bool Powerr, String Fan_Speed, int temperature_setpoint, String AC_MODE) {

  if (Powerr == true) {
    if (Fan_Speed != Last_Fan_Speed) {
      if (buttonpressed < 4) {
        buttonpressed++;
        if (buttonpressed == 4 && adjust_setpoint == false) {
          adjust_setpoint = true;
          Beep(50, 3);
        } else if (buttonpressed == 4 && adjust_setpoint == true) {
          adjust_setpoint = false;
          Beep(1000, 1);
        }
      } else {
        buttonpressed = 1;
      }
      Last_Fan_Speed = Fan_Speed;
    }

    if (adjust_setpoint == true) {
      
      if (temperature_setpoint > 19) {
        int cfm = (temperature_setpoint - 20) * 10;
        preferences.begin("CFM", false);
        preferences.putInt("cfm", cfm);
        preferences.end();
        CFM_s = map(cfm, 0, 100, servo_close_pos, servo_open_pos);
        MoveServo(CFM_s, 1, servo_delay);
      }
    }  //
    else {
      /*Check Setpoint Condition*/
      if (AC_MODE == "COOL") {
        if (last_temp_setpoint > temp_by_ds18b20) {
          MoveServo(servo_close_pos, 1, servo_delay);
        } else if (last_temp_setpoint <= temp_by_ds18b20) {
          MoveServo(CFM_s, 1, servo_delay);
        }
      } else if (AC_MODE == "HEAT") {
        if (last_temp_setpoint < temp_by_ds18b20) {
          MoveServo(servo_close_pos, 1, servo_delay);
        } else if (last_temp_setpoint >= temp_by_ds18b20) {
          MoveServo(CFM_s, 1, servo_delay);
        }
      }
    }
  }  //
  else {
    adjust_setpoint = false;
    buttonpressed = 0;
  }
}


void setup() {
  Int_Servo();
  // preferences.begin("Pot", false);
  // preferences.putInt("Min", 0);
  // preferences.putInt("Max", 0);
  // preferences.end();
#ifdef DEBUG
  Serial.begin(115200);
#endif
  preferences.begin("Pot", false);
  minval = preferences.getInt("Min", 0);
  maxval = preferences.getInt("Max", 0);
  preferences.end();

  Pot_Calib(minval, maxval);

  int last_pot_value = ReadPot(34);
  last_pos_servo = map(last_pot_value, 0, 100, servo_close_pos, servo_open_pos);

  pinMode(Buzzer_Pin, OUTPUT);
  digitalWrite(Buzzer_Pin, LOW);
  pinMode(LED_Pin, OUTPUT);
  digitalWrite(LED_Pin, LOW);

  MoveServo(servo_open_pos, 1, servo_delay);
  delay(1000);
  MoveServo(servo_close_pos, 1, servo_delay);
  irrecv.enableIRIn();          // Start the receiver
  display.setBrightness(0x0a);  // set the brightness (0x00 to 0x0f)
  Beep(200, 1);
  digitalWrite(LED_Pin, HIGH);
  last_read_time = millis();
  preferences.begin("CFM", false);
  CFM = preferences.getInt("cfm", 50);
  preferences.end();
  CFM_s = map(CFM, 0, 100, servo_close_pos, servo_open_pos);
}


void loop() {
  if (millis() - last_read_time >= 1500) {
    temp_by_ds18b20 = readDS18B20Temperature();
    last_read_time = millis();
  }
  /*Check Remote Condition*/
  Power_OnOff(acPower);
  if (acPower == true && temperatureSetpoint != last_temp_setpoint) {
    show_on_led(1, temperatureSetpoint);
    last_temp_setpoint = temperatureSetpoint;
    previousMillis = millis();
  } else if (acPower == true && temperatureSetpoint == last_temp_setpoint && millis() >= previousMillis + 1200) {
    show_on_led(0, temp_by_ds18b20);  // Display temperature in Celsius
  }


  Control_CFM(acPower, fanSpeed, last_temp_setpoint, acMode);
  /*Check IR Remote Input*/
  if (irrecv.decode(&results)) {
    // Serial.println(resultToHumanReadableBasic(&results));
    // Serial.println(results.value, BIN);
    if (results.bits == 48 && results.decode_type == KELON) {
      Beep(50, 1);
      Decoder_Remote(results.value);
      Debug();
    }
    irrecv.resume();
  }
}
