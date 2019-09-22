#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ESP8266SSDP.h>

const char* host = "Irrigation Controller";
const char* ssid = "Morpheus";
const char* password = "b5eea63f65";
const char* mqtt_user = "CDW-SmartHouse";
const char* mqtt_pass = "!M0rpheus";

#define mqtt_server "192.168.0.3"
#define openblind_topic "binds/kitchen/open"
#define closeblind_topic "blinds/kitchen/close"

//This can be used to output the date the code was compiled
const char compile_date[] = __DATE__ " " __TIME__;

//Define Pins
const int OPEN_BUTTON = 5; //D1
const int CLOSE_BUTTON = 4; //D2
const int TOP_LIMIT_SWITCH = 14; //D5
const int BOTTOM_LIMIT_SWITCH = 12; //D6
const int MOTOR_PLUS = ?; //D?
const int MOTOR_MINUS = ?; //D?

String strTopic;
String strPayload;

//Initialize Objects
WiFiClient espKitchenBlindController;
PubSubClient client(espKitchenBlindController);

void setup() {
  Serial.begin(115200);

  pinMode(OPEN_BUTTON, INPUT);
  pinMode(CLOSE_BUTTON, INPUT);
  pinMode(TOP_LIMIT_SWITCH, INPUT);
  pinMode(BOTTOM_LIMIT_SWITCH, INPUT);
  pinMode(MOTOR_PLUS, OUTPUT);
  pinMode(MOTOR_MINUS, OUTPUT);
  
  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback); //callback is the function that gets called for a topic sub

  ArduinoOTA.setHostname("Kitchen Blind Controller");
  ArduinoOTA.begin();
}

void loop() {
  //If MQTT client can't connect to broker, then reconnect
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); //the mqtt function that processes MQTT messages
  ArduinoOTA.handle();
}

void callback(char* topic, byte* payload, unsigned int length) {
  //Handle mqtt messages received by this NodeMCU
  payload[length] = '\0';
  strTopic = String((char*)topic);
Serial.println("Topic " + strTopic);
  String command = String((char*)payload);
  Serial.println("Command " + command);
  if (strTopic == pooltopup_topic) {

    if (command == "ON") {
      digitalWrite(SWIMMING_POOL, LOW);
    } else {
      digitalWrite(SWIMMING_POOL, HIGH);
    }
  }
  else if ( strTopic ==  zone1_topic) {
    if (command == "ON") {
      digitalWrite(ZONE_1, LOW);
    } else {
      digitalWrite(ZONE_1, HIGH);
    }
  }
  else if ( strTopic ==  zone2_topic) {
    if (command == "ON") {
      digitalWrite(ZONE_2, LOW);
    } else {
      digitalWrite(ZONE_2, HIGH);
    }
  }
  else if ( strTopic ==  zone3_topic ) {
    if (command == "ON") {
      digitalWrite(ZONE_3, LOW);
    } else {
      digitalWrite(ZONE_3, HIGH);
    }
  }
}

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC: ");
  Serial.println(WiFi.macAddress());
}

void reconnect() {
  //Reconnect to Wifi and to MQTT. If Wifi is already connected, then autoconnect doesn't do anything.
  Serial.print("Attempting MQTT connection...");
  if (client.connect(host, mqtt_user, mqtt_pass)) {
    Serial.println("connected");
    client.subscribe("irrigation/controller/#");
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    delay(5000);
  }
}
