// src/network.cpp
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include "config_balancas.h"
#include "network.h"

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

static IPAddress mqttIp;
static uint16_t mqttPort = 1883;
static bool mqttServerConfigured = false;

static void conectarWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;

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

static void descobrirMqttViaMDNS() {
    Serial.println("Procurando broker MQTT via mDNS (_mqtt._tcp.local)...");
    int n = MDNS.queryService("mqtt", "tcp");

    if (n == 0) {
        Serial.println("Nenhum servico MQTT encontrado via mDNS.");
        return;
    }

    mqttIp   = MDNS.IP(0);
    mqttPort = MDNS.port(0);
    mqttServerConfigured = true;

    mqttClient.setServer(mqttIp, mqttPort);

    Serial.print("Broker MQTT encontrado em ");
    Serial.print(mqttIp);
    Serial.print(":");
    Serial.println(mqttPort);
}

static void reconectarMQTT() {
    if (!mqttServerConfigured) return;

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

void networkSetup() {
    conectarWiFi();

    if (!MDNS.begin("esp32-balancas")) {
        Serial.println("Falha ao iniciar mDNS.");
    } else {
        Serial.println("mDNS iniciado como esp32-balancas.local");
        descobrirMqttViaMDNS();
    }

    // Opcional: fallback para config fixa
    // if (!mqttServerConfigured) {
    //     mqttClient.setServer(MQTT_BROKER, MQTT_PORTA);
    //     mqttServerConfigured = true;
    // }

    if (mqttServerConfigured) {
        reconectarMQTT();
    } else {
        Serial.println("ATENCAO: nenhum broker MQTT configurado (mDNS falhou).");
    }
}

void networkLoop() {
    if (WiFi.status() != WL_CONNECTED) {
        conectarWiFi();
    }
    if (mqttServerConfigured && !mqttClient.connected()) {
        reconectarMQTT();
    }
    mqttClient.loop();
}

bool mqttPublish(const char* topic, const char* payload) {
    if (!mqttServerConfigured || !mqttClient.connected()) {
        Serial.println("mqttPublish: MQTT nao conectado, nao foi possivel publicar.");
        return false;
    }
    bool ok = mqttClient.publish(topic, payload);
    if (!ok) {
        Serial.println("mqttPublish: falha ao publicar.");
    }
    return ok;
}