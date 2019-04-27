#include <SPI.h>
#include <MFRC522.h>


#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(7, 8); // CE, CSN
const byte address[6] = "00001";
int incomingByte = 0;   


#define RST_PIN         5           // Configurable, see typical pin layout above
#define SS_PIN          53          // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;

#include "MPU9250.h"

// an MPU9250 object with the MPU-9250 sensor on I2C bus 0 with address 0x68
MPU9250 IMU(Wire,0x68);
int MFCR_status;
int IMU_status;



void setup() {
  // put your setup code here, to run once:
    Serial.begin(9600); // Initialize serial communications with the PC
    while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card

    radio.begin();
    radio.openWritingPipe(address);
    radio.setPALevel(RF24_PA_MIN);
    radio.stopListening();
    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    Serial.println(F("Scan a MIFARE Classic PICC to demonstrate read and write."));
    Serial.print(F("Using key (for A and B):"));
    //dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
    Serial.println();

    Serial.println(F("BEWARE: Data will be written to the PICC, in sector #1"));
    pinMode(12,OUTPUT);
    digitalWrite(12,HIGH);
    delay(500);
    digitalWrite(12,LOW);


  // start communication with IMU 
  IMU_status = IMU.begin();
  if (IMU_status < 0) {
    Serial.println("IMU initialization unsuccessful");
    Serial.println("Check IMU wiring or try cycling power");
    Serial.print("Status: ");
    Serial.println(IMU_status);
    //while(1) {}
  }
  // setting the accelerometer full scale range to +/-8G 
  IMU.setAccelRange(MPU9250::ACCEL_RANGE_8G);
  // setting the gyroscope full scale range to +/-500 deg/s
  IMU.setGyroRange(MPU9250::GYRO_RANGE_500DPS);
  // setting DLPF bandwidth to 20 Hz
  IMU.setDlpfBandwidth(MPU9250::DLPF_BANDWIDTH_20HZ);
  // setting SRD to 19 for a 50 Hz update rate
  IMU.setSrd(19);
}

#define IDLE_STATE 1
#define SCAN_TAG 2
#define ID_TAG 3
#define ACCEL 4
#define POURING 5
#define BROADCAST 6

int state = 1;
long pour_total = 0;
long pour_initial_time = 0;

unsigned char out_data[16];
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


bool tag_activated(){
  if ( ! mfrc522.PICC_IsNewCardPresent())
        return false;

    // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
        return false;

  return true; 
}


bool match_UPC(unsigned char *rec_data, unsigned char *static_data, byte data_len){
  for (byte i = 0; i<data_len; i++){
    if (rec_data[i] != static_data[i]){
      return false;
    }
  }
  return true;
}


int read_tag_in(unsigned char* out_data, byte od_size){
  
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("This sample only works with MIFARE Classic cards."));
        return 1;
    }
    
    // In this sample we use the second sector,
    // that is: sector #1, covering block #4 up to and including block #7
    //byte sector         = 1;
    byte blockAddr      = 5;
    /*
    unsigned char dataBlockN[]    = {
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0xF7, 0x31, // 1111 0111 0011 0001
        0x8C, 0xEF, 0x00, 0x00, // 1000 1100 1110 1111
        0x00, 0x00, 0x00, 0x00  // neutral tag code
    };
*/
    byte trailerBlock   = 7;
    MFRC522::StatusCode MFRC_status;
    unsigned char tag_buffer[18];
    byte tb_size = sizeof(tag_buffer);

    
    MFRC_status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (MFRC_status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(MFRC_status));
        return 1;
    }


    MFRC_status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, tag_buffer, &tb_size);
    if (MFRC_status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(MFRC_status));
        return 0;
 
    }

    else {
        Serial.print("Updated bottle present "); Serial.print(" "); 
        for (byte i = 11; i < 16; i++) {
            Serial.print(tag_buffer[i] < 0x10 ? " 0" : " ");
            Serial.print(tag_buffer[i], HEX);
        }
        for (byte i = 0; i<od_size; i++){
          out_data[i] = tag_buffer[i];
        }
        
        Serial.println();
        Serial.println("ready to pour");
        digitalWrite(12,HIGH);
        delay(500);
        digitalWrite(12,LOW);
    }


    
    Serial.println();
        // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
}

bool read_accel(){
    // read the sensor
    IMU.readSensor();
  
    double y=-1*IMU.getAccelY_mss();
    double z=IMU.getAccelZ_mss();
    double x=IMU.getAccelX_mss();
 /*   Serial.print("Acceleration Due to Gravity in X, Y and Z direction\t");
    Serial.print(x,6);
    Serial.print("   ");
    Serial.print(y,6);
    Serial.print("   ");
 */   Serial.println(z,6);
    double phi=atan2(sqrt(x*x+y*y),z)*180/3.1415;
    double theta=atan2(y,x)*180/3.1415;
  /*  Serial.print("Angles phi and theta of Gravity vector     ");
    Serial.print(phi,6);
    Serial.print("    ");
    Serial.println(theta,6);
  */ 
  if (theta<-0.87266 && theta>-2.44346 && phi<1.39626 && phi>0.34906){
      digitalWrite(12,HIGH);
      return true;
      }
    else{
      digitalWrite(12,LOW);
      return false;
      }

      
    //delay(20);

}

void loop () {
switch(state){

  case IDLE_STATE:
      {
        bool read_tag = tag_activated();
        if (read_tag){
          state = SCAN_TAG;
        }
      }
    break;

  case SCAN_TAG:
    {
   // Serial.println("SCANIN");
        int test = read_tag_in(out_data, data_size);
        state = ID_TAG;
    }
    break;

  case ID_TAG:
    {
      if (match_UPC(out_data, dataBlockN, data_size)){
              Serial.println("Tag: Neutral");
              state = IDLE_STATE;
      }
      else{
        state = ACCEL;
        pour_total = 0;
      }
    }
    break;

  case ACCEL:
    {
      bool critical_angle;
      bool read_tag;
      read_tag = tag_activated();
      critical_angle = read_accel();
      if (critical_angle){
        pour_initial_time = millis();
        state = POURING;
      }
      else if(read_tag){
        state = BROADCAST;
      }
    }

  case POURING:
    {
      bool critical_angle;
      critical_angle = read_accel();
      if(!critical_angle){
        state = ACCEL;
        pour_total += (millis() - pour_initial_time);
        }
    }

}

}
