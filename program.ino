#include "time.h"
#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED screen setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Preferences
const char* ssid = ""; // wifi ssid
const char* password = ""; // wifi password
String departsNowMsg = "Nyt"; // shown when departure offset is zero
String stopID = "0804"; // Tampere public transport stop code
bool showUnitSymbol = true; // show 'x min'

// Waltti API endpoint URL
String serverName = "http://api.digitransit.fi/routing/v1/routers/waltti/index/graphql";

// Utilize NTP for getting the current time in epoch
const char* ntpServer = "pool.ntp.org";

// Print msg to the top of the display
void printOledTitle(String msg) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(msg);
}

// Print a large message to the centre of the display
void printOledMainMsg(String msg) {
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(0, 25);
    display.println(msg);
}

// Remove unsupported characters from messages
String serializeStrForOled(String str) {
  String serialized = str;
  serialized.replace("ä", "a");
  serialized.replace("ö", "o");
  serialized.replace("å", "a");
  return serialized;
}

// Print error to display
void showError(String msg) {
  display.clearDisplay();
  printOledTitle("An error occured");
  printOledMainMsg("NET");
  display.display(); 
}

void setup() {
  Serial.begin(115200); 
  WiFi.begin(ssid, password);

  // Wait for WiFi
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Wait for display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  // Setup NTP with GMT timezone since then departure times can be compared to them
  configTime(0, 0, ntpServer);
}

void loop() {
  if(WiFi.status()== WL_CONNECTED){
    WiFiClient client;
    HTTPClient http;
    http.begin(client, serverName);

    // Setup HTTP payload
    http.addHeader("Content-Type", "application/json");
    // GraphQL request body as a QL String. TODO: store the stop ID in a variable
    String ql = R"({"query": "{stop(id: \"tampere:0804\") {name, stoptimesWithoutPatterns(numberOfDepartures: 1) {serviceDay realtimeDeparture}}}"})";

    // Send POST request
    int httpResponseCode = http.POST(ql);

    // Parse JSON into JsonObject
    String response = http.getString();
    StaticJsonDocument<512> doc;
    deserializeJson(doc, response);

    // Get current time in epoch
    time_t now;
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    time(&now);
    unsigned long epochTime = now;

    // Read values from JSON and calculate how many minutes there are til the next departure
    JsonObject dataStop = doc["data"]["stop"];
    JsonObject dataStopNumbers = dataStop["stoptimesWithoutPatterns"][0]; // next vehicle event object
    String dataStopName = serializeStrForOled(dataStop["name"]); // name of the selected stop
    long dataStopServiceDay = dataStopNumbers["serviceDay"]; // epoch time of the day the next vehicle departs (should be current day)
    long dataStopTimeOffset = dataStopNumbers["realtimeDeparture"]; // epoch time offset for next departure (compared to serviceDay)
    long dataStopDepartureTime = dataStopServiceDay + dataStopTimeOffset; // absolute epoch time for next departure
    float departureOffsetTime = (dataStopDepartureTime - epochTime) / 60; // relative offset for next departure in minutes
    String departureScreen = String(departureOffsetTime, 0); // relative departure time as string

    // Print Nyt/Now when the time goes to zero
    if (departureOffsetTime <= 0) {
      departureScreen = departsNowMsg;
    } else if (showUnitSymbol) {
      departureScreen = departureScreen + " min";
    }

    // Print to screen
    if (httpResponseCode < 0 || httpResponseCode > 200) {
      // If the request wasn't successful, show an error code on the display
      showError("NETERR");
    } else if (departureOffsetTime < 0 || departureOffsetTime > 22000) {
      // If the offset is less than zero or larger than ~6h, show an error code on the display
      showError("NUMERR");
    } else {
      // Everything ok, print data on the display
      Serial.println(departureOffsetTime);
      display.clearDisplay();
      printOledTitle(dataStopName);
      printOledMainMsg(departureScreen);
      display.display(); 
    }

    http.end();
  }

  // TODO: implement some logic that reduces the amount of HTTP requests
  delay(10000);
}
