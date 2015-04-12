#define CRC_POLY 0x31 // CRC-8 = 0x31 is for: x8 + x5 + x4 + 1
#define DEBUG 1


#define RF_PORT PORTB
#define RF_DDR DDRB
#define RF_PIN 2


int frame; 
int device_id; 
char sign;
int temperature;
float TempC;

void setup(){
  Serial.begin(9600);
  pinMode(2,INPUT);
}
void loop(){
  unsigned int frame[20];
  receive(frame);
  if (is_crc_valid(frame[9])){
    // Now convert the data      
    // Station ID
    device_id = ((frame[0] << 4) | (frame[1] >> 4));
    Serial.println(device_id,HEX);

    // Temperature outside 
      sign = (frame[1] >>3) & 1;
    temperature =  ((frame[1] & ~(0xff <<1)) << 8) | frame[2] ;
    if (sign) {
      TempC = ((float)(~temperature)+sign) / 10;
    }
    else {
      TempC = (float) temperature / 10;
    }
    
  }
  Serial.print("Outside Temperature = ");
  Serial.println( TempC);
  delay(2000);
}



// Calculate CRC
static uint8_t compute_crc(uint8_t b) {
  uint8_t do_xor;
  uint8_t reg;

  reg = 0;
  do_xor = (reg & 0x80);

  reg <<=1;
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
  } 
  else {
    if (DEBUG) {
      Serial.print("CRC Error... Calculated is "); 
      Serial.print(crc_computed, HEX);
      Serial.print(" Received is "); 
      Serial.println(crcByte, HEX);
    }

  }
  return result;
}


// ******** SPI + RFM 12B functs   ************************************************************************

unsigned short rf12_xfer(unsigned short value)
{	uint8_t i;

	for (i=0; i<16; i++)
	{	if (value&32768)

		value<<=1;
		if (RF_PIN&(1<<SDO))
			value|=1;

	}
	return value;
}


void rf12_ready(void)
{
while (!(RF_PIN&(1<<SDO))); // wait until FIFO ready

}

void rf12_rxdata(unsigned char *data, unsigned char number)
{	uint8_t  i;

	for (i=0; i<number; i++)
	{	//rf12_ready();
		*data++=rf12_xfer(0xB000);
	}

}

//Receive message, data[] 
void receive(unsigned int *data){  
  unsigned char mesage[20];

  if (DEBUG) { 
    Serial.println("Listening...."); 
  }

  rf12_rxdata(mesage,20);

  if (DEBUG) {
    Serial.print("Received HEX raw data: ");
    for (int i=0; i<20; i++) { 

      Serial.print(mesage[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }

  //Write message into table
  for (int i=0; i<20; i++) { 
    data[i] = (unsigned int) mesage[i];
  }
}





