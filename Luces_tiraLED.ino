#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <StreamString.h>
#include <Adafruit_NeoPixel.h>
 
#define MyApiKey "e8777e2c-1132-4c0e-b01c-f8db37d13e56" // TODO: Change to your sinric API Key. Your API Key is displayed on sinric.com dashboard
#define MySSID "Fluir" // TODO: Change to your Wifi network SSID
#define MyWifiPassword "fluir_01" // TODO: Change to your Wifi network password

#define SERVER_URL "iot.sinric.com"
#define SERVER_PORT 80 

#define HEARTBEAT_INTERVAL 300000 // 5 Minutes 

#define PIN        1 //LED GPIO

#define NUMPIXELS 16 // Popular NeoPixel ring size
#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient client;

uint64_t heartbeatTimestamp = 0;
bool isConnected = false;

void setPowerStateOnServer(String deviceId, String value);
void setTargetTemperatureOnServer(String deviceId, String value, String scale);
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
int h,s,b;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  WiFiMulti.addAP(MySSID, MyWifiPassword);
  Serial.println();
  Serial.print("Connecting to Wifi: ");
  Serial.println(MySSID);  
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  
  // Waiting for Wifi connect
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if(WiFiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_BUILTIN, HIGH); 
  }

  // server address, port and URL
  webSocket.begin(SERVER_URL, SERVER_PORT, "/");

  // event handler
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", MyApiKey);
  
  // try again every 5000ms if connection has failed
  webSocket.setReconnectInterval(5000);   // If you see 'class WebSocketsClient' has no member named 'setReconnectInterval' error update arduinoWebSockets
}

void loop() {
  webSocket.loop();
  
  if(isConnected) {
      uint64_t now = millis();
      
      // Send heartbeat in order to avoid disconnections during ISP resetting IPs over night. Thanks @MacSass
      if((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
          heartbeatTimestamp = now;
          webSocket.sendTXT("H");          
      }
  }   
}
  
void turnOn(String deviceId) {
  if (deviceId == "luzuno") // Device ID of first device
  {  
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
  }     
}

void turnOff(String deviceId) {
   if (deviceId == "luzuno") // Device ID of first device
   {  
     Serial.print("Turn off Device ID: ");
     Serial.println(deviceId);
   }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      isConnected = false;    
      Serial.printf("[webSocketEvent] Webservice disconnected from server!\n");
      break;
    case WStype_CONNECTED: {
      isConnected = true;
      Serial.printf("[webSocketEvent] Service connected to server at url: %s\n", payload);
      Serial.printf("[webSocketEvent] Waiting for commands from server ...\n");        
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[webSocketEvent] get text: %s\n", payload);
#if ARDUINOJSON_VERSION_MAJOR == 5
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject((char*)payload);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
        DynamicJsonDocument json(1024);
        deserializeJson(json, (char*) payload);      
#endif        
        String deviceId = json ["deviceId"];     
        String action = json ["action"];
        
        if(action == "setPowerState") {
            // alexa, turn on tv ==> {"deviceId":"xx","action":"setPowerState","value":"ON"}
            String value = json ["value"];
            if(value == "ON") {
                turnOn(deviceId);
                 theaterChaseRainbow(50);
            } else {
                turnOff(deviceId);
                pixels.clear(); // Set all pixel colors to 'off'
                pixels.show(); 
            }        
        }
        else if(action == "AdjustBrightness") {
            // alexa, dim lights  ==>{"deviceId":"xxx","action":"AdjustBrightness","value":-25}
           String brillo = json ["value"];
           pixels.setBrightness(brillo.toInt());
           pixels.show();
        }       
        else if(action == "AdjustBrightness") {
            // alexa, dim lights  ==>{"deviceId":"xx","action":"AdjustBrightness","value":-25}
             String brillo = json ["value"];
            pixels.setBrightness(brillo.toInt());
            pixels.show();
        }
        else if(action == "SetBrightness") {
           //alexa, set the lights to 50% ==> {"deviceId":"xx","action":"SetBrightness","value":50}
           String brillo = json ["value"];
            pixels.setBrightness(brillo.toInt());
            pixels.show();
        }
        else if(action == "SetColor") {
           //alexa, set the lights to red ==> {"deviceId":"xx","action":"SetColor","value":{"hue":0,"saturation":1,"brightness":1}}
           String val_h = json ["value"]["hue"];
           String val_s = json ["value"]["saturation"];
           String val_b = json ["value"]["brightness"];
           h = val_h.toInt();
           s = val_s.toInt();
           b = val_b.toInt();
            for(int i=0; i<NUMPIXELS; i++) { // For each pixel...
                   // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
                    // Here we're using a moderately bright green color:
               if(h==120)
               {
                pixels.setPixelColor(i, pixels.Color(0, 255, 0));
               }
               if(h==240)
                 {
                 pixels.setPixelColor(i, pixels.Color(0, 0, 255));
                 }
               if(h==0)
                 {
                   pixels.setPixelColor(i, pixels.Color(255, 0, 0));
                  }
            }
           pixels.show();   // Send the updated pixel colors to the hardware.
           delay(DELAYVAL); // Pause before next pass through loop
//           Serial.print("Valor: h ");
//           Serial.print(h);
//           Serial.print("s ");
//           Serial.print(s);
//           Serial.print("b ");
//           Serial.print(b);  
        }
        else if(action == "IncreaseColorTemperature") {
           //alexa, set the lights softer ==> {"deviceId":"xxx","action":"IncreaseColorTemperature"}
        }
        else if(action == "IncreaseColorTemperature") {
           //alexa, set the lights softer ==> {"deviceId":"xxx","action":"IncreaseColorTemperature"}
        }
        else if(action == "SetColorTemperature") {
           //alexa, set the lights softer ==> {"deviceId":"xxx","action":"SetColorTemperature","value":2200}
        }
      }
      break;
    case WStype_BIN:
      Serial.printf("[webSocketEvent] get binary length: %u\n", length);
      break;
    default: break;
  }
}

// If you are going to use a push button to on/off the switch manually, use this function to update the status on the server
// so it will reflect on Alexa app.
// eg: setPowerStateOnServer("deviceid", "ON")

// Call ONLY If status changed. DO NOT CALL THIS IN loop() and overload the server. 

void setPowerStateOnServer(String deviceId, String value) {
#if ARDUINOJSON_VERSION_MAJOR == 5
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
  DynamicJsonDocument root(1024);
#endif        
  root["deviceId"] = deviceId;
  root["action"] = "setPowerState";
  root["value"] = value;
  StreamString databuf;
#if ARDUINOJSON_VERSION_MAJOR == 5
  root.printTo(databuf);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
  serializeJson(root, databuf);
#endif  
  
  webSocket.sendTXT(databuf);
}

 //Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      pixels.show();

      delay(wait);

      for (uint16_t i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
