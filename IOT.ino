#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  21
#define RST_PIN 22

MFRC522 rfid(SS_PIN, RST_PIN);

const char* ssid = "SUA_REDE_WIFI";
const char* password = "SUA_SENHA_WIFI";

String serverURL = ""; // sua API

#define LED_VERDE  2   // Pronto para sair
#define LED_VERMELHO  4 // Erro
#define LED_AMARELO  5  // Status verificando
#define LED_BRANCO   15 // Cadastrando tag


void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_BRANCO, OUTPUT);

  connectWiFi();
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  String tagID = getTagID();

  Serial.println("Tag detectada: " + tagID);

  // Verifica status da tag via API
  digitalWrite(LED_AMARELO, HIGH);
  String status = verificarStatusTag(tagID);
  digitalWrite(LED_AMARELO, LOW);

  if (status == "cadastro") {
    digitalWrite(LED_BRANCO, HIGH);
    cadastrarTag(tagID);
    delay(1000);
    digitalWrite(LED_BRANCO, LOW);
  }
  else if (status == "pronto") {
    digitalWrite(LED_VERDE, HIGH);
    delay(2000);
    digitalWrite(LED_VERDE, LOW);
    removerTag(tagID); // Moto saiu
  }
  else if (status == "erro") {
    digitalWrite(LED_VERMELHO, HIGH);
    delay(3000);
    digitalWrite(LED_VERMELHO, LOW);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" conectado!");
}

String getTagID() {
  String tag = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    tag += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    tag += String(rfid.uid.uidByte[i], HEX);
  }
  tag.toUpperCase();
  return tag;
}

String verificarStatusTag(String tagID) {
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    http.begin(serverURL + "/status/" + tagID);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      http.end();
      return payload;
    } else {
      http.end();
      return "erro";
    }
  }
  return "erro";
}

void cadastrarTag(String tagID) {
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    http.begin(serverURL + "/cadastrar");
    http.addHeader("Content-Type", "application/json");
    String body = "{\"tag\":\"" + tagID + "\"}";
    int httpCode = http.POST(body);
    http.end();
  }
}

void removerTag(String tagID) {
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    http.begin(serverURL + "/remover/" + tagID);
    int httpCode = http.GET();
    http.end();
  }
}
