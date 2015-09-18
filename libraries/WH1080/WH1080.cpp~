#include "WH1080.h"
WH1080::WH1080(void){
}

uint8_t WH1080::_crc8( uint8_t *addr, uint8_t len){
/*
* Function taken from Luc Small (http://lucsmall.com), itself
* derived from the OneWire Arduino library. Modifications to
* the polynomial according to Fine Offset's CRC8 calulations.
*/
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

bool WH1080::crcCheck(void){
	if (this->buffer_idx >= 10 && this->_crc8(this->byte_buffer, 9) == this->byte_buffer[9]) {
		Serial.println("");
		Serial.println("CRC Passed");
		return true;
	}
	return false;
}

void WH1080::process(void){
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
  this->now = micros();

  if (transition_t <= this->now)
    this->duration = this->now - transition_t;
  else
    this->duration = (~transition_t) + this->now;

  if (newVal != this->val) { // then transitioning state

    /*
     *  We update the transition time for the pulse, and
     *  change the current state of the input this->value.
     */
    transition_t = this->now;
    this->val = newVal;

    /*
     *  If the pulse width (hi or low) is outside the
     *  range of the Fine Offset signal, then ignore them.
     */
    if (this->duration < (SHORT_PULSE - SHORT_MARGIN) || this->duration > (LONG_PULSE + LONG_MARGIN)) {
      // Meaningless pulse
      //Serial.println("Meaningles Pluse");
      return;
    }

    /*
     *  If we reach here, then we have seen a potentially
     *  this->valid pulse. Shift the bit into the register.
     */
    if (newVal == 1) {
      // rising edge of a pulse (0 -> 1)
    } else {
      // falling edge of a pulse (1 -> 0)
      if ( this->duration >= (SHORT_PULSE - SHORT_MARGIN) && this->duration <= (SHORT_PULSE + SHORT_MARGIN) ) {
        // short pulse is binary '1'
        this->shift_register = (this->shift_register << 1) | 0x01;
        this->bit_count++;

      } else if (this->duration >= (LONG_PULSE - LONG_MARGIN) && this->duration <= (LONG_PULSE + LONG_MARGIN)) {
        // long pulse is binary '0'
        this->shift_register = (this->shift_register << 1);
        this->bit_count++;

      }
    }

    // Look for signature of 0xfa (4 bits 0xf0 pre-amble + 0xa)
    if ((this->shift_register & 0xff) == 0xfa && this->buffer_idx == 0) {
      // Found signature - discard pre-amble and leave 0x0a.
      this->shift_register = 0x0a;
      this->bit_count = 4;
      this->sig_seen = 1;  // Flag that the signature has been seen.


    } else if (this->bit_count == 8 && this->sig_seen) {
      // Got a byte, so store it if we have room.
      if (this->buffer_idx < BUFFER_SIZE) {
        byte_buffer[this->buffer_idx++] = (byte)(this->shift_register & 0xff);
      } else {
#ifdef DEBUG
        Serial.println("Buffer Undersize");
#endif
      }

      this->shift_register = 0;
      this->bit_count = 0;
    }

  } else {

    /*
     *  Have we reached timeout on this->duration? If so, process any
     *  bytes present in the buffer and then reset the state
     *  variables.
     */
    if (this->duration > 5000) {

      if (this->buffer_idx > 0) {
        /*
         *  If we have enough bytes, then verify the checksum.
         */
       if (crcCheck()) {
          this->decode();
        }

        this->buffer_idx = 0;
      }

      this->shift_register = 0;
      this->bit_count = 0;
      this->sig_seen = 0;
    }

  }
}

void WH1080::decode(void){
/*
          Device ID is nibbles 'abc'
          Need to take 'ab' and shift left 4 bits
          */
          this->_deviceID = byte_buffer[0];
          this->_deviceID = this->_deviceID << 4;

          // this->now need to add the high 4 bits of [1] (c)
          this->_deviceID = this->_deviceID + (byte_buffer[1] >> 4);

          /*
          Temp is stored in nibbles 'def'
          So we need to right shift off the 4 bit in element [2]
          Store it in t, right shift the this->value by 8 bits and add
          element [3]
          subtracting 400 (0x190) should give us an interger this->value for temp
          0x20e - 0×190 = 0x7e
          */
          this->_temperature = (byte_buffer[1] & 0x0f);
          this->_temperature = this->_temperature * 256;
          this->_temperature = this->_temperature + byte_buffer[2];
          this->_temperature = this->_temperature - 400;

          /*
          Humidity is Stored in nibbles 'gh'
          */
          this->_humidity = byte_buffer[3];

          /*
          Wind speed is Stored in nibbles 'ij'
          */;
          this->_windSpeed = byte_buffer[4];

	  /*
          Wind Gust speed is Stored in nibbles 'kl'
          */
          this->_windGustSpeed = byte_buffer[5];

          /*
            Wind Direction is Stored in nibble 'r'
            */
          this->_windDirection = (byte_buffer[8] & 0x0f);

          /*
          Rainfall count is nibble 'nop'
          */
          // extract low nibble from 'n'
	this->_rainFallCounter = (byte_buffer[6] & 0x0f);
          // Shift to high byte
          this->_rainFallCounter = this->_rainFallCounter * 256;
          // add 'op' as low byte
          this->_rainFallCounter = this->_rainFallCounter + byte_buffer[7];
          // Rainfall is a rolling counter and will reset after 0xfff

}


void WH1080::test (void){
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
            m: unkthis->nown
            n: rainfall counter high nibble
            op: rainfall counter
            q: battery-low   indicator
            r: wind direction
            st: checksum
          */
	byte test_data[] = {0xa1, 0x82, 0x0e, 0x5d, 0x02, 0x04, 0x00, 0x4e, 0x06, 0x86};
	for (int l = 0; l < BUFFER_SIZE; l++){
		byte_buffer[l] = test_data[l];	
	}
	this->decode();

}

int WH1080::getDeviceID(void){
	return this->_deviceID;
}

int WH1080::getTemperature(void){
	return this->_temperature;
}

byte WH1080::getHumidity(void){
	return this->_humidity;
}

int WH1080::getWindSpeed(void){
	return this->_windSpeed;
}

int WH1080::getWindGustSpeed(void){
	return this->_windGustSpeed;
}

int WH1080::getWindDirection(void){
	return this->_windDirection;
}
int WH1080::getRainFallCounter(void){
	return this->_rainFallCounter;
}


