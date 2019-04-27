#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(7, 8); // CE, CSN
const byte address[6] = "00001";
int incomingByte = 0;   
void setup() {
  Serial.begin(9600);
 // radio.begin();
 // radio.openWritingPipe(address);
 // radio.openWritingPipe(0xF0F0F0F0E1LL);
 // radio.setPALevel(RF24_PA_MIN);
  //radio.setChannel(0x76);
  //radio.enableDynamicPayloads();
 // radio.stopListening();
  //radio.powerUp();
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
}

void loop() {
  //const char text[] = "Incoming message from DEER.";
  //radio.write(&text, sizeof(text));
  //delay(1000);
  /*if (Serial.available() > 0) {
        String str = Serial.readString();
        char charBuf[str.length()+1];
        str.toCharArray(charBuf, str.length()+1);
        radio.write(&charBuf, sizeof(charBuf));
                // read the incoming byte:
        }
        */
   const char text[] = "DEER";
   radio.write(&text, sizeof(text));
   Serial.print("Sent: ");
   Serial.println(text);
   delay(1000);
}
