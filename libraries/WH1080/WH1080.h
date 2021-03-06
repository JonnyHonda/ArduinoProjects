#include <inttypes.h>

#if ARDUINO >= 100
#include "Arduino.h"       // for delayMicroseconds, digitalPinToBitMask, etc
#else
#include "WProgram.h"      // for delayMicroseconds
#include "pins_arduino.h"  // for digitalPinToBitMask, etc
#endif

#define FALSE 0
#define TRUE  1

#define SHORT_PULSE  500
#define LONG_PULSE  1500
#define SHORT_MARGIN 300
#define LONG_MARGIN 300

#define BUFFER_SIZE  10

class WH1080{
  private:
	byte byte_buffer[BUFFER_SIZE];
	int _deviceID;
	int _temperature;
	byte _humidity;
	int _windSpeed;
	int _windGustSpeed;
	int _windDirection;
	int _rainFallCounter;

	byte buffer_idx = 0;
	byte sig_seen = 0;
	unsigned short shift_register = 0;
	byte bit_count = 0;
	int val = 0;
	unsigned long transition_t = micros();
	unsigned long now, duration;
	uint8_t _crc8( uint8_t *addr, uint8_t len);
	bool crcCheck(void);

  public:
	WH1080(void);
	void process(void);
	void test(void);

	void decode(void);
	int getDeviceID(void);
	int getTemperature(void);
	byte getHumidity(void);
	int getWindSpeed(void);
	int getWindGustSpeed(void);
	int getWindDirection(void);
	int getRainFallCounter(void);
};
