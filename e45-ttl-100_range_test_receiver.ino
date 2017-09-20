//#include <VirtualWire.h>
#include <Adafruit_SSD1306.h>

/*
 * Code for moving device
 */

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define LED_PIN 13

uint8_t counter = 0; 

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

  Serial.begin(9600);
}

int currentPacket = 0;
uint8_t errorRate = 0;
uint8_t expectedPacket = 0;

long lastReceive = millis();
long oledUpdate = millis();

uint32_t lastSentMillis = 0;

enum dataStates {
  IDLE,
  HEADER_RECEIVED,
  DATA_RECEIVED,
  CRC
};

uint8_t protocolState = IDLE;
uint8_t dataBytesReceived = 0;
uint32_t incomingData;

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

        int32_t diff = (micros() - incomingData) / 1000;

        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Delay: ");
        display.print(diff); 
        display.print("ms");
        display.display(); 
      }
    }
  }

  if (millis() - lastSentMillis > 500) {
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
    delay(30);
    Serial.begin(9600);
  }

  /*
  bool isError = false;

  if (Serial.available()) {
    currentPacket = Serial.read();
    lastReceive = millis();

    if (currentPacket != expectedPacket) {
      isError = true;
      errorRate++; 
    }

    expectedPacket = currentPacket + 1;

    if (!isError) {
      if (errorRate > 0) {
        errorRate--;
      }
    }
  }

  if (!isError && millis() - lastReceive > 200) {
    errorRate++;
    isError = true;
    lastReceive = millis();
  }

  if (errorRate > 100) {
    errorRate= 100;
  }
 
  if (millis() - oledUpdate > 150) {
    display.clearDisplay();
    
    display.setCursor(0, 0);
    display.print("Packet: ");
    display.print(currentPacket);

    display.setCursor(0, 10);
    display.print("Error rate: ");
    display.print(errorRate);
    
    
    display.display(); 

    oledUpdate = millis();
  }
  */
}
