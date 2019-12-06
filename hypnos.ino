#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFiClientSecure.h>

#define MODULE_NAME "Hypnos"
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914c"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define MAX_WIFI_RETRY_SECONDS 10
#define LED_PIN 18

const char* test_root_ca= \
    "-----BEGIN CERTIFICATE-----\n" \
    "MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G\n" \
    "A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp\n" \
    "Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1\n" \
    "MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG\n" \
    "A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n" \
    "hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL\n" \
    "v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8\n" \
    "eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq\n" \
    "tTleiDTsvHgMCJiEbKjNS7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzd\n" \
    "C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa\n" \
    "zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB\n" \
    "mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH\n" \
    "V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n\n" \
    "bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG\n" \
    "3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs\n" \
    "J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO\n" \
    "291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS\n" \
    "ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd\n" \
    "AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7\n" \
    "TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==\n" \
    "-----END CERTIFICATE-----\n";

boolean tryConnectWifi() {
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < MAX_WIFI_RETRY_SECONDS) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
    i++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Successfully connected");
    return true;
  } else {
    Serial.println("Failed to connect.");
    return false;
  }
}

boolean tryPreviousWifi() {
  Serial.println("Attempting to connect to previous connected ssid.");
  WiFi.begin();
  return tryConnectWifi();
}

boolean setupWifi(const char* ssid, const char* password) {
  Serial.print("Attempting to connect to ssid: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  return tryConnectWifi();
}

void accessGivenEndpoint() {
  WiFiClientSecure client;
  const char* server = "asia-northeast1-suica-versa.cloudfunctions.net";
  client.setCACert(test_root_ca);

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, 443))
    Serial.println("Connection failed!");
  else {
    Serial.println("Connected to server!");
    client.println("GET https://asia-northeast1-suica-versa.cloudfunctions.net/kpiAlert?hasAlert=1 HTTP/1.0");
    client.println("Host: asia-northeast1-suica-versa.cloudfunctions.net");
    client.println("Connection: close");
    client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }
    // TODO: 上の読み込み結果でKPIがあれば下を呼ぶように変更
    digitalWrite(LED_PIN, HIGH);

    client.stop();
  }
}

void handleBLEInput(const char* str){
  Serial.println(str);
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, str);

  // const char* ssid = doc["ssid"];
  // const char* password = doc["password"];
  // const bool hasCheckOrder = false;
  if( !doc["ssid"].isNull() && !doc["password"].isNull()) {
    setupWifi(doc["ssid"], doc["password"]);
  } else if (!doc["check"].isNull()) {
    accessGivenEndpoint();
  }
}

class BLECallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) handleBLEInput(value.c_str());
    }
};

void initializeDevise() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("Starting Module...");
  delay(100);
}

void setupBLE() {
  Serial.println("Starting BLE work!");
  BLEDevice::init(MODULE_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic->setCallbacks(new BLECallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("BLE Started.");
}

void setup() {
  initializeDevise();
  if (!tryPreviousWifi()) {
    setupBLE();
  } else {
    accessGivenEndpoint();
  }
}

void loop() {
  delay(2000);
}
