#include <Arduino.h>
#include <HX711.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- CONFIGURAÇÃO DE WI-FI ---
const char* WIFI_SSID  = "202";
const char* WIFI_SENHA = "1234567890ap202";

// --- CONFIGURAÇÃO DO BROKER MQTT ---
const char* MQTT_BROKER = "192.168.15.2";
const int   MQTT_PORTA  = 1883;

// --- OLED ---
#define TELA_LARGURA 128
#define TELA_ALTURA  64
#define OLED_RESET   -1
Adafruit_SSD1306 display(TELA_LARGURA, TELA_ALTURA, &Wire, OLED_RESET);

// --- Objetos de rede ---
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// --- EMA ---
const float ALPHA = 0.3;

// ====== CONFIGURAÇÃO DAS LOAD CELLS ======
struct LoadCellConfig {
  int   pinoDT;
  int   pinoSCK;
  float fatorCalibracao;
  const char* mqttTopico;
};

struct LoadCellState {
  HX711 hx;
  float ema;
  bool  primeiraLeitura;
  float ultimoKg;
};

// Ajuste aqui o número de balanças e suas configs
const int NUM_LOAD_CELLS = 1;

LoadCellConfig loadCellsConfig[NUM_LOAD_CELLS] = {
  // pinoDT, pinoSCK, fatorCalibracao,          tópico MQTT
  { 4,      5,       302.4550,        "chope/canal1/peso" },
};

LoadCellState loadCells[NUM_LOAD_CELLS];

// --- Controle de tempo sem usar delay() ---
unsigned long ultimoEnvio = 0;
const unsigned long INTERVALO_ENVIO = 2000; // 2s para publicar

// ====== FUNÇÕES AUXILIARES ======
float atualizarEMA(float novo_valor, float &ema, bool &primeira_leitura) {
  if (primeira_leitura) {
    ema = novo_valor;
    primeira_leitura = false;
  } else {
    ema = ALPHA * novo_valor + (1 - ALPHA) * ema;
  }
  return ema;
}

void mostrarPesosNoDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  for (int i = 0; i < NUM_LOAD_CELLS; i++) {
    display.print("C");
    display.print(i + 1);
    display.print(": ");
    display.print(loadCells[i].ultimoKg, 2);
    display.println(" kg");
  }

  display.display();
}

void mostrarMensagemDisplay(String linha1, String linha2) {
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

// --- Wi-Fi ---
void conectarWiFi() {
  Serial.print("Conectando no WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_SENHA);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi conectado! IP do ESP32: ");
  Serial.println(WiFi.localIP());
}

// --- MQTT ---
void reconectarMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando no broker MQTT...");
    if (mqttClient.connect("ESP32_MultiLoadCell")) { // ID único
      Serial.println(" conectado!");
    } else {
      Serial.print(" falhou, codigo de erro: ");
      Serial.print(mqttClient.state());
      Serial.println(" tentando de novo em 2 segundos");
      delay(2000);
    }
  }
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== LEITURA DE PESO - MULTIPLAS LOAD CELLS (MQTT) ===");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("ERRO: display OLED nao encontrado no endereco 0x3C.");
  } else {
    mostrarMensagemDisplay("Iniciando...", "");
  }

  // Inicializa todas as load cells
  for (int i = 0; i < NUM_LOAD_CELLS; i++) {
    auto &cfg = loadCellsConfig[i];
    auto &st  = loadCells[i];

    st.hx.begin(cfg.pinoDT, cfg.pinoSCK);
    st.hx.set_scale(cfg.fatorCalibracao);
    st.hx.tare(20);
    st.ema = 0.0;
    st.primeiraLeitura = true;
    st.ultimoKg = 0.0;

    Serial.print("LoadCell ");
    Serial.print(i + 1);
    Serial.print(" DT=");
    Serial.print(cfg.pinoDT);
    Serial.print(" SCK=");
    Serial.print(cfg.pinoSCK);
    Serial.print(" fator=");
    Serial.println(cfg.fatorCalibracao);
  }

  conectarWiFi();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORTA);

  Serial.println("Pronto. Todas as balancas zeradas e calibradas.");
  Serial.println();
}

// ====== LOOP ======
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  if (!mqttClient.connected()) {
    reconectarMQTT();
  }

  mqttClient.loop();

  // Leitura de todas as load cells
  for (int i = 0; i < NUM_LOAD_CELLS; i++) {
    auto &cfg = loadCellsConfig[i];
    auto &st  = loadCells[i];

    if (st.hx.is_ready()) {
      float peso_g_bruto = st.hx.get_units(1);
      float peso_kg_bruto = peso_g_bruto / 1000.0;
      float peso_kg_suavizado = atualizarEMA(peso_kg_bruto, st.ema, st.primeiraLeitura);

      st.ultimoKg = peso_kg_suavizado;

      // Log serial por canal
      Serial.print("Canal ");
      Serial.print(i + 1);
      Serial.print(" -> ");
      Serial.print(peso_kg_suavizado, 2);
      Serial.println(" kg");
    } else {
      Serial.print("HX711 do canal ");
      Serial.print(i + 1);
      Serial.println(" nao encontrado. Verifique a fiacao.");
    }
  }

  // Atualiza display com todos os pesos
  mostrarPesosNoDisplay();

  // Envio MQTT (todos os canais de uma vez a cada INTERVALO_ENVIO)
  unsigned long agora = millis();
  if (agora - ultimoEnvio >= INTERVALO_ENVIO) {
    ultimoEnvio = agora;

    for (int i = 0; i < NUM_LOAD_CELLS; i++) {
      auto &cfg = loadCellsConfig[i];
      auto &st  = loadCells[i];

      String payload = String(st.ultimoKg, 2);
      mqttClient.publish(cfg.mqttTopico, payload.c_str());

      Serial.print("Publicado no topico ");
      Serial.print(cfg.mqttTopico);
      Serial.print(": ");
      Serial.println(payload);
    }
  }

  delay(100); // só para não espancar o HX711/serial
}