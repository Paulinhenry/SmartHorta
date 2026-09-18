import serial
import csv
import os
import struct
from datetime import datetime

PORTA = "COM6"
BAUDRATE = 115200
ARQUIVO = r"C:\Users\lgvip\OneDrive\Documentos\PlatformIO\Projects\ESP32_v1\Dados\dados.csv"

ser = serial.Serial(PORTA, BAUDRATE)



arquivoExiste = os.path.exists(ARQUIVO)

with open(ARQUIVO, "a", newline="") as arquivo:
    escritor = csv.writer(arquivo)

    if not arquivoExiste:
        escritor.writerow([
            "Hora",
            "Minuto",
            "Luminosidade",
            "Temperatura",
            "Umidade"
        ])

    print("Conectado ao ESP32!")
    print("Salvando dados em:", ARQUIVO)

    while True:
        horaAtual = datetime.now()
        hora = horaAtual.hour
        minuto = horaAtual.minute

        byte = ser.read(1)

        if byte == b'\xAA':
            cabecalho = ser.read(3)
            if cabecalho == b'\xBB\xCC\xDD':
                restante = ser.read(60)
                msg = b'\xAA\xBB\xCC\xDD' + restante
                if len(msg) == 64:
                    luminosidade = struct.unpack("<f", msg[10:14])[0]
                    temperatura = struct.unpack("<f", msg[14:18])[0]
                    umidade = msg[18]

                    valores = [
                        hora,
                        minuto,
                        luminosidade,
                        temperatura,
                        umidade
                    ]

                    escritor.writerow(valores)
                    arquivo.flush()

                    print("SALVO:", valores)