#include <Arduino.h>
#include <HX711.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>          // Biblioteca de Wi-Fi, já vem embutida no framework do ESP32
#include <PubSubClient.h>  // Biblioteca de MQTT que acabamos de adicionar no platformio.ini

// --- CONFIGURAÇÃO DE WI-FI ---
// >>> SUBSTITUA pelos dados da sua rede <
const char* WIFI_SSID = "202";  // nome da sua rede Wi-Fi
const char* WIFI_SENHA = "1234567890ap202";

// --- CONFIGURAÇÃO DO BROKER MQTT ---
const char* MQTT_BROKER = "192.168.15.2";  // IP do seu PC, onde o Mosquitto está rodando
const int MQTT_PORTA = 1883;
const char* MQTT_TOPICO = "chope/canal1/peso";  // "endereço" onde vamos publicar o peso

// --- PINOS DO HX711 ---
const int PINO_DT  = 4;
const int PINO_SCK = 5;

float FATOR_CALIBRACAO = 302.4550;

// --- OLED ---
#define TELA_LARGURA 128
#define TELA_ALTURA  64
#define OLED_RESET   -1
Adafruit_SSD1306 display(TELA_LARGURA, TELA_ALTURA, &Wire, OLED_RESET);

HX711 balanca;

// --- Objetos de rede: um cuida da conexão Wi-Fi "crua",
// o outro usa essa conexão para falar MQTT por cima dela ---
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// --- EMA (igual antes) ---
const float ALPHA = 0.3;
float peso_suavizado_ema = 0;
bool primeira_leitura = true;

float atualizarEMA(float novo_valor) {
  if (primeira_leitura) {
    peso_suavizado_ema = novo_valor;
    primeira_leitura = false;
  } else {
    peso_suavizado_ema = ALPHA * novo_valor + (1 - ALPHA) * peso_suavizado_ema;
  }
  return peso_suavizado_ema;
}

void mostrarNoDisplay(String linha1, String linha2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(linha1);
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.println(linha2);
  display.display();
}

// --- FUNÇÃO: conectar no Wi-Fi ---
void conectarWiFi() {
  Serial.print("Conectando no WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_SENHA);

  // WiFi.status() devolve o estado atual da conexão. Ficamos num loop
  // simples esperando até o status virar "conectado" (WL_CONNECTED)
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);          // aqui um delay curto é aceitável, é só durante a conexão inicial
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi conectado! IP do ESP32: ");
  Serial.println(WiFi.localIP());  // mostra o IP que o roteador deu pro ESP32 - útil pra debug
}

// --- FUNÇÃO: (re)conectar no broker MQTT ---
// Criamos uma função separada porque a conexão MQTT pode cair
// (Wi-Fi instável, broker reiniciado, etc.) - assim conseguimos
// tentar reconectar automaticamente sempre que necessário.
void reconectarMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando no broker MQTT...");

    // "ESP32_Canal1" é o "nome de identificação" único desse dispositivo
    // no broker - cada ESP32 vai precisar de um nome diferente quando
    // tivermos mais de um (senão eles brigam pela mesma identidade)
    if (mqttClient.connect("ESP32_Canal1")) {
      Serial.println(" conectado!");
    } else {
      Serial.print(" falhou, codigo de erro: ");
      Serial.print(mqttClient.state());
      Serial.println(" tentando de novo em 2 segundos");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== LEITURA DE PESO - CANAL 1 (com MQTT) ===");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("ERRO: display OLED nao encontrado no endereco 0x3C.");
  } else {
    mostrarNoDisplay("Iniciando...", "");
  }

  balanca.begin(PINO_DT, PINO_SCK);
  balanca.set_scale(FATOR_CALIBRACAO);
  balanca.tare(20);

  conectarWiFi();

  // Diz para a biblioteca MQTT qual endereço e porta usar
  mqttClient.setServer(MQTT_BROKER, MQTT_PORTA);

  Serial.println("Pronto. Balanca zerada e calibrada.");
  Serial.println();
}

// --- Controle de tempo sem usar delay() ---
unsigned long ultimoEnvio = 0;              // guarda "quando" foi o último envio
const unsigned long INTERVALO_ENVIO = 2000; // enviar a cada 2000ms (2 segundos)

void loop() {
  // Garante que o Wi-Fi continua conectado; se cair, tenta de novo
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // Garante que o MQTT continua conectado
  if (!mqttClient.connected()) {
    reconectarMQTT();
  }

  // client.loop() precisa ser chamado com frequência - é o que mantém
  // a "conversa" com o broker viva por baixo dos panos (envia pings,
  // processa mensagens recebidas, etc.)
  mqttClient.loop();

  if (balanca.is_ready()) {
    float peso_g_bruto = balanca.get_units(1);
    float peso_kg_bruto = peso_g_bruto / 1000.0;
    float peso_kg_suavizado = atualizarEMA(peso_kg_bruto);

    mostrarNoDisplay("Peso (EMA):", String(peso_kg_suavizado, 2) + " kg");

    // --- Envio via MQTT, controlado por tempo, sem travar o loop ---
    unsigned long agora = millis();  // millis() = quanto tempo (em ms) o ESP32 está ligado
    if (agora - ultimoEnvio >= INTERVALO_ENVIO) {
      ultimoEnvio = agora;

      // Convertemos o número (float) para texto (String), porque
      // MQTT só transmite dados como texto/bytes, não "números" diretamente
      String payload = String(peso_kg_suavizado, 2);

      mqttClient.publish(MQTT_TOPICO, payload.c_str());

      Serial.print("Publicado no topico ");
      Serial.print(MQTT_TOPICO);
      Serial.print(": ");
      Serial.println(payload);
    }
  } else {
    Serial.println("HX711 nao encontrado. Verifique a fiacao.");
    mostrarNoDisplay("Erro:", "HX711 nao\nencontrado");
  }

  delay(100);  // esse delay curto ainda é ok - só afeta a leitura do sensor,
               // o envio MQTT já está controlado separadamente pelo millis()
}