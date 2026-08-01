// include/network.h
#pragma once

#include <Arduino.h>

// Inicializa WiFi + descoberta MQTT via mDNS + configura PubSubClient
void networkSetup();

// Mantém WiFi/MQTT vivos (chamar em todo loop)
void networkLoop();

// Publica uma mensagem MQTT (retorna false se não estiver conectado)
bool mqttPublish(const char* topic, const char* payload);