#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DFRobot_BMP280.h>
#include <SparkFun_I2C_Mux_Arduino_Library.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>


#define SEA_LEVEL_PRESSURE 1013.25f  //海平面压强
#define i2c_Address 0x3C             //oled屏幕的I2C地址
#define SCREEN_WIDTH 128             //oled屏幕宽度
#define SCREEN_HEIGHT 64             //oled屏幕高度



typedef DFRobot_BMP280_IIC BMP;
Adafruit_AHTX0 aht;  //aht实例
BMP bmp(&Wire, BMP::eSdoLow);
QWIICMUX myMux;
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_MPU6050 mpu;
WiFiServer server(80);
WiFiClient client;
float temp = 0;                               //温度平均值
float humi = 0;                               //湿度平均值
char str[6] = { 0 };                          //字符串缓存用于OLED显示
const char *ssid = "lijun";                   //WIFI名称
const char *password = "libingkunqiaojun19";  //密码
sensors_event_t aht_humi, aht_temp, a, g, tempmpu;
uint32_t press;
float alti;


void ahtget();
void bmpget();
void mpuset();
void mpuget();
void send2client();
void oleddisplay();


void setup() {
  /*初始化I2C总线*/
  myMux.begin();
  /* 初始化UART0（板载USB-UART）*/
  Serial.begin(115200);
  delay(250);  // 等待oled屏幕启动
  myMux.setPort(1);
  display.begin(i2c_Address, true);
  display.clearDisplay();
  display.display();
  delay(1000);
  display.clearDisplay();  //清空显示缓存
  Serial.println("Oled initialized");


  /* AHT10初始化*/
  myMux.setPort(0);
  while (!aht.begin()) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("AHT10 or AHT20 found");

  /* WiFi初始化，连接AP*/
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected!");
  server.begin();

  /*BMP280初始化*/
  myMux.setPort(2);
  bmp.reset();
  while (bmp.begin() != BMP::eStatusOK) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("bmp begin success");
  delay(100);


  /*MPU6050初始化*/
  myMux.setPort(3);
  while (!mpu.begin()) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("MPU6050 Found!");
  delay(100);
  mpuset();
}

void loop() {
  ahtget();
  bmpget();
  mpuget();
  send2client();
  oleddisplay();
}


void ahtget() {
  /* 读取温湿度*/
  myMux.setPort(0);
  aht.getEvent(&aht_humi, &aht_temp);
  temp = aht_temp.temperature;
  humi = aht_humi.relative_humidity;
  delay(10);
}

void bmpget() {
  /*获取气压和海拔数据*/
  myMux.setPort(2);
  press = bmp.getPressure();
  alti = bmp.calAltitude(SEA_LEVEL_PRESSURE, press);
  delay(20);
}

void mpuset() {
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
    case MPU6050_RANGE_2_G:
      Serial.println("+-2G");
      break;
    case MPU6050_RANGE_4_G:
      Serial.println("+-4G");
      break;
    case MPU6050_RANGE_8_G:
      Serial.println("+-8G");
      break;
    case MPU6050_RANGE_16_G:
      Serial.println("+-16G");
      break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
    case MPU6050_RANGE_250_DEG:
      Serial.println("+- 250 deg/s");
      break;
    case MPU6050_RANGE_500_DEG:
      Serial.println("+- 500 deg/s");
      break;
    case MPU6050_RANGE_1000_DEG:
      Serial.println("+- 1000 deg/s");
      break;
    case MPU6050_RANGE_2000_DEG:
      Serial.println("+- 2000 deg/s");
      break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_94_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
    case MPU6050_BAND_260_HZ:
      Serial.println("260 Hz");
      break;
    case MPU6050_BAND_184_HZ:
      Serial.println("184 Hz");
      break;
    case MPU6050_BAND_94_HZ:
      Serial.println("94 Hz");
      break;
    case MPU6050_BAND_44_HZ:
      Serial.println("44 Hz");
      break;
    case MPU6050_BAND_21_HZ:
      Serial.println("21 Hz");
      break;
    case MPU6050_BAND_10_HZ:
      Serial.println("10 Hz");
      break;
    case MPU6050_BAND_5_HZ:
      Serial.println("5 Hz");
      break;
  }
}

void mpuget() {
  /*获取陀螺仪数据*/
  myMux.setPort(3);
  mpu.getEvent(&a, &g, &tempmpu);
}

void send2client() {
  client = server.available();
  client.flush();

  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n ";
  s += "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><meta http-equiv=\"Refresh\" content=\"5\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, minimum-scale=0.5, maximum-scale=2.0, user-scalable=yes\" /><meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\"><title>Web Server Test System</title><style>  h2,h1{line-height:1%;}body {  margin: 0;padding: 0;width: 340px;  background: LightCyan}.button { width: 100px; height: 100px;  text-align: center; font-weight: 100; color: darkcyan;  margin: 0 40px 40px 0;  position: relative; border: 6px solid darkcyan; background: LightCyan;  font-size: 20px;  border-radius: 50%;}.top1 { width: 360px; height: 45px; color: #FFF;  border: 1px solid darkcyan; background: darkcyan; font-size: 25px;  border-radius: 0%;}</style></head><body><a href=\"./pin?light1=1\"><button class=\"button top1\">&#x6E29;&#x6E7F;&#x5EA6;&#x663E;&#x793A;</button></a><center style=\"left: 20px; position: relative;\"></br><a href=\"./pin?light1=1\"><button type=\"button\" class=\"button\" value=\"temp\">温度<span style=\"color: red;font-size: 25px;\">";

  s += (temp);
  s += "°C</span></button></a><a href=\"./pin?light1=0\"><button type=\"button\" class=\"button\" value=\"humi\">湿度<span style=\"color: green;font-size: 25px;\">";

  s += (humi);
  s += "%</span></button>";
  s += "<button type=\"button\" class=\"button\" value=\"press\">气压<span style=\"color: green;font-size: 25px;\">";

  s += (press / 1000);
  s += "kPa</span></button>";
  s += "<button type=\"button\" class=\"button\" value=\"alti\">高度<span style=\"color: green;font-size: 25px;\">";

  s += (alti);
  s += "m</span></button><p>加速度xyz分别为：";
  s += a.acceleration.x;
  s += "\n";
  s += a.acceleration.y;
  s += "\n";
  s += a.acceleration.z;
  s += "\n";
  s += "</p><p>陀螺xyz分别为：";
  s += g.gyro.x;
  s += "\n";
  s += g.gyro.y;
  s += "\n";
  s += g.gyro.z;
  s += "\n";
  s += "</p></a></br></center></body></html>";

  client.print(s);
}

void oleddisplay() {
  myMux.setPort(1);
  String oleds = "Temperature:";
  oleds += (temp);
  oleds += ";C\nHumidity:";
  oleds += (humi);
  oleds += "%;\nPressure:";
  oleds += (press / 1000);
  oleds += "kPa;\nAltitude:";
  oleds += (alti);
  oleds += "m;\nAcceleration xyz:";
  oleds += ((float)a.acceleration.x);
  oleds += " ";
  oleds += a.acceleration.y;
  oleds += " ";
  oleds += a.acceleration.z;
  oleds += ";\nGyro xyz:";
  oleds += g.gyro.x;
  oleds += " ";
  oleds += g.gyro.y;
  oleds += " ";
  oleds += g.gyro.z;
  oleds += ";\n";
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println(oleds);
  display.display();
  delay(500);
  display.clearDisplay();
}
