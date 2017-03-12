
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Ethernet.h>
#include <LCD12864RSPI.h>
#include <stdlib.h>
#include <string.h>
#include <ArduinoJson.h>

void TryToConnectServer();

String comdata;
String data_display;
String returnValue;

#define RX_PIN_NUMBER 6
#define TX_PIN_NUMBER 7

SoftwareSerial mySerial(RX_PIN_NUMBER,TX_PIN_NUMBER);

#define uchar unsigned char//lcd12864显示
#define AR_SIZE(a)   sizeof(a)/sizeof(a[0])
uchar show0[] = {
  0xCA,0xA5,0xB6,0xB9,0xB5,0xE7,0xD7,0xD3,0xBC,0xE0,0xB2,0xE2,0xCA,0xFD,0xD6,0xB5
};        //圣豆电子监测数值

uchar show1[] = {
  0xBC,0xE0,0xB2,0xE2,0xC4,0xE0,0xCE,0xBB            //监测泥位
};
uchar show2[] = {
  0xA1,0xA1,0xC3,0xD7                               //米
};

/*#define APIKEY         "eXwNg7a3aUIlRaM4" // replace your pachube 
#define SENSORID       "80rtRbOkaCcDKNrO3U"   //light sensor id 5厂 4号池*/

#define APIKEY         "eXwNg7a3aUIlRaM4" // replace your pachube 
#define SENSORID       "s1P3WVOfAymIYmODXG"   //light sensor id test 2号池

// assign a MAC address for the ethernet controller.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
// fill in your address here:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

// fill in an available IP address on your network here,
// for manual configuration:
byte ip[] = {192, 168, 1, 96};
//byte ip[] = {192,168,0,11};
byte gateway[] = {192, 168, 168, 1};
byte subnet[] = {255, 255, 255, 0};
byte dnServer[] = {192, 168, 168, 1};

// initialize the library instance:
EthernetClient client;

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(118,126,13,66);      // numeric IP for api.wsncloud.com
char server[] = "wushui.sundaytek.com";   // name address for wsncloud API

boolean flagServerConnected = false;
boolean ResponseBegin = false;
const char * ValueUrl = "home/receiver/index";
const char * CodeUrl = "home/signal/notify";
const char * CleanStrat = "100";
const char * CleanEnd = "101";

////////////////////////////////////////////////////////////////////


unsigned long clear_last_time = 0;//鏈�鍚庝竴娆″啿娲楁椂闂�
unsigned long clear_period = 1800; //鍐叉礂闂撮殧鏃堕棿锛氬崟浣嶇
unsigned int clear_time = 90; //鍐叉礂鏃堕棿锛氬崟浣嶇
unsigned int clear_motor_pin = 4;//
unsigned int system_reset_pin = 5;

unsigned long upload_last_time = 0;//鏈�鍚庝竴娆′笂浼犳椂闂�
unsigned long upload_period = 8; //涓婁紶闂撮殧鏃堕棿
boolean flagUpload = false;
unsigned long upload_time = 0; 

//char data_display[40];
char *dest_str;

char niwei[4];
int pin_value = 0;   
//numdata鏄垎鎷嗕箣鍚庣殑鏁板瓧鏁扮粍
char numdata[6] = {0};
int mark = 0;

void setup() {
  
  Serial.begin(9600);
  mySerial.begin(9600);
  //mySerial3.begin(9600);
  pinMode(clear_motor_pin, OUTPUT);
  pinMode(system_reset_pin, OUTPUT);
  digitalWrite(system_reset_pin , LOW);
  LCDA.Initialise(); // 屏幕初始化
  delay(100);
  //LCDA.CLEAR();
  LCDA.DisplayString(0,2,show1,AR_SIZE(show1));
  //LCDA.DisplayString(2,2,(unsigned char *)niwei,4);delay(10);
  LCDA.DisplayString(2,4,show2,AR_SIZE(show2));delay(10);             

  mySerial.println("start setup ethernet");
  //Use the DHCP client IP address
  if (Ethernet.begin(mac) == 0) {
    mySerial.println("Failed to configure Ethernet using DHCP");
    // DHCP failed, so use a fixed IP address:
    Ethernet.begin(mac, ip, dnServer, gateway, subnet);
  }
  mySerial.print("IP = ");
  mySerial.println(Ethernet.localIP());
  
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    flagServerConnected = true;
    mySerial.println("server connected.");
  } else {
    TryToConnectServer();
  }
}

