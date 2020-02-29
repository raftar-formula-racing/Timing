#include <SoftwareSerial.h>          //1) for arduinos on both sides
SoftwareSerial ArduinoUno(9,8);
#include <SD.h>
#include<SPI.h>

long unsigned int time1=791;
const int trigPin = 6;
const int echoPin = 7;
long duration, duration1;
int distance=1600,r=0;
int count=0, lap=0;
unsigned long time0=791;

void setup() {
  Serial.begin(9600);
  ArduinoUno.begin(4800);  
  pinMode(trigPin, OUTPUT); 
  pinMode(echoPin, INPUT); 
}

void loop() {

  time0=millis();

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance= duration*0.034/2;
  Serial.print("distance :");
  Serial.println(distance);
  
  delay(10);

  if(distance<500){
    ArduinoUno.println(1);
    Serial.println(1);
    delay(2000);
    ArduinoUno.println(0);
    Serial.println(0);
  }else{
    r = 0;
    ArduinoUno.println(0);
    Serial.println(0);
  }

}

#include <SoftwareSerial.h>       // 2) for nodemcu on one side
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClient.h> 
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <FirebaseArduino.h>

#define NTP_OFFSET   5 * 30 * 60      // In seconds
#define NTP_INTERVAL 60 * 60    // In miliseconds
#define NTP_ADDRESS  "europe.pool.ntp.org"

char time1_str[40],time2_str[40];
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

SoftwareSerial NodeMCU(4,0);
int round_no = 0,lap = 1,round_backup = 0;

#define FIREBASE_HOST "sector-time.firebaseio.com"
#define FIREBASE_AUTH "aIZ9wO5kYJQjlbklaETeqq4717VysrunvGAxp7BZ"
#define WIFI_SSID "adams"
#define WIFI_PASSWORD "adamsnet"

const char *host = "http://worldclockapi.com/api/json/est/now";

