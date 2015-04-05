
//*****************************************************
//RFM12B OKK sections taken from the original Jeenode OKK Raw example
//Tested with Arduino 0021
// polling RFM12B to decode FSK iT+ with a Jeenode from Jeelabs.
// device  : iT+ TX29IT 04/2010 v36 D1 LA Crosse Technology (c)
// info    : http://forum.jeelabs.net/node/110
//           http://fredboboss.free.fr/tx29/tx29_sw.php
//           http://www.f6fbb.org/domo/sensors/
//           benedikt.k http://www.mikrocontroller.net/topic/67273
//           rinie,marf,joop 1 nov 2011
// *****************************************************
// * Resouces used:-
// * http://www.susa.net/wordpress/2012/08/raspberry-pi-reading-wh1081-weather-sensors-using-an-rfm01-and-rfm12b/
// * jeenode_lacross_rx by Rufik
// * Adafruit BMP085 library
// *****************************************************
// 09/03/2013 V 1.13 WH1080 868Mhz OOK fine offset Arduino decoder
//                      by Steve Pyle
//******************************************************
//************        Change log        ****************
//V1.0 Initial release 09/01/2013
//V1.10 Added Baro - BMP05 as per Adafruit
//V1.11 Added DHT22 Humidity / Temp sensor to RX and added Battery Status (need to confirm correct operation of Battery status)
//V1.12 Added windchill calculation
//V1.13 Added Dew Point Calculation (need to confirm OK)
// *****************************************************


#include <Wire.h>
#include <util/delay.h>
#include "Arduino.h"






#define clrb(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define setb(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

#define RF_PORT PORTB
#define RF_DDR DDRB
#define RF_PIN 2

#define SDI 3
#define SCK 5
#define CS 2
#define SDO 4
#define DHT22_PIN 4 // Define DHT22 port


#define CRC_POLY 0x31 // CRC-8 = 0x31 is for: x8 + x5 + x4 + 1
#define DEBUG 1 // set to 1 to see debug messages


int frame;
int device_id;
int temperature_raw;
int temperature;
int humidity;
float wind_avg_ms;
float wind_avg_mph;
int wind_avg_raw;
int wind_gust_raw;
float wind_gust_ms;
float wind_gust_mph;
float rain;
int rain_raw;
int wind_direction;
float pressure;
float readPressure;
int MSB;
int LSB;
String s;
char sign;
int Bat;
float DHT22_Hum;
float DHT22_Temp;
float TempC;
float wind_chill;
float dew_point;
float Hum;



char *direction_name[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};



