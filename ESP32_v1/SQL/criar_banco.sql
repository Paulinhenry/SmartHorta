-- ============================================================
-- SmartHorta — Script de Criação do Banco de Dados
-- Banco: Machbase Neo (Time-Series Database)
-- ============================================================
-- Executar este script no painel web do Machbase Neo
-- (http://localhost:5654) ou via machbase-neo shell.
-- ============================================================

-- TAG TABLE para armazenar leituras dos sensores.
--
-- Estrutura:
--   NAME        = sensor_id (ex: "LUZ-A1-01", "TMP-A1-01", "UMI-A1-01")
--   TIME        = timestamp da leitura (gerado automaticamente pelo Python)
--   VALOR       = valor numérico da leitura (SUMMARIZED gera estatísticas automáticas)
--   CANTEIRO_ID = identificador do canteiro (ex: "A1", "B1")
--   VARIAVEL    = nome da variável medida (ex: "luminosidade", "temperatura", "umidade_solo")
--   UNIDADE     = unidade de medida (ex: "lux", "°C", "%")
--
-- A keyword SUMMARIZED na coluna VALOR faz o Machbase gerar automaticamente
-- a view v$leituras_stat com estatísticas (min, max, count, sum) por sensor.

CREATE TAG TABLE IF NOT EXISTS leituras (
    NAME        VARCHAR(80)  PRIMARY KEY,
    TIME        DATETIME     BASETIME,
    VALOR       DOUBLE       SUMMARIZED,
    CANTEIRO_ID VARCHAR(10),
    VARIAVEL    VARCHAR(30),
    UNIDADE     VARCHAR(10)
);

-- ============================================================
-- Exemplos de consulta (para referência)
-- ============================================================

-- Todas as leituras do sensor de luminosidade do canteiro A1:
-- SELECT * FROM leituras WHERE NAME = 'LUZ-A1-01' ORDER BY TIME DESC LIMIT 100;

-- Média de temperatura dos últimos 60 minutos:
-- SELECT AVG(VALOR) FROM leituras WHERE NAME = 'TMP-A1-01' AND TIME > NOW() - 3600000000000;

-- Estatísticas automáticas (geradas pelo SUMMARIZED):
-- SELECT * FROM v$leituras_stat WHERE NAME = 'UMI-A1-01';
