#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <Wire.h>

#define sensorLuz 36
#define sensorUmidade 39
#define sensorTemperatura 4
#define DHTTYPE DHT22

// ============================================================
// Configuração do canteiro
// Altere este valor para cada ESP32 diferente (ex: "A1", "B1")
// ============================================================
const char CANTEIRO_ID[3] = "A1";

const float resistorReferencia = 10000.0;

float converterParaLux(int leituraLuminosidade);
void gravarPlaca(float lux, float temperatura, uint8_t umidade);

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(sensorTemperatura, DHTTYPE);
Preferences prefs;

// ============================================================
// Monta e envia o pacote de 64 bytes via Serial
//
// Layout do pacote:
//   Bytes 0-3  : Header mágico (0xAA 0xBB 0xCC 0xDD)
//   Bytes 4-5  : Canteiro ID em ASCII (ex: 'A', '1')
//   Bytes 10-13: Luminosidade (float, 4 bytes, little-endian)
//   Bytes 14-17: Temperatura  (float, 4 bytes, little-endian)
//   Byte  18   : Umidade      (uint8_t, 1 byte)
//   Bytes 62-63: Checksum     (uint16_t, little-endian)
// ============================================================
void gravarPlaca(float lux, float temperatura, uint8_t umidade) {
  const int TAMANHO_PACOTE = 64;
  uint8_t pacote[TAMANHO_PACOTE] = {0};

  // Header mágico
  pacote[0] = 0xAA;
  pacote[1] = 0xBB;
  pacote[2] = 0xCC;
  pacote[3] = 0xDD;

  // Canteiro ID (2 bytes ASCII)
  pacote[4] = CANTEIRO_ID[0];
  pacote[5] = CANTEIRO_ID[1];

  // Dados dos sensores
  memcpy(&pacote[10], &lux, sizeof(float));
  memcpy(&pacote[14], &temperatura, sizeof(float));
  pacote[18] = umidade;

  // Checksum (soma de todos os bytes exceto os 2 últimos)
  uint16_t checksum = 0;

  for (int i = 0; i < TAMANHO_PACOTE - 2; i++) {
    checksum += pacote[i];
  }

  pacote[62] = checksum & 0xFF;
  pacote[63] = (checksum >> 8) & 0xFF;

  Serial.write(pacote, TAMANHO_PACOTE);
  delay(200);
}

float lerTemperatura() {
  float temperatura = dht.readTemperature();

  lcd.setCursor(0, 0); // coluna 0, linha 0
  lcd.print("T:");
  lcd.print(temperatura);
  lcd.print("C ");

  delay(200);

  return temperatura;
}

float lerLuminosidade() {
  int valorluminosidade = analogRead(sensorLuz);
  long porcentagemLuminosidade = (long)valorluminosidade * 100 / 4096;

  float lux = converterParaLux(valorluminosidade);

  lcd.setCursor(0, 1); // coluna 0, linha 1
  lcd.print("L:");
  lcd.print(lux);
  lcd.print("lux ");

  delay(200);

  return lux;
}

uint8_t lerUmidade() {
  int valorUmidade = analogRead(sensorUmidade);
  uint8_t porcentagemUmidade = (4096 - valorUmidade) * 100.0 / 4096;

  lcd.setCursor(10, 0); // coluna 10, linha 0
  lcd.print("U:");
  lcd.print(porcentagemUmidade);

  lcd.print("% ");

  delay(200);

  return porcentagemUmidade;
}

float converterParaLux(int leituraLuminosidade) {
  if (leituraLuminosidade > 0) {
    // 1. Calcula a tensão lida no pino A0
    float tensao = leituraLuminosidade * (5.0 / 4096.0);

    // 2. Calcula a resistência do LDR em ohms
    float resistenciaLDR = resistorReferencia * (5 / tensao - 1.0);

    // 3. Aproximação do valor em Lux (lm/m²) para o modelo do Tinkercad
    float lux = 500.0 / (resistenciaLDR / 1000.0);

    delay(200);

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
}

void loop() {
  float temperatura = lerTemperatura();
  float luminosidade = lerLuminosidade();
  uint8_t umidade = lerUmidade();

  gravarPlaca(luminosidade, temperatura, umidade);

  delay(2000);
}
