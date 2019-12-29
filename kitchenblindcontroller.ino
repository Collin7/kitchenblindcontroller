#include <EasyButton.h>
#include <Credentials.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ESP8266SSDP.h>
#include <SimpleTimer.h>

const char* host = "Kitchen Blind Controller";
#define operateblind_topic "blinds/kitchen/action"
#define restart_topic "blinds/kitchen/restart"
#define CLOSE_BUTTON 0 //D3
#define OPEN_BUTTON 2  //D4

//This can be used to output the date the code was compiled
const char compile_date[] = __DATE__ " " __TIME__;

//Define Pins
const int TOP_LIMIT_SWITCH = 14;    //D5
const int BOTTOM_LIMIT_SWITCH = 12; //D6
const int IN_1 = 5;                 //D1
const int IN_2 = 4;                 //D2

String strTopic;
String strPayload;
bool didPublishState = false;

//Initialize Objects
WiFiClient espKitchenBlindController;
PubSubClient client(espKitchenBlindController);
SimpleTimer timer;
EasyButton closeButton(CLOSE_BUTTON);
EasyButton openButton(OPEN_BUTTON);

void setup() {
  Serial.begin(115200);

  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(TOP_LIMIT_SWITCH, INPUT_PULLUP);
  pinMode(BOTTOM_LIMIT_SWITCH, INPUT_PULLUP);

  closeButton.begin();
  openButton.begin();
  closeButton.onPressed(closeButtonPressed);
  openButton.onPressed(openButtonPressed);

  // Set initial rotation direction
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, LOW);

  setup_wifi();

  client.setServer(MQTT_SERVER, MQTT_PORT);
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

void closeButtonPressed() {
  closeBlind();
}

void openButtonPressed() {
  openBlind();
}
void buttonPressHandler() {
  openButton.read();
  closeButton.read();
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
  if (digitalRead(BOTTOM_LIMIT_SWITCH) == HIGH) { //Make sure blind is currently closed
    didPublishState = false;
    digitalWrite(IN_1, HIGH);
    digitalWrite(IN_2, LOW);
    delay(2000);  // This delay to to allow the blind to release the limit switch else the low state calls stopmotor before it can start moving
  }
}

void closeBlind() {
  if (digitalRead(TOP_LIMIT_SWITCH) == HIGH) { //Make sure blind is currently open before closing it
    didPublishState = false;
    digitalWrite(IN_1, LOW);
    digitalWrite(IN_2, HIGH);
    delay(2000);  // This delay to to allow the blind to release the limit switch else the low state calls stopmotor before it can start moving
  }
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
  } else if (strTopic == restart_topic) {
    restartESP();
  }
}

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

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
  if (client.connect(host, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.println("connected");
    client.subscribe("blinds/kitchen/#");
  } else {
    stopBlind();
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    delay(5000);
  }
}

void restartESP() {
  ESP.restart();
}
