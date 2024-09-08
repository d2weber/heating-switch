#include <SPI.h>
#include <EthernetENC.h>
#include <LowPower.h>
#include "types.h"


EthernetServer server(80);

const int HEATING_PIN = 9;

void setup() {Serial.begin(9600);
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


  Method method = parseMethod(client);

  if (method == Method::UNKNOWN) {
    return;
  }

  if (!(client.read() == '/' && client.read() == ' ')) {
    Serial.println("Unknown path");
    client.println("HTTP/1.1 404 Not Found");
    client.println();
  } else if (method == Method::GET) {
    Serial.println("GET");
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");  // the connection will be closed after completion of the response
    // client.println("Refresh: 5");  // refresh the page automatically every 5 sec
    client.println();
    client.print("<!DOCTYPE HTML>\n<html>\n<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /></head>\n<body>\nHeating is ");
    client.print(digitalRead(HEATING_PIN) ? "on" : "off");
    client.println("<br />\n<form method=\"post\"><input type=\"submit\" value=\"Switch\"></form>\n</body>\n</html>");
  } else if (method == Method::POST) {
    Serial.println("POST");
    digitalWrite(HEATING_PIN, !digitalRead(HEATING_PIN));
    client.println("HTTP/1.1 303 See Other");
    client.println("Location: /");
    client.println();
  }

  client.stop();
  Serial.println("Client disconnected");
}

Method parseMethod(EthernetClient& client) {
  char buf[5];
  switch (client.readBytesUntil(' ', buf, 5)) {
    case 3:
      if (strncmp(buf, "GET", 3) == 0) {
        return Method::GET;
      }
      break;
    case 4:
      if (strncmp(buf, "POST", 4) == 0) {
        return Method::POST;
      }
      break;
    case 0:
      Serial.println("readBytesUntil was 0");
      // fall through
    default:
      Serial.print("Unhandled method.");
  }
  return Method::UNKNOWN;
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