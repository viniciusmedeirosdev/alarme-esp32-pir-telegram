# Alarme ESP32 com PIR e Telegram

Alarme de movimento com ESP32, sensor PIR, LED e buzzer. Enquanto o PIR está ativo, o LED pisca e a sirene alterna entre 1200 e 2000 Hz a cada 250 ms. O envio de notificações pelo Telegram é opcional e fica desativado com as credenciais vazias fornecidas neste repositório.

Projeto de origem: [PIR funcionando no Wokwi](https://wokwi.com/projects/480238657361155073). O crédito original do diagrama, `Anonymous maker`, foi preservado.

## Componentes e conexões

Montagem do `diagram.json`, para simulação:

| Componente | Conexão no ESP32 DevKitC V4 |
|---|---|
| PIR | OUT no GPIO13; VCC em 3V3; GND em GND |
| LED vermelho | GPIO18 → resistor de 220 Ω → ânodo; cátodo em GND |
| Buzzer | Pino 2 no GPIO19; pino 1 em GND |

## Funcionamento

O programa usa duas tarefas FreeRTOS. A tarefa do alarme lê o PIR e controla LED e buzzer, funcionando mesmo durante a espera pelo Wi-Fi. Após a conexão, uma interrupção na borda de subida do GPIO13 notifica a tarefa de envio; a requisição HTTPS ocorre nessa tarefa, fora da interrupção. Com credenciais configuradas, há também uma mensagem ao conectar. A tarefa que executaria `loop()` é encerrada ao final de `setup()`.

## Simular no Wokwi

1. Crie um [projeto ESP32](https://wokwi.com/projects/new/esp32).
2. Copie os arquivos `sketch.ino`, `diagram.json` e `secrets.example.h` deste repositório para o projeto.
3. Mantenha `BOT_TOKEN` e `CHAT_ID` vazios. Inicie a simulação; a rede usada é `Wokwi-GUEST`, sem senha.
4. Clique no PIR e selecione **Simulate Motion**. Observe o LED, a sirene e o monitor serial em 115200 baud.

Por padrão, o PIR mantém a saída ativa por 5 segundos e ignora novos movimentos por 1,2 segundo após desativá-la. Novos eventos durante o período ativo prolongam esse período.

## Habilitar o Telegram localmente

Copie `secrets.example.h` para `secrets.h` na mesma pasta do sketch e preencha a rede Wi-Fi, `BOT_TOKEN` e `CHAT_ID`. O código usa `secrets.h` quando presente; caso contrário, usa o exemplo. `secrets.h` está no `.gitignore` e deve permanecer apenas local: não copie credenciais para um projeto público do Wokwi.

O HTTPS herdado do protótipo usa `setInsecure()`, que desativa a validação do certificado do servidor.

## Verificação

Foi feita revisão estática do código e da estrutura JSON do diagrama. Compilação, simulação, funcionamento em hardware e envio pelo Telegram não foram verificados nesta publicação.

Referências: [PIR no Wokwi](https://docs.wokwi.com/parts/wokwi-pir-motion-sensor), [Wi-Fi no Wokwi](https://docs.wokwi.com/guides/esp32-wifi), [Arduino ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/).
