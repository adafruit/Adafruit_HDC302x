#include <Adafruit_HDC302x.h>

Adafruit_HDC302x hdc = Adafruit_HDC302x();

void setup() {
  Serial.begin(115200);

  Serial.println("Adafruit HDC302x Simple Test");

  if (! hdc.begin(0x44, &Wire)) {
    Serial.println("Could not find sensor?");
    while (1);
  }
  delay(1000);
}

void loop() {
  double temp = 0.0;
  double RH = 0.0;

  hdc.readTemperatureHumidityOnDemand(temp, RH, TRIGGERMODE_LP0);

  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(RH);
  Serial.println(" %");
  delay(2000);
  
}