void loop() {
   lcd_display();
   cleaning();
   upload_time = millis();
  
  if (upload_time >= upload_last_time) {
    if ((upload_time - upload_last_time) > (upload_period * 1000)) { //10s upload
      flagUpload = true;
    }
  } else {
    if (((4294967295 - upload_last_time) + upload_time) > (upload_period * 1000)) { //10s upload
      flagUpload = true;
    }
  }
  if(flagUpload == true){
      if (client.connect(server,80)) {
           flagServerConnected = true;
           mySerial.println("Server connection is OK.");
      }else{
           TryToConnectServer();
      }

      if(flagServerConnected == true){
         Receive_String_3();
          SendDataToServer(comdata, ValueUrl); 
          upload_last_time = millis();
      }else if(flagServerConnected == false){
          mySerial.println("Data lost..");   
      }
      flagUpload = false;
  }
  delay(500);
  //free(dest_str);//涓婁紶鏈嶅姟鍣ㄦ垚鍔熷悗閲婃斁鍐呭瓨
 }


/*void Receive_String()
{
 while(Serial2.available()>0)
 {
  comdata += char(Serial2.read());
  delay(2); //Here must be delay(2)
 }
 if(comdata.length() > 0)
 {
  
  //data_display = comdata;
  dest_str = (char *)malloc(sizeof(char) * (sizeof(data_display) + 1));
  strncpy(dest_str, data_display, sizeof(data_display));
  comdata = ""; 
 }
}*/

/*void Receive_String()
{
 while(Serial.available()>0)
 {
  comdata += char(Serial.read());
  delay(2); //Here must be delay(2)
 }
 if(comdata.length() > 0)
 {
  //Serial.println(comdata);
  data_display = comdata;
  char cArr[comdata.length()]; 
  comdata.toCharArray(cArr,comdata.length());
  //Serial.println(cArr);
  dest_str = (char *)malloc(sizeof(char) * (sizeof(cArr)+1));
  strncpy(dest_str, cArr, sizeof(cArr));
  Serial.println(dest_str);
  comdata = ""; 
 }
}*/

 void Receive_String_3()
{
 Serial.begin(9600);
 char terminator = '}'; 
 if(Serial.available()>0)
 {
  if (Serial.find("{"))
   {
    comdata = Serial.readStringUntil(terminator);
    Serial.end();
    }
 }
 /*if(comdata.length() > 0)
 {
  //Serial.println(comdata);
  data_display = comdata;
  char cArr[comdata.length()]; 
  comdata.toCharArray(cArr,comdata.length());
  //Serial.println(cArr);
  dest_str = (char *)malloc(sizeof(char) * (sizeof(cArr)+1));
  strncpy(dest_str, cArr, sizeof(cArr));
  Serial.println(dest_str);
  comdata = ""; 
 }*/
}
void SendDataToServer(const String thisData ,const char * url)
{
    mySerial.print("uploading data [");
    mySerial.print(thisData);
    mySerial.println("]");
    
    // send the HTTP PUT request:
    client.print("POST /");
    client.print(url);
    client.print("?ak=");
    client.print(APIKEY);
    client.print("&id=");
    client.print(SENSORID);
   // if(strcmp(url,ValueUrl) == 0){
    client.print("&value=");
   // }else if(strcmp(url,CodeUrl) == 0){
   //   client.print("&code=");
   // }
    client.print(thisData);
    client.println(" HTTP/1.1");
    client.println("Host:wushui.sundaytek.com");
    client.println();
    delay(100);
    mySerial.println("upload end...");
    flagServerConnected = false;
    //获取服务器命令
    readPage();       
}

//鑷姩娓呮礂鍑芥暟
void cleaning()
{
  unsigned long c_time = millis();

  short a = 0;
  if (c_time >= clear_last_time) {
    if ((c_time - clear_last_time) > (clear_period * 1000)) { //30mins clean
      a = 1;
    }
  } else {
    if (((4294967295 - clear_last_time) + c_time) > (clear_period * 1000)) { //30mins clean
      a = 1;
    }
  }
  if (a > 0) {

      //TryToConnectServer();
      
      //SendDataToServer(CleanStrat, CodeUrl);
      digitalWrite(clear_motor_pin , HIGH);// 鐢垫満寮�濮嬫竻娲�
      clear_last_time = millis();//璁颁綇鐢垫満娓呮礂寮�濮嬫椂闂�      
      delay(clear_time * 1000);//鐢垫満娓呮礂鏃堕棿 1.5min
      digitalWrite(clear_motor_pin , LOW);//鐢垫満娓呮礂瀹屾垚
      delay(100);

      //TryToConnectServer();
      
      //SendDataToServer(CleanEnd, CodeUrl);
        
  }
}
//灏濊瘯閲嶆柊杩炴帴鏈嶅姟鍣�
void TryToConnectServer()
{
  int i = 0;
     for(i = 0;i < 10;i++)
     {
         client.stop();
         Ethernet.begin(mac,ip);
         mySerial.println("can`t connect server,close and try again...");
         if (client.connect(server, 80)){
              flagServerConnected = true;
              mySerial.println("server connected.");
              return;
         }
         delay(3000);
         if(i == 9)
         {  
            flagServerConnected = false;
            mySerial.println("can`t connect to server.");
            mySerial.println("The system will restart!");
      
      digitalWrite(system_reset_pin , HIGH);
      delay(50);
      digitalWrite(system_reset_pin , LOW);
          }
      }
}

