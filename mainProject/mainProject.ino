//=====================LIBRUARY==========================
#include "ThingsBoard.h"
#include <ESP8266WiFi.h>
/*=====================================================*/
              //Thingsboard server
#define TOKEN                 "2QnCGHdV7cSiNrEOXfEL"
#define THINGSBOARD_SERVER    "demo.thingsboard.io"
#define BAUDRATE_DEBUG      115200

#define ssid                  "PhuocThinhPC"
#define pass                  "123412345"

//======================INITIALIZE=======================

// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;

//========================PIN=============================
const int pump = D6;           
const int humidity_pin = A0;
const int button = D3;          //Chọn chế độ                => khởi động không thành công nếu kéo MỨC THẤP
const int buzzer = D2;
const int IRprox = D1;
//======================VARIABLES=========================
int humid_v = 0;
int phantramao;
static int phantramthat;
static int mode_v = 0;              //%2 == 0 => Self Operating; %2 == 1 => Server Mode


bool pumpStatus = false;             
bool previousState = false;
bool subscribed = false;            //Check subscribe Topic
//======================FUNCTION==========================

  
void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network

  WiFi.begin(ssid,pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to AP");
}
void reconnect() {
  // Loop until we're reconnected
  status = WiFi.status();
  if ( status != WL_CONNECTED) {
    WiFi.begin(ssid,pass);    
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  }
}
void getMoisture(void)
{
  humid_v = analogRead(humidity_pin);
  phantramao = map(humid_v,0,1024,0,100);
  phantramthat = 100 - phantramao;
}
void sendMoisture(void)
{
  tb.sendTelemetryInt("moisture", phantramthat);
}

RPC_Response processSwitchChange(const RPC_Data &data)
{
  Serial.println("Received the set switch method");
  char params[10];
  serializeJson(data, params);
  Serial.println(params);
  String _params = params;
  if (_params == "true") {
    Serial.println("True");
    pumpStatus = true;
    digitalWrite(pump, HIGH);
    return RPC_Response("getValue_1", "true");
  }
  else  if(_params == "false") {
    Serial.println("False");
    pumpStatus = false;
    digitalWrite(pump, LOW);
    digitalWrite(buzzer, LOW);
    return RPC_Response("getValue_1", "false");
  }
  else return RPC_Response("getValue_1","0");
}
const size_t callbacks_size = 1;
RPC_Callback callbacks[callbacks_size] = {
  { "getValue_1",         processSwitchChange }   // enter the name of your switch variable inside the string
};
void setup()
{
  Serial.begin(BAUDRATE_DEBUG);
  
  pinMode(humidity_pin, INPUT);
  pinMode(button, INPUT_PULLUP);
  pinMode(pump, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(IRprox, INPUT);
  //digitalWrite(pump, LOW);
  
  WiFi.begin(ssid,pass);
  InitWiFi();
  //tb.connect(THINGSBOARD_SERVER, TOKEN);
  if (!tb.connected()) {
    // Connect to the ThingsBoard
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);
    if (tb.connect(THINGSBOARD_SERVER, TOKEN)) 
      Serial.println("[DONE]");
    else
    {
      Serial.println("[FAIL] : retry in 5s");
      delay(5000);
    }
  }
}

void loop()
{  
  delay(1000);
  if (digitalRead(button) == LOW)
  {
    if (mode_v ==0)
    {
      Serial.println("[MODE 1] : Server Mode");
      mode_v++;
    }
    else
    {
      Serial.println("[MODE 2] : Self-operating");
      mode_v--;
    }
  }
  
  switch(mode_v)
  {
    case 0:
    if(subscribed == true)
    {
      if(tb.RPC_Unsubscribe()) Serial.println("Unsubscribed THINGSBOARD");
      subscribed = false;
    }
    getMoisture();
    Serial.print("Humidity: ");
    Serial.println(phantramthat);
    if(phantramthat <= 6)
    {
      digitalWrite(pump, HIGH);   //Relay ON
      digitalWrite(buzzer, LOW);
      if( digitalRead(IRprox) ==HIGH)  //NO water out
      {
        digitalWrite(pump, LOW);  //Relay OFF
        Serial.println("Pump Station is not operating");
        digitalWrite(buzzer, HIGH);
      }
      //if(IRprox == 0) => Relay OFF
    }
    else 
    {
      digitalWrite(pump, LOW);
      Serial.print("Humidity: ");
      Serial.println(phantramthat);
    }
    
    break;
    case 1:
      if (WiFi.status() != WL_CONNECTED) 
      {
        reconnect();
      }
      getMoisture();
      sendMoisture();
      if (!subscribed) {
        Serial.println("Subscribing for RPC...");
        if (!tb.RPC_Subscribe(callbacks, callbacks_size)) {
          Serial.println("Failed to subscribe for RPC");
          return;
        }
        Serial.println("Subscribe done");
        subscribed = true;
      }
      break;
  }
  
  tb.loop();
}
