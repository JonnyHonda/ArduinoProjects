#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>


#include "./WH1080.h"
#define DEBUG 1
WH1080 station;
byte buffer[BUFFER_SIZE];
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 20);
// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);
unsigned long timer;
void setup() {
  Serial.begin(115200);
  Serial.print("Starting up Webserver...");
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.println(" ... Done.");
  
}

void loop() {
  // put your main code here, to run repeatedly:
  timer = millis();
  station.process();
  //station.test();

  EthernetClient client = server.available();
  if (client) {

    Serial.println("new client");

    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        Serial.write(c);

        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 90");  // refresh the page automatically every 3 Min
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          if (station.getDeviceID() == 0) {
              client.println("Still waiting for data from the weather station");
          } else {
            client.print("Station Id : "); client.print(station.getDeviceID(),HEX); client.println("<br>");
            client.print("Temperature : "); client.print((float)station.getTemperature()/10); client.println("<br>");
            client.print("Humidity : "); client.println(station.getHumidity()); client.println("<br>");
            client.print("Wind Speed : "); client.print(station.getWindSpeed()); client.println("<br>");
            client.print("Wind Gust Speed : "); client.print(station.getWindGustSpeed()); client.println("<br>");
            client.print("Wind Direction : "); client.print(station.getWindDirection()); client.println("<br>");
            client.print("Rain Fall Counter : "); client.print(station.getRainFallCounter()); client.println("<br>");
            client.print("Time");client.print(timer % 60);client.println("<br>");
            station.getRawData(buffer);
            for (int l = 0; l < BUFFER_SIZE; l++) {
              client.println(buffer[l], HEX);
            }
          }
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();

    Serial.println(" ");
    Serial.println("client disconnected");

  }

}
