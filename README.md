# Tram Stop Display
Show the departure time of the next tram from the given stop in Tampere

![](https://i.imgur.com/utCJt4N.jpg)

## Implementation
Utilizes ESP32 and an Adaftruit OLED display. Connect the SCL and SDA pins to some digital pins on your ESP32 (I used G21 for SCL and G22 for SDA).

## Usage
Change the variable values under "Preferences" part in the source code.

```
const char* ssid = "";
const char* password = "";
String departsNowMsg = "Nyt";
String stopID = "";
bool showUnitSymbol = true;
```

You can search a bus/tram stop by its name here: https://www.nysse.fi/aikataulut-ja-reitit.html?state=/en/lines.  
Use the four digit stop code as the `stopID`.
