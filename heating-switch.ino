#include <SPI.h>
#include <EthernetENC.h>
#include <LowPower.h>
#include "types.h"


EthernetServer server(80);

const int HEATING_PIN = 9;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Heating switch server");

  pinMode(HEATING_PIN, OUTPUT);
  digitalWrite(HEATING_PIN, HIGH);

  Ethernet.setHostname("casa");
  byte mac[] = {
    0xDE, 0xAD, 0xBE, 0x9A, 0x49, 0x29
  };
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
  Serial.print("New client: ");
  switch ( parse(client) ) {
    case Action::UnknownPath:
      Serial.print("Unknown path");
      client.println("HTTP/1.1 404 Not Found");
      client.println();
      break;
    case Action::Get:
      Serial.print("GET");
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      // client.println("Connection: close");  // the connection will be closed after completion of the response
      // client.println("Refresh: 5");  // refresh the page automatically every 5 sec
      client.println();
      client.print("<!DOCTYPE HTML>\n<html>\n<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /></head>\n<body>\nHeating is ");
      client.print(digitalRead(HEATING_PIN) ? "on" : "off");
      client.println("<br />\n<form method=\"post\"><input type=\"submit\" value=\"Switch\"></form>\n</body>\n</html>");
      break;
    case Action::Post:
      Serial.print("POST");
      digitalWrite(HEATING_PIN, !digitalRead(HEATING_PIN));
      client.println("HTTP/1.1 303 See Other");
      client.println("Location: /");
      client.println();
      break;
    case Action::Ignore:
      break;
  }
  client.stop();
  Serial.println(". Client disconnected");
}

void readUntilEmptyLine(EthernetClient& client, bool verbose = false) {
  bool currentLineIsBlank = true;
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      if (verbose)
        Serial.write(c);
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

Action parse(EthernetClient& client) {
  // parse method
  Action res;
  char buf[5];
  size_t n_read = client.readBytesUntil(' ', buf, 5);
  if (n_read == 3 && strncmp(buf, "GET", 3) == 0) {
    res = Action::Get;
  } else if (n_read == 4 && strncmp(buf, "POST", 4) == 0) {
    res = Action::Post;
  } else {
    Serial.println("Cannot parse:");
    for (size_t i = 0; i < n_read; ++i)
      Serial.print(buf[i]);
    readUntilEmptyLine(client, true);
    return Action::Ignore;
  }

  // parse path
  if (!(client.read() == '/' && client.read() == ' ')) {
    return Action::UnknownPath;
  }

  return res;
}
