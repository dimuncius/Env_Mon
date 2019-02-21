#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <EEPROM.h>

WiFiUDP Udp1;
WiFiUDP Udp2;

int packetSize;
byte txAddr;
byte rxBuffer[11];
byte txBuffer[11];
byte *UART_rxBuffer;

word CRC;
word rxCRC;

String Terminal_Command;
int UDP_Num;

unsigned int localPort[4]= {4101,4102,4103,4104};

char *ssid = "ESP_AP"; 
char *password = "1qazxsw2";

void setup() 
{ 
  EEPROM.begin(24);
  Serial.begin(115200);
  for(int i=0;i<8;i++) if((EEPROM.read(i)>=33)&(EEPROM.read(i)<127)) ssid[i]=EEPROM.read(i);
  for(int i=0;i<8;i++) if((EEPROM.read(i+8)>=33)&(EEPROM.read(i+8)<127)) ssid[i]=EEPROM.read(i+8);
  for(int i=0;i<4;i++) if(EEPROM.read(17+2*i)!=0) localPort[i]=(EEPROM.read(17+2*i)<<8)+EEPROM.read(16+2*i);
  WiFi.softAP(ssid, password);
  Udp1.begin(localPort[0]); Udp2.begin(localPort[1]);
  Serial.println("  "); 
  Serial.print("SSID = "); 
  Serial.println(ssid);
  Serial.print("PASS = "); 
  Serial.println(password);
  Serial.println("Localports:");
  for  ( int i=0;i<4;i++) Serial.println(localPort[i]);
  txBuffer[0]=0xFF;
}

void loop() 
{
 {
  int cnt=0;

  txBuffer[0]=0x01;
  txBuffer[1]=0x03;
  txBuffer[2]=0x006;
  for(int i=3;i<11;i+=2) txBuffer[i]=0xFF;
  for(int i=4;i<11;i+=2) txBuffer[i]=0x00;
}  
    packetSize= Udp1.parsePacket();
    if (packetSize) 
      {
        int len = Udp1.read(rxBuffer, 11);
        if(txBuffer[0]=rxBuffer[0])
        {
        Udp1.beginPacket(Udp1.remoteIP(),Udp1.remotePort());         
          for(int j=0;j<11;j++)
            {Udp1.write(txBuffer[j]);}
           Udp1.endPacket();
        }
          else
            {
        Udp1.beginPacket(Udp1.remoteIP(),Udp1.remotePort());         
          for(int j=0;j<11;j++)
            {Udp1.write(rxBuffer[j]);}
           Udp1.endPacket();
        }
        delay(10);
        for(int j=0;j<11;j++)
            {Serial.println(rxBuffer[j]);}
        
        byte Data_Only[6]={rxBuffer[3], rxBuffer[4], rxBuffer[5],rxBuffer[6], rxBuffer[7], rxBuffer[8]};
        rxCRC = (rxBuffer[10]<<8)+rxBuffer[11];
        CRC = Get_CRC(Data_Only,6);     
          for(int j=0;j<11;j++) Serial.write(rxBuffer[j]);   
          
      }
      packetSize= Udp2.parsePacket();
    if (packetSize) 
      {
        int len = Udp2.read(rxBuffer, 11);
        if(txBuffer[0]=rxBuffer[0])
        {
        Udp1.beginPacket(Udp2.remoteIP(),Udp2.remotePort());         
          for(int j=0;j<11;j++)
            {Udp2.write(txBuffer[j]);}
           Udp2.endPacket();
        }
          else
            {
        Udp1.beginPacket(Udp2.remoteIP(),Udp2.remotePort());         
          for(int j=0;j<11;j++)
            {Udp2.write(rxBuffer[j]);}
           Udp2.endPacket();
        }
        delay(10);
		}
        for(int j=0;j<11;j++)
            {Serial.println(rxBuffer[j]);}
        
        byte Data_Only[6]={rxBuffer[3], rxBuffer[4], rxBuffer[5],rxBuffer[6], rxBuffer[7], rxBuffer[8]};
        rxCRC = (rxBuffer[10]<<8)+rxBuffer[11];
        CRC = Get_CRC(Data_Only,6);     
          for(int j=0;j<11;j++) Serial.write(rxBuffer[j]);   
          
      }
  Command_Parser();

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

void Command_Parser()
{
  if(Serial.available()) 
    Terminal_Command=Serial.readString();
  if(Terminal_Command.substring(0,8)=="SET SSID")
  {
     if(Terminal_Command.substring(9).length()<8)
     {
        Terminal_Command.substring(9).toCharArray(ssid,Terminal_Command.length()-9);
        Serial.print("SSID = ");
        Serial.println(ssid);
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        for(int i=0;i<8;i++)
        if(i<Terminal_Command.substring(9).length()-1)
          EEPROM.write(i,ssid[i]);
        else EEPROM.write(i,0);
        EEPROM.commit();
      }
      else 
        Send_Error(0x00);
      Terminal_Command="PARSED";      
   }
   if(Terminal_Command.substring(0,8)=="SET PASS")
   {
      Terminal_Command.substring(9).toCharArray(password,Terminal_Command.length()-9);
      if((Terminal_Command.substring(9).length()>=8)&&(Terminal_Command.substring(9).length()<8))
      {
        Serial.print("Master's Pass = ");
        Serial.println(password);
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        for(int i=0;i<8;i++)
        if(i<Terminal_Command.substring(9).length()-1)
          EEPROM.write(i+8,password[i]);
        else EEPROM.write(i+8,0);
        EEPROM.commit();
      } 
      else
        Send_Error(0x01);
      Terminal_Command="PARSED";
   }
    if(Terminal_Command.substring(0,13)=="SET LOCALPORT")
   {
      if(Terminal_Command.substring(14,15).toInt()<4)
      {   
        UDP_Num=Terminal_Command.substring(14,15).toInt();
        if(Terminal_Command.substring(16).toInt()<=9999)
        {
          localPort[UDP_Num]=Terminal_Command.substring(16).toInt();
          EEPROM.write(16+UDP_Num*2,lowByte(localPort[UDP_Num]));
          EEPROM.write(17+UDP_Num*2,highByte(localPort[UDP_Num]));
          EEPROM.commit();
        }
        else
          Send_Error(0x02);
      }
      else
        Send_Error(0x03);
      Terminal_Command="PARSED";     
   }
   if(Terminal_Command=="CLEAR EEPROM")
      {
        for(int i=0; i<24;i++)
          EEPROM.write(i,0);
        EEPROM.end();
        Terminal_Command="PARSED";
      }

}

void Send_Error(int error_num)
{
  byte Error_Data[6]={0x00,0x00,0x00,0x00,0x00,error_num};
  word Error_CRC=Get_CRC(Error_Data,6);
  byte Error_Buffer[11]={0xFF,4,6,Error_Data[0],Error_Data[1],Error_Data[2],Error_Data[3],Error_Data[4],Error_Data[5],highByte(Error_CRC),lowByte(Error_CRC)};
  for(int i=0;i<11;i++)
    Serial.write(Error_Buffer[i]);
}




