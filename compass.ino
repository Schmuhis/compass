#include <Adafruit_NeoPixel.h>
#include <QMC5883LCompass.h>
#include <math.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#define RING_PIN 5
#define NUMPIXELS 12
#define NORTH 0
#define EAST 3
#define SOUTH 6
#define WEST 9
#define RADIANS 57.2958

Adafruit_NeoPixel pixels =
  Adafruit_NeoPixel(NUMPIXELS, RING_PIN, NEO_GRB + NEO_KHZ800);

uint32_t white = pixels.Color(255, 255, 255);
uint32_t red = pixels.Color(255, 0, 0);
uint32_t green = pixels.Color(0, 255, 0);
uint32_t blue = pixels.Color(0, 0, 255);
uint32_t snake_colour = pixels.Color(144, 1, 122);

const char* UUID = "db1a60b8-02dd-47cf-8a7d-40f2bfc4f33e";

uint32_t i;
int az;  // azimuth
uint32_t compass_interval;

const char* ssid = "Techbase Guest";
const char* password = "Hackaburg25";
const char* websocket_server = "echo.websocket.events";  // Example secure WebSocket server


QMC5883LCompass compass;
WebSocketsClient webSocket;

void sendCompassMoinRequest() {
  // Construct CompassMoinRequest
  Serial.println("WebSocket Connected");
  Serial.println("Trying to register to Hide-n-Seek Websocket");
  JsonDocument doc;
  doc["userId"] = UUID;
  String registrationJsonChar;
  serializeJson(doc, registrationJsonChar);
  webSocket.sendTXT(registrationJsonChar);
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket Disconnected");
      break;
    case WStype_CONNECTED:
      sendCompassMoinRequest();
      // webSocket.sendTXT("Hello");
      break;
    case WStype_TEXT:
      Serial.printf("Received: %s\n", payload);
      break;
  }
}
void init_compass() {
  compass.init();
  compass.setCalibrationOffsets(-161.00, 515.00, -503.00);
  compass.setCalibrationScales(0.87, 0.85, 1.50);
}

void init_LED_ring() {
  pixels.begin();
  pixels.setBrightness(50);
}

void setup() {
  Serial.begin(9600);
  init_compass();
  init_LED_ring();
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  webSocket.beginSSL(websocket_server, 443, "/");  // WSS
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

uint32_t get_compass_interval(int azimuth) {
  unsigned long a = (azimuth > -0.5) ? azimuth / 30.0 : (azimuth + 360) / 30.0;
  unsigned long r = a - (int)a;
  byte sexdec = 0;
  sexdec = (r >= .5) ? ceil(a) : floor(a);
  return sexdec;
}

float get_lat_direction(float my_lat, float other_lat) {
  return other_lat - my_lat;
}

float get_long_direction(float my_long, float other_long) {
  return other_long - my_long;
}

void set_LED_direction(uint32_t compass_interval) {
  for (i = 0; i < NUMPIXELS; i++) {
    if (i != compass_interval) {
      pixels.setPixelColor(i, blue);
    }
  }
  pixels.setPixelColor(compass_interval, red);
  pixels.show();
}

float get_compass_azimuth() {
  compass.read();
  return compass.getAzimuth();
}

int get_compass_interval() {
  compass_interval = get_compass_interval(az);

  Serial.print(" Azimuth: ");
  Serial.print(az);

  Serial.print(" Interval: ");
  Serial.print(compass_interval);

  return compass_interval;
}

float get_azimuth_lat_long(float lat_dir, float long_dir) {
  float cos_theta = lat_dir / sqrt(lat_dir * lat_dir + long_dir * long_dir);
  return acos(cos_theta) * RADIANS;
}

void loop() {
  float my_lat = 20;
  float my_long = 20;
  float other_lat = 30;
  float other_long = 50;

  float lat_dir = get_lat_direction(my_lat, other_lat);
  float long_dir = get_long_direction(my_long, other_long);
  Serial.print(" lat_dir: ");
  Serial.print(lat_dir);
  Serial.print(" long_dir: ");
  Serial.print(long_dir);
  Serial.println();

  az = get_azimuth_lat_long(lat_dir, long_dir);
  // az = get_compass_azimuth();

  compass_interval = get_compass_interval(az);
  set_LED_direction(compass_interval);
  delay(100);
  webSocket.loop();
}