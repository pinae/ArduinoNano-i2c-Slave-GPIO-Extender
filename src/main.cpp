#include <Arduino.h>
#include <Wire.h>

#define I2C_ADDR 64

char ioConfigA = 0;
char ioConfigB = 0;
char readState = '\x12';

void printBinary(char data) {
  for (unsigned short i = 0; i < 8; i++) {
    Serial.print((data >> (7 - i)) & 1);
  }
}

void configurePins(unsigned short bank, char configuration) {
  switch (bank) {
    case 0: ioConfigA = configuration; break;
    case 1: ioConfigB = configuration & '\x0f'; break;
    default: Serial.println("Error: Unknown Bank selected."); return;
  }
  for (unsigned short i = 0; i < 8; i++) {
    if ((configuration & (1 << i)) && ((i + 2 + bank * 8) <= 13)) {
      pinMode(i + 2 + bank * 8, INPUT);
    } else {
      pinMode(i + 2 + bank * 8, OUTPUT);
    }
  }
  Serial.print("New I/O configuration (Bank ");
  Serial.print(bank);
  Serial.print("): ");
  printBinary(configuration);
  Serial.println(".");
}

void setOutput(unsigned short bank, char data) {
  char ioConfig;
  switch (bank) {
    case 0: ioConfig = ioConfigA; break;
    case 1: ioConfig = ioConfigB; break;
    default: Serial.println("Error: Unknown Bank selected."); return;
  }
  Serial.print("I/O config:  ");
  printBinary(ioConfig);
  Serial.println(".");
  Serial.print("Output data: ");
  printBinary(data);
  Serial.println(".");
  for (unsigned short i = 0; i < 8; i++) {
    if (!(ioConfig & (1 << i))) {
      digitalWrite(i + 2 + bank * 8, data & (1 << i));
    }
  }
}

char readInput(unsigned short bank) {
  unsigned int bitcount;
  char ioConfig;
  switch (bank) {
    case 0: ioConfig = ioConfigA; bitcount = 8; break;
    case 1: ioConfig = ioConfigB; bitcount = 4; break;
    default: Serial.println("Error: Unknown Bank selected."); return '\x00';
  }
  char bits = 0;
  for (unsigned short i = 0; i < bitcount; i++) {
    bits = bits << 1;
    if (ioConfig & (1 << (bitcount - 1 - i))) bits += digitalRead(1 + bitcount + bank * 8 - i);
  }
  Serial.print("Input data: ");
  printBinary(bits);
  Serial.println(".");
  return bits;
}

bool checkNextByte() {
  if (Wire.available() <= 0) {
    Serial.println("Error: There was no further Byte in the data.");
    return false;
  } else {
    return true;
  }
}

void printOpError(char op) {
  Serial.print("Error: Unknown OP-Byte (Byte 0): "); 
  Serial.print((unsigned int) op); 
  Serial.println(".");
}

void decodeMessage() {
  if (Wire.available() <= 0) {
    Serial.println("Error: There was no OP-Byte (Byte 0) in the data.");
    return;
  }
  char op = Wire.read();
  switch (op) {
    case '\x00': if (checkNextByte()) configurePins(0, Wire.read()); break;
    case '\x01': if (checkNextByte()) configurePins(1, Wire.read()); break;
    case '\x12': readState = '\x12'; break;
    case '\x13': readState = '\x13'; break;
    case '\x14': if (checkNextByte()) setOutput(0, Wire.read()); break;
    case '\x15': if (checkNextByte()) setOutput(1, Wire.read()); break;
    case '\x20': readState = op; break;
    case '\x21': readState = op; break;
    case '\x22': readState = op; break;
    case '\x23': readState = op; break;
    case '\x26': readState = op; break;
    case '\x27': readState = op; break;
    default: printOpError(op); break;
  }
}

void receiveEvent(int howMany) {
  Serial.print("i2c Message (");
  Serial.print(howMany);
  Serial.println(" Bytes long): ");
  if ((1 <= howMany) && (2 >= howMany)) decodeMessage();
  else {
    Serial.print("Unable to handle a Message of this length! Message content: ");
    while (0 < Wire.available()) {
      char c = Wire.read();
      Serial.print((unsigned int) c);
      Serial.print(" ");
    }
    Serial.println();
  }
}

void writeInt(uint32_t x) {
  Wire.write((x >> 24) & 0xFF);
  Wire.write((x >> 16) & 0xFF);
  Wire.write((x >> 8) & 0xFF);
  Wire.write(x & 0xFF);
}

void requestEvent() {
  switch (readState) {
    case '\x12': Wire.write(readInput(0)); break;
    case '\x13': Wire.write(readInput(1)); break;
    case '\x20': writeInt(analogRead(23)); break;
    case '\x21': writeInt(analogRead(24)); break;
    case '\x22': writeInt(analogRead(25)); break;
    case '\x23': writeInt(analogRead(26)); break;
    case '\x26': writeInt(analogRead(29)); break;
    case '\x27': writeInt(analogRead(30)); break;
    default: Serial.println("Error: Unknown readState."); break;
  }
}

void setup() {
  Serial.begin(115200);
  configurePins(0, '\x00');
  configurePins(1, '\x00');
  Wire.begin(I2C_ADDR);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  Serial.println("Setup complete.");
}

void loop() {
  delay(1000);
}