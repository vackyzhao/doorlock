/*
 arduino nano v3 舵机 蜂鸣器 继电器 按钮 门磁 模式切换开关
概述：以atmg328p为核心芯片基于Arduino nano v3 开发的nfc门锁
主要模块：Arduino nano v3、RC522 nfc模块、倾斜传感器、干簧管传感器、舵机、继电器。（可选wifi/蓝牙/gsm模块）
功能概述：通过校刷nfc园卡通过舵机实现自动开门。门磁传感器（干簧管）判断门是否关闭并自动锁门。安装在把手上的倾斜传感器判断内部是否开门并通过舵机打开门锁。
*/
#include <SPI.h>
#include <MFRC522.h>
#include <string.h>
#include "stdlib.h"
#include "stdio.h"
#include <Servo.h>
#include <EEPROM.h>

//D9,D10,D11,D12,D13为SPI使用引脚
#define SS_PIN 10//D10 SPI SS引脚
#define RST_PIN 9//D9 SPI RST引脚

#define SERVO_PIN 5//D5 舵机输出引脚（PWM）*D3、D5、D6、D9、D10、D11为PWM
#define BEEP_PIN 0//D0 蜂鸣器输出引脚
#define RELAY_PIN 2//D2 继电器输出引脚
#define KEY_PIN 0//A0 按键输入引脚（模拟）
#define DOOR_PIN 1//A1 门磁输入引脚（模拟）
#define MODE_PIN 2//A2 模式开关输入引脚（模拟）


Servo myservo;
MFRC522 rfid(SS_PIN, RST_PIN); //实例化类


byte nuidPICC[4];// 初始化数组用于存储读取到的NUID 
String ID[10];// 初始化字符串数组用于存储UID 
String NAME[10];
int MODE=0;//模式 0为自动锁门 1为手动锁门
int DOORCLOSE;//0为没关门，1为已关门
int DOORLOCK;//0为没上锁，1为已上锁
 
void setup() { 
  Serial.begin(9600);
  pinMode(RELAY_PIN,OUTPUT);
  pinMode(BEEP_PIN,OUTPUT);
  digitalWrite(RELAY_PIN,LOW);
  digitalWrite(BEEP_PIN,LOW);
  //NFC模块初始化
  SPI.begin(); // 初始化SPI总线
  rfid.PCD_Init(); // 初始化 MFRC522 
  //舵机初始化
  myservo.attach(SERVO_PIN);
  //将EEPROM上的地址0用于储存当前的锁闭状态
  DOORLOCK=EEPROM.read(0);
  
  //所有人的校园卡（实体/手机）UID
  ID[0]="9764162122";
  ID[1]="177155161122";
  ID[2]="";
  ID[3]="";
  ID[4]="";
  ID[5]="";
  ID[6]="";
  ID[7]="";
  ID[8]="";
  ID[9]="";
}
 
void loop() {
  //首先判断当前模式
  if(analogRead(MODE_PIN)>=1000)
  {
    //进入自动模式
    AUTO();
    }
    else
    {
      //进入手动模式
      MANUAL();
      }
 if(rfid.PICC_IsNewCardPresent()&&rfid.PICC_ReadCardSerial())
 {
  int i=identify();
  if(i==1)
  {
    UNLOCK();
    digitalWrite(BEEP_PIN,HIGH);
    delay(200);
    digitalWrite(BEEP_PIN,LOW);
    delay(200);
    digitalWrite(BEEP_PIN,HIGH);
    delay(200);
    digitalWrite(BEEP_PIN,LOW);
    }
    else
    {
      digitalWrite(BEEP_PIN,HIGH);
      delay(500);
      digitalWrite(BEEP_PIN,LOW);
      }
  }
  delay(100);
}

int identify() 
{
  String a;
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  for (byte i = 0; i < 4; i++) 
  {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }   
  
  for (byte i = 0; i < rfid.uid.size; i++) 
  {
    a+=rfid.uid.uidByte[i];
    }
    for(int i=0;i<=10;i++)
    {
      if(i==10)
       {
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        return 0;
       }
      if(a==ID[i])
      {
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        return 1;
       }
     }
}

void AUTO()
{
  //判断是否关门并锁门
  if((DOORCLOSE==0)&&(analogRead(DOOR_PIN)>=1000))
  {
    LOCK();
    }
    //读取当前门关闭状态
   if(analogRead(DOOR_PIN)>=1000)
   {
    DOORCLOSE=1;
    }
    else
    {
      DOORCLOSE=0;
      }
  //按下按钮开/关门
  if(analogRead(KEY_PIN)>=1000)
  {
    if(DOORLOCK==1)
    {
      UNLOCK;
      }
      else
      {
        LOCK();
        }
    }
    //判断是否用钥匙开门
  if((DOORCLOSE==1)&&(analogRead(DOOR_PIN)<1000)&&(DOORLOCK==1))
  {
    DOORLOCK=0;
    EEPROM.write(0,DOORLOCK);
    }
      return ;
  }
  
void MANUAL()
{
  //按下按钮开/关门
  if(DOORLOCK==1)
    {
      UNLOCK;
      }
      else
      {
       LOCK();
        }
  //判断是否用钥匙开门
  if((DOORCLOSE==1)&&(analogRead(DOOR_PIN)<1000)&&(DOORLOCK==1))
  {
    DOORLOCK=0;
    EEPROM.write(0,DOORLOCK);
    }
        return ;
  }
  
void LOCK()
{
  digitalWrite(RELAY_PIN,HIGH);
  delay(500);
  myservo.write(360);
  delay(500);
  digitalWrite(RELAY_PIN,LOW);
  DOORLOCK=1;
   EEPROM.write(0,DOORLOCK);
  return ;
  }
  
void UNLOCK()
{
  digitalWrite(RELAY_PIN,HIGH);
  delay(500);
  myservo.write(0);
  delay(500);
  digitalWrite(RELAY_PIN,LOW);
  DOORLOCK=0;
  EEPROM.write(0,DOORLOCK);
  return ;
  }
