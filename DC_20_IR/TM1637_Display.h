uint8_t m_pinClk_;
uint8_t m_pinDIO_;
uint8_t m_brightness;
unsigned int m_bitDelay;

#define SEG_A 0b00000001
#define SEG_B 0b00000010
#define SEG_C 0b00000100
#define SEG_D 0b00001000
#define SEG_E 0b00010000
#define SEG_F 0b00100000
#define SEG_G 0b01000000
#define SEG_DP 0b10000000

#define TM1637_I2C_COMM1 0x40
#define TM1637_I2C_COMM2 0xC0
#define TM1637_I2C_COMM3 0x80

const int CLK_ = 18;  // CLK_ pin to D5 on TM1637
const int DIO_ = 19;  // DIO_ pin to D7 on TM1637

const uint8_t digitToSegment[] = {
  // XGFEDCBA
  0b00111111,  // 0
  0b00000110,  // 1
  0b01011011,  // 2
  0b01001111,  // 3
  0b01100110,  // 4
  0b01101101,  // 5
  0b01111101,  // 6
  0b00000111,  // 7
  0b01111111,  // 8
  0b01101111,  // 9
  0b01110111,  // A
  0b01111100,  // b
  0b00111001,  // C
  0b01011110,  // d
  0b01111001,  // E
  0b01110001   // F
};

static const uint8_t minusSegments = 0b01000000;

#define DEFAULT_BIT_DELAY 100

class TM1637Display {

public:

  //! Initialize a TM1637Display object, setting the clock and
  TM1637Display(uint8_t pinClk_, uint8_t pinDIO_, unsigned int bitDelay = DEFAULT_BIT_DELAY) {
    // Copy the pin numbers
    m_pinClk_ = pinClk_;
    m_pinDIO_ = pinDIO_;
    m_bitDelay = bitDelay;

    // Set the pin direction and default value.
    // Both pins are set as inputs, allowing the pull-up resistors to pull them up
    pinMode(m_pinClk_, INPUT);
    pinMode(m_pinDIO_, INPUT);
    digitalWrite(m_pinClk_, LOW);
    digitalWrite(m_pinDIO_, LOW);
  }

  //! Sets the brightness of the display.
  void setBrightness(uint8_t brightness, bool on = true) {
    m_brightness = (brightness & 0x7) | (on ? 0x08 : 0x00);
  }

  //! Display arbitrary data on the module
  void setSegments(const uint8_t segments[], uint8_t length = 4, uint8_t pos = 0) {
    // Write COMM1
    start();
    writeByte(TM1637_I2C_COMM1);
    stop();

    // Write COMM2 + first digit address
    start();
    writeByte(TM1637_I2C_COMM2 + (pos & 0x03));

    // Write the data bytes
    for (uint8_t k = 0; k < length; k++)
      writeByte(segments[k]);

    stop();

    // Write COMM3 + brightness
    start();
    writeByte(TM1637_I2C_COMM3 + (m_brightness & 0x0f));
    stop();
  }

  //! Clear the display
  void clear() {
    uint8_t data[] = { 0, 0, 0, 0 };
    setSegments(data);
  }

  //! Translate a single digit into 7 segment code
  static uint8_t encodeDigit(uint8_t digit) {
    return digitToSegment[digit & 0x0f];
  }

  void bitDelay() {
    delayMicroseconds(m_bitDelay);
  }

  void start() {
    pinMode(m_pinDIO_, OUTPUT);
    bitDelay();
  }

  void stop() {
    pinMode(m_pinDIO_, OUTPUT);
    bitDelay();
    pinMode(m_pinClk_, INPUT);
    bitDelay();
    pinMode(m_pinDIO_, INPUT);
    bitDelay();
  }

  bool writeByte(uint8_t b) {
    uint8_t data = b;

    // 8 Data Bits
    for (uint8_t i = 0; i < 8; i++) {
      // CLK_ low
      pinMode(m_pinClk_, OUTPUT);
      bitDelay();

      // Set data bit
      if (data & 0x01)
        pinMode(m_pinDIO_, INPUT);
      else
        pinMode(m_pinDIO_, OUTPUT);

      bitDelay();

      // CLK_ high
      pinMode(m_pinClk_, INPUT);
      bitDelay();
      data = data >> 1;
    }

    // Wait for acknowledge
    // CLK_ to zero
    pinMode(m_pinClk_, OUTPUT);
    pinMode(m_pinDIO_, INPUT);
    bitDelay();

    // CLK_ to high
    pinMode(m_pinClk_, INPUT);
    bitDelay();
    uint8_t ack = digitalRead(m_pinDIO_);
    if (ack == 0)
      pinMode(m_pinDIO_, OUTPUT);


    bitDelay();
    pinMode(m_pinClk_, OUTPUT);
    bitDelay();

    return ack;
  }
};


// Create a display object
TM1637Display display(CLK_, DIO_);

void show_on_led(uint8_t mode, uint8_t Temp) {
  uint8_t segs_1[] = { 0x00, display.encodeDigit(Temp / 10), display.encodeDigit(Temp % 10), 0x00 };

  if (Temp == 0) {
    segs_1[1] = 0x00;
    segs_1[2] = 0x00;
  }  //

  switch (mode) {
    case 0:
      segs_1[0] = 0x00;
      display.setSegments(segs_1);
      break;
    case 1:
      segs_1[0] = 0x01;
      display.setSegments(segs_1);
      break;
    case 2:
      segs_1[0] = 0x01 | 0x08;
      display.setSegments(segs_1);
      break;
    case 3:
      segs_1[0] = 0x01 | 0x08 | 0x40;
      display.setSegments(segs_1);
      break;
    default:

      break;
  }
}
