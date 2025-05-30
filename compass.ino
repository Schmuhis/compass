#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <QMC5883LCompass.h>
#include <WebSocketsClient.h>
#include <math.h>

#define RING_PIN 12
#define NUMPIXELS 12
#define RADIANS 180.0 / PI
#define SCALE 1000
#define N_INTERVALS 12
#define OFFSET 5
#define BUTTON_PIN 23

Adafruit_NeoPixel pixels =
    Adafruit_NeoPixel(NUMPIXELS, RING_PIN, NEO_GRB + NEO_KHZ800);

uint32_t white = pixels.Color(255, 255, 255);
uint32_t red = pixels.Color(255, 0, 0);
uint32_t green = pixels.Color(0, 255, 0);
uint32_t blue = pixels.Color(0, 0, 255);
uint32_t snake_colour = pixels.Color(144, 1, 122);
uint32_t background_colour = pixels.Color(232, 128, 30);

const char *GAME_PHASE = "UNKOWN";
const char *UUID = "febc6409-0ba1-47f0-bca4-a75ec5888aa9";
const char *CONNECT_ID = "espCompass_db1a60b8_connect";
int i, j = 0, k;
uint32_t compass_interval;
const char *CONNECTION_STATE = "UNKNOWN";

const char *ssid = "Techbase Guest";
const char *password = "Hackaburg25";
const char *websocket_server =
  "hide-and-seek-unxw.onrender.com";

float hider_lat = 50; // 90 degree for +, 270 for -
float hider_long = 0; // 0 degree for +, 180 for -
float seeker_lat = 0;
float seeker_long = 0;

int button_state = HIGH;

QMC5883LCompass compass;
WebSocketsClient webSocket;

void sendCompassMoinRequest() {
  // Construct CompassMoinRequest
  Serial.println("WebSocket Connected");
  Serial.println("Trying to register to Hide-n-Seek Websocket");

  JsonDocument doc;
  doc["id"] = CONNECT_ID;
  doc["message"]["type"] = "compassMoinRequest";
  doc["message"]["value"]["userId"] = UUID;
  String registrationJsonChar;
  serializeJson(doc, registrationJsonChar);
  webSocket.sendTXT(registrationJsonChar);
}

void handleUpdateStateEvent(JsonDocument updateEvent) {

  if (updateEvent["message"]["value"]["state"]["room"]["gamePhase"] ==
      "seeking") { // change to seeking for prod
    String hiderId =
        updateEvent["message"]["value"]["state"]["room"]["hiderId"];
    hider_long = updateEvent["message"]["value"]["state"]["room"]["positions"]
                            [hiderId]["long"];
    hider_lat = updateEvent["message"]["value"]["state"]["room"]["positions"]
                           [hiderId]["lat"];
    seeker_long = updateEvent["message"]["value"]["state"]["room"]["positions"]
                             [UUID]["long"];
    seeker_lat = updateEvent["message"]["value"]["state"]["room"]["positions"]
                            [UUID]["lat"];
    Serial.print("Updated Location Stats, Hider:");
    Serial.printf("%f.8", hider_lat);
    Serial.print(", ");
    Serial.printf("%f.8", hider_long);
    Serial.print(", Seeker");
    Serial.printf("%f.8", seeker_lat);
    Serial.print(", ");
    Serial.printf("%f.8\n", seeker_long);
  }
}

void handleResponse(uint8_t *payload) {
  JsonDocument doc;
  deserializeJson(doc, payload);
  if (doc["id"] == CONNECT_ID && CONNECTION_STATE != "CONNECTED") {
    Serial.println(
        "Received same connect id, switching connection state to 'CONNECTED'");
    CONNECTION_STATE = "CONNECTED";
  }

  if (doc["message"]["type"] == "updateStateEvent") {
    handleUpdateStateEvent(doc);
  }
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
  case WStype_DISCONNECTED:
    Serial.println("WebSocket Disconnected");
    break;
  case WStype_CONNECTED:
    sendCompassMoinRequest();
    break;
  case WStype_TEXT:
    // Serial.printf("Received: %s\n", payload);
    handleResponse(payload);

    break;
  }
}
void init_compass() {
  compass.init();
  compass.setCalibrationOffsets(-486.00, 369.00, 278.00);
  compass.setCalibrationScales(1.03, 0.97, 1.01);
}

void init_LED_ring() {
  pixels.begin();
  pixels.setBrightness(50);
}

