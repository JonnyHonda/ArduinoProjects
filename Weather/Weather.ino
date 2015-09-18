int rainPin = 2;
int anemometerPin = 3;
volatile int rainCounter = 0;
volatile int anemometerCounter = 0;
int interval = 1000;
unsigned long t = 0;
int anemometerValue;
int x = 0;
float windAverage = 0;

void setup() {
    attachInterrupt(digitalPinToInterrupt(rainPin), incrementRainCounter, CHANGE);
    attachInterrupt(digitalPinToInterrupt(anemometerPin), incrementAnemometer, RISING);
}

void loop() {
  if (millis() > interval + t){
    anemometerValue += anemometerCounter;
    anemometerCounter = 0;    
      if (x == 30){
        x = 0;
        windAverage = (float)anemometerValue / 30;        
      }
    }
  
    t = millis();
}

void incrementRainCounter() {
  // Do rain guage increments here
    rainCounter++;
}

void incrementAnemometer(){
  //Do wind speed calulations here
  anemometerCounter++;
}

