#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(7, 8); // CE, CSN
const byte address[6] = "00001";



void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
}



byte data_size = 16;
unsigned char dataBlock1[]    = {
    0x00, 0x00, 0x00, 0x00, // first bytes for timing settings
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x12, // last 5 bytes for UPC 80480015404
    0xBC, 0xFB, 0x94, 0x2C  
    };

unsigned char dataBlock2[]    = {
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x14, // last 5 bytes for UPC 89540333496
    0xD9, 0x05, 0x0F, 0xB8  
    };

unsigned char dataBlockN[]    = {
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0xF7, 0x31, // 1111 0111 0011 0001
    0x8C, 0xEF, 0x00, 0x00, // 1000 1100 1110 1111
    0x00, 0x00, 0x00, 0x00  // neutral tag code
    };



void loop() {
  if (radio.available()) {
    unsigned char text[16] = "";
    radio.read(&text, sizeof(text));
    Serial.println("DATA CAPUTRED");

    if (match_UPC(text, dataBlockN, data_size)){
            Serial.println("Tag: Neutral");
    }
    if (match_UPC(text, dataBlock1, data_size)){
            Serial.println("Tag: 1");
    }
    if (match_UPC(text, dataBlock2, data_size)){
            Serial.println("Tag: 2");
    }
  }
}


bool match_UPC(unsigned char *rec_data, unsigned char *static_data, byte data_len){
  for (byte i = 0; i<data_len; i++){
    if (rec_data[i] != static_data[i]){
      return false;
    }
  }
  return true;
}
