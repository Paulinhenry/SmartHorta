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

# URL da API do Machbase Neo
# Para uso LOCAL:  http://127.0.0.1:5654/db/query
# Para uso NUVEM:  http://<IP_PUBLICO_DA_VM>:5654/db/query
MACHBASE_URL = "http://127.0.0.1:5654/db/query"

# ============================================================
# Funções auxiliares
# ============================================================

def validar_qualidade(variavel, valor):
    """
    Valida se o valor da leitura está dentro dos limites aceitáveis.
    Retorna 'valido' ou 'suspeito'.
    """
    if variavel == "temperatura":
        if valor < -10 or valor > 60:
            return "suspeito"
    elif variavel == "umidade_solo":
        if valor < 0 or valor > 100:
            return "suspeito"
    elif variavel == "luminosidade":
        if valor < 0:
            return "suspeito"
    return "valido"


def inserir_no_machbase(sensor_id, timestamp, valor, canteiro_id, variavel, unidade):
    """
    Insere uma leitura individual na TAG TABLE 'leituras' do Machbase Neo
    via API REST HTTP POST.
    """
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


# ============================================================
# Conexão serial
# ============================================================
ser = serial.Serial(PORTA, BAUDRATE)

# ============================================================
# Abertura do CSV
# ============================================================
arquivoExiste = os.path.exists(ARQUIVO)

with open(ARQUIVO, "a", newline="") as arquivo:
    escritor = csv.writer(arquivo)

    if not arquivoExiste:
        escritor.writerow([
            "Timestamp",
            "Canteiro",
            "Sensor",
            "Variavel",
            "Valor",
            "Unidade",
            "Qualidade"
        ])

    print("=" * 60)
    print("SmartHorta — Gravador Serial + Machbase Neo")
    print("=" * 60)
    print(f"Porta serial : {PORTA}")
    print(f"Arquivo CSV  : {ARQUIVO}")
    print(f"Machbase URL : {MACHBASE_URL}")
    print("=" * 60)
    print("Aguardando dados do ESP32...\n")

    # ============================================================
    # Loop principal — leitura contínua da serial
    # ============================================================
    while True:
        byte = ser.read(1)

        if byte == b'\xAA':
            cabecalho = ser.read(3)
            if cabecalho == b'\xBB\xCC\xDD':
                restante = ser.read(60)
                msg = b'\xAA\xBB\xCC\xDD' + restante

                if len(msg) == 64:
                    # Timestamp da leitura
                    horaAtual = datetime.now()
                    timestamp = horaAtual.strftime('%Y-%m-%d %H:%M:%S')

                    # Decodifica canteiro_id (bytes 4-5, ASCII)
                    canteiro_id = chr(msg[4]) + chr(msg[5])

                    # Decodifica valores dos sensores
                    luminosidade = struct.unpack("<f", msg[10:14])[0]
                    temperatura = struct.unpack("<f", msg[14:18])[0]
                    umidade = msg[18]

                    # Gera sensor_ids
                    sensor_luz = f"LUZ-{canteiro_id}-01"
                    sensor_tmp = f"TMP-{canteiro_id}-01"
                    sensor_umi = f"UMI-{canteiro_id}-01"

                    # Valida qualidade
                    q_luz = validar_qualidade("luminosidade", luminosidade)
                    q_tmp = validar_qualidade("temperatura", temperatura)
                    q_umi = validar_qualidade("umidade_solo", umidade)

                    # Define as 3 leituras
                    leituras = [
                        (sensor_luz, "luminosidade", luminosidade, "lux", q_luz),
                        (sensor_tmp, "temperatura", temperatura, "°C", q_tmp),
                        (sensor_umi, "umidade_solo", float(umidade), "%", q_umi),
                    ]

                    print(f"[{timestamp}] Canteiro {canteiro_id}:")

                    for sensor_id, variavel, valor, unidade, qualidade in leituras:
                        # Salva no CSV
                        escritor.writerow([
                            timestamp,
                            canteiro_id,
                            sensor_id,
                            variavel,
                            f"{valor:.2f}",
                            unidade,
                            qualidade
                        ])

                        # Envia para o Machbase
                        ok = inserir_no_machbase(
                            sensor_id, timestamp, valor,
                            canteiro_id, variavel, unidade
                        )

                        status = "✓" if ok else "✗"
                        flag = f" [{qualidade.upper()}]" if qualidade != "valido" else ""
                        print(f"  {status} {sensor_id}: {valor:.2f} {unidade}{flag}")

                    arquivo.flush()
                    print()