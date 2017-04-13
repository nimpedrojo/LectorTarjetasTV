#include <ESP8266WiFi.h>
#include "MFRC522.h"
#include <virtuabotixRTC.h> 

#define RST_PIN 0 
#define SS_PIN  15  
#define RELAY_PIN 16 


int inicio=0;
int ultimo=0;
int horainicio=0;
int minutoinicio=0;
int horaactual=0;
int minutoactual=0;
int tiemporestante=0;
int tiempoinicial=0;
int tiempoactual=0;


const char* ssid     = "******";
const char* password = "*********";
const long interval = 30000;
const char* host = "****.****.****";

// myRTC(clock, data, rst)
virtuabotixRTC myRTC(5, 4, 2);
MFRC522 rfid(SS_PIN, RST_PIN); 
MFRC522::MIFARE_Key key; 

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  Serial.begin(115200);
  delay(10);
  SPI.begin(); 
  rfid.PCD_Init(); 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
 
  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");  
}

void loop() { 
  myRTC.updateTime();
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  calcularTiempoPasado(client);
    
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  if ( ! rfid.PICC_ReadCardSerial())
    return;

  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }


  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(host);
  
  
  String url = "/tarjetas/";
  url +=rfid.uid.uidByte[rfid.uid.size-1];
 
  client.println(
    String("GET ") + url + " HTTP/1.1\r\n" +
    "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
               
  delay(10);
  while(client.connected()) {
    
    String line = client.readStringUntil('\r');
    String result = line.substring(1,2);
    
    if (result=="[") {
      Serial.print("Response: ");
      Serial.println(line);
      inicio=line.indexOf("tiempo");
      ultimo=line.indexOf("]");
     
      if (line.indexOf("id") >= 0 ) {
        Serial.println("Usuario detectado");
        if (line.indexOf("ON") >= 0 ) {
          horainicio=myRTC.hours;
          minutoinicio=myRTC.minutes;
          tiempoinicial=(horainicio * 60) + minutoinicio;
          tiemporestante=line.substring(inicio +9,ultimo -2).toInt();
          Serial.print("Tiempo restante");
          Serial.print(tiemporestante);
          Serial.println("");
          Serial.print("Tiempo inicio");
          Serial.print(tiempoinicial);
          digitalWrite(RELAY_PIN, HIGH); 
        }
        if (line.indexOf("OFF") >= 0 ) {
          horainicio=0;
          minutoinicio=0;
          tiemporestante=0;
          digitalWrite(RELAY_PIN, LOW); //Relay OFF
        }
       } else {
          Serial.println("False");
       }
    }
    
  }
  delay(500);
}
void calcularTiempoPasado(WiFiClient client2){
  if (digitalRead(RELAY_PIN)==HIGH){
    minutoactual=myRTC.minutes ;
    Serial.println("Tiempo actual");
    Serial.println(minutoactual);
    
    if (minutoactual - minutoinicio > tiemporestante){
      digitalWrite(RELAY_PIN, LOW);
      String url2 = "/time";
      client2.println(
        String("GET ") + url2 + " HTTP/1.1\r\n" +
      "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
       
      }
  }
}
