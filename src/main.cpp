#include <Arduino.h>
#include <Ticker.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <BH1750.h>
#define LED_COUNT 3
#define LED_GREEN 4
#define LED_YELLOW 5
#define LED_RED 18
#define DHT_PIN 13
bool turnOn = false;


#define WIFI_SSID "sogumi"
#define WIFI_PASSWORD "vincun123"
#define MQTT_BROKER "broker.emqx.io"
#define MQTT_TOPIC_PUBLISH "esp32_dean/data/lux"
#define MQTT_TOPIC_PUBLISH2 "esp32_dean/data/humid"
#define MQTT_TOPIC_PUBLISH3 "esp32_dean/data/temp"
#define MQTT_TOPIC_SUBSCRIBE "esp32_dean/cmd"

const uint8_t arLed[LED_COUNT] = {LED_RED, LED_YELLOW, LED_GREEN};

float globalHumidity = 0, globalTemp = 0, globalLux = 0;
int g_nCount=0;
Ticker timer1Sec;
Ticker timerLedBuiltinOff;
Ticker timerLedRedOff;
Ticker timerYellowOff;
Ticker timerReadSensor;
Ticker timerPublish;
Ticker timerMqtt;
Ticker timerPublishTemp, timerPublishHumid, timerPublishLux, ledOff;

DHTesp dht;
BH1750 lightMeter;

char g_szDeviceId[30];
WiFiClient espClient;
PubSubClient mqtt(espClient);
boolean mqttConnect();
void onPublishMessageTemp();
void onPublishMessageHumid();
void onPublishMessageLux();

void WifiConnect();

void setup () 
{


Serial.begin(9600);
delay(100);
for (uint8_t i=0; i<LED_COUNT; i++)
pinMode(arLed[i], OUTPUT);
Wire.begin();
lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
Serial.printf("Free Memory: %d\n", ESP.getFreeHeap());
dht.setup(DHT_PIN, DHTesp::DHT11);
lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
Serial.println("System ready");

WifiConnect();
mqttConnect();

#if defined(ESP32)
  timerPublishTemp.attach_ms(5000, onPublishMessageTemp);
  timerPublishHumid.attach_ms(7000, onPublishMessageHumid);
  timerPublishLux.attach_ms(4000, onPublishMessageLux);

#endif
}

void loop()
{
  mqtt.loop();
}


void mqttCallback(char* topic, byte* payload, unsigned int len) 

{
String strTopic = topic;
  int8_t idx = strTopic.lastIndexOf('/') + 1;
  String strDev = strTopic.substring(idx);
  Serial.printf("==> Recv [%s]: ", topic);
  Serial.write(payload, len);
  Serial.println();
  if (strcmp(topic, MQTT_TOPIC_SUBSCRIBE) == 0) {
    payload[len] = '\0';
    Serial.printf("==> Recv [%s]: ", payload);
    if (strcmp((char*)payload, "led-on") == 0) {
      turnOn = true;
      digitalWrite(LED_YELLOW, HIGH);
    }
    else if (strcmp((char*)payload, "led-off") == 0) {
      turnOn = false;
      digitalWrite(LED_YELLOW, LOW);
    }
  }
}
void onPublishMessage()
{
char szTopic[50];
char szData[10];
digitalWrite(LED_BUILTIN, HIGH);
float fHumidity = dht.getHumidity();
float fTemperature = dht.getTemperature();
float lux = lightMeter.readLightLevel();

if (dht.getStatus()==DHTesp::ERROR_NONE)
{
Serial.printf("Temperature: %.2f C, Humidity: %.2f %%, light: %.2f\n",
fTemperature, fHumidity, lux);
sprintf(szTopic, "%s/temp", MQTT_TOPIC_PUBLISH);
sprintf(szData, "%.2f", fTemperature);
mqtt.publish(szTopic, szData);

sprintf(szTopic, "%s/humidity", MQTT_TOPIC_PUBLISH);
sprintf(szData, "%.2f", fHumidity);
mqtt.publish(szTopic, szData);

}
else{
Serial.printf("Light: %.2f lx\n", lux);
sprintf(szTopic, "%s/light", MQTT_TOPIC_PUBLISH);
sprintf(szData, "%.2f", lux);
mqtt.publish(szTopic, szData);
}
}

