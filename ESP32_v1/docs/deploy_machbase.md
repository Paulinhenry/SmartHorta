# Deploy do Machbase Neo — SmartHorta

Guia de referência para instalar e rodar o banco de dados Machbase Neo, usado pelo projeto SmartHorta para armazenar leituras dos sensores em tempo real.

---

## Fase 1 — Local (Desenvolvimento e Testes)

### Opção A: Com Docker Desktop (Recomendado)

1. Instale o [Docker Desktop para Windows](https://www.docker.com/products/docker-desktop/).
2. Abra o PowerShell e execute:

```powershell
docker pull machbase/machbase-neo
docker run -d --name machbase -p 5654:5654 -p 5656:5656 -v machbase_data:/data machbase/machbase-neo
```

3. Acesse o painel web: [http://localhost:5654](http://localhost:5654)

### Opção B: Sem Docker (Binário Direto)

1. Baixe o executável em [machbase.com/neo](https://machbase.com/neo)
2. Extraia e execute:

```powershell
.\machbase-neo.exe serve
```

3. Acesse o painel web: [http://localhost:5654](http://localhost:5654)

### Verificação

Abra o navegador em `http://localhost:5654` e execute no editor SQL:

```sql
SELECT NOW();
```

Se retornar a data/hora atual, o banco está funcionando.

---

## Fase 2 — Nuvem Oracle Cloud (Coleta 24h / Produção)

### Pré-requisitos

- Conta na Oracle Cloud: [cloud.oracle.com](https://cloud.oracle.com)
- O tier "Always Free" **não cobra** e não expira.

### Passo a Passo

1. **Criar a VM**:
   - Acesse "Compute > Instances > Create Instance"
   - Imagem: Ubuntu 22.04 (ou superior)
   - Shape: VM.Standard.E2.1.Micro (Always Free)
   - Gere e baixe a chave SSH (.pem)

2. **Conectar via SSH**:

```bash
ssh -i sua_chave.pem ubuntu@<IP_PUBLICO>
```

3. **Instalar Docker e rodar o Machbase**:

```bash
sudo apt update && sudo apt install -y docker.io
sudo docker run -d --name machbase \
  --restart unless-stopped \
  -p 5654:5654 -p 5656:5656 \
  -v machbase_data:/data \
  machbase/machbase-neo
```

4. **Liberar portas no firewall da Oracle**:
   - Vá em "Networking > Virtual Cloud Networks > sua VCN > Security Lists"
   - Adicione regras de entrada (Ingress) para as portas **5654** e **5656** (TCP, origem 0.0.0.0/0)

5. **Liberar portas no firewall do Ubuntu**:

```bash
sudo iptables -I INPUT -p tcp --dport 5654 -j ACCEPT
sudo iptables -I INPUT -p tcp --dport 5656 -j ACCEPT
sudo netfilter-persistent save
```

6. **Verificar acesso remoto**:
   - No navegador do seu PC, acesse: `http://<IP_PUBLICO>:5654`

7. **Atualizar o script Python**:

No arquivo `gravadorSerial.py`, altere:

```python
MACHBASE_URL = "http://<IP_PUBLICO>:5654/db/query"
```

### Segurança (Recomendado)

Para proteger o acesso ao banco na internet, use um túnel SSH ao invés de expor a porta diretamente:

```bash
# No seu PC Windows (PowerShell):
ssh -L 5654:localhost:5654 -i sua_chave.pem ubuntu@<IP_PUBLICO>
```

Assim, o `gravadorSerial.py` continua usando `http://127.0.0.1:5654/db/query` e o tráfego é criptografado.

---

## Comandos Úteis

| Ação | Comando |
|------|---------|
| Ver status do container | `docker ps` |
| Parar o Machbase | `docker stop machbase` |
| Iniciar o Machbase | `docker start machbase` |
| Ver logs | `docker logs machbase` |
| Acessar o shell SQL | `docker exec -it machbase machbase-neo shell` |
| Remover tudo (cuidado!) | `docker rm -f machbase && docker volume rm machbase_data` |