void waiting_LED() {
  for (j = 0; j < NUMPIXELS; j++) {
    pixels.setPixelColor(j, red);
  }
  pixels.show();
  delay(500);
  for (j = 0; j < NUMPIXELS; j++) {
    pixels.setPixelColor(j, blue);
  }
  pixels.show();
  delay(500);
}

void setup() {
  Serial.begin(9600);
  init_compass();
  init_LED_ring();

  // Setup calibration button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    waiting_LED();
  }
  Serial.println("\nWiFi connected");

  webSocket.beginSSL(websocket_server, 443, "/api/websocket"); // WSS
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

uint32_t get_compass_interval(int azimuth) {
  azimuth = 360 - azimuth;
  float a = azimuth / (360.0 / N_INTERVALS);
  float r = a - (int)a;
  int sexdec = 0;
  sexdec = (r >= .5) ? ceil(a) : floor(a);
  Serial.printf("Interval: %d\n", sexdec);
  return sexdec;
}

float get_direction(float my_lat, float other_lat, bool scaled) {
  float dist = other_lat - my_lat;
  float scaled_dist = other_lat * SCALE - my_lat * SCALE;
  if (scaled)
    return scaled_dist;
  else
    return dist;
}

void set_LED_direction(uint32_t compass_interval) {
  for (i = 0; i < NUMPIXELS; i++)
    pixels.setPixelColor(i, blue);
  int len = NUMPIXELS / N_INTERVALS;

  for (i = - floor(0.5 * len) - 1; i < ceil(0.5 * len) + 1; i++) {
    int pixel = compass_interval * len + i;
    pixel = (pixel + OFFSET + NUMPIXELS) % NUMPIXELS;
    pixels.setPixelColor(pixel, red);
  }
  pixels.show();
}

float get_compass_azimuth() {
  compass.read();
  return compass.getAzimuth();
}

float get_azimuth_lat_long(float lat_dir, float long_dir) {
  float cos_theta = lat_dir / sqrt(lat_dir * lat_dir + long_dir * long_dir);
  float theta = acosf(cos_theta) * RADIANS;
  Serial.printf("Lat: %f, Long: %f, cos = %f, theta: %f\n", lat_dir, long_dir, cos_theta, theta);
  if (lat_dir < 0.0) {
    theta = 360.0 - theta;
  }
  return theta;
}

uint32_t get_compass_interval_for_dir(float hl, float hlo, float sl,
                                      float slo) {
  float lat_dir_scaled = get_direction(sl, hl, 1);
  float long_dir_scaled = get_direction(slo, hlo, 1);

  float lat_long_az_scaled =
    get_azimuth_lat_long(lat_dir_scaled, long_dir_scaled);
  float az = get_compass_azimuth() + 180.0;

  float compass_az = lat_long_az_scaled + az;
  compass_az = compass_az > 360 ? compass_az - 360 : compass_az;
  Serial.printf("Azimuth : %f\n", compass_az);

  return get_compass_interval(compass_az);
}

void show_disconnecting_LEDs() {
  for (i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, background_colour);
  }
  for (k = 0; k < 3; k++) {
    pixels.setPixelColor((j + k + 1) % NUMPIXELS, snake_colour);
  }
  pixels.show();
  j = (j + 1) % NUMPIXELS;
  delay(100);
}

void handle_LED() {
  if (CONNECTION_STATE == "CONNECTED") {
    compass_interval = get_compass_interval_for_dir(hider_lat, hider_long,
                                                    seeker_lat, seeker_long);
    set_LED_direction(compass_interval);
  } else {
    show_disconnecting_LEDs();
  }
}

void calibration_LEDs() {
  for (i = 0; i < 2; i++) {
    for (j = 0; j < NUMPIXELS; j++) {
      pixels.setPixelColor(j, background_colour);
    }
    pixels.show();
    delay(500);
    for (j = 0; j < NUMPIXELS; j++) {
      pixels.setPixelColor(j, snake_colour);
    }
    pixels.show();
    delay(500);
  }
}

void calibrate_compass() {
  Serial.println("Started calibrating...");
  calibration_LEDs();
  for (j = 0; j < NUMPIXELS; j++) {
    pixels.setPixelColor(j, red);
  }
  pixels.show();
  compass.calibrate();
  calibration_LEDs();
  Serial.println("Calibration done.");
}

void loop() {
  button_state = digitalRead(BUTTON_PIN);

  if (button_state == LOW) {
    Serial.println(button_state);
    calibrate_compass();
    button_state = HIGH;
  } else {
    webSocket.loop();
    handle_LED();
  }
}
