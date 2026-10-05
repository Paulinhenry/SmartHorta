#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <DHT.h>
#include <DHT_U.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Wire.h>
#include <time.h>
#include "esp_wpa2.h"
#include <esp_wifi.h>

#define sensorLuz 36
#define sensorUmidade 39
#define sensorTemperatura 4
#define DHTTYPE DHT22

// Altere para cada ESP32 diferente (ex: "A1", "B1")
const char CANTEIRO_ID[3] = "A1";

const float resistorReferencia = 10000.0;

const char *WIFI_SSID = "Galaxy A53 5G95B9";
const char *WIFI_USER = ""; // Usuário/E-mail da rede empresarial/faculdade
const char *WIFI_PASSWORD = "abcdefgh";

// Ajustar para o IP real do Machbase (local ou nuvem)
const char *MACHBASE_URL = "http://172.29.110.58:5654/db/query";

float converterParaLux(int leituraLuminosidade);
void enviarLeituras(float lux, float temperatura, uint8_t umidade);
bool inserirNoMachbase(const String &sensorId, float valor,
                        const char *variavel, const char *unidade);
void conectarWifi();

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(sensorTemperatura, DHTTYPE);
Preferences prefs;

// ============================================================
// Conecta (ou reconecta) ao Wi-Fi (Suporta Wi-Fi comum e WPA2 Enterprise)
// ============================================================
void conectarWifi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);

  // Se a variável WIFI_USER estiver preenchida, usa WPA2-Enterprise (redes de faculdade/empresa).
  // Se estiver vazia (""), usa conexão Wi-Fi padrão (roteador/roteador do celular).
  if (strlen(WIFI_USER) > 0) {
    Serial.println("Conectando ao Wi-Fi (WPA2 Enterprise)...");
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)WIFI_USER, strlen(WIFI_USER));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)WIFI_USER, strlen(WIFI_USER));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)WIFI_PASSWORD, strlen(WIFI_PASSWORD));
    esp_wifi_sta_wpa2_ent_enable();
    WiFi.begin(WIFI_SSID);
  } else {
    Serial.println("Conectando ao Wi-Fi convencional...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nFalha ao conectar no Wi-Fi.");
  }
}

String urlEncode(const String &str) {
  String encoded = "";
  for (size_t i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else if (c == ' ') {
      encoded += '+';
    } else {
      char buf[4];
      sprintf(buf, "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }
  return encoded;
}

// ============================================================
// Envia uma leitura individual pro Machbase via HTTP POST,
// no mesmo formato de INSERT que o gravadorSerial.py usava
// ============================================================
bool inserirNoMachbase(const String &sensorId, float valor,
                        const char *variavel, const char *unidade) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.begin(MACHBASE_URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setTimeout(5000);

  // Obtem timestamp real via NTP (mesmo formato do gravadorSerial.py)
  struct tm timeinfo;
  char timestamp[20];
  if (getLocalTime(&timeinfo)) {
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
  } else {
    Serial.println("  AVISO: Hora NTP indisponivel, usando fallback");
    strcpy(timestamp, "1970-01-01 00:00:00");
  }

  String query = "INSERT INTO leituras (NAME, TIME, VALOR, CANTEIRO_ID, VARIAVEL, UNIDADE) "
                 "VALUES ('" + sensorId + "', TO_DATE('" + String(timestamp) + "', 'YYYY-MM-DD HH24:MI:SS'), " + String(valor, 4) +
                 ", '" + String(CANTEIRO_ID) + "', '" + variavel + "', '" + unidade + "')";

  String body = "q=" + urlEncode(query);
  int codigo = http.POST(body);

  bool ok = (codigo == 200);
  if (!ok) {
    String resposta = http.getString();
    Serial.println("  ERRO MACHBASE [" + sensorId + "]: HTTP " + String(codigo) + " -> " + resposta);
  }

  http.end();
  return ok;
}

void enviarLeituras(float lux, float temperatura, uint8_t umidade) {
  conectarWifi();

  String sensorLuzId = "LUZ-" + String(CANTEIRO_ID) + "-01";
  String sensorTmpId = "TMP-" + String(CANTEIRO_ID) + "-01";
  String sensorUmiId = "UMI-" + String(CANTEIRO_ID) + "-01";

  bool okLuz = inserirNoMachbase(sensorLuzId, lux, "luminosidade", "lux");
  bool okTmp = inserirNoMachbase(sensorTmpId, temperatura, "temperatura", "C");
  bool okUmi = inserirNoMachbase(sensorUmiId, (float)umidade, "umidade_solo", "%");

  Serial.printf("Luz:%s Temp:%s Umidade:%s\n",
                okLuz ? "OK" : "FALHOU",
                okTmp ? "OK" : "FALHOU",
                okUmi ? "OK" : "FALHOU");
}

float lerTemperatura() {
  float temperatura = dht.readTemperature();

  // DHT pode retornar NaN em caso de falha de leitura
  if (isnan(temperatura)) {
    Serial.println("ERRO: Falha ao ler DHT22");
    lcd.setCursor(0, 0);
    lcd.print("T: ERRO   ");
    return -1;
  }

  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperatura);
  lcd.print("C ");

  delay(200);

  return temperatura;
}

float lerLuminosidade() {
  int valorluminosidade = analogRead(sensorLuz);
  float lux = converterParaLux(valorluminosidade);

  lcd.setCursor(0, 1);
  lcd.print("L:");
  lcd.print(lux);
  lcd.print("lux ");

  delay(200);

  return lux;
}

uint8_t lerUmidade() {
  int valorUmidade = analogRead(sensorUmidade);
  uint8_t porcentagemUmidade = (4096 - valorUmidade) * 100.0 / 4096;

  lcd.setCursor(10, 0);
  lcd.print("U:");
  lcd.print(porcentagemUmidade);
  lcd.print("% ");

  delay(200);

  return porcentagemUmidade;
}

float converterParaLux(int leituraLuminosidade) {
  if (leituraLuminosidade > 0) {
    float tensao = leituraLuminosidade * (5.0 / 4096.0);
    float resistenciaLDR = resistorReferencia * (5 / tensao - 1.0);
    float lux = 500.0 / (resistenciaLDR / 1000.0);
    return lux;
  }
  return 0.0;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  dht.begin();
  lcd.init();
  lcd.backlight();

  conectarWifi();

  // Sincroniza relógio via NTP (UTC-3 = Brasília)
  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Sincronizando NTP");
  struct tm timeinfo;
  int tentativas = 0;
  while (!getLocalTime(&timeinfo) && tentativas < 10) {
    Serial.print(".");
    delay(1000);
    tentativas++;
  }
  if (tentativas < 10) {
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.println("\nHora sincronizada: " + String(buf));
  } else {
    Serial.println("\nFalha ao sincronizar NTP.");
  }
}

void loop() {
  lcd.clear();

  float temperatura = lerTemperatura();
  float luminosidade = lerLuminosidade();
  uint8_t umidade = lerUmidade();

  // Não envia dados se a temperatura falhou (sentinela -1)
  if (temperatura >= 0) {
    enviarLeituras(luminosidade, temperatura, umidade);
  }

  delay(2000);
}