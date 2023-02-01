// IMPORTS

// Arduino Json
#include <ArduinoJson.h>

// MQTT Client
#include <PubSubClient.h>

// WEMOS D1 Plattform
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

//DHT22 Library
#include <DHT.h>
// Adafruit unified library: Deps
#include <DHT_U.h>

// VARS
// DHT pin as D2
#define DHTPIN D7

// DHT type as 22
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// Rele
#define RELE D6
uint8_t state = LOW;
// MQTT

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <TZ.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>

// Update these with values suitable for your network.
const char* ssid = "Azir";
const char* password = "The iterations works haha";
const char* mqtt_server = "10d41b8b2c124dd7aba746a0ab363b7c.s2.eu.hivemq.cloud";

// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
BearSSL::CertStore certStore;

WiFiClientSecure espClient;
PubSubClient * client;
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (500)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void setDateTime() {
  // You can use your own timezone, but the exact time is not used at all.
  // Only the date is needed for validating the certificates.
  configTime(TZ_Europe_Berlin, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("%s %s", tzname[0], asctime(&timeinfo));
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String content = "";
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    content.concat((char)payload[i]);
  }
  Serial.println(content);

  if (content == "on") {
    state = HIGH;
    digitalWrite(RELE, state);
    delay(5000);
    Serial.print("True");
  }
  else if (content == "off") {
    state = LOW;
    digitalWrite(RELE, state);
    delay(5000);
    Serial.print("False");
  }
  // Switch on the LED if the first character is present
  if ((char)payload[0] != NULL) {
    digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
  } else {
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
  }
}


void reconnect() {
  // Loop until we’re reconnected
  while (!client->connected()) {
    Serial.print("Attempting MQTT connection…");
    String clientId = "ESP8266Client - MyClient";
    // Attempt to connect
    // Insert your password
    if (client->connect(clientId.c_str(), "futon", "kodokushi")) {
      Serial.println("connected");
      // Once connected, publish an announcement…
      //client->publish("bomb-state", "im here");
      // … and resubscribe
      client->subscribe("bomb-state");
    } else {
      Serial.print("failed, rc = ");
      Serial.print(client->state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  delay(500);
  // Rele status

  // When opening the Serial Monitor, select 9600 Baud
  Serial.begin(9600);
  delay(500);
  pinMode(RELE, OUTPUT);
  state = LOW;
  digitalWrite(RELE, state);

  // DHT sensor
  dht.begin();

  LittleFS.begin();
  setup_wifi();
  setDateTime();

  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output

  // you can use the insecure mode, when you want to avoid the certificates
  //espclient->setInsecure();

  int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  Serial.printf("Number of CA certs read: %d\n", numCerts);
  if (numCerts == 0) {
    Serial.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
    return; // Can't connect to anything w/o certs!
  }

  BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();
  // Integrate the cert store with this connection
  bear->setCertStore(&certStore);

  client = new PubSubClient(*bear);

  client->setServer(mqtt_server, 8883);
  client->setCallback(callback);
}

void bombState() {
  int temp = dht.readTemperature();  // obtencion de valor de temperatura
  int hum = dht.readHumidity();

  StaticJsonDocument<200> doc;
  doc["temperature"] = temp;
  doc["humidity"] = hum;
  serializeJson(doc, msg);
  delay(500);

  Serial.print(msg);
  
  
  
  if (temp > 34) {
    state = HIGH;
    digitalWrite(RELE, state);
    delay(5000);
    state = LOW;
    delay(5000);
    digitalWrite(RELE, state);
    client->publish("sensors", msg);
  }
  else if (temp <= 34) {
    state = LOW;
    digitalWrite(RELE, state);
    delay(5000);
    client->publish("sensors", msg);
  }

  
  
}
void loop() {
  if (!client->connected()) {
    reconnect();
  }
  client->loop();
  bombState();
}
