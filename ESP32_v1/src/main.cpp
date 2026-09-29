#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <DHT.h>
#include <DHT_U.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Wire.h>

#define sensorLuz 36
#define sensorUmidade 39
#define sensorTemperatura 4
#define DHTTYPE DHT22

// Altere para cada ESP32 diferente (ex: "A1", "B1")
const char CANTEIRO_ID[3] = "A1";

const float resistorReferencia = 10000.0;

const char *WIFI_SSID = "Wife_Santos 2.4G";
const char *WIFI_PASSWORD = "phph2224";

// Ajustar para o IP real do Machbase (local ou nuvem)
const char *MACHBASE_URL = "http://IP_DO_MACHBASE:5654/db/query";

float converterParaLux(int leituraLuminosidade);
void enviarLeituras(float lux, float temperatura, uint8_t umidade);
bool inserirNoMachbase(const String &sensorId, float valor,
                        const char *variavel, const char *unidade);
void conectarWifi();

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(sensorTemperatura, DHTTYPE);
Preferences prefs;

// ============================================================
// Conecta (ou reconecta) ao Wi-Fi
// ============================================================
void conectarWifi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nFalha ao conectar no Wi-Fi.");
  }
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

  // Timestamp via NOW() — ESP32 não tem RTC confiável sem NTP
  String query = "INSERT INTO leituras (NAME, TIME, VALOR, CANTEIRO_ID, VARIAVEL, UNIDADE) "
                 "VALUES ('" + sensorId + "', NOW(), " + String(valor, 4) +
                 ", '" + String(CANTEIRO_ID) + "', '" + variavel + "', '" + unidade + "')";

  String body = "q=" + query;
  int codigo = http.POST(body);

  bool ok = (codigo == 200);
  if (!ok) {
    Serial.println("  ERRO MACHBASE [" + sensorId + "]: HTTP " + String(codigo));
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