#include <LiquidCrystal_I2C.h>

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <Wire.h>
#include <BH1750.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <EEPROM.h>

#include <stdio.h>
#include <stdlib.h>

LiquidCrystal_I2C lcd(0x3F,16,2);

OneWire oneWire(D5);
DallasTemperature sensors(&oneWire);
BH1750 lightMeter(0x23);

WiFiUDP Udp;

char *ssid = "ESP_AP";  
char *password = "1qazxsw2";

byte rxBuffer[11];

byte txBuffer[11];
byte Slave_Addr=0x01;
byte Func=2;
byte Data_Count=6;
word Temp;
word Lux;
word Hum;
byte CRC_Hi;
byte CRC_Lo;
word CRC;
word tmp;
char *state;
String Terminal_Command;

unsigned int localPort = 4101;
IPAddress ipMaster(192, 168, 4, 1);
IPAddress ipSlave(192, 168, 4, 3);
IPAddress Subnet(255, 255, 255, 0);

void setup() 
{
  pinMode(A0,INPUT);
  Wire.begin();
  sensors.begin();
  lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE);  
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA); 
  WiFi.config(ipSlave, ipMaster, Subnet);
  Udp.begin(localPort);
  lcd.init();
  lcd.backlight();
  lcd.home();
  lcd.setCursor(0,0);
  lcd.print(ssid);
  lcd.setCursor(0,1);
  lcd.print(password);
  delay(1000);
  lcd.setCursor(0,0);
  lcd.print("             ");
  lcd.setCursor(0,1);
  lcd.print("             ");
}

void loop() 
{ 
  sensors.requestTemperatures(); 
  Temp = sensors.getTempCByIndex(0);
  Lux = lightMeter.readLightLevel();
  Hum =analogRead(A0);  
  byte Data_Only[6]={highByte(Temp),lowByte(Temp),highByte(Lux),lowByte(Lux),highByte(Hum),lowByte(Hum)};
  CRC = Get_CRC(Data_Only,6);
  CRC_Hi=highByte(CRC);
  CRC_Lo=lowByte(CRC);  
  byte txBuffer[11]={Slave_Addr, Func, Data_Count, Data_Only[0], Data_Only[1],Data_Only[2],Data_Only[3],Data_Only[4],Data_Only[5], CRC_Hi, CRC_Lo};  
  Serial.println("");
  Udp.beginPacket(ipMaster,localPort); 
  for(int i=0;i<11;i++)  
      Udp.write(txBuffer[i]);
    Udp.endPacket(); 
  delay(500);
  lcd.setCursor(0,0);
  lcd.print("        ");
  lcd.setCursor(0,0);
  lcd.print("Sent    ");
  Serial.print("Send: ");
  for(int i=0;i<11;i++)
    if(i<10) 
    {
      Serial.print(txBuffer[i],HEX);
      Serial.print(" ");
    }
    else 
      Serial.println(txBuffer[i],HEX); 
  delay(10);  
  int packetSize = Udp.parsePacket();
  if (packetSize) 
  {
    int len = Udp.read(rxBuffer, 11);
    lcd.setCursor(0,0);
    lcd.print("        ");
    lcd.setCursor(0,0);
    lcd.print("Received");
      Serial.print("Receive(IP/Port/Size/Data): ");
      Serial.print(Udp.remoteIP());
      Serial.print(" / ");
      Serial.print(Udp.remotePort());
      Serial.print(" / ");
      Serial.print(packetSize);
      Serial.print(" / ");
      for(int i=0;i<11;i++)
        if (i<10) 
        {
          Serial.print(rxBuffer[i],HEX);
          Serial.print(" ");
        }
        else 
          Serial.println(rxBuffer[i],HEX);  
  }
  if((rxBuffer[0]=txBuffer[0])&&(rxBuffer[1]=0x03))
  for(int i=0;i<3;i++)
            {
              tmp=(rxBuffer[2*i+3]<<8)+rxBuffer[2*i+4];
                switch(tmp)
                {
                  case 0x0000: 
                  {
                    state="OK";
                    break;
                  }
                  case 0xFF00: 
                  {
                    state="Hi";
                    break;
                  }
                  case 0x00FF: 
                  {
                    state="Lo";
                    break;
                  }
                }
                switch(i)
                {
                  case 0:
                  {
                    lcd.setCursor(9,0);
                    lcd.print("Tmp:");
                    lcd.setCursor(13,0);
                    lcd.print(state);
                    break;
                  }
                  case 1:
                  {
                    lcd.setCursor(0,1);
                    lcd.print("Lux:");
                    lcd.setCursor(4,1);
                    lcd.print(state);
                    break;
                  }
                  case 2:
                  {
                    lcd.setCursor(7,1);
                    lcd.print("Hum:");
                    lcd.setCursor(11,1);
                    lcd.print(state);
                    break;
                  }
                }
            }
  delay(500);
}
word Get_CRC(byte *buf, word len)
{
  word crc = 0xFFFF; 
  for (word pos = 0; pos < len; pos++) 
  {
    crc ^= (word)buf[pos];            
    for (word i = 8; i != 0; i--) 
    {    
      if ((crc & 0x0001) != 0) 
      {      
        crc >>= 1;                    
        crc ^= 0xA001;
      }
      else                          
        crc >>= 1;                    
    }
  }
  return crc; 
}


