#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <DHTesp.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); 
const int encoderPinA = 18;
const int encoderPinB = 19;
const int buttonPin = 5; 
volatile boolean aSetLast = false;
volatile boolean bSetLast = false;
bool tt = true;
int numberOfNetworks = 0;
int selectedNetwork = 0;
String wifiNames[30]; 
bool isConnected = false; 

#define pinDht 15
DHTesp dhtSensor;

#define WIFI_AP "Không có pass"
#define WIFI_PASS "nhappassdi"

#define TB_SERVER "thingsboard.cloud"
#define TOKEN "sWIBjACDv22OrUNYZ2ry"
constexpr uint16_t MAX_MESSAGE_SIZE = 256U;

WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

void setup()
{
  Serial.begin(115200);
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP); 
  attachInterrupt(digitalPinToInterrupt(encoderPinA), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderPinB), updateEncoder, CHANGE);

  lcd.init();                      
  lcd.backlight();                

  lcd.setCursor(0, 0);
  lcd.print("Scanning WiFi...");

  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Setup done");
  dhtSensor.setup(pinDht,DHTesp::DHT22);
  //connectToWiFi();
  //connectToThingsBoard();
}

void loop()
{
  if (numberOfNetworks == 0) {
    
    numberOfNetworks = WiFi.scanNetworks();
    Serial.println("Scan done");

    for (int i = 0; i < numberOfNetworks; ++i) {
      wifiNames[i] = WiFi.SSID(i);
    }
  }

  if (digitalRead(buttonPin) == LOW) {
    tt = !tt;
    
    delay(100);
  }
  if(tt == false){
    displaySelectedWiFiInfo();
    connectToWiFi();
      if (!tb.connected()) {
    connectToThingsBoard();
  }
    TempAndHumidity data =dhtSensor.getTempAndHumidity();
  float temp = data.temperature;
  int hum = data.humidity;

  Serial.println(temp);
  Serial.println(hum);

  sendDataToThingsBoard(temp, hum);

  delay(200);
  }
  else{
   displayWiFiMenu();
  }
  delay(300); 



  tb.loop();
}

void displayWiFiMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Networks:");
  if (selectedNetwork <= 0) selectedNetwork = 0;
  if (selectedNetwork >= numberOfNetworks) selectedNetwork = numberOfNetworks - 1;
  int startIndex = selectedNetwork - 1;
  if (startIndex < 0) {
    startIndex = 0;
  }
  int endIndex = min(startIndex + 3, numberOfNetworks);

  for (int i = startIndex; i < endIndex; ++i) {
    lcd.setCursor(0, i - startIndex + 1);
    if (i == selectedNetwork) {
      lcd.print(">");
    } else {
      lcd.print(" ");
    }
    lcd.print(wifiNames[i]);
  }
}

void displaySelectedWiFiInfo() {
  if (selectedNetwork >= 0 && selectedNetwork < numberOfNetworks) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DA CHON: ");
    lcd.setCursor(0, 1);
    lcd.print(wifiNames[selectedNetwork]);
    lcd.setCursor(0, 2);
    lcd.print("RSSI: ");
    lcd.print(WiFi.RSSI(selectedNetwork));
  }
}

void updateEncoder() {
  
  boolean aSetNew = digitalRead(encoderPinA);
  boolean bSetNew = digitalRead(encoderPinB);

  
  if (aSetLast == LOW && bSetLast == LOW) {
    if (aSetNew == HIGH && bSetNew == LOW) {
      selectedNetwork++;
    } else if (aSetNew == LOW && bSetNew == HIGH) {
      selectedNetwork--;
    }
  }

  
  aSetLast = aSetNew;
  bSetLast = bSetNew;
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    WiFi.begin(WIFI_AP, WIFI_PASS, 6);
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi.");
  } else {
    Serial.println("\nConnected to WiFi");
  }
}

void connectToThingsBoard() {
  if (!tb.connected()) {
    Serial.println("Connecting to ThingsBoard server");
    
    if (!tb.connect(TB_SERVER, TOKEN)) {
      Serial.println("Failed to connect to ThingsBoard");
    } else {
      Serial.println("Connected to ThingsBoard");
    }
  }
}
void sendDataToThingsBoard(float temp, int hum) {
  String jsonData = "{\"tempIn\":" + String(temp) + ", \"humIn\":" + String(hum) + "}";
  tb.sendTelemetryJson(jsonData.c_str());
  Serial.println("Data sent");
}
