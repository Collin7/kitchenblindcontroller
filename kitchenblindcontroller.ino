#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EasyButton.h>
#include <Credentials.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ESP8266SSDP.h>
#include <SimpleTimer.h>

const char* DEVICE_NAME = "KITCHEN BLIND CONTROLLER";

const char* operateblind_topic = "blinds/kitchen/action";
const char* restart_topic = "blinds/kitchen/restart";

//Define Pins
const int CLOSE_BUTTON = 13;          //D7
const int OPEN_BUTTON = 0;          //D3
const int TOP_LIMIT_SWITCH = 14;     //D5
const int BOTTOM_LIMIT_SWITCH = 12;  //D6
const int IN_1 = 5;                  //D1
const int IN_2 = 4;                  //D2
long motorStartTime = 0;

bool didPublishState = false;
bool isMotorRunning = false;

//Initialize Objects
WiFiClient espKitchenBlindController;
PubSubClient client(espKitchenBlindController);
SimpleTimer timer;
EasyButton closeButton(CLOSE_BUTTON);
EasyButton openButton(OPEN_BUTTON);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "za.pool.ntp.org", 7200, 60000);

void setup() {
  Serial.begin(115200);

  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(TOP_LIMIT_SWITCH, INPUT_PULLUP);
  pinMode(BOTTOM_LIMIT_SWITCH, INPUT_PULLUP);

  openButton.begin();
  closeButton.begin();

  closeButton.onPressed(closeButtonPressed);
  openButton.onPressed(openButtonPressed);

  // Set initial rotation direction
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, LOW);

  setupWiFi();
  setupMQTT();
  setupOTA();

  timeClient.begin();

  timer.setInterval(90000, stateCheckin);  //publish blind state every 1 min
}

void loop() {
  //If MQTT client can't connect to broker, then reconnect
  if (!client.connected()) {
    reconnect();
  }
  client.loop();  //the mqtt function that processes MQTT messages
  ArduinoOTA.handle();
  buttonPressHandler();
  limitSwitchHandler();
  timer.run();

  checkMotorRuntime();
}

void checkMotorRuntime() {
  if (isMotorRunning) {
    unsigned long timeNow = timeClient.getEpochTime();
    if (timeNow - motorStartTime >= 30) {
      stopBlind();
    }
  }
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
    Serial.println("Top LIMIT SWITCH STOPPING BLIND");
    stopBlind();
    if (digitalRead(TOP_LIMIT_SWITCH) == LOW && !didPublishState) {
      client.publish("blinds/kitchen/state", "OPEN", true);
      didPublishState = true;
    }

    //Blind is closed
  } else if (digitalRead(BOTTOM_LIMIT_SWITCH) == LOW) {
    delay(500);  // This delay is because the reed switch changes state before the blind is all the way closed
    Serial.println("Getting called for bottom limit switch low");
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
  motorStartTime = timeClient.getEpochTime();
  isMotorRunning = true;
  if (digitalRead(BOTTOM_LIMIT_SWITCH) == HIGH) {  //Make sure blind is currently closed
    didPublishState = false;
    digitalWrite(IN_1, HIGH);
    digitalWrite(IN_2, LOW);
    delay(2000);  // This delay to to allow the blind to release the limit switch else the low state calls stopmotor before it can start moving
  }
}

void closeBlind() {
  motorStartTime = timeClient.getEpochTime();
  isMotorRunning = true;
  if (digitalRead(TOP_LIMIT_SWITCH) == HIGH) {  //Make sure blind is currently open before closing it
    didPublishState = false;
    digitalWrite(IN_1, LOW);
    digitalWrite(IN_2, HIGH);
    delay(2000);  // This delay to to allow the blind to release the limit switch else the low state calls stopmotor before it can start moving
  }
}

void stopBlind() {
  Serial.println("StopBlind method executed");
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, LOW);
  isMotorRunning = false;
}

void callback(char* topic, byte* payload, unsigned int length) {
  //Handle mqtt messages received by this NodeMCU
  payload[length] = '\0';
  char* command = (char*)payload;

  if (strcmp(topic, operateblind_topic) == 0) {
    if (strcmp(command, "OPEN") == 0) {
      openBlind();
    } else if (strcmp(command, "CLOSE") == 0) {
      closeBlind();
    } else if (strcmp(command, "STOP") == 0) {
      client.publish("blinds/kitchen/state", "STOPPED", true);
      stopBlind();
    }
  } else if (strcmp(topic, restart_topic) == 0) {
    restartESP();
  }
}

void setupWiFi() {

  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    // Timeout if connection takes to long (15 seconds)
    if (millis() - startTime > 15000) {
      restartESP();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC: ");
  Serial.println(WiFi.macAddress());
}

void setupMQTT() {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);  // callback is the function that gets called for a topic subscription

  Serial.println("Attempting MQTT connection...");
  if (client.connect(DEVICE_NAME, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.println("Connected to MQTT broker");
    subscribeMQTTTopics();
  } else {
    Serial.print("Failed to connect to MQTT broker. RC=");
    Serial.println(client.state());
    Serial.println("Restarting ESP...");
    restartESP();
  }
}

void subscribeMQTTTopics() {
  client.subscribe("blinds/kitchen/#");
}

void setupOTA() {
  ArduinoOTA.setHostname(DEVICE_NAME);
  ArduinoOTA.begin();
}

void reconnect() {

  // Reconnect to WiFi and MQTT
  if (WiFi.status() != WL_CONNECTED) {
    setupWiFi();
  }

  // Check if MQTT connection is not established
  if (!client.connected()) {
    setupMQTT();
  }
}

void restartESP() {
  ESP.restart();
}
