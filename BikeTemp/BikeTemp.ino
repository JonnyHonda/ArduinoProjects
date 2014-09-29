/**
  BikeTemp.ino
  Author: John Burrin (JonnyHonda)

  This code uses two DS18B20 temperature sensors and displays their readings
  on a 1 row, 16 character display. 
  The purpose is to sample the temperature of the water cooling system and the ambient 
  air temperature.
  If the water sensor exceeds WATER_MAX_TEMP then a warning LED is illuminated also if the air temperature is
  below AIR_MIN_TEMP the other LED is illuminated.
**/
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>

// Data wire for DS18B20s is plugged into pin 10 on the Arduino
#define ONE_WIRE_BUS 10

#define WATER_MAX_TEMP 85
#define AIR_MIN_TEMP 3

// Define 2 pins to use as outputs for warning LEDs
int led[2] = {8,9};

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
int state = 0;
int interval = 5000;
unsigned long t = 0;
float temp;


char* strarray[] = {"Water = ", "Air   = "};

// Initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
void setup(void)
{
  // start serial port
  Serial.begin(9600);

  // Setup pins for Warning LEDs
  pinMode(led[0], OUTPUT); 
  pinMode(led[1], OUTPUT);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.setCursor(0,0);
  lcd.print("Ready");
  
  // Start up the library
  sensors.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
}


void loop(void){
// By do it this way the temperatures are aways updated and
// and no delays are used
  sensors.requestTemperatures(); // Send the command to get temperatures   
  lcd.setCursor(0,0);
  lcd.print(strarray[state]);
  lcd.setCursor(0,1);
  temp = sensors.getTempCByIndex(state);
  if( millis() > (interval + t)){
    if(state == 0){
      (temp >= WATER_MAX_TEMP ) ? digitalWrite(led[state], HIGH) : digitalWrite(led[state], LOW);
      state = 1;
    }else{
      (temp <= AIR_MIN_TEMP ) ? digitalWrite(led[state], HIGH) : digitalWrite(led[state], LOW);
      state = 0;
    }
  t = millis();
}
  lcd.print(temp);
  // Celsius symbol
  lcd.print((char)223);
  lcd.print("C  "); 
}


