#include <Adafruit_SSD1306.h>

#define SEND_DELAY 250
#define BUFFER_LEN 21
#define BUFFER_DELAY 200
#define OLED_UPDATE_DELAY 200
#define UART_SPEED 57600

/*
 * Code for moving device
 */

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define LED_PIN 13

void setup() {
  /*
   * Initialize OLED display
   */
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.display();

  pinMode(LED_PIN, OUTPUT);

  delay(1000);

  Serial.begin(UART_SPEED);
}

enum dataStates {
  IDLE,
  HEADER_RECEIVED,
  DATA_RECEIVED,
  CRC
};

uint8_t protocolState = IDLE;
uint8_t dataBytesReceived = 0;
uint32_t incomingData;

uint8_t packetLostBuffer[BUFFER_LEN] = {0};
uint8_t bufferPointer = 0;
uint8_t bufferLocked = 0;
uint32_t lastBufferReset = 0;

void storeBufferSuccess() {
  if (bufferLocked == 0) {
    packetLostBuffer[bufferPointer] = 1;
    bufferLocked = 1;
  }
}

void storeBufferFailure() {
  if (bufferLocked == 0) {
    packetLostBuffer[bufferPointer] = 0;
    bufferLocked = 1;
  }
}

void resetBuffer() {
  bufferPointer++;

  if (bufferPointer == BUFFER_LEN) {
    bufferPointer = 0;
  }

  packetLostBuffer[bufferPointer] = 2;
  bufferLocked = 0;
  
  lastBufferReset = millis();
}

uint8_t getSuccessCount() {
  uint8_t out = 0;

  for (uint8_t i = 0; i < BUFFER_LEN; i++) {
    if (packetLostBuffer[i] == 1) {
      out++;
    }
  }

  if (out > BUFFER_LEN - 1) {
    out = BUFFER_LEN - 1;
  }

  return out;
}

uint8_t getSuccessRate() {
  return (100 / (BUFFER_LEN - 1)) * getSuccessCount();
}

void checkBuffer() {

  if (millis() - lastBufferReset > BUFFER_DELAY) {
    storeBufferFailure();
  }
  
}

uint32_t lastOledUpdate = 0;
uint32_t lastSentMillis = 0;
uint32_t lastPacketDelay = 0;

void loop() {

  if (Serial.available()) {

    uint8_t data = Serial.read();

    if (protocolState == IDLE && data == 0xfe) {

      protocolState = HEADER_RECEIVED;
      dataBytesReceived = 0;
      incomingData = 0;
    } else if (protocolState == HEADER_RECEIVED) {

      if (dataBytesReceived == 0) {
        incomingData = data;
      } else {
        //Shift data to the left
        incomingData = (uint32_t) incomingData << 8;
        incomingData = (uint32_t) ((uint32_t) incomingData) + data;
      }

      dataBytesReceived++;

      if (dataBytesReceived == 4) {
        protocolState = IDLE;

        lastPacketDelay = (micros() - incomingData) / 1000;
        
        storeBufferSuccess();
      }
    }
  }

  checkBuffer();

  if (millis() - lastOledUpdate > OLED_UPDATE_DELAY) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Delay: ");
    display.print(lastPacketDelay); 
    display.print("ms");

    display.setCursor(0, 12);
    display.print("Success: ");
    display.print(getSuccessRate()); 
    display.print("%");
    
    display.display(); 
    lastOledUpdate = millis();
  }

  if (millis() - lastSentMillis > SEND_DELAY) {
    Serial.write(0xff);

    uint32_t toSend = micros();

    byte buf[4];
    buf[0] = toSend & 255;
    buf[1] = (toSend >> 8) & 255;
    buf[2] = (toSend >> 16) & 255;
    buf[3] = (toSend >> 24) & 255;
  
    Serial.write(buf, sizeof(buf));
    Serial.end();
    lastSentMillis = millis();
    delay(10);

    resetBuffer();
    
    Serial.begin(UART_SPEED);
  }
}
