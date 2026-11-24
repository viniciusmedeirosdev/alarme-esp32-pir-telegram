#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

// configuracoes do projeto
#define PIR_PIN 13
#define LED_PIN 18      // led que pisca enquanto tiver movimento
#define BUZZER_PIN 19   // buzzer do alarme


// tarefa que eu uso no lugar do loop pra mandar a mensagem
TaskHandle_t tarefaTelegram = NULL;

// troca os espacos e acentos do texto pra poder mandar na requisicao
String urlEncode(const String& s) {
  String r; char hex[4];
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (isalnum((unsigned char)c) || c=='-' || c=='_' || c=='.' || c=='~') r += c;
    else { sprintf(hex, "%%%02X", (unsigned char)c); r += hex; }
  }
  return r;
}

// manda a mensagem pro telegram usando a api do bot
void enviarTelegram(const String& texto) {
  // Sem credenciais, funciona apenas o alarme local.
  if (!BOT_TOKEN[0] || !CHAT_ID[0]) return;
  WiFiClientSecure client;
  client.setInsecure();               // nao valida o certificado, so pra funcionar o https
  HTTPClient https;
  String url = String("https://api.telegram.org/bot") + BOT_TOKEN + "/sendMessage";
  https.begin(client, url);
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  // se der erro mostra o que o telegram respondeu
  int codigo = https.POST("chat_id=" + String(CHAT_ID) + "&text=" + urlEncode(texto));
  if (codigo != 200) {
    Serial.printf("Erro no Telegram: HTTP %d\n", codigo);
    Serial.println(https.getString());
  }
  https.end();
}

// tarefa que fica parada esperando a interrupcao avisar que teve movimento
// fiz separado porque mandar mensagem demora e nao pode ficar dentro da interrupcao
void tarefaEnvio(void* parametro) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);   // fica esperando aqui
    Serial.println("Movimento detectado!");
    enviarTelegram("Movimento detectado pelo sensor PIR!");
  }
}

// tarefa do alarme: enquanto o PIR estiver em HIGH o led pisca
// e o buzzer faz som de sirene alternando entre dois tons
void tarefaAlarme(void* parametro) {
  bool estado = false;
  bool tocando = false;
  for (;;) {
    if (digitalRead(PIR_PIN) == HIGH) {
      estado = !estado;
      digitalWrite(LED_PIN, estado);
      tone(BUZZER_PIN, estado ? 2000 : 1200);   // sirene de dois tons
      tocando = true;
      vTaskDelay(pdMS_TO_TICKS(250));
    } else {
      if (tocando) {               // acabou o movimento: desliga tudo
        noTone(BUZZER_PIN);
        digitalWrite(LED_PIN, LOW);
        tocando = false;
        estado = false;
      }
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }
}

// interrupcao do sensor, e chamada quando o pino do PIR vai pra HIGH
// aqui so acorda a tarefa que manda a mensagem
void IRAM_ATTR handleInterrupt() {
  BaseType_t acordou = pdFALSE;
  if (tarefaTelegram != NULL) {
    vTaskNotifyGiveFromISR(tarefaTelegram, &acordou);
  }
  if (acordou) portYIELD_FROM_ISR();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // o alarme local roda numa tarefa propria, funciona mesmo sem wi-fi
  xTaskCreate(tarefaAlarme, "alarme", 4096, NULL, 2, NULL);

  // canal 6 e o canal da rede do wokwi, assim conecta mais rapido
  WiFi.mode(WIFI_STA);        // so modo estacao, sem access point
  WiFi.setSleep(false);       // sem economia de energia, responde mais rapido
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);
  Serial.print("Conectando ao Wi-Fi");
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
    // so tenta de novo se passar de 30 segundos (antes reiniciava com 10s
    // e podia cortar uma conexao que estava quase pronta)
    if (millis() - inicio > 30000) {
      Serial.println(" demorou, tentando de novo");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);
      inicio = millis();
    }
  }
  Serial.printf(" conectado em %lu ms!\n", millis() - inicio);

  xTaskCreate(tarefaEnvio, "telegram", 8192, NULL, 1, &tarefaTelegram);
  enviarTelegram("ESP32 online - sensor PIR pronto");
  Serial.println("Sensor PIR pronto");

  // liga a interrupcao no pino do sensor
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), handleInterrupt, RISING);

  // apaga a tarefa do loop, daqui pra frente so a interrupcao funciona
  vTaskDelete(NULL);
}

// o arduino nao compila sem o loop, mas ele nunca roda
// porque eu apago a tarefa dele no final do setup
void loop() {}
