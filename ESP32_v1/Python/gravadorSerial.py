import serial
import csv
import os

PORTA = "COM6"
BAUDRATE = 115200
ARQUIVO = r"C:\Users\lgvip\OneDrive\Documentos\PlatformIO\Projects\ESP32_v1\Dados\dados.csv"

ser = serial.Serial(PORTA, BAUDRATE)

arquivoExiste = os.path.exists(ARQUIVO)

with open(ARQUIVO, "a", newline="") as arquivo:
    escritor = csv.writer(arquivo)

    if not arquivoExiste:
        escritor.writerow([
            "Luminosidade",
            "Temperatura",
            "Umidade"
        ])

    print("Conectado ao ESP32!")
    print("Salvando dados em:", ARQUIVO)

    while True:
        linha = ser.readline().decode("utf-8").strip()

        print("Recebido:", linha)

        if linha.startswith("CSV:"):
            dados = linha[4:]
            valores = dados.split(",")

            if len(valores) == 3:
                escritor.writerow(valores)
                arquivo.flush()

                print("SALVO:", valores)