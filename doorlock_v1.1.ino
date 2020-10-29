/*
   doorlock_v1.1
  arduino nano v3 舵机 蜂鸣器 继电器 按钮 门关闭传感器 锁定传感器 模式切换开关
  概述：以atmg328p为核心芯片基于Arduino nano v3 开发的nfc门锁
  主要模块：Arduino nano v3、RC522 nfc模块、倾斜传感器、干簧管传感器、舵机、继电器。（可选wifi/蓝牙/gsm模块）
  功能概述：通过校刷nfc园卡通过舵机实现自动开门。 门关闭传感器 锁定传感器判断门是否关闭并自动锁门。安装在把手上的倾斜传感器判断内部是否开门并通过舵机打开门锁。
*/
// 需要调用SPI（NFC模块），MFRC522（NFC，模块），SERVO（舵机）库
#include <SPI.h>
#include <MFRC522.h>
#include <string.h>
#include "stdlib.h"
#include "stdio.h"
#include <Servo.h>

//SPI 引脚重定义
//D9,D10,D11,D12,D13为SPI使用引脚
#define SS_PIN 10//D10 SPI SS引脚
#define RST_PIN 9//D9 SPI RST引脚

//输出IO引脚重定义（D）
#define SERVO_PIN 5//D5 舵机输出引脚（PWM）*D3、D5、D6、D9、D10、D11为PWM
#define BEEP_PIN 2//D2 蜂鸣器输出引脚
#define RELAY_PIN 3//D3 继电器输出引脚

//输入IO引脚重定义（A）
#define MODE_PIN 0//A0 模式开关输入引脚（模拟）
#define KEY_PIN 1//A1 按键输入引脚（模拟）
#define DOOR_PIN 2//A2 门关闭传感器输入引脚（模拟）
#define LOCK_PIN 3//A3 门锁传感器输入引脚（模拟）


//舵机开关角度重定义
#define LOCKANGLE 180 //关门时角度
#define UNLOCKANGLE 0//开门时角度



Servo myservo;//舵机
MFRC522 rfid(SS_PIN, RST_PIN); //实例化类


byte nuidPICC[4];// 初始化数组用于存储读取到的NUID
String ID[10];// 初始化字符串数组用于存储UID
int MODE = 0; //模式 0为自动锁门 1为手动锁门

void setup() {
  Serial.begin(9600);
  //继电器、蜂鸣器初始化并设置为低电平（关闭）状态
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BEEP_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BEEP_PIN, LOW);
  //NFC模块初始化
  SPI.begin(); // 初始化SPI总线
  rfid.PCD_Init(); // 初始化 MFRC522
  //舵机初始化
  myservo.attach(SERVO_PIN);
  //读取当前门关闭与锁闭状态
  READDOOR();
  READLOCK();

  //所有人的校园卡（实体/手机）UID
  ID[0] = "9764162122";
  ID[1] = "177155161122";
  ID[2] = "209103164122";
  ID[3] = "1602114938";
  ID[4] = "";
  ID[5] = "";
  ID[6] = "";
  ID[7] = "";
  ID[8] = "";
  ID[9] = "";
}

void loop() {
  //判断当前模式（自动/手动）并 自动关门或通过按钮解锁/锁定
  if (analogRead(MODE_PIN) >= 1000)
  {
    //进入自动模式
    AUTO();
  }
  else
  {
    //进入手动模式
    MANUAL();
  }
  //刷卡并进行鉴权
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) //识别到卡并且卡可读时
  {
    int i = identify();
    if (i == 1) //识别成功
    {
      UNLOCK();//开锁
      TRUE_BEEP();//蜂鸣器成功提醒

      //自动打开门后等待五秒防止误判
      delay(5000);

    }
    else//识别失败
    {
      FALSE_BEEP();//蜂鸣器失败提醒
    }
  }
  delay(100);//每个循环等100ms
}

int identify() //鉴权函数
{
  String a;//读取到的UID字符串

  //读卡
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  for (byte i = 0; i < 4; i++)
  {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }

  for (byte i = 0; i < rfid.uid.size; i++)
  {
    a += rfid.uid.uidByte[i];
  }

  //比对
  for (int i = 0; i <= 10; i++)
  {
    if (i == 10) //识别失败
    {
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return 0;
    }
    if (a == ID[i]) //识别成功
    {
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return 1;
    }
  }
}

void AUTO()
{
  Serial.println("AUTO");
  //判断是否关门（门关了没上锁）并锁门
  if ((READDOOR() == 1) && (READLOCK() == 0))
  {
    delay(5000);
  }
  if ((READDOOR() == 1) && (READLOCK() == 0))
  {
    LOCK();
  }



  //门锁定按下按钮开门
  if ((analogRead(KEY_PIN) >= 1000) && (READLOCK() == 1))
  {
    UNLOCK();
    delay(5000);
  }
  return ;
}

void MANUAL()
{
  Serial.println("MANUAL");
  //按下按钮开/关门
  if ((analogRead(KEY_PIN) >= 1000) && (READLOCK() == 1))
  {
    UNLOCK();
    delay(5000);
  }
  if ((analogRead(KEY_PIN) >= 1000) && (READLOCK() == 0))
  {
    LOCK();
    delay(5000);
  }
  return ;
}

//锁门模块
void LOCK()
{
  while (READLOCK() == 0)
  {
    digitalWrite(RELAY_PIN, HIGH); //舵机通电锁定
    delay(500);//等待继电器动作


    myservo.write(LOCKANGLE);//转到锁定角度



    delay(500);//等待舵机动作
    digitalWrite(RELAY_PIN, LOW); //舵机断电释放
    READLOCK();//读取当前锁定状态
  }

  return ;
}

//开门模块
void UNLOCK()
{
  while (READLOCK() == 1)
  {
    digitalWrite(RELAY_PIN, HIGH); //舵机通电锁定
    delay(500);//等待继电器动作

    myservo.write(UNLOCKANGLE);//转到解锁角度



    delay(500);//等待舵机动作
    digitalWrite(RELAY_PIN, LOW); //舵机断电释放
    READLOCK();//读取当前锁定状态
  }

  return ;
}

//读取门关闭状态
int READDOOR()
{
  if (analogRead(DOOR_PIN) >= 1000)
  {
    Serial.println("CLOSE");
    return 1;
  }
  else
  {
    Serial.println("OPEN");
    return 0;
  }
  return ;
}

//读取门锁定状态
int READLOCK()
{
  if (analogRead(LOCK_PIN) >= 1000)
  {
    Serial.println("LOCK");
    return 1;
  }
  else
  {
    Serial.println("UNLOCK");
    return 0;
  }
}

//蜂鸣器成功提示
//短声两下
void TRUE_BEEP()
{
  digitalWrite(BEEP_PIN, HIGH);
  delay(200);
  digitalWrite(BEEP_PIN, LOW);
  delay(200);
  digitalWrite(BEEP_PIN, HIGH);
  delay(200);
  digitalWrite(BEEP_PIN, LOW);
  return ;
}

//蜂鸣器失败提示
//长声一下
void FALSE_BEEP()
{
  digitalWrite(BEEP_PIN, HIGH);
  delay(600);
  digitalWrite(BEEP_PIN, LOW);
  return ;
}
