-- SmartHorta — Criação do Banco (Machbase Neo)
-- Executar no painel web (http://localhost:5654) ou via machbase-neo shell.

-- NAME        = sensor_id (ex: "LUZ-A1-01", "TMP-A1-01", "UMI-A1-01")
-- TIME        = timestamp da leitura
-- VALOR       = valor numérico (SUMMARIZED gera view v$leituras_stat automaticamente)
-- CANTEIRO_ID = identificador do canteiro (ex: "A1", "B1")
-- VARIAVEL    = variável medida (ex: "luminosidade", "temperatura", "umidade_solo")
-- UNIDADE     = unidade de medida (ex: "lux", "°C", "%")

CREATE TAG TABLE IF NOT EXISTS leituras (
    NAME        VARCHAR(80)  PRIMARY KEY,
    TIME        DATETIME     BASETIME,
    VALOR       DOUBLE       SUMMARIZED,
    CANTEIRO_ID VARCHAR(10),
    VARIAVEL    VARCHAR(30),
    UNIDADE     VARCHAR(10)
);

-- Consultas úteis:
-- SELECT * FROM leituras WHERE NAME = 'LUZ-A1-01' ORDER BY TIME DESC LIMIT 100;
-- SELECT AVG(VALOR) FROM leituras WHERE NAME = 'TMP-A1-01' AND TIME > NOW() - 3600000000000;
-- SELECT * FROM v$leituras_stat WHERE NAME = 'UMI-A1-01';
