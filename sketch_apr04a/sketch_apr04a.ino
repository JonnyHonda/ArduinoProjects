/**
** WH1080 decoder for OOK devices
** Compiles with Arduino 1.0.1  
** M Baldwin March 2013
**
** Resources used
** http://www.susa.net/wordpress/2012/08/raspberry-pi-reading-wh1081-weather-sensors-using-an-rfm01-and-rfm12b/
** http://www.sevenwatt.com/main/wh1080-protocol-v2-fsk/
** http://lucsmall.com/2012/04/27/weather-station-hacking-part-1/
**
** Future updates
** Add wind chill - unless I add it to LUA script
** Reduce packet sent via serial for inclusion in HAH
** Add this code to existing CC decoder combining OOK receiver and RFM12b @ 433MHz
*/

#define PIN_433 A0 // AIO1 = 433 MHz receiver

volatile word pulse_433;
volatile unsigned long old = 0, packet_count = 0; 
volatile unsigned long spacing, average_interval;
word last_433; // never accessed outside ISR's
volatile word pulse_count = 0; //
byte packet[10];
byte state = 0;
bool packet_acquired = false;
char *direction_name[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
    

// State to track pulse durations measured in the interrupt code
ISR(PCINT1_vect)
  {
    pulse_433 = 0;
    if (digitalRead(PIN_433) == LOW)
      {
        word ticker = micros();
        pulse_433 = ticker - last_433;
        last_433 = ticker;
      }
    if (pulse_433 > 3000)
      {
        state = 0;// reset data gathering
      }
  }

void setup()
  {
    Serial.begin(57600);
    Serial.println();
    Serial.println("WH1080 433MHz OOK decoder");
  
    pinMode(PIN_433, INPUT);
    digitalWrite(PIN_433, 1);   // pull-up

    // interrupt on pin change
    bitSet(PCMSK1, PIN_433 - 14);
    bitSet(PCICR, PCIE1);
  
    // enable interrupts
    sei();
    Serial.println("Interrupts set up");
  }

void loop()
  {
    byte i;
    unsigned long now;
    //byte *packet;
  
    if (pulse_433)
      {
        cli();//disable interrupts and store pulse width
        word pulse = pulse_433;
        pulse_433 = 0;
        sei();//re-enable interrupts
      
        extractData(pulse);//extract data from pulse stream
      
        if (state == 3)//pulse stream complete
          {
            state = 4;
            now = millis();
            spacing = now - old;
            old = now;
            packet_count ++;
            average_interval = now / packet_count;     
         
            Serial.print("Packet count: ");
            Serial.println(packet_count, DEC);
            Serial.print("Spacing: ");
            Serial.println(spacing, DEC);
            Serial.print("Average spacing: ");
            Serial.println(average_interval, DEC);
     
            Serial.println("Packet Datastream: ");
            Serial.println("ab cd ef gh ij kl mn op qr st");
            for(i=0;i<=9;i++)
              {
                if ((int) packet[i]<16) Serial.print('0');
                Serial.print(packet[i], HEX);
                Serial.print(" ");
              }
              
            Serial.print("crc: ");
            Serial.print(calculate_crc(), HEX);
            Serial.println((valid_crc() ? " GOOD" : " BAD"));
  
            Serial.print("Sensor ID: 0x");
            Serial.println(get_sensor_id(), HEX);

            Serial.print("Humidity: ");
            Serial.print(get_humidity(), DEC);
            Serial.println("%");
      
            Serial.print("Average wind speed: ");
            Serial.print(get_avg_wind(), DEC);
            Serial.println("m/s");
      
            Serial.print("Wind gust speed: ");
            Serial.print(get_gust_wind(), DEC);
            Serial.println("m/s");
      
            Serial.print("Wind direction: ");
            //get_wind_direction();
            Serial.println(direction_name[get_wind_direction()]);
            
            Serial.print("Rainfall: ");
            Serial.print(get_rain(), DEC);
            Serial.println("mm");
      
            Serial.print("Temperature: ");
            Serial.println(get_temperature_formatted());
            Serial.println("--------------");
            state = 0;//reset ready for next stream
          }
      }
  }

static void extractData(word interval)
  {
    /*  state 0 == reset state 
        state 1 == looking for preamble
        state 2 == got preamble, load rest of packet
        state 3 == packet complete
    */
    bool sample;
    byte preamble;
    byte preamble_check = B00011110;//check if we see the end of the preamble
    byte preamble_mask  = B00011111;//mask for preamble test (can be reduced if needed)
    static byte packet_no, bit_no, preamble_byte;
    if (interval >= 1400 && interval <= 1600)// 1 is indicated by 1500uS pulse
      {
        sample = 1;
      }
    else if (interval >= 2400  && interval <= 2600)// 0 is indicated by a 2500uS pulse
      {
        sample = 0;    
      }
    else
      {
        state = 0;//if interval not in range reset and start looking for preamble again
        return;
      } 
  
    if(state == 0)// reset
      {
        // reset stuff
        preamble_byte = 0; 
        packet[0] = packet[1] = packet[2] = packet[3] = packet[4] = packet[5] = packet[6] = packet[7] = packet[8] = packet[9] = 0;
        state = 1;
      } 
  
      
    if (state == 1)// acquire preamble
      {
        preamble_byte <<= 1;// shift preamble byte left
        preamble_byte |= sample;//OR with new bit
        preamble = preamble_byte & preamble_mask;
        if (preamble == preamble_check)// we've acquired the preamble
          {
            packet_no = 0;
            bit_no = 2;// start at 2 because we know the first two bits are 10
            packet[0] = packet[1] = packet[2] = packet[3] = packet[4] = packet[5] = packet[6] = packet[7] = packet[8] = packet[9] = 0;
            packet[0] <<= 1;
            packet[0] |= 1;// first byte after preamble contains 10 so add 1
            packet[0] <<= 1;
            packet[0] |= 0;// first byte after preamble contains 10 so add 0 
            state = 2;
          }
        return; //not got the preamble yet so return
      }
  
    if (state == 2)// acquire packet
      {
        packet[packet_no] <<= 1;
        packet[packet_no] |= sample; //add bit to packet stream
      
        bit_no ++;
        if(bit_no > 7)
          {
            bit_no = 0;
            packet_no ++;
          }
    
        if (packet_no > 9)// got full packet stream
          {
            state = 3;
          }
      }
  }

byte calculate_crc()
  {
    return _crc8(packet, 9);
  }

bool valid_crc()
  {
    return (calculate_crc() == packet[9]);
  }

int get_sensor_id()
  {
    return (packet[0] << 4) + (packet[1] >> 4);
  }

byte get_humidity()
  {
    return packet[3];
  }

byte get_avg_wind()
  {
    return packet[4];
  }

byte get_gust_wind()
  {
    return packet[5];
  }

byte get_wind_direction()
  {
    int direction = packet[8] & 0x0f;
  }

byte get_rain()
  {
    return packet[7];
  }


/* Temperature in deci-degrees. e.g. 251 = 25.1 */
int get_temperature()
  {
    int temperature;
    temperature = ((packet[1] & B00000111) << 8) + packet[2];
    // make negative
    if (packet[1] & B00001000)
      {
        temperature = -temperature;
      }
    temperature -= 400;
    return temperature;
  }


String get_temperature_formatted()
  {
    int temperature;
    byte whole, partial;
    String s;
   
    temperature = ((packet[1] & B00000111) << 8) + packet[2];
    temperature -= 400;//temp offset by 400 points
    whole = temperature / 10;
    partial = temperature - (whole*10);
    s = String();
    if (packet[1] & B00001000)
      {
        s += String('-');
      }
    s += String(whole, DEC);
    s += '.';
    s += String(partial, DEC);
    return s;
  }


uint8_t _crc8( uint8_t *addr, uint8_t len)//nice crc snippet from OneWire library
  {
    uint8_t crc = 0;

    // Indicated changes are from reference CRC-8 function in OneWire library
    while (len--)
      {
        uint8_t inbyte = *addr++;
        for (uint8_t i = 8; i; i--)
          {
            uint8_t mix = (crc ^ inbyte) & 0x80; // changed from & 0x01
            crc <<= 1; // changed from right shift
            if (mix) crc ^= 0x31;// changed from 0x8C;
            inbyte <<= 1; // changed from right shift
          }
      }
    return crc;
  }
