// Exemplo de configuração — NÃO conter segredos neste arquivo.
// Copie este arquivo para `config_balancas.h` e preencha os valores reais.
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
    // DT, SCK,  nome,                   tópico MQTT,            fatorCalibracao
    { 4,   5,  "NOME_DA_SUA_BALANCA", "seu/topico/mqtt",      0.0 },
};
// ^ depois da calibração você só vem aqui e atualiza fatorCalibracao

// Wi-Fi
static const char* WIFI_SSID  = "NOME_DA_SUA_REDE";
static const char* WIFI_SENHA = "SENHA_DA_SUA_REDE";

// MQTT broker
static const char* MQTT_BROKER = "192.168.X.X";
static const int   MQTT_PORTA  = 1883;
