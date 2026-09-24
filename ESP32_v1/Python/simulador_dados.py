import time
import random
import requests
from datetime import datetime

# ============================================================
# Configuração
# ============================================================
MACHBASE_URL = "http://127.0.0.1:5654/db/query"
CANTEIRO_ID = "A1"

print("=" * 60)
print("SmartHorta — Simulador de Dados (Sem ESP32)")
print("=" * 60)
print(f"Enviando dados para: {MACHBASE_URL}")
print("Pressione Ctrl+C para parar.")
print("=" * 60)

def inserir_no_machbase(sensor_id, timestamp, valor, canteiro_id, variavel, unidade):
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
            print(f"  ERRO MACHBASE: {resposta.text}")
            return False
    except Exception as e:
        print(f"  FALHA DE CONEXÃO: Certifique-se que o Docker está rodando. Erro: {e}")
        return False

# ============================================================
# Loop de simulação
# ============================================================
try:
    while True:
        # Gera timestamp
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

        # Gera dados falsos realistas
        luminosidade = random.uniform(80.0, 150.0)    # lux
        temperatura = random.uniform(20.0, 30.0)      # °C
        umidade = random.uniform(40.0, 60.0)          # %

        sensor_luz = f"LUZ-{CANTEIRO_ID}-01"
        sensor_tmp = f"TMP-{CANTEIRO_ID}-01"
        sensor_umi = f"UMI-{CANTEIRO_ID}-01"

        leituras = [
            (sensor_luz, "luminosidade", luminosidade, "lux"),
            (sensor_tmp, "temperatura", temperatura, "°C"),
            (sensor_umi, "umidade_solo", umidade, "%"),
        ]

        print(f"\n[{timestamp}] Simulando Canteiro {CANTEIRO_ID}:")

        for sensor_id, variavel, valor, unidade in leituras:
            ok = inserir_no_machbase(sensor_id, timestamp, valor, CANTEIRO_ID, variavel, unidade)
            status = "[OK]" if ok else "[ERRO]"
            print(f"  {status} {sensor_id}: {valor:.2f} {unidade}")

        # Espera 5 segundos antes de enviar o próximo lote
        time.sleep(5)

except KeyboardInterrupt:
    print("\nSimulação encerrada pelo usuário.")
