/*********基于esp8266的天气时钟以及温湿度检测**********/
//它有两种显示页，初始页：当前室内温湿度以及当前时间的显示
/****************第二页：当前所在地区的天气状况（其中地区需要自己更改）*/
//配网功能：（第一次使用：需要使用手机连接名为“WeatherClock”的WIFI，每一次更换连接的WIFI都需要重新配网）
//温湿度检测：你需要另外买一个温湿度传感器，我用的是DHT20
//注意！！！：因为获取时间我用的苏宁易购的接口，所以会有几率获取不成功因为服务器访问量太大
//"Model.h"是我自己写的汉字和天气状况显示的库，都是BMP图片取模（取模工具和取模方法都包含在文件夹中）
//所用到的库：
//https://github.com/arduino-libraries/ArduinoHttpClient                     /*#include <ArduinoHttpClient.h>
//https://arduinojson.org/?utm_source=meta&utm_medium=library.properties     /*#include <ArduinoJson.h> 
//https://github.com/RobTillaart/DHT20                                       /*#include "DHT20.h"
//https://github.com/ThingPulse/esp8266-oled-ssd1306                         /*#include "SSD1306Wire.h"
//https://github.com/taichi-maker/WiFiManager                                /*#include <WiFiManager.h> 

#include "DHT20.h"
#include <ArduinoJson.h>  
#include <ESP8266WiFi.h>  //连接WIFI头文件
#include "SSD1306Wire.h"  //屏幕头文件
#include "Model.h"        //屏幕打印头文件
#include <ArduinoHttpClient.h> //Get请求
#include <WiFiManager.h> //配网

SSD1306Wire display(0x3c, 4, 5);//实例化屏幕显示(屏幕地址，SDA，SCL)
DHT20 DHT;  //实例化温湿度传感器
StaticJsonDocument<200> doc; //实例化json解析。。。不想写这个了，其实在获取天气的时候都没用到，那个方法更简单
WiFiManager wifiManager; //实例化自动配网，这个库是太极创客改写的更适合中国宝宝体质的汉化版库

//获取天气计时（每5分钟获取一次）
unsigned long previousMillis = 0;        
const long interval = 300000;  
//获取时间计时（每20秒获取一次）
unsigned long previousMillis1 = 0;       
const long interval1 = 20000; 

//定义WIFI名称以及密码//因为增加自动配网，所以注释掉代码
/*
const char* ssid = "CMCC-HYD66";
const char* password = "88888888";
*/
//定义室内温湿度变量
double Hum = 0, Tem = 0;  
double BAOJING = 50;//定义湿度超过阈值报警，此功能已经删除/***没用***/
bool key_num = false;//判断按键按下做翻页动作
//定义现在的气温，天气状况对应图片
int Now_Tem = 0; //当前天气温度变量
int Code_Num = 0;//当前天气状况代码
String Now_Time; //当前时间变量

WiFiClient wifi; //这个应该只是下面这个HttpClient的必须的传参
HttpClient client = HttpClient(wifi, "api.seniverse.com", 80);//连接服务器（WiFi，网址，端口号）
HttpClient client1 = HttpClient(wifi, "quan.suning.com", 80);

String api_key = "S7hm1A5XzDg1fv0_Z";     //你自己注册的心知天气的api的密钥
String location = "qinhuangdao";         //想要获取天气的城市（应该是小写全拼详情自己去查心知天气的开发文档）

