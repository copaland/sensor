//1-16-3 https://youtu.be/6rfDrpsOGMY 를 기초로 작업
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "PMS.h"

// 아래의 6개설정은 사용자 환경에 맞게 수정하세요.
const char* ssid = "405902-2.4G"; // 와이파이 AP, 또는 스마트폰의 핫스판 이름
const char* password = "k01033887147";  // 와이파이 AP, 또는 스마트폰의 핫스판 이름
const char* mqtt_server = "broker.mqtt-dashboard.com"; //브로커 주소
const char* outTopic = "/kdi/outTopic"; // 이름이 중복되지 않게 설정 기록
const char* inTopic = "/kdi/inTopic"; // 이름이 중복되지 않게 설정 기록
const char* clientName = "";  // setup 함수에서 자동생성
String sChipID;
char cChipID[20];

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

//json을 위한 설정
StaticJsonDocument<200> doc;
DeserializationError error;
JsonObject root;

//PMS5003 센서를 위한 변수
PMS pms(Serial);
PMS::DATA data;
const int ledPin =  D7;
int dustValue=0,setValue=40;
String inputString;

// RGB Linght를 위한 설정

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, 0);
  
  Serial.begin(9600);
  setup_wifi();

  //이름 자동으로 생성
  sChipID=String(ESP.getChipId(),HEX);
  sChipID.toCharArray(cChipID,sChipID.length()+1);
  clientName=&cChipID[0];
  Serial.println(clientName);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// 통신에서 문자가 들어오면 이 함수의 payload 배열에 저장된다.
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  deserializeJson(doc,payload);
  root = doc.as<JsonObject>();
  const char* sChipidin = root["chip"];
  if( sChipID.equals(sChipidin)) {
    setValue = root["set"];
    Serial.println(setValue);
  }
}

// mqtt 통신에 지속적으로 접속한다.
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientName)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(outTopic, "Reconnected");
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void DoResult() {
  inputString="{\"chip\":\""+sChipID+"\",\"dust\":"+dustValue+"} ";
  inputString.toCharArray(msg, inputString.length());
  Serial.println(msg);
  client.publish(outTopic, msg);
  if(dustValue>setValue)
    digitalWrite(ledPin, 1);
  else
    digitalWrite(ledPin, 0);
}

void loop() {
  if (pms.read(data)) {
    dustValue=data.PM_AE_UG_2_5;
    //Serial.print("PM 2.5 (ug/m3): ");
    //Serial.println(dustValue);
    //Serial.println();
    DoResult();
  }
   
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

}