//Receive message, data[]
void receive(unsigned int *data)
{ 
  unsigned char mesage[20];

  if (DEBUG) {
    Serial.println("Listening....");
  }

  rf12_rxdata(mesage, 20);
  
  if (DEBUG) {
    Serial.print("Received HEX raw data: ");
    for (int i = 0; i < 20; i++) {

      Serial.print(mesage[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }

  //Write message into table
  for (int i = 0; i < 20; i++) {
    data[i] = (unsigned int) mesage[i];
  }
}



// Calculate CRC
static uint8_t compute_crc(uint8_t b) {
  uint8_t do_xor;
  uint8_t reg;

  reg = 0;
  do_xor = (reg & 0x80);

  reg <<= 1;
  reg |= b;

  if (do_xor) {
    reg ^= CRC_POLY;
  }

  return reg;
}




//check if CRC is OK
boolean is_crc_valid(unsigned int crcByte) {
  boolean result = false;

  uint8_t crc_computed = compute_crc((uint8_t) crcByte);
  if ((unsigned int) crc_computed == crcByte) {
    result = true;
    if (DEBUG) {
      Serial.print("CRC OK: ");
      Serial.println(crc_computed, HEX);
    }
  } else {
    if (DEBUG) {
      Serial.print("CRC Error... Calculated is "); Serial.print(crc_computed, HEX);
      Serial.print(" Received is "); Serial.println(crcByte, HEX);
    }

  }
  return result;
}




//Check device ID starts with A7 (My WH1080 stn ID)
boolean is_msg_valid(unsigned int msgFirstByte) {
  boolean result = false;

  unsigned int msgFlag = (msgFirstByte);
  if (msgFlag == 167) {
    result = true;
    if (DEBUG) {
      Serial.println("OK");
    }
  } else {
    if (DEBUG) {
      Serial.print("Msg does not start with A7, it's: ");
      Serial.println(msgFlag, HEX);
    }
  }
  return result;
}



void rf12_rxdata(unsigned char *data, unsigned char number)
{ 
  Serial.println("Strart rf12_rxdata");
  uint8_t  i;
  rf12_xfer(0x82C8);			// receiver on
  rf12_xfer(0xCA81);			// set FIFO mode
  rf12_xfer(0xCA83);			// enable FIFO
  for (i = 0; i < number; i++)
  { 
    rf12_ready();
    *data++ = rf12_xfer(0xB000);
  }
  rf12_xfer(0x8208);			// Receiver off
  Serial.println("Exit rf12_rxdata");
}

// ******** SPI + RFM 12B functs   ************************************************************************

unsigned short rf12_xfer(unsigned short value)
{ uint8_t i;

  clrb(RF_PORT, CS);
  for (i = 0; i < 16; i++)
  { if (value & 32768)
      setb(RF_PORT, SDI);
    else
      clrb(RF_PORT, SDI);
    value <<= 1;
    if (RF_PIN & (1 << SDO))
      value |= 1;
    setb(RF_PORT, SCK);
    asm("nop");
    asm("nop");
    clrb(RF_PORT, SCK);
  }
  setb(RF_PORT, CS);
  return value;
}


void rf12_ready(void)
{
  Serial.println("Strart rf12_ready");
  clrb(RF_PORT, CS);
  asm("nop");
  asm("nop");
  while (!(RF_PIN & (1 << SDO))); // wait until FIFO ready
  setb(RF_PORT, CS);
  Serial.println("End rf12_ready");
}


static void rf12_la_init()
{
  RF_DDR = (1 << SDI) | (1 << SCK) | (1 << CS);
  RF_PORT = (1 << CS);
  for (uint8_t  i = 0; i < 10; i++) _delay_ms(10); // wait until POR done
  rf12_xfer(0x80E8); // 80e8 CONFIGURATION EL,EF,868 band,12.5pF
  rf12_xfer(0xA67c); // a67c FREQUENCY SETTING 868.300
  rf12_xfer(0xC613); // c613 DATA RATE c613  17.241 kbps
  rf12_xfer(0xC26a); // c26a DATA FILTER COMMAND
  rf12_xfer(0xCA12); // ca12 FIFO AND RESET  8,SYNC,!ff,DR
  rf12_xfer(0xCEd4); // ced4 SYNCHRON PATTERN  0x2dd4
  rf12_xfer(0xC49f); // c49f AFC during VDI HIGH +15 -15 AFC_control_commAND
  rf12_xfer(0x94a0); // 94a0 RECEIVER CONTROL VDI Medium 134khz LNA max DRRSI 103 dbm
  rf12_xfer(0xCC77); // cc77 not in RFM01
  rf12_xfer(0x9872); // 9872 transmitter not checked
  rf12_xfer(0xE000); // e000 NOT USE
  rf12_xfer(0xC800); // c800 NOT USE
  rf12_xfer(0xC040); // c040 1.66MHz,2.2V
}


void setup () {
  pinMode(RF_PIN, INPUT);
  Serial.begin(57600);
  if (DEBUG) {
    Serial.println("WH1080 Weather Station Wireless Receiver ");
  }
  rf12_la_init();
  if (DEBUG) {
    Serial.println("RF Module setup complete");
  }

}


void loop ()
{
  unsigned int frame[20];
  //got message now

  receive(frame);
  //check if valid for Fineoffset (WH1080)
  if (DEBUG) Serial.println(frame[0]);
  
  if (is_msg_valid(frame[0])) {
    // then check CRC OK
    if (is_crc_valid(frame[9]))

    {
      // Now convert the data
      // Station ID
      device_id = ((frame[0] << 4) | (frame[1] >> 4));


      // Temperature outside
      sign = (frame[1] >> 3) & 1    ;
      temperature =  ((frame[1] & ~(0xff << 1)) << 8) | frame[2] ;
      if (sign) TempC = ((float)(~temperature) + sign) / 10;
      else TempC = (float) temperature / 10;


      // Humidty Outside
      humidity = frame[3];

      // Wind Avg
      wind_avg_raw = frame[4];
      wind_avg_ms = ((float)wind_avg_raw * 34.0) / 100;
      if (DEBUG) {
        wind_avg_mph = wind_avg_ms * 2.23693629;
      }

      // Wind Gust
      wind_gust_raw = frame[5];
      wind_gust_ms = ((float)wind_gust_raw * 34.0) / 100;
      if (DEBUG) {
        wind_gust_mph = wind_gust_ms * 2.23693629;
      }

      // Wind chill calc from http://en.wikipedia.org/wiki/Wind_chill
      if ((TempC < 10.0) && (wind_gust_ms > 1.3))
      {
        wind_chill = (13.12 + 0.6215 * TempC - 11.37 * pow ((wind_gust_ms * 3.6) , 0.16) + 0.3968 * TempC * pow ((wind_gust_ms * 3.6), 0.16));
      }
      else
      {
        wind_chill = TempC;
      }

      // Dew point Calc from wikipedia
      Hum = log10 (humidity) - 2 / 0.4343 + (17.62 * TempC) / (243.12 + TempC);
      dew_point = 243.12 * Hum / (17.62 - Hum);

      // Rain
      rain_raw = ((frame[6] & 0x0f) << 8) | frame[7];
      rain = ((float)rain_raw / 0.3);

      // Wind Direction
      wind_direction = frame[8] & 0x0f;
      char *direction_str = direction_name[wind_direction];



      // Battery status
      Bat = (frame[8] & 0x80);

      // DHT22 indoor Humidity and Temperature
      delay(2000);




      if (DEBUG) {
        Serial.print("ID=");
        Serial.println(device_id);
        Serial.print("Temperature=");
        Serial.print(TempC, 1 );
        Serial.println (" C");
        Serial.print("Humidity=");
        Serial.println(humidity);
        Serial.print("Wind avg=");
        Serial.print(wind_avg_ms);
        Serial.print("m/s or ");
        Serial.print(wind_avg_mph);
        Serial.println("mph");
        Serial.print("Wind Gust=");
        Serial.print(wind_gust_ms);
        Serial.print("m/s or ");
        Serial.print(wind_gust_mph);
        Serial.println("mph");
        Serial.print("Wind Direction=");
        Serial.println(direction_str);
        Serial.print("Wind Chill=");
        Serial.print(wind_chill, 1);
        Serial.println(" C");
        Serial.print("Dew Point=");
        Serial.print(dew_point, 1);
        Serial.println(" C");
        Serial.print("Total rain=");
        Serial.print(rain, 1);
        Serial.println("mm");

        Serial.println(" C");
        Serial.print("Pressure=");
        Serial.print(pressure, 1);
        Serial.println(" hPa");

        Serial.println("%");
        Serial.print("Indoor Temperature DHT22=");
        Serial.print(DHT22_Temp, 1);
        Serial.println("C");
        Serial.print("Battery Status=");
        Serial.print(Bat);
        Serial.println(" ");
        Serial.flush();
      }
      // Print raw data to serial port
      Serial.print(device_id);
      Serial.print(" ");
      Serial.print(TempC, 1);
      Serial.print (" ");
      Serial.print(humidity);
      Serial.print(" ");
      Serial.print(wind_avg_ms);
      Serial.print(" ");
      Serial.print(wind_gust_ms);
      Serial.print(" ");
      Serial.print(direction_str);
      Serial.print(" ");
      Serial.print(wind_chill, 1);
      Serial.print(" ");
      Serial.print(dew_point, 1);
      Serial.print(" ");
      Serial.print(rain, 1);
      Serial.print(" ");
      Serial.print(pressure, 1);
      Serial.print(" ");
      Serial.print(DHT22_Hum, 1);
      Serial.print(" ");
      Serial.print(DHT22_Temp, 1);
      Serial.print(" ");
      Serial.println(Bat);

    }
  }
}