void setup() {
  
  timeClient.begin();
  Serial.begin(9600);
  NodeMCU.begin(4800);
  pinMode(4,INPUT);
  pinMode(0,OUTPUT);  

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

char sect1_time1_path[100],sect2_time2_path[100],sect2_result_path[100],laptime_result_path[100],sect1_prev_time2_path[100],sect1_prev_time1_path[100];
char result[5],result2[5],result3[5];
float val = 0,sect2_time,laptime,time1,time2,time3;
String timestamp3 = "0";
char timestamp_1[19] = "0",timestamp_2[19] = "0",timestamp_3[19] = "0";

void loop() {
  Serial.println("na");
  while(NodeMCU.available()>0){
    Serial.println(val);
    val = NodeMCU.parseFloat();
    if(val == 1){
      HTTPClient http;
      http.begin(host); 
      int httpCode = http.GET();            
      String payload = http.getString();
      if(httpCode == 200)
      {
        const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
        DynamicJsonBuffer jsonBuffer(capacity);
        JsonObject& root = jsonBuffer.parseObject(payload);
        if (!root.success()) {
          return;
        }
        timestamp3 = root["currentFileTime"].as<char*>();
      }
      round_no = Firebase.getInt("round");
      if(round_no != 0){
        round_backup = round_no;
      }
      if(round_no == 0){
        round_no = round_backup;
      }
      itoa(round_no, result, 10);
      itoa(lap, result2, 10);
      
      sprintf(sect1_time1_path,"%s%s%s%s%s%s","round_",result,"/sector1","/lap_",result2,"/time_1");
      Firebase.setString(sect1_time1_path,timestamp3);
      if(lap != 1) {
        itoa(lap-1, result3, 10);
        sprintf(laptime_result_path,"%s%s%s%s%s%s","result/round_",result,"/","lap_",result3,"/laptime");
        sprintf(sect1_prev_time1_path,"%s%s%s%s%s%s","round_",result,"/sector1/","lap_",result3,"/time_1");
        Firebase.getString(sect1_prev_time1_path).toCharArray(timestamp_1, 19);
        sprintf(sect1_prev_time2_path,"%s%s%s%s%s%s","round_",result,"/sector1/","lap_",result3,"/time_2");
        Firebase.getString(sect1_prev_time2_path).toCharArray(timestamp_2, 19);
        timestamp3.toCharArray(timestamp_3, 19);
        time1 = chararraytofloat(timestamp_1)/10000;
        time2 = chararraytofloat(timestamp_2)/10000;
        time3 = chararraytofloat(timestamp_3)/10000;
        sect2_time = time3 - time2;
        laptime = time3 - time1;
        sprintf(laptime_result_path,"%s%s%s%s%s%s","result/round_",result,"/","lap_",result3,"/laptime");
        Firebase.setFloat(laptime_result_path,laptime);
        sprintf(sect2_result_path,"%s%s%s%s%s%s","result/round_",result,"/","lap_",result3,"/sect2");
        Firebase.setFloat(sect2_result_path,sect2_time);


      }
      lap++;
      //sprintf(sect2_result_path,"%s%s%s%s%s%s","result/round_",result,"/","lap_",result2,"/sector2");

    }
  }
}

float chararraytofloat(char timestamp[19]) {
  float a = 0;
  int i;
  char ch;
  for(i = 0; i < 9; i++){
    ch = timestamp[6+i];
    a += int(ch-48)*power10(8-i);
  }
  return a;
}

float power10(int a){
  int i;
  float num = 1;
  for(i = 0; i < a; i++){
    num *= 10.0;
  }
  return num;
}

#include <SoftwareSerial.h>         3) for nodemcu on other side
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClient.h> 
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <FirebaseArduino.h>

#define NTP_OFFSET   5 * 30 * 60      // In seconds
#define NTP_INTERVAL 60 * 60    // In miliseconds
#define NTP_ADDRESS  "europe.pool.ntp.org"

char time1_str[40],time2_str[40],laptime_str[40];
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

SoftwareSerial NodeMCU(4,0);
int round_no = 0,lap = 1,round_backup = 0,check = 0;

#define FIREBASE_HOST "sector-time.firebaseio.com"
#define FIREBASE_AUTH "aIZ9wO5kYJQjlbklaETeqq4717VysrunvGAxp7BZ"
#define WIFI_SSID "karthi"
#define WIFI_PASSWORD "abcd123l"

const char *host = "http://worldclockapi.com/api/json/est/now";

void setup() {
  timeClient.begin();
  Serial.begin(9600);
  NodeMCU.begin(4800);
  pinMode(4,INPUT);
  pinMode(0,OUTPUT);  

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

Char sect1_time1_path[100],sect1_time2_path[100],sect1_result_path[100],sect2_result_path[100],laptime_result_path[100],sect_prev_path[100];
float time1,time2,sect1_time;
char result[5],result2[5];
float val = 0,time_sec = 0;
String timestamp2 = "0";
char timestamp_1[19] = "0",timestamp_2[19] = "0",prev_timestamp_1[19] = "0";

void loop() {
  
  while(NodeMCU.available()>0){
    val = NodeMCU.parseFloat();
    if(lap == 1 && check == 0 || check == 1){
      check++;
    }
    if(val == 1 && check == 2){
      HTTPClient http;
      http.begin(host); 
      int httpCode = http.GET();            
      String payload = http.getString();
      if(httpCode == 200)
      {
        const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
        DynamicJsonBuffer jsonBuffer(capacity);
        JsonObject& root = jsonBuffer.parseObject(payload);
        if (!root.success()) {
          return;
        }
        timestamp2 = root["currentFileTime"].as<char*>();
      }
      round_no = Firebase.getInt("round");
      if(round_no != 0){
        round_backup = round_no;
      }
      if(round_no == 0){
        round_no = round_backup;
      }
      Serial.println(round_no);
      itoa(round_no, result, 10);
      itoa(lap, result2, 10);
      sprintf(sect1_time2_path,"%s%s%s%s%s%s","round_",result,"/sector1/","lap_",result2,"/time_2");
      Firebase.setString(sect1_time2_path,timestamp2);
      sprintf(sect1_result_path,"%s%s%s%s%s%s","result/round_",result,"/","lap_",result2,"/sect1");
      sprintf(sect1_time1_path,"%s%s%s%s%s%s","round_",result,"/sector1/","lap_",result2,"/time_1");
      Firebase.getString(sect1_time1_path).toCharArray(timestamp_1, 19);
      timestamp2.toCharArray(timestamp_2, 19);
      time1 = chararraytofloat(timestamp_1)/10000;     
      time2 = chararraytofloat(timestamp_2)/10000;   
      sect1_time = time2 - time1;   
      Firebase.setFloat(sect1_result_path,sect1_time);
      lap++;
    }
  }
}

float chararraytofloat(char timestamp[19]) {
  float a = 0;
  int i;
  char ch;
  for(i = 0; i < 9; i++){
    ch = timestamp[6+i];
    a += int(ch-48)*power10(8-i);
  }
  return a;
}

float power10(int a){
  int i;
  float num = 1;
  for(i = 0; i < a; i++){
    num *= 10.0;
  }
  return num;
}
