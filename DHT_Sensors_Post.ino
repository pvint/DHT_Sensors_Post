#include <Arduino.h>
#include <WebServer.h>

// for HTTP POST
#include <HTTPClient.h>


#include "Ticker.h"
#include "DHTesp.h"

// The html for each sensor is currently hard coded below in index_html etc
#define NUM_SENSORS 2

// Set pin for each sensor
int dhtPin[NUM_SENSORS];

#include <WiFi.h>

const char* ssid     = "SSID";
const char* password = "PASSPHRASE";

const char* serverName = "http://192.168.1.173/espSensorPost.php";
String apiKeyValue = "tPmAT5Ab3j7F9";

DHTesp dhtSensor[NUM_SENSORS];  
TempAndHumidity sensorData;


void setup()
{
  Serial.begin(115200);
  delay(4000);

  Serial.printf("Serve sensor data from %d DHT Temp/Humidity sensors\n", NUM_SENSORS);
  dhtPin[0] = 21;
  dhtPin[1] = 22;

  for (int i = 0; i < NUM_SENSORS; i++)
  {
    dhtSensor[i].setup(dhtPin[i], DHTesp::DHT22);
  }

      Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    
}

void post(String name, String location, float value)
{
  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  // Prepare your HTTP POST request data
  String httpRequestData = "api_key=" + apiKeyValue + "&sensor=" + name
                          + "&location=" + location + "&value=" + String(value);
  Serial.print("httpRequestData: ");
  Serial.println(httpRequestData);

  int httpResponseCode = http.POST(httpRequestData);
  if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
}

void loop()
{
  // Get each sensor value and send as HTTP POST to server
  float val = dhtSensor[0].getTemperature();
  String sensorName = "Temperature";
  String sensorLocation = "Outside";

  post(sensorName, sensorLocation, val);

  val = dhtSensor[0].getHumidity();
  sensorName = "Humidity";

  post(sensorName, sensorLocation, val);
  
  sensorLocation = "Outside";
  val = dhtSensor[1].getTemperature();
  sensorName = "Temperature";

  post(sensorName, sensorLocation, val);
  

  val = dhtSensor[1].getHumidity();
  sensorName = "Humidity";

  post(sensorName, sensorLocation, val);
    delay(10000);
}
