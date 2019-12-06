#include <Arduino.h>
#include <WiFi.h>

// for HTTP POST
#include <HTTPClient.h>

#include "Ticker.h"
#include "DHTesp.h"

// for OTA
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

#include "ping.h"
// for ping test
IPAddress gateway;

// The html for each sensor is currently hard coded below in index_html etc
#define NUM_SENSORS 2

// Set pin for each sensor
int dhtPin[NUM_SENSORS];

#include <WiFi.h>

const char* host = "ESP32-Downstairs";
const char* ssid     = "SSID";
const char* password = "PASSPHRASE";

const char* serverName = "http://192.168.1.173/espSensorPost.php";
String apiKeyValue = "tPmAT5Ab3j7F9";

DHTesp dhtSensor[NUM_SENSORS];  
TempAndHumidity sensorData;

WebServer server(80);


// OTA STUFF

/*
 * Login page
 */

const char* loginIndex = 
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>ESP32 Login Page</b></font></center>"
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<td>Username:</td>"
        "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
"</script>";
 
/*
 * Server Index Page
 */
 
const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";


void setup()
{
  Serial.begin(115200);
  delay(1000);

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
    
      /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }

    Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
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
  // I'm getting disconnected from WiFi every day or so... test:
  if (!ping_start(gateway, 4, 0, 0, 5))
  {
    WiFi.disconnect();
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
  }
  
  // Get each sensor value and send as HTTP POST to server
  float val = dhtSensor[0].getTemperature();
  String sensorName = "Temperature";
  String sensorLocation = "Wine Room";

  post(sensorName, sensorLocation, val);

  val = dhtSensor[0].getHumidity();
  sensorName = "Humidity";

  post(sensorName, sensorLocation, val);
  
  sensorLocation = "Downstairs";
  val = dhtSensor[1].getTemperature();
  sensorName = "Temperature";

  post(sensorName, sensorLocation, val);
  

  val = dhtSensor[1].getHumidity();
  sensorName = "Humidity";

  post(sensorName, sensorLocation, val);


  // OTA Stuff - TODO - should be in a separate thread!
    server.handleClient();
    delay(10000);
}
