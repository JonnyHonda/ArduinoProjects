
#define SHORT_PULSE  500
#define LONG_PULSE  1500
#define SHORT_MARGIN 300
#define LONG_MARGIN 300
//#define DEBUG 1
int val = 0;
unsigned long transition_t = micros();
unsigned long now, duration;


#define BUFFER_SIZE  16

byte byte_buffer[BUFFER_SIZE];
byte buffer_idx = 0;

byte sig_seen = 0;

unsigned short shift_register = 0;
byte bit_count = 0;


char *direction_name[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
/*
* Function taken from Luc Small (http://lucsmall.com), itself
* derived from the OneWire Arduino library. Modifications to
* the polynomial according to Fine Offset's CRC8 calulations.
*/
uint8_t _crc8( uint8_t *addr, uint8_t len)
{
  uint8_t crc = 0;

  // Indicated changes are from reference CRC-8 function in OneWire library
  while (len--) {
    uint8_t inbyte = *addr++;
    uint8_t i;
    for (i = 8; i; i--) {
      uint8_t mix = (crc ^ inbyte) & 0x80; // changed from & 0x01
      crc <<= 1; // changed from right shift
      if (mix) crc ^= 0x31;// changed from 0x8C;
      inbyte <<= 1; // changed from right shift
    }
  }
  return crc;
}

void setup() {

  // put your setup code here, to run once:
  pinMode(2, INPUT);
  pinMode(13, OUTPUT);
  //Bridge.begin();
  Serial.begin(115200);

  digitalWrite(7, LOW);   // sets the LED off
  // prints title with ending line break
  Serial.println("WH1080RF Monitor");
  Serial.flush();
}

void loop() {

  // put your main code here, to run repeatedly:

  int newVal;
  for (int i = 0; i < 10; i++) {
    newVal += digitalRead(2) ? 1 : 0;
    delayMicroseconds(5);
  }
  newVal = (newVal + 5) / 10;

  /*
   * Handle situations where the clock has rolled over
   * between transitions (happens every ~70 mins).
   */
  now = micros();

  if (transition_t <= now)
    duration = now - transition_t;
  else
    duration = (~transition_t) + now;

  if (newVal != val) { // then transitioning state

    /*
     *  We update the transition time for the pulse, and
     *  change the current state of the input value.
     */
    transition_t = now;
    val = newVal;

    /*
     *  If the pulse width (hi or low) is outside the
     *  range of the Fine Offset signal, then ignore them.
     */
    if (duration < (SHORT_PULSE - SHORT_MARGIN) || duration > (LONG_PULSE + LONG_MARGIN)) {
      // Meaningless pulse
      //Serial.println("Meaningles Pluse");
      return;
    }

    /*
     *  If we reach here, then we have seen a potentially
     *  valid pulse. Shift the bit into the register.
     */
    if (newVal == 1) {
      // rising edge of a pulse (0 -> 1)
    } else {
      // falling edge of a pulse (1 -> 0)
      if ( duration >= (SHORT_PULSE - SHORT_MARGIN) && duration <= (SHORT_PULSE + SHORT_MARGIN) ) {
        // short pulse is binary '1'
        shift_register = (shift_register << 1) | 0x01;
        bit_count++;

      } else if (duration >= (LONG_PULSE - LONG_MARGIN) && duration <= (LONG_PULSE + LONG_MARGIN)) {
        // long pulse is binary '0'
        shift_register = (shift_register << 1);
        bit_count++;

      }
    }

    // Look for signature of 0xfa (4 bits 0xf0 pre-amble + 0xa)
    if ((shift_register & 0xff) == 0xfa && buffer_idx == 0) {
      // Found signature - discard pre-amble and leave 0x0a.
      shift_register = 0x0a;
      bit_count = 4;
      sig_seen = 1;  // Flag that the signature has been seen.


    } else if (bit_count == 8 && sig_seen) {
      // Got a byte, so store it if we have room.
      if (buffer_idx < BUFFER_SIZE) {
        byte_buffer[buffer_idx++] = (byte)(shift_register & 0xff);
      } else {
#ifdef DEBUG
        Serial.println("Buffer Undersize");
#endif
      }

      shift_register = 0;
      bit_count = 0;
    }

  } else {

    /*
     *  Have we reached timeout on duration? If so, process any
     *  bytes present in the buffer and then reset the state
     *  variables.
     */
    if (duration > 5000) {

      if (buffer_idx > 0) {
        /*
         *  If we have enough bytes, then verify the checksum.
         */
        if (buffer_idx >= 10 && _crc8(byte_buffer, 9) == byte_buffer[9]) {
          Serial.println("");
          Serial.println("CRC Passed");
          /*
          *  Dump the bytes to the Serial.
          */
          Serial.print("10 Bytes Found ");
          Serial.println(buffer_idx);

          for (int i = 0; i < buffer_idx; i++) {
            //        for (byte mask = 0x80; mask; mask >>= 1) {
            //          Serial.print(mask & byte_buffer[i] ? '1' : '0');
            //        }
            //        Serial.print(' ');
            //        Serial.print(byte_buffer[i]);
            Serial.print(' ');
            Serial.print(byte_buffer[i], HEX);
          }
          Serial.println("");
          /*
          Data Sample
          Byte     0  1  2  3  4  5  6  7  8  9
          Nibble  ab cd ef gh ij kl mn op qr st
          Example a1 82 0e 5d 02 04 00 4e 06 86

            abc: device identifier
            def: temperature
            gh: humidity
            ij: average wind speed low byte
            kl: gust wind speed low byte
            m: unknown
            n: rainfall counter high nibble
            op: rainfall counter
            q: battery-low   indicator
            r: wind direction
            st: checksum
          */


          /*
          Device ID is nibbles 'abc'
          Need to take 'ab' and shift left 4 bits
          */
          int deviceID = byte_buffer[0];
          deviceID = deviceID << 4;

          // now need to add the high 4 bits of [1] (c)
          deviceID = deviceID + (byte_buffer[1] >> 4);
          //    if (deviceID == 2582){
          // Outpout to Pin 13
          digitalWrite(13, HIGH);   // sets the LED on
          Serial.print("Device id           = ");
          Serial.println(deviceID, HEX);
          /*
          Temp is stored in nibbles 'def'
          So we need to right shift off the 4 bit in element [2]
          Store it in t, right shift the value by 8 bits and add
          element [3]
          subtracting 400 (0x190) should give us an interger value for temp
          0x20e - 0×190 = 0x7e
          */
          int t = (byte_buffer[1] & 0x0f);
          t = t * 256;
          t = t + byte_buffer[2];
          t = t - 400;
          float tempC = t / 10.0;
          Serial.print("Outdoor Temperature = ");
          Serial.print(tempC);
          Serial.print((char)223);
          Serial.println("c");

          /*
          Humidity is Stored in nibbles 'gh'
          */
          int humidity;
          humidity = byte_buffer[3];
          Serial.print("Outdoor Humidity    = ");
          Serial.print(humidity);
          Serial.println("%");

          /*
          Wind speed is Stored in nibbles 'ij'
          */
          int windAvg;
          windAvg = byte_buffer[4];
          Serial.print("Avg Wind Speed      = ");
          Serial.print(windAvg * 0.34);
          Serial.println("m/s");

          /*
          Wind Gust speed is Stored in nibbles 'kl'
          */
          int windGust;
          windGust = byte_buffer[5];
          Serial.print("Avg Gust Speed      = ");
          Serial.print(windGust * 0.34);
          Serial.println("m/s");

          /*
            Wind Direction is Stored in nibble 'r'
            */
          int windDirection;
          // fetch the low nibble
          windDirection = (byte_buffer[8] & 0x0f);
          Serial.print("Wind Direction      = ");
          Serial.println(direction_name[windDirection]);

          /*
          Rainfall count is nibble 'nop'
          */
          // extract low nibble from 'n'
          int rainfall = (byte_buffer[6] & 0x0f);
          // Shift to high byte
          rainfall = rainfall * 256;
          // add 'op' as low byte
          rainfall = rainfall + byte_buffer[7];
          // Rainfall is a rolling counter and will reset after 0xfff
          Serial.print("Rainfall counter   = ");
          Serial.println(rainfall);

          delay(750);                  // waits for a 3/4 second
          digitalWrite(13, LOW);    // sets the LED off
          //    }
        }

        buffer_idx = 0;
      }

      shift_register = 0;
      bit_count = 0;
      sig_seen = 0;
    }

  }
}  // end loop()
