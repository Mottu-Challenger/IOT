# Projeto RFID com ESP32 e Integração com Google Sheets

## Descrição

Este projeto utiliza um ESP32 conectado a um leitor RFID para identificar tags, controlar LEDs que indicam o status da leitura e registrar as interações (cadastro, remoção e status) em uma planilha Google Sheets usando um Google Apps Script.

---

## Objetivo

* Detectar tags RFID e realizar ações baseadas no status da tag.
* Indicar visualmente o status da tag via LEDs (verde, amarelo, vermelho, branco).
* Registrar automaticamente todas as ações em uma planilha Google Sheets para acompanhamento.
* Fazer tudo isso usando apenas o ESP32 e o Google Sheets, sem precisar de servidores complexos ou outras tecnologias avançadas.

---

## Como funciona

1. O ESP32 conecta à rede Wi-Fi.
2. Detecta uma tag RFID.
3. Verifica o status da tag (exemplo simplificado: se o último caractere do ID é 'A' → cadastro, 'B' → pronto, senão erro).
4. Acende o LED correspondente para indicar o status.
5. Envia um registro da ação para a planilha Google Sheets via HTTP POST em JSON.
6. A planilha grava a data/hora, ID da tag, ação e status recebidos.

---

## Requisitos

* ESP32 com leitor RFID MFRC522.
* Rede Wi-Fi disponível.
* Planilha Google Sheets.
* Google Apps Script configurado para receber os dados.

---

## Código ESP32 (Arduino)

```cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  21
#define RST_PIN 22

MFRC522 rfid(SS_PIN, RST_PIN);

const char* ssid = "SUA_REDE_WIFI";
const char* password = "SUA_SENHA_WIFI";

String serverURL = "https://script.google.com/macros/s/SEU_ENDPOINT/exec";

#define LED_VERDE  2   
#define LED_VERMELHO  4 
#define LED_AMARELO  5  
#define LED_BRANCO   15 

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
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String tagID = getTagID();
  Serial.println("Tag detectada: " + tagID);

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
    removerTag(tagID);
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

void sendToSheet(String tagID, String action, String status) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    String json = "{\"tagID\":\"" + tagID + "\",\"action\":\"" + action + "\",\"status\":\"" + status + "\"}";
    int httpCode = http.POST(json);
    Serial.println("Google Sheets response code: " + String(httpCode));
    http.end();
  }
}

void cadastrarTag(String tagID) {
  Serial.println("Cadastrando tag: " + tagID);
  sendToSheet(tagID, "cadastrar", "sucesso");
}

void removerTag(String tagID) {
  Serial.println("Removendo tag: " + tagID);
  sendToSheet(tagID, "remover", "sucesso");
}

String verificarStatusTag(String tagID) {
  char lastChar = tagID.charAt(tagID.length() - 1);
  if (lastChar == 'A') return "cadastro";
  else if (lastChar == 'B') return "pronto";
  else return "erro";
}
```

---

## Configurando o Google Sheets

### 1. Criar a planilha

* Abra o Google Sheets.
* Crie uma nova planilha.
* Opcional: renomeie a aba principal para **Registros**.

### 2. Criar o Apps Script

* No menu, clique em **Extensões > Apps Script**.
* Apague o código padrão e cole o código abaixo:

```javascript
function doPost(e) {
  try {
    var dados = JSON.parse(e.postData.contents);

    var tagID = dados.tagID || "N/A";
    var action = dados.action || "N/A";
    var status = dados.status || "N/A";

    var planilha = SpreadsheetApp.getActiveSpreadsheet();
    var aba = planilha.getSheetByName("Registros");

    if (!aba) {
      aba = planilha.insertSheet("Registros");
      aba.appendRow(["Data e Hora", "Tag ID", "Ação", "Status"]);
    }

    var dataHora = new Date();

    aba.appendRow([dataHora, tagID, action, status]);

    return ContentService
      .createTextOutput(JSON.stringify({result:"sucesso"}))
      .setMimeType(ContentService.MimeType.JSON);

  } catch (erro) {
    return ContentService
      .createTextOutput(JSON.stringify({result:"erro", message: erro.message}))
      .setMimeType(ContentService.MimeType.JSON);
  }
}
```

### 3. Publicar o Web App

* No menu do Apps Script, clique em **Deploy > New deployment**.
* Selecione **Web app**.
* Em **Quem tem acesso:** escolha **Qualquer pessoa, mesmo anônima**.
* Clique em **Deploy**.
* Copie o URL gerado e cole no seu código ESP32 na variável `serverURL`.

---

## Funcionamento da planilha

| Data e Hora         | Tag ID   | Ação      | Status  |
| ------------------- | -------- | --------- | ------- |
| 2025-05-25 15:30:12 | 0AF31B7C | cadastrar | sucesso |
| 2025-05-25 15:35:44 | 0AF31B7B | remover   | sucesso |