void readPage(){
  mySerial.println("start read recever data");
  unsigned long read_start_time = millis();
  unsigned long read_end_time = 0;
  while(1){
    read_end_time = millis();
    if(read_end_time - read_start_time > 10000 && (4294967295 - read_start_time + read_end_time) > 10000)
    {
     mySerial.println("Read server data is fail!");
     break; 
    }
    
    if(client.available()) {
      char c = client.read();
      mySerial.print(c);
        if (c == '{')
          ResponseBegin = true;
        else if (c == '}')
        {
          returnValue += c;
          ResponseBegin = false;
          break;
        }
        if (ResponseBegin)
          returnValue += c;
          if(returnValue.length() > 100)
          {
           ResponseBegin = false;
           break; 
           }   
    }
  }
  if (returnValue.length() !=0 && (ResponseBegin == false))
  {
    mySerial.println(returnValue);
    
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(returnValue);
//    if (!root.success())
//    {
//      mySerial.println("parseObject() failed");
//      return;
//    }
    int status = root["status"];
    const char * command = root["command"][0];
    const char * height = root["height"];
    strcpy(niwei,height);
    
    mySerial.println(status);
    mySerial.println(command);
    mySerial.println(height);
    
    if (strcmp(command,"1001") == 0) {
      mySerial.println("turn on the MOTOR");   
      digitalWrite(clear_motor_pin, HIGH);
      my_delay(clear_time);
      digitalWrite(clear_motor_pin, LOW);
    } 
     returnValue = "";     
  }
}
//延时函数
void my_delay(int n)
{
    int i = 0;
    for(i = 0;i < n;i++)
    { 
      delay(1000);     
    }
}
//debug函数
/*void debug_display(void)
{
    LCDA.CLEAR();
    LCDA.DisplayString(0,0,(unsigned char *)"2.5:",AR_SIZE("2.5:"));
    LCDA.DisplayString(0,2,(unsigned char *)light1,AR_SIZE(light1));delay(10);
    LCDA.DisplayString(0,4,(unsigned char *)"2.8:",AR_SIZE("2.8:"));
    LCDA.DisplayString(0,6,(unsigned char *)light2,AR_SIZE(light2));delay(10);     
    LCDA.DisplayString(1,0,(unsigned char *)"3.0:",AR_SIZE("3.0:"));
    LCDA.DisplayString(1,2,(unsigned char *)light3,AR_SIZE(light3));delay(10);
    LCDA.DisplayString(1,4,(unsigned char *)"3.3:",AR_SIZE("3.3:"));
    LCDA.DisplayString(1,6,(unsigned char *)light4,AR_SIZE(light4));delay(10);
    LCDA.DisplayString(2,0,(unsigned char *)"3.5:",AR_SIZE("3.5:"));
    LCDA.DisplayString(2,2,(unsigned char *)light5,AR_SIZE(light5));delay(10);
    LCDA.DisplayString(2,4,(unsigned char *)"3.8:",AR_SIZE("3.8:"));
    LCDA.DisplayString(2,6,(unsigned char *)light6,AR_SIZE(light6));delay(10);
    LCDA.DisplayString(3,0,(unsigned char *)"4.0:",AR_SIZE("4.0:"));
    LCDA.DisplayString(3,2,(unsigned char *)light7,AR_SIZE(light7));delay(10);
    LCDA.DisplayString(3,4,(unsigned char *)"4.3:",AR_SIZE("4.3:"));
    LCDA.DisplayString(3,6,(unsigned char *)light8,AR_SIZE(light8));delay(10);
}*/
//LCD显示
void lcd_display(void)
{
     //pin_value = digitalRead(debug_pin);
     //if(pin_value == 0)
     //{
          LCDA.CLEAR();
          LCDA.DisplayString(0,2,show1,AR_SIZE(show1));
          LCDA.DisplayString(2,2,(unsigned char *)niwei,4);delay(10);
          LCDA.DisplayString(2,4,show2,AR_SIZE(show2));delay(10);                           
      //}else{
       //   debug_display();
     // }
}


