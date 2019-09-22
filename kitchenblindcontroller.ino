#include <Bounce2.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ESP8266SSDP.h>

const char* host = "Kitchen Blind Controller";
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
const int OPEN_BUTTON = 0; //D3
const int CLOSE_BUTTON = 2; //D4
const int TOP_LIMIT_SWITCH = 4; //D2
const int BOTTOM_LIMIT_SWITCH = 5; //D1
const int IN_1 = 14; //D5
const int IN_2 = 12; //D6

String strTopic;
String strPayload;

//Initialize Objects
WiFiClient espKitchenBlindController;
PubSubClient client(espKitchenBlindController);
Bounce  bouncer1  = Bounce();
Bounce  bouncer2  = Bounce();
Bounce  bouncer3  = Bounce();
Bounce  bouncer4  = Bounce();

void setup() {
  Serial.begin(115200);

  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(OPEN_BUTTON, INPUT);
  pinMode(CLOSE_BUTTON, INPUT);
  pinMode(TOP_LIMIT_SWITCH, INPUT);
  pinMode(BOTTOM_LIMIT_SWITCH, INPUT);

  bouncer1.attach(OPEN_BUTTON);
  bouncer2.attach(CLOSE_BUTTON);
  bouncer3.attach(TOP_LIMIT_SWITCH);
  bouncer4.attach(BOTTOM_LIMIT_SWITCH);
  
  bouncer1.interval(10);
  bouncer2.interval(10);
  bouncer3.interval(10);
  bouncer4.interval(10);
  
  // Set initial rotation direction
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, LOW);

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback); //callback is the function that gets called for a topic sub

  ArduinoOTA.setHostname("Kitchen Blind Controller");
  ArduinoOTA.begin();
}

void loop() {
  //If MQTT client can't connect to broker, then reconnect
  if (!client.connected()) {
    Serial.println("Client not connected for some reason");
    reconnect();
  }
  client.loop(); //the mqtt function that processes MQTT messages
  ArduinoOTA.handle();
  buttonPressHandler();
  limitSwitchHandler();
}

void buttonPressHandler() {
  if (digitalRead(OPEN_BUTTON) == LOW) {
    Serial.println("Closing the blind");
    openBlind();
  }

  if (digitalRead(CLOSE_BUTTON) == LOW) {
    Serial.println("Opening the blind");
    closeBlind();
  }
}

void limitSwitchHandler() {
  if (digitalRead(TOP_LIMIT_SWITCH) == LOW) {
    Serial.println("Top limit switch pressed");
    stopBlind();
  }

  if (digitalRead(BOTTOM_LIMIT_SWITCH) == LOW) {
    Serial.println("Bottom limit switch pressed");
    stopBlind();
  }
}

void stopBlind() {
//  digitalWrite(EN_A, HIGH);
    digitalWrite(IN_1, LOW);
    digitalWrite(IN_2, LOW);
}

void openBlind() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
}

void closeBlind() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
}

void callback(char* topic, byte* payload, unsigned int length) {
  //Handle mqtt messages received by this NodeMCU
  payload[length] = '\0';
  strTopic = String((char*)topic);
  Serial.println("Topic " + strTopic);
  String command = String((char*)payload);
  Serial.println("Command " + command);
//  if (strTopic == pooltopup_topic) {
//
//    if (command == "OPEN") {
//      digitalWrite(SWIMMING_POOL, LOW);
//    } else {
//      digitalWrite(SWIMMING_POOL, HIGH);
//    }
//  }
//  else if ( strTopic ==  zone1_topic) {
//    if (command == "ON") {
//      digitalWrite(ZONE_1, LOW);
//    } else {
//      digitalWrite(ZONE_1, HIGH);
//    }
//  }
//  else if ( strTopic ==  zone2_topic) {
//    if (command == "ON") {
//      digitalWrite(ZONE_2, LOW);
//    } else {
//      digitalWrite(ZONE_2, HIGH);
//    }
//  }
//  else if ( strTopic ==  zone3_topic ) {
//    if (command == "ON") {
//      digitalWrite(ZONE_3, LOW);
//    } else {
//      digitalWrite(ZONE_3, HIGH);
//    }
//  }
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
    client.subscribe("kitchen/blind/controller/#");
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    delay(5000);
  }
}
