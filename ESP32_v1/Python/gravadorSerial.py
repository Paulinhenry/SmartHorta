import serial
import csv
import os
import struct
import requests
from datetime import datetime

# ============================================================
# Configuração
# ============================================================
PORTA = "COM4"
BAUDRATE = 115200
ARQUIVO = r"C:\Users\PAULO\Desktop\SmartHorta\ESP32_v1\Dados\dados.csv"

# Ajustar para o IP real do Machbase (local ou nuvem)
MACHBASE_URL = "http://127.0.0.1:5654/db/query"


def validar_qualidade(variavel, valor):
    """Retorna 'suspeito' se o valor estiver fora dos limites aceitáveis."""
    limites = {
        "temperatura": (-10, 60),
        "umidade_solo": (0, 100),
        "luminosidade": (0, float("inf")),
    }
    faixa = limites.get(variavel)
    if faixa and not (faixa[0] <= valor <= faixa[1]):
        return "suspeito"
    return "valido"


def inserir_no_machbase(sensor_id, timestamp, valor, canteiro_id, variavel, unidade):
    """Insere uma leitura na TAG TABLE 'leituras' do Machbase Neo via HTTP POST."""
    query = (
        f"INSERT INTO leituras (NAME, TIME, VALOR, CANTEIRO_ID, VARIAVEL, UNIDADE) "
        f"VALUES ('{sensor_id}', TO_DATE('{timestamp}', 'YYYY-MM-DD HH24:MI:SS'), "
        f"{valor:.4f}, '{canteiro_id}', '{variavel}', '{unidade}')"
    )

    try:
        resposta = requests.post(MACHBASE_URL, data={"q": query}, timeout=5)
        if resposta.status_code == 200:
            return True
        else:
            print(f"  ERRO MACHBASE [{sensor_id}]: {resposta.text}")
            return False
    except requests.exceptions.ConnectionError:
        print(f"  MACHBASE OFFLINE — dado salvo apenas no CSV")
        return False
    except Exception as e:
        print(f"  FALHA MACHBASE [{sensor_id}]: {e}")
        return False


ser = serial.Serial(PORTA, BAUDRATE)

try:
    arquivoExiste = os.path.exists(ARQUIVO)

    with open(ARQUIVO, "a", newline="") as arquivo:
        escritor = csv.writer(arquivo)

        if not arquivoExiste:
            escritor.writerow([
                "Timestamp", "Canteiro", "Sensor",
                "Variavel", "Valor", "Unidade", "Qualidade"
            ])

        print("=" * 60)
        print("SmartHorta — Gravador Serial + Machbase Neo")
        print("=" * 60)
        print(f"Porta serial : {PORTA}")
        print(f"Arquivo CSV  : {ARQUIVO}")
        print(f"Machbase URL : {MACHBASE_URL}")
        print("=" * 60)
        print("Aguardando dados do ESP32...\n")

        while True:
            try:
                byte = ser.read(1)
            except serial.SerialException:
                print("ERRO: Conexão serial perdida. Reconectando...")
                ser.close()
                try:
                    ser = serial.Serial(PORTA, BAUDRATE)
                    print("Reconectado com sucesso.")
                except serial.SerialException:
                    print("Falha ao reconectar. Encerrando.")
                    break
                continue

            if byte == b'\xAA':
                cabecalho = ser.read(3)
                if cabecalho == b'\xBB\xCC\xDD':
                    restante = ser.read(60)
                    msg = b'\xAA\xBB\xCC\xDD' + restante

                    if len(msg) == 64:
                        horaAtual = datetime.now()
                        timestamp = horaAtual.strftime('%Y-%m-%d %H:%M:%S')

                        canteiro_id = chr(msg[4]) + chr(msg[5])

                        luminosidade = struct.unpack("<f", msg[10:14])[0]
                        temperatura = struct.unpack("<f", msg[14:18])[0]
                        umidade = msg[18]

                        sensor_luz = f"LUZ-{canteiro_id}-01"
                        sensor_tmp = f"TMP-{canteiro_id}-01"
                        sensor_umi = f"UMI-{canteiro_id}-01"

                        q_luz = validar_qualidade("luminosidade", luminosidade)
                        q_tmp = validar_qualidade("temperatura", temperatura)
                        q_umi = validar_qualidade("umidade_solo", umidade)

                        leituras = [
                            (sensor_luz, "luminosidade", luminosidade, "lux", q_luz),
                            (sensor_tmp, "temperatura", temperatura, "°C", q_tmp),
                            (sensor_umi, "umidade_solo", float(umidade), "%", q_umi),
                        ]

                        print(f"[{timestamp}] Canteiro {canteiro_id}:")

                        for sensor_id, variavel, valor, unidade, qualidade in leituras:
                            escritor.writerow([
                                timestamp, canteiro_id, sensor_id,
                                variavel, f"{valor:.2f}", unidade, qualidade
                            ])

                            ok = inserir_no_machbase(
                                sensor_id, timestamp, valor,
                                canteiro_id, variavel, unidade
                            )

                            status = "✓" if ok else "✗"
                            flag = f" [{qualidade.upper()}]" if qualidade != "valido" else ""
                            print(f"  {status} {sensor_id}: {valor:.2f} {unidade}{flag}")

                        arquivo.flush()
                        print()

except KeyboardInterrupt:
    print("\nEncerrado pelo usuário.")
finally:
    ser.close()
    print("Porta serial fechada.")