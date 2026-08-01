#include <Arduino.h>
#include <HX711.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config_balancas.h"   // <--- mesmo header

// OLED
#define TELA_LARGURA 128
#define TELA_ALTURA  64
#define OLED_RESET   -1
Adafruit_SSD1306 display(TELA_LARGURA, TELA_ALTURA, &Wire, OLED_RESET);

// Rede / MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// EMA
const float ALPHA = 0.3;

struct LoadCellState {
    HX711 hx;
    float ema;
    bool  primeiraLeitura;
    float ultimoKg;
};

LoadCellState loadCellsState[NUM_LOAD_CELLS];

unsigned long ultimoEnvio = 0;
const unsigned long INTERVALO_ENVIO = 2000;

// --- helpers ---
float atualizarEMA(float novo, float &ema, bool &first) {
    if (first) {
        ema = novo;
        first = false;
    } else {
        ema = ALPHA * novo + (1 - ALPHA) * ema;
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
        display.print(loadCellsState[i].ultimoKg, 2);
        display.println(" kg");
    }
    display.display();
}

void mostrarMensagemDisplay(String l1, String l2) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(l1);
    display.setCursor(0, 20);
    display.setTextSize(2);
    display.println(l2);
    display.display();
}

void conectarWiFi() {
    Serial.print("Conectando no WiFi: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_SENHA);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("WiFi conectado! IP: ");
    Serial.println(WiFi.localIP());
}

void reconectarMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Conectando no broker MQTT...");
        if (mqttClient.connect("ESP32_MultiLoadCell")) {
            Serial.println(" conectado!");
        } else {
            Serial.print(" falhou, codigo: ");
            Serial.print(mqttClient.state());
            Serial.println(" tentando de novo em 2s");
            delay(2000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=== MULTIPLAS LOAD CELLS (MQTT) ===");

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("ERRO: OLED nao encontrado.");
    } else {
        mostrarMensagemDisplay("Iniciando...", "");
    }

    // Inicializa HX711 de cada canal
    for (int i = 0; i < NUM_LOAD_CELLS; i++) {
        const auto &cfg = LOAD_CELLS[i];
        auto &st = loadCellsState[i];

        st.hx.begin(cfg.pinoDT, cfg.pinoSCK);
        st.hx.set_scale(cfg.fatorCalibracao);
        st.hx.tare(20);

        st.ema = 0.0f;
        st.primeiraLeitura = true;
        st.ultimoKg = 0.0f;

        Serial.print(cfg.nome);
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
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        conectarWiFi();
    }
    if (!mqttClient.connected()) {
        reconectarMQTT();
    }
    mqttClient.loop();

    // Leitura de todos os canais
    for (int i = 0; i < NUM_LOAD_CELLS; i++) {
        auto &st  = loadCellsState[i];
        const auto &cfg = LOAD_CELLS[i];

        if (st.hx.is_ready()) {
            float peso_g   = st.hx.get_units(1);
            float peso_kg  = peso_g / 1000.0f;
            float suavizado = atualizarEMA(peso_kg, st.ema, st.primeiraLeitura);
            st.ultimoKg = suavizado;

            Serial.print(cfg.nome);
            Serial.print(" -> ");
            Serial.print(suavizado, 2);
            Serial.println(" kg");
        } else {
            Serial.print("HX711 de ");
            Serial.print(cfg.nome);
            Serial.println(" nao encontrado.");
        }
    }

    mostrarPesosNoDisplay();

    unsigned long agora = millis();
    if (agora - ultimoEnvio >= INTERVALO_ENVIO) {
        ultimoEnvio = agora;
        for (int i = 0; i < NUM_LOAD_CELLS; i++) {
            const auto &cfg = LOAD_CELLS[i];
            auto &st = loadCellsState[i];

            String payload = String(st.ultimoKg, 2);
            mqttClient.publish(cfg.mqttTopico, payload.c_str());

            Serial.print("Publicado em ");
            Serial.print(cfg.mqttTopico);
            Serial.print(": ");
            Serial.println(payload);
        }
    }

    delay(100);
}