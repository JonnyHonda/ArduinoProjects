#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 10

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
int state = 0;
int interval = 5000;
int t = 0;


char* strarray[] = {"Water = ", "Air   = "};

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
void setup(void)
{
  // start serial port
  Serial.begin(9600);
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.setCursor(0,0);
  lcd.print("Ready");
  delay(2000);
  // Start up the library
  sensors.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
}


void loop(void){
    if( millis() > (interval + t)){
        if(state == 0){
            state = 1;
        }else{
            state = 0;
        }
        t = millis();
    }
    Serial.println(t);
    sensors.requestTemperatures(); // Send the command to get temperatures   
    lcd.setCursor(0,0);
    lcd.print(strarray[state]);
    lcd.setCursor(0,1);
    lcd.print(sensors.getTempCByIndex(state));
    lcd.print((char)223);
    lcd.print("C"); 

}

