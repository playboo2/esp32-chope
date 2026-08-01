#pragma once
#include <Arduino.h>

// Configuração de cada load cell / HX711
struct LoadCellConfig {
    int   pinoDT;
    int   pinoSCK;
    const char* nome;          // só pra logs / calibração
    const char* mqttTopico;    // usado no código final
    float fatorCalibracao;     // 0.0 enquanto não calibrado
};

// Número de balanças
static const int NUM_LOAD_CELLS = 1;

// <<< EDITE SOMENTE ESTE BLOCO PARA CONFIGURAR O SISTEMA >>>
static const LoadCellConfig LOAD_CELLS[NUM_LOAD_CELLS] = {
    // DT, SCK,  nome,      tópico MQTT,           fatorCalibracao
    { 4,   5,  "Canal 1", "chope/canal1/peso", 302.4550 },
};
// ^ depois da calibração você só vem aqui e atualiza fatorCalibracao

// Wi-Fi
static const char* WIFI_SSID  = "202";
static const char* WIFI_SENHA = "1234567890ap202";

// MQTT broker
static const char* MQTT_BROKER = "192.168.15.2";
static const int   MQTT_PORTA  = 1883;