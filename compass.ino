#include <Adafruit_NeoPixel.h>
#include <QMC5883LCompass.h>

#define RING_PIN 5
#define NUMPIXELS 12
#define NORTH 0
#define EAST 3
#define SOUTH 6
#define WEST 9

Adafruit_NeoPixel pixels =
    Adafruit_NeoPixel(NUMPIXELS, RING_PIN, NEO_GRB + NEO_KHZ800);

uint32_t white = pixels.Color(255, 255, 255);
uint32_t red = pixels.Color(255, 0, 0);
uint32_t green = pixels.Color(0, 255, 0);
uint32_t blue = pixels.Color(0, 0, 255);
uint32_t snake_colour = pixels.Color(144, 1, 122);

uint32_t i;
int az; // azimuth
uint32_t compass_interval;

QMC5883LCompass compass;

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
}

uint32_t get_compass_interval(int azimuth) {
  unsigned long a = (azimuth > -0.5) ? azimuth / 30.0 : (azimuth + 360) / 30.0;
  Serial.print(a);
  unsigned long r = a - (int)a;
  byte sexdec = 0;
  sexdec = (r >= .5) ? ceil(a) : floor(a);
  return sexdec;
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

int get_compass_interval() {
  compass.read();
  az = compass.getAzimuth();
  compass_interval = get_compass_interval(az);

  Serial.print(" Azimuth: ");
  Serial.print(az);

  Serial.print(" Interval: ");
  Serial.print(compass_interval);

  Serial.println();

  return compass_interval;
}

void loop() {
  compass_interval = get_compass_interval();
  set_LED_direction(compass_interval);
  delay(100);
}