void onPublishMessageTemp()
{
  char szTopic[50];
  char szData[10];
  digitalWrite(LED_BUILTIN, HIGH);
  float temperature = dht.getTemperature();
  if (dht.getStatus() == DHTesp::ERROR_NONE)
  {
    Serial.printf("Temperature: %.2f C\n",
                  temperature);
    sprintf(szTopic, "%s/temp", MQTT_TOPIC_PUBLISH3);
    sprintf(szData, "%.2f", temperature);
    mqtt.publish(MQTT_TOPIC_PUBLISH3, szData);

  }
  else
    Serial.printf("\n");

}

void onPublishMessageHumid()
{
  char szTopic[50];
  char szData[10];
  digitalWrite(LED_BUILTIN, HIGH);
  float humidity = dht.getHumidity();
  if (dht.getStatus() == DHTesp::ERROR_NONE)
  {
    Serial.printf("Humidity: %.2f %%\n",
                  humidity);

    sprintf(szTopic, "%s/humidity", MQTT_TOPIC_PUBLISH2);
    sprintf(szData, "%.2f", humidity);
    mqtt.publish(MQTT_TOPIC_PUBLISH2, szData);
  }
  else
    Serial.printf("\n");
}
void onPublishMessageLux()
{
  char szTopic[50];
  char szData[10];
  digitalWrite(LED_BUILTIN, HIGH);
  float lux = lightMeter.readLightLevel();
  if (dht.getStatus() == DHTesp::ERROR_NONE)
  {
    Serial.printf("light: %.2f\n",
                  lux);
  }
  else
    Serial.printf("Light: %.2f lux\n", lux);
  sprintf(szTopic, "%s/light", MQTT_TOPIC_PUBLISH);
  sprintf(szData, "%.2f", lux);
  mqtt.publish(MQTT_TOPIC_PUBLISH, szData);
  ledOff.once_ms(100, []()
                 { digitalWrite(LED_BUILTIN, LOW); });

  if (lightMeter.readLightLevel() < 400)
  {
    Serial.printf("Pintu Tertutup = %.2f lux\n", lux);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
  }
  else if (lightMeter.readLightLevel() > 400)
  {
    Serial.printf("Pintu Terbuka = %.2f lux\n", lux);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
  }
}

boolean mqttConnect() {
sprintf(g_szDeviceId, "esp32_%08X",(uint32_t)ESP.getEfuseMac());
mqtt.setServer(MQTT_BROKER, 1883);
mqtt.setCallback(mqttCallback);
Serial.printf("Connecting to %s clientId: %s\n", MQTT_BROKER, g_szDeviceId);

boolean fMqttConnected = false;
for (int i=0; i<3 && !fMqttConnected; i++) {
Serial.print("Connecting to mqtt broker...");
fMqttConnected = mqtt.connect(g_szDeviceId);
if (fMqttConnected == false) {
Serial.print(" fail, rc=");
Serial.println(mqtt.state());
delay(1000);
}
}

if (fMqttConnected)
{
Serial.println(" success");
mqtt.subscribe(MQTT_TOPIC_SUBSCRIBE);
Serial.printf("Subcribe topic: %s\n", MQTT_TOPIC_SUBSCRIBE);

}
return mqtt.connected();
}

void WifiConnect()
{
WiFi.mode(WIFI_STA);
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
while (WiFi.waitForConnectResult() != WL_CONNECTED) {
Serial.println("Connection Failed! Rebooting...");
delay(5000);
ESP.restart();
}
Serial.print("System connected with IP address: ");
Serial.println(WiFi.localIP());
Serial.printf("RSSI: %d\n", WiFi.RSSI());
}

