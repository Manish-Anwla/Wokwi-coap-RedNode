#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include "DHTesp.h"

#define DHT_PIN 18

const char* ssid = "Wokwi-GUEST";
const char* password = "";

IPAddress server(172, 26, 1, 212);   // IP address of container where coap(libcoap) server is running
int serverPort = 5683;               // port no

WiFiUDP udp;
Coap coap(udp);
DHTesp dht;

// ✅ Callback to handle server response
void callback_response(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("📩 Response received from server:");
  Serial.print("From "); Serial.print(ip); Serial.print(":"); Serial.println(port);

  // Print raw payload
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = '\0';

  Serial.print("➡ Payload: ");
  Serial.println(p);

  Serial.print("➡ Code: ");
  Serial.println(packet.code);

  Serial.print("➡ Message ID: ");
  Serial.println(packet.messageid);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("CoAP DHT sensor data sender");

  // Initialize DHT sensor
  dht.setup(DHT_PIN, DHTesp::DHT22);
  Serial.println("DHT22 sensor initialized");

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { 
    delay(300); 
    Serial.print("."); 
  }
  Serial.println();
  Serial.println("ESP32 IP: " + WiFi.localIP().toString());

  // Initialize UDP socket BEFORE starting CoAP
  udp.begin(33333);  
  
  // Initialize CoAP client with response callback
  coap.response(callback_response);
  coap.start();
  
  Serial.println("CoAP client ready");
}

void loop() {
  TempAndHumidity data = dht.getTempAndHumidity();

  if (!isnan(data.temperature) && !isnan(data.humidity)) {
    String payload = "{\"temperature\":" + String(data.temperature, 2) +
                     ",\"humidity\":" + String(data.humidity, 2) +
                     ",\"device\":\"esp32_wokwi\",\"timestamp\":" + String(millis()) + "}";

    char buffer[200];
    payload.toCharArray(buffer, sizeof(buffer));

    Serial.println("📦 Payload: " + payload);

    // Send PUT request
    int msgId = coap.put(server, serverPort, "example_data", buffer);
    Serial.println("📤 CoAP PUT sent, Message ID: " + String(msgId));

    delay(10);        // give UDP stack time to flush
    coap.loop();      // process incoming responses
  } else {
    Serial.println("❌ DHT22 sensor error");
  }

  delay(5000);
}