void setup() {
  Serial.begin(9600);
  Serial.println("");
  pinMode(D7, INPUT);   //将D8引脚设置为输出模式
  pinMode(D6, INPUT);   //将D8引脚设置为输出模式
  pinMode(D4, OUTPUT);   //将D8引脚设置为输出模式
  DHT.begin();  //通过作者文档，必读初始化DHT
  display.init(); //初始化屏幕
  display.flipScreenVertically();//屏幕反转
  display.setTextAlignment(TEXT_ALIGN_LEFT);  //设置文本对齐方式
  display.setFont(ArialMT_Plain_16);          //设置字体大小
  //WIFI_init(); //启动WIFI
  wifiManager.autoConnect("WeatherClock");
  Time_Get();  //获取当前时间
  Weather_Get(); //获取当前天气
}
/*因为增加自动配网，所以注释掉代码因为增加自动配网，所以注释掉代码
//连接WIFI
void WIFI_init(){
  WiFi.mode(WIFI_STA);//设置WIFI为STA模式
  WiFi.begin(ssid, password);//连接WIFI
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");
  Serial.print("IP Address:");
  Serial.println(WiFi.localIP());
}
*/
//通过API获取天气
void Weather_Get() {
  //API
  String reqRes = "/v3/weather/now.json?key="+api_key+"&location="+location+"&language=zh-Hans&unit=c"; 
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.print("api.seniverse.com");
  //请求数据
  client.get(reqRes);
  int statusCode = client.responseStatusCode();//返回状态码
  String response = client.responseBody();//返回body
  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
  // 尝试连接服务器，如果返回200成功
  if (statusCode == 200) {
    // 使用find跳过HTTP响应头
    if (client.find("\r\n\r\n")) {
      Serial.println("Found Header End. Start Parsing.");
    }
    /******这个json解析太复杂，可以看下面获取时间的方法*******************/
    deserializeJson(doc, response);
    JsonObject results_0 = doc["results"][0];
    JsonObject results_0_now = results_0["now"];                           //没用
    const char* results_0_now_text = results_0_now["text"];                //天气状况
    const char* results_0_now_code = results_0_now["code"];                //图片编号
    const char* results_0_now_temperature = results_0_now["temperature"];  //气温
    String s = results_0_now_temperature;
    String s1 = results_0_now_code;
    Now_Tem = (s.substring(0, 3)).toInt();
    Code_Num = (s1.substring(0, 2)).toInt();
    Serial.println(Code_Num);
  } else {
    Serial.println(" connection failed!");
  }
  //断开客户端与服务器连接工作
  client.stop();
}

//通过API获取时间
void Time_Get() {
  //APIpath
  String reqRes = "/getSysTime.do";
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.print("quan.suning.com");
  //请求数据
  client1.get(reqRes);
  int statusCode = client1.responseStatusCode();//返回状态码
  String response = client1.responseBody();//返回body
  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
  // 尝试连接服务器，如果返回200成功
  if (statusCode == 200) {
    // 使用find跳过HTTP响应头
    if (client1.find("\r\n\r\n")) {
      Serial.println("Found Header End. Start Parsing.");
    }
    //返回的response是字符串，可以直接用substring截取可用数据片段
    Now_Time = (response.substring(13, 29));
  } else {
    Serial.println(" connection failed!");
  }
  //断开客户端与服务器连接工作
  client.stop();
}

void DHT_Get() {
  DHT.read();                  //通过作者文档，在读取温湿度之前，必须运行DHT.read()，通过温湿度传感器传输的字节得到当前温湿度
  Hum = DHT.getHumidity();     //调用库函数，读取湿度
  Tem = DHT.getTemperature();  //调用库函数，读取温度
}
void DHT_display() {
  display.clear();
  //当前温度显示  Ch_x以及Ch_y的定义在Model.h头文件里，因为汉字大小都是16*16的，eg：两个字就是（16*2）*16/（宽*高）
  display.drawXbm(0, 0, Ch_x*2, Ch_y, Dangqian_ch);//（横向坐标，纵向坐标，图片宽，图片高，图片字模数组）
  display.drawXbm(32, 0, Ch_x*2, Ch_y, Wendu_ch);
  display.drawXbm(64, 0, Ch_x, Ch_y, Maohao_ch);
  display.drawString(70, 0, String(Tem));  //设置显示位置以及显示内容（横向坐标，纵向坐标，string字符串）
  display.drawString(109, 0, "°C");         //设置显示位置以及显示内容39
  //当前湿度显示
  display.drawXbm(0, 18, Ch_x*2, Ch_y, Dangqian_ch);
  display.drawXbm(32, 18, Ch_2_x, Ch_y, Shidu_ch);
  display.drawXbm(64, 18, Ch_x, Ch_y, Maohao_ch);
  display.drawString(70, 18, String(Hum));  //设置显示位置以及显示内容
  display.drawString(109, 18, "RH");         //设置显示位置以及显示内容
  //当前时间显示
  display.drawXbm(0, 34, Ch_x*2, Ch_y, Dangqian_ch);
  display.drawXbm(32, 34, Ch_x*2, Ch_y, Shijian_ch);
  display.drawXbm(64, 34, Ch_x, Ch_y, Maohao_ch);
  display.drawString(0, 49, Now_Time);
  display.display();
}

void Weather_display() {
  display.clear();
  //通过返回的天气代号打印天气状况和天气对应照片
  switch(Code_Num){
    // 晴 白天0 晚上1
    case 0:display.drawXbm(0, 0, sunny_x, sunny_y, sunny);display.drawXbm(61, 17, Ch_x, Ch_y, Qing_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 1:display.drawXbm(0, 0, sunny_x, sunny_y, sunny);display.drawXbm(61, 17, Ch_x, Ch_y, Qing_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 多云 4
    case 4:display.drawXbm(0, 0, cloudy_x, cloudy_y, cloudy);display.drawXbm(61, 17, Ch_x*2, Ch_y, Duoyun_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 晴间多云 5、7
    case 5:display.drawXbm(0, 0, PartlyCloudy_x, PartlyCloudy_y, PartlyCloudy);display.drawXbm(61, 17, Ch_x*4, Ch_y, Qingjianduoyun_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 7:display.drawXbm(0, 0, PartlyCloudy_x, PartlyCloudy_y, PartlyCloudy);display.drawXbm(61, 17, Ch_x*4, Ch_y, Qingjianduoyun_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 阴间多云 6、8
    case 6:display.drawXbm(0, 0, MostlyCloudy_x, MostlyCloudy_y, MostlyCloudy);display.drawXbm(61, 17, Ch_x*4, Ch_y, Yinjianduoyun_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 8:display.drawXbm(0, 0, MostlyCloudy_x, MostlyCloudy_y, MostlyCloudy);display.drawXbm(61, 17, Ch_x*4, Ch_y, Yinjianduoyun_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    //  阴 9
    case 9:display.drawXbm(0, 0, Overcast_x, Overcast_y, Overcast);display.drawXbm(61, 17, Ch_x, Ch_y, Yin_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 阵雨 10
    case 10:display.drawXbm(0, 0, Shower_x, Shower_y, Shower);display.drawXbm(61, 17, Ch_x*2, Ch_y, Zhenyu_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 雷阵雨 11
    case 11:display.drawXbm(0, 0, Thundershower_x, Thundershower_y, Thundershower);display.drawXbm(61, 17, Ch_x*3, Ch_y, Leizhenyu_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 雷阵雨伴有冰雹 12
    case 12:display.drawXbm(0, 0, ThundershowerWithHail_x, ThundershowerWithHail_y, ThundershowerWithHail);display.drawXbm(61, 17, Ch_x*5, Ch_y, Leizhenyubingbao_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 小雨、中雨、大于、暴雨、大暴雨、特大暴雨 13、14、15、16、17、18
    case 13:display.drawXbm(0, 0, Rain_x, Rain_y, Rain);display.drawXbm(61, 17, Ch_x*2, Ch_y, Xiaoyu_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 14:display.drawXbm(0, 0, Rain_x, Rain_y, Rain);display.drawXbm(61, 17, Ch_x*2, Ch_y, Zhongyu_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 15:display.drawXbm(0, 0, Rain_x, Rain_y, Rain);display.drawXbm(61, 17, Ch_x*2, Ch_y, Dayu_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 16:display.drawXbm(0, 0, Rain_x, Rain_y, Rain);display.drawXbm(61, 17, Ch_x*2, Ch_y, Baoyu_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 17:display.drawXbm(0, 0, Rain_x, Rain_y, Rain);display.drawXbm(61, 17, Ch_x*3, Ch_y, Dabaoyu_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 18:display.drawXbm(0, 0, Rain_x, Rain_y, Rain);display.drawXbm(61, 17, Ch_x*4, Ch_y, Tedabaoyu_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 冻雨 19
    case 19:display.drawXbm(0, 0, IceRain_x, IceRain_y, IceRain);display.drawXbm(61, 17, Ch_x*2, Ch_y, Dongyu_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 雨夹雪 20
    case 20:display.drawXbm(0, 0, Sleet_x, Sleet_y, Sleet);display.drawXbm(61, 17, Ch_x*3, Ch_y, Yujiaxue_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 阵雪 21
    case 21:display.drawXbm(0, 0, SnowFlurry_x, SnowFlurry_y, SnowFlurry);display.drawXbm(61, 17, Ch_x*2, Ch_y, Zhenxue_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 小雪、中雪、大雪、暴雪 22、23、24、25
    case 22:display.drawXbm(0, 0, Snow_x, Snow_y, Snow);display.drawXbm(61, 17, Ch_x*2, Ch_y, Xiaoxue_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 23:display.drawXbm(0, 0, Snow_x, Snow_y, Snow);display.drawXbm(61, 17, Ch_x*2, Ch_y, Zhongxue_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 24:display.drawXbm(0, 0, Snow_x, Snow_y, Snow);display.drawXbm(61, 17, Ch_x*2, Ch_y, Daxue_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 25:display.drawXbm(0, 0, Snow_x, Snow_y, Snow);display.drawXbm(61, 17, Ch_x*2, Ch_y, Baoxue_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 浮尘、扬沙 26、27
    case 26:display.drawXbm(0, 0, Dust_x, Dust_y, Dust);display.drawXbm(61, 17, Ch_x*2, Ch_y, Fuchen_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 27:display.drawXbm(0, 0, Dust_x, Dust_y, Dust);display.drawXbm(61, 17, Ch_x*2, Ch_y, Yangsha_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 沙尘暴、强沙尘暴 28、29
    case 28:display.drawXbm(0, 0, Duststorm_x, Duststorm_y, Duststorm);display.drawXbm(61, 17, Ch_x*3, Ch_y, Shachenbao_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 29:display.drawXbm(0, 0, Duststorm_x, Duststorm_y, Duststorm);display.drawXbm(61, 17, Ch_x*4, Ch_y, Qiangshachenbao_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 雾 30
    case 30:display.drawXbm(0, 0, Foggy_x, Foggy_y, Foggy);display.drawXbm(61, 17, Ch_x, Ch_y, Wu_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 霾 31
    case 31:display.drawXbm(0, 0, Haze_x, Haze_y, Haze);display.drawXbm(61, 17, Ch_x, Ch_y, Mai_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 风、大风 32、33
    case 32:display.drawXbm(0, 0, Windy_x, Windy_y, Windy);display.drawXbm(61, 17, Ch_x, Ch_y, Feng_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 33:display.drawXbm(0, 0, Windy_x, Windy_y, Windy);display.drawXbm(61, 17, Ch_x*2, Ch_y, Dafeng_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 飓风、热带风暴 34、35
    case 34:display.drawXbm(0, 0, Hurricane_x, Hurricane_y, Hurricane);display.drawXbm(61, 17, Ch_x*2, Ch_y, Jufeng_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    case 35:display.drawXbm(0, 0, Hurricane_x, Hurricane_y, Hurricane);display.drawXbm(61, 17, Ch_x*4, Ch_y, Redaifengbao_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 龙卷风 36
    case 36:display.drawXbm(0, 0, Tornado_x, Tornado_y, Tornado);display.drawXbm(61, 17, Ch_x*3, Ch_y, Longjuanfeng_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 冷 37
    case 37:display.drawXbm(0, 0, Cold_x, Cold_y, Cold);display.drawXbm(61, 17, Ch_x, Ch_y, Leng_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 热 38
    case 38:display.drawXbm(0, 0, Hot_x, Hot_y, Hot);display.drawXbm(61, 17, Ch_x, Ch_y, Re_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
    // 未知 99
    case 99:display.drawXbm(0, 0, Unknown_x, Unknown_y, Unknown);display.drawXbm(61, 17, Ch_x*2, Ch_y, Weizhi_ch);display.drawXbm(61, 34, Ch_x*4, Ch_y, Dangqianqiwen_ch);display.drawXbm(124, 34, Ch_x, Ch_y, Maohao_ch);display.drawString(61, 49, String(Now_Tem));display.drawString(94, 49, "°C");break;
  }
  //显示当前城市
  display.drawXbm(61, 0, Ch_x*3, Ch_y, Qinhuangdao_ch);
  display.display();
}

void loop() {
  //判断按键是否被按下，bool类型的key_num取反，if判断转换显示页面
  while (digitalRead(D7) == LOW) { 
    delay(5);//软件防抖
    while (digitalRead(D7) == LOW) {
      key_num=!key_num;
    }
  }
  switch (key_num) {
    case false:
      DHT_Get();
      DHT_display();
      break;
    case true: Weather_display(); break;
  }
  //定义计时器，每过60秒重新获取当前天气
  //通过获取当前millis和上一个millis的差值，如果这个差值大于我们设定的时间间隔（当前程序中是 1000 毫秒），
  //则程序可以进入if语句中执行相应的程序，并且更新millis；
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    Weather_Get();
  }
  //时间刷新倒计时（每20秒刷新一次）
  if (currentMillis - previousMillis1 >= interval1) {
    previousMillis1 = currentMillis;
    Time_Get();
  }
}