byte byte_buffer[10];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // put your main code here, to run repeatedly:
  byte_buffer[0] = 0xa1;
  byte_buffer[1] = 0x62;
  byte_buffer[2] = 0x18;
  byte_buffer[3] = 0x41;
  byte_buffer[4] = 0x0e;
  byte_buffer[5] = 0x16;
  byte_buffer[6] = 0x20;
  byte_buffer[7] = 0x74;
  byte_buffer[8] = 0x00;
  byte_buffer[9] = 0xb9;

  for (int i = 0; i < 10; i++) {
    Serial.print(' ');
    Serial.print(byte_buffer[i], HEX);
  }
  Serial.println("");
  Serial.println("-----------");

  int t = (byte_buffer[1] & 0x0f);

  t = t * 256;
  t = t + byte_buffer[2];
  t = t - 400;
  Serial.println (t);
  Serial.println("-----------");
}

void loop() {

}
