#include <SD_MMC.h>
#include <sd_defines.h>
#include<WiFi.h> 

//RFID Solenoid Lock

#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <SoftwareSerial.h>

//3v = 3v3
//SDA = D2
//SCK = D18
//MOSI = D23
//MISO = D19
//GND= GND
//RST= D5
String openPassword = "1234"; 
String closePassword = "4321"; 
String inputPassword;
#define ROW_NUM     4 // four rows
#define COLUMN_NUM  3 // three columns
#define SS_PIN 2 
#define RST_PIN 5

//set wifi 
const char* ssid = "your ssid";
const char* password = "password";
String apiKey = "31WCBN2E2E0FM1PH";
const char*server="api.thingspeak.com";
//unsigned int dataFieldOne = 1;
//unsigned int dataFieldTwe = 2;
//unsigned int dataFieldThree = 3;
WiFiClient client;

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte pin_rows[ROW_NUM]      = {14, 27, 26, 25}; // GIOP19, GIOP18, GIOP5, GIOP17 connect to the row pins
byte pin_column[COLUMN_NUM] = {33, 32, 21}; 

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
byte keyTagUID[4] = {0xFF, 0xFF, 0xFF, 0xFF};

SoftwareSerial sim900A(16, 17); 

int relayPin(){
  return 13;
}

//int buzzerPin(){
  //return 22;
//}

String UserNamePassword[2][3] = {
  {"Isuru", "415c181a", "1234"},
  {"Bandara", "", "5678"},
};

bool loopBreak = true;

void connectInternet(){ 
    delay(100);
    WiFi.begin(ssid,password);
    Serial.print("Connecting to : ");
    Serial.print(ssid);
    while(WiFi.status() != WL_CONNECTED){
      delay(1000);
      Serial.print(".");
    }
    Serial.println("Connected...");
    Serial.print("IP Address : ");
    Serial.print(WiFi.localIP()); 
}

void updateToDataIoT(String name, String doorOpenStatus, String authStatus){
    if(client.connect(server,80)){
    String postStr = apiKey;
    postStr +="&field1=";
    postStr += name;

    postStr += "&field2=";
    postStr += doorOpenStatus;

    postStr += "&field3=";
    postStr += authStatus;

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\r\n\r\n");
    client.print(postStr);

    Serial.println("Sending data into IoT platform......");
    }
}

void checkconnectionGsmModule(){
  Serial.println ("========> checkconnectionGsmModule()");
  Serial.println (Serial.available());
  Serial.println (Serial.read());
  SendMessage();
     
}

void SendMessage() {
  Serial.println ("========> sendSMS()");
  Serial.println ("Sending Message, please wait…");
  sim900A.println("AT+CMGF=1"); // Text Mode initialization
  delay(1000);
  Serial.println ("Set SMS Number");
  sim900A.println("AT+CMGS=\"+94778045641\"\r"); // Receiver’s Mobile Number
  delay(1000);
  Serial.println ("Set SMS Content");
  sim900A.println("Granted"); // Message content
  delay(100);
  Serial.println ("Done");
  sim900A.println((char)26);
  delay(1000);
  Serial.println ("Message sent successfully");

}


void connectRFID(){
  Serial.println("==============> connectRFID()");
  while (!Serial);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
	SPI.begin();			// Init SPI bus
	mfrc522.PCD_Init();		// Init MFRC522
	delay(4);				// Optional delay. Some board do need more time after init to be ready, see Readme
	mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
	Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

}


void getRfidUUId(){
  Serial.println("==============> getRfidUUId()");
	// Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
	if ( ! mfrc522.PICC_IsNewCardPresent()) {
		return;
	}
	// Select one of the cards
	if ( ! mfrc522.PICC_ReadCardSerial()) {
		return;
	}
	// Dump debug info about the card; PICC_HaltA() is automatically called
	mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
}


void rfidRead(){
  Serial.println("==============> rfidRead()");
 // Check if a card is present
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // Get the UID of the card
    String cardUID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      cardUID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
      cardUID += String(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println("Card UID: " + cardUID);   // print card UID
    
    // Check if the card UID matches the authorized UID
    if(!cardUID.isEmpty()){
      bool isGranted = false;
      for(int i = 0; i < 2; i++){
        Serial.println(UserNamePassword[i][1]);
         if (UserNamePassword[i][1] == cardUID) {
            Serial.println("Access granted. Opening the door...");
            digitalWrite(relayPin(), LOW);
            updateToDataIoT(UserNamePassword[i][0], "OPEN", "ACCESS_GRANTED");
            isGranted = true;
            loopBreak = false;
          } 
      }
      if(!isGranted){
          Serial.println("Granted");
          //digitalWrite(buzzerPin(), LOW);
          updateToDataIoT("UNKNOWN_USER", "CLOSED", "ACCESS_DENIED");
          SendMessage();
          loopBreak = false;
      }
    }
   

    // Halt PICC communication to prepare for a new card
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}

void keyPad(){
  loopBreak = true;
   Serial.println("==============> keyPad()");
  char key = keypad.getKey();
  String key1 = String(key)  ;
  
  if (key) {
    Serial.println("Pressed key");
    Serial.println(key1);
    if(key1 == "#"){
      inputPassword = "";
      Serial.println("Enter password open the door:");
      while(1){
        String inputKey =  String(keypad.getKey());
        inputPassword += inputKey;
        if(!inputKey.isEmpty()){
          Serial.println("input keys");
          Serial.println(inputPassword);

          Serial.println("inputPassword length");
          Serial.println(inputPassword.length());

          Serial.println("openPassword length");
          Serial.println(openPassword.length());

          bool isGranted = false;
          bool isLengthEqual = false;
          for(int i = 0; i < 2; i++ ){
              if(inputPassword.length() == UserNamePassword[i][2].length()){
                isLengthEqual = true;
                if(UserNamePassword[i][2].equals(inputPassword)){
                  Serial.println("Match password");
                  digitalWrite(relayPin(), LOW);
                  updateToDataIoT(UserNamePassword[i][0], "OPEN", "ACCESS_GRANTED");
                  inputPassword = "";
                  delay(8000);
                  isGranted = true;
                  break;
                }
              }
          }
          if(!isGranted && isLengthEqual){
              Serial.println("Granted");
              inputPassword = "";
              //digitalWrite(buzzerPin(), LOW);
              SendMessage();
              updateToDataIoT("UNKNOWN_USER", "CLOSED", "ACCESS_DENIED");
              delay(2000);
              break;
          }

          if(isGranted && isLengthEqual){
            break;
          }
          inputKey = "";
        }
      }
    }else if(key1 == "*"){
      connectRFID();
      while(loopBreak){
         rfidRead();
         delay(1000);
      }
    }
  }
} 

void setup() 
{
  Serial.begin(9600);   // Initiate a serial communication
  sim900A.begin(9600);     // Initialize the SIM900A module
  connectRFID();
  getRfidUUId();
  connectInternet();
  pinMode(relayPin(), OUTPUT);
  //pinMode(buzzerPin(), OUTPUT);

  digitalWrite(relayPin(), HIGH);
 // digitalWrite(buzzerPin(), HIGH);

  sim900A.println("AT");                       // Check if the module is responding
  delay(1000);
  sim900A.println("AT+CMGF=1");                // Set SMS mode to text
  delay(1000);
  sim900A.println("AT+CNMI=1,2,0,0,0");        // Enable receiving of new SMS messages
  delay(1000);


}
void loop() 
{
    digitalWrite(relayPin(), HIGH);
    //digitalWrite(buzzerPin(), HIGH);
   

    keyPad();
    delay(1000);
}
