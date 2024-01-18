#include <Arduino.h>
int key=0;  
void setup() {
  // 设置波特率
  Serial.begin(9600);
  Serial.println("");
  // 设置引脚模式
  pinMode(D4, OUTPUT);//设置D4为输出模式
  pinMode(D3,INPUT);     //0脚做输入——KEY引脚
}
void loop() {
  key=digitalRead(D3); //将0脚检测得电平赋值给key;
  if(key==LOW) digitalWrite(D4,LOW);    //如果检测为低电平，就点亮LED灯
  else  digitalWrite(D4,HIGH);   //否则就点亮LED灯
  
  if (digitalRead(D4) == HIGH) Serial.println("LED关闭");//高电平，输出LED关闭
  else  Serial.println("LED打开");//低电平，输出LED打开
}

