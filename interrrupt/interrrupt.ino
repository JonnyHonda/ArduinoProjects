#define RF_IN 2
int pin = 13;
volatile int state = LOW;
volatile byte got_interval = 0;
volatile byte interval = 0;
byte buffer[20];
int bufferCount = 0;


void setup()
{
  Serial.begin(115200);
  pinMode(pin, OUTPUT);
  attachInterrupt(0, blink, CHANGE);
}

void loop()
{
  digitalWrite(pin, state);
  if (bufferCount == 19){
   for (int l=0; l <=19; l++){
    Serial.print(buffer[l]);
   }
   Serial.println();
  }
}

void blink()
{
  static byte count = 0;
  static byte was_hi = 0;
  
  
  if (digitalRead(RF_IN) == HIGH) {
  //  digitalWrite(LED_ACTIVITY, HIGH);
    count++;
    was_hi = 1; 
  } else {
 //   digitalWrite(LED_ACTIVITY, LOW);
    if (was_hi) {
      was_hi = 0;
      interval = count;
      got_interval = 1;
      count = 0;
    }
  }
  
}
