#include <SPI.h>
#include <EthernetENC.h>
#include <LowPower.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0x9A, 0x49, 0x29
};
// IPAddress ip(192, 168, 178, 177);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

const int HEATING_PIN = 9;

void setup() {
  pinMode(HEATING_PIN, OUTPUT);
  digitalWrite(HEATING_PIN, HIGH);

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Ethernet WebServer Example");

  // start the Ethernet connection and the server:
  Ethernet.setHostname("casa");
  Ethernet.begin(mac);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    Serial.println(Ethernet.hardwareStatus());
    while (true) {
      delay(1);  // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}


void loop() {
  Ethernet.maintain();
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    handleClient(client);
  } else {
    Serial.flush();
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
  }
}


void handleClient(EthernetClient& client) {
  Serial.println("new client");

  String method = client.readStringUntil(' ');  // TODO: limit maximum read size

  String path = client.readStringUntil(' ');  // TODO: limit maximum read size

  readUntilEmptyLine(client);

  Serial.print("New request: ");
  Serial.print(method.c_str());
  Serial.print(" -> ");
  Serial.print(path.c_str());
  Serial.println();

  if (path != "/") {
    client.println("HTTP/1.1 404 Not Found");
    client.println();
  }

  if (method == "GET") {
    // send a standard HTTP response header
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");  // the connection will be closed after completion of the response
    // client.println("Refresh: 5");  // refresh the page automatically every 5 sec
    client.println();
    client.print("<!DOCTYPE HTML>\n<html>\n<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /></head>\n<body>\nHeating is ");
    client.print(digitalRead(HEATING_PIN) ? "on" : "off");
    client.println("<br />\n<form method=\"post\"><input type=\"submit\" value=\"Switch\"></form>\n</body>\n</html>");
  } else if (method == "POST") {
    digitalWrite(HEATING_PIN, !digitalRead(HEATING_PIN));
    client.println("HTTP/1.1 303 See Other");
    client.println("Location: /");
    client.println();
  }
  // give the web browser time to receive the data
  // delay(1);
  // close the connection:
  client.stop();
  Serial.println("client disconnected");
}

void readUntilEmptyLine(EthernetClient& client) {
  bool currentLineIsBlank = true;
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      // Serial.write(c);
      if (c == '\n' && currentLineIsBlank) {
        break;
      }
      if (c == '\n') {
        currentLineIsBlank = true;
      } else if (c != '\r') {
        currentLineIsBlank = false;
      }
    }
  }
}