#include <Servo.h> 
#define BLINKER_WIFI
#include <Blinker.h>
 
char auth[] = "cc0407cbfb51";//密钥
char ssid[] = "HUAWEI-CR1LZ2";
char pswd[] = "52580000";
 
// 新建组件对象按键
BlinkerButton Button1("btn-max");   //位置1 按钮 数据键名
BlinkerButton Button2("btn-min");   //位置2 按钮 数据键名

 
int servo_max = 180;//舵机最大值
int servo_min = 0;//舵机最小值
Servo myservo;
  
void button1_callback(const String & state) {    //位置1 按钮
    BLINKER_LOG("get button state: ", servo_max);
    myservo.write(servo_max);
    delay(2000);//延时函数2000ms
}
 void button2_callback(const String & state) {    //位置2 按钮
    BLINKER_LOG("get button state: ", servo_min);
    myservo.write(servo_min);
    delay(2000);//延时函数2000ms
} 

void setup() {
    Serial.begin(115200);//波特率    
    Blinker.begin(auth, ssid, pswd);//链接WIFI匹配软件密钥
    Button1.attach(button1_callback);
    Button2.attach(button2_callback);
    myservo.attach(D2);//控制引脚
    
}
 
void loop() {
  Blinker.run();
 }