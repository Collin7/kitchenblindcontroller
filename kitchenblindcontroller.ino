#include <Bounce2.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ESP8266SSDP.h>
#include <SimpleTimer.h>

const char* host = "Kitchen Blind Controller";
const char* ssid = "SSID HERE";
const char* password = "WIFI PASSWORD HERE";
const char* mqtt_user = "MQTT USERNAME";
const char* mqtt_pass = "MQTT PASSWORD HERE";

#define mqtt_server "192.168.0.3"
#define operateblind_topic "blinds/kitchen/action"

//This can be used to output the date the code was compiled
const char compile_date[] = __DATE__ " " __TIME__;

//Define Pins
const int CLOSE_BUTTON = 0; //D3
const int OPEN_BUTTON = 2; //D4
const int TOP_LIMIT_SWITCH = 14; //D5
const int BOTTOM_LIMIT_SWITCH = 12; //D6
const int IN_1 = 5; //D1
const int IN_2 = 4; //D2

String strTopic;
String strPayload;
bool didPublishState = false;

//Initialize Objects
WiFiClient espKitchenBlindController;
PubSubClient client(espKitchenBlindController);
Bounce  bouncer1  = Bounce();
Bounce  bouncer2  = Bounce();
SimpleTimer timer;

void setup() {
  Serial.begin(115200);

  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(OPEN_BUTTON, INPUT);
  pinMode(CLOSE_BUTTON, INPUT);
  pinMode(TOP_LIMIT_SWITCH, INPUT_PULLUP);
  pinMode(BOTTOM_LIMIT_SWITCH, INPUT_PULLUP);

  bouncer1.attach(OPEN_BUTTON);
  bouncer2.attach(CLOSE_BUTTON);

  bouncer1.interval(5);
  bouncer2.interval(5);

  // Set initial rotation direction
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, LOW);

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback); //callback is the function that gets called for a topic sub

  ArduinoOTA.setHostname("Kitchen Blind Controller");
  ArduinoOTA.begin();

  timer.setInterval(90000, stateCheckin); //publish blind state every 1 min
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
  timer.run();
}

void buttonPressHandler() {
  bouncer1.update();
  bouncer2.update();

  // Get the updated value :
  int value1 = bouncer1.read();
  int value2 = bouncer2.read();
  if (digitalRead(OPEN_BUTTON) == LOW) {
    Serial.println("Opening the blind");
    openBlind();
  } else if (digitalRead(CLOSE_BUTTON) == LOW) {
    Serial.println("Closing the blind");
    closeBlind();
  }
}

void limitSwitchHandler() {
  //Blind is open
  if (digitalRead(TOP_LIMIT_SWITCH) == LOW) {
    stopBlind();
    if (digitalRead(TOP_LIMIT_SWITCH) == LOW && !didPublishState) {
      client.publish("blinds/kitchen/state", "OPEN", true);
      didPublishState = true;
    }

    //Blind is closed
  } else if (digitalRead(BOTTOM_LIMIT_SWITCH) == LOW) {
    stopBlind();
    if (digitalRead(BOTTOM_LIMIT_SWITCH) == LOW && !didPublishState) {
      didPublishState = true;
      client.publish("blinds/kitchen/state", "CLOSED", true);
    }
  }
}

void stateCheckin() {
  if (digitalRead(TOP_LIMIT_SWITCH) == LOW) {
    client.publish("blinds/kitchen/state", "OPEN", true);
  }
  if (digitalRead(TOP_LIMIT_SWITCH) == HIGH) {
    client.publish("blinds/kitchen/state", "CLOSED", true);
  }
}

void openBlind() {
  didPublishState = false;
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  delay(2000);  // This delay to to allow the blind to release the limit switch else the low state calls stopmotor before it can start moving
}

void closeBlind() {
  didPublishState = false;
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  delay(2000);  // This delay to to allow the blind to release the limit switch else the low state calls stopmotor before it can start moving
}

void stopBlind() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, LOW);
}

void callback(char* topic, byte* payload, unsigned int length) {
  //Handle mqtt messages received by this NodeMCU
  payload[length] = '\0';
  strTopic = String((char*)topic);
  Serial.println("Topic " + strTopic);
  String command = String((char*)payload);
  Serial.println("Command " + command);
  if (strTopic == operateblind_topic) {
    if (command == "OPEN") {
      openBlind();
    } else if (command == "CLOSE") {
      closeBlind();
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
    client.subscribe("blinds/kitchen/action");
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    delay(5000);
  }
}
