// BMP085 include libs
#include <Wire.h>
#include <Adafruit_BMP085.h>

// Instansiate the bmp objet
Adafruit_BMP085 bmp;

// DHT libs
#include "DHT.h"
#define DHTPIN 5
#define DHTTYPE DHT22   // DHT 22  (AM2302)

// Instansiate the DHT object
DHT dht(DHTPIN, DHTTYPE);

const int ledPin = 13;       // the pin that the LED is attached to

const int  rainTipperPin = 2; // Pin the rain tipper is connected to, this *must* be an interrupt enabled pin

// Some vaiable declared as volatile for use in the rain tipper interupt
volatile unsigned long int  rainTipperCounter = 0;
volatile int rainTipperState = 0;
volatile int lastRainTipperState = 0;

// Various variables for use with the non interrupt sensors
float temperature = 0;
float pressure = 0;
float dhtTemperature = 0;
float humidity = 0;

int interval = 2000;
unsigned long int t = 0;
const int solarCellPin = 1;
int solarCellValue = 0;

void setup() {
  // initialize the rain tipper pin as a input:
  pinMode(rainTipperPin, INPUT);
  // initialize the anemometer pin as a input:
  //pinMode(anemometerPin, INPUT);

  // initialize the LED as an output:
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
  dht.begin();
  bmp.begin();
  attachInterrupt(digitalPinToInterrupt(rainTipperPin), incrementRainTippper, CHANGE);
}

void loop() {
  solarCellValue = analogRead(solarCellPin);
  temperature = bmp.readTemperature();
  pressure = bmp.readPressure();
  // The dht22 is slow but I want to avoid putting a delay in that freezes the code
  // so we'll use a millis interval
  if (millis() > (interval + t)) {
    digitalWrite(ledPin, HIGH);
    humidity = dht.readHumidity();
    dhtTemperature = dht.readTemperature();

    Serial.println("**************************************");
    Serial.print ("Pressure = "); Serial.println(pressure);
    Serial.print ("Humidity = "); Serial.println(humidity);
    Serial.print ("Temperature = "); Serial.println(temperature);
    Serial.print ("DHT Temperature = "); Serial.println(dhtTemperature);
    Serial.print ("Rain Tipper Counter = "); Serial.println(rainTipperCounter);
    Serial.print ("Solar Cell Value = "); Serial.println(solarCellValue);
    Serial.println();
    t = millis();
    digitalWrite(ledPin, LOW);
  }

}


void incrementRainTippper() {
  // read the Tipper input pin:
  rainTipperState = digitalRead(rainTipperPin);

  // compare the rainTipperState to its previous state
  if (rainTipperState != lastRainTipperState) {
    // if the state has changed, increment the counter
    if (rainTipperState == HIGH) {
      // if the current state is HIGH then the button
      // wend from off to on:
      rainTipperCounter++;
      //Serial.print("Rain Tipper Counter:  ");
      // Serial.println(rainTipperCounter);
    }
    // Delay a little bit to avoid bouncing
    delay(50);
  }
  // save the current state as the last state,
  //for next time through the loop
  lastRainTipperState = rainTipperState;
}

