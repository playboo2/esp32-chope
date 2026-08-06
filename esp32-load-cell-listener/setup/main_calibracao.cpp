/*
  PASSO 1 - CALIBRAÇÃO DE UM CANAL (1 célula de carga + 1 HX711 + 1 ESP32)
  ---------------------------------------------------------------------
  Objetivo: descobrir o "fator de calibração" da SUA célula de carga.
  Esse fator é individual - mesmo duas células do mesmo modelo/lote
  costumam ter valores levemente diferentes.

  COMO USAR:
  1) Renomeie este arquivo para "main.cpp" dentro da pasta src/
     (ou copie o conteúdo para dentro do main.cpp existente).
  2) Monte o circuito conforme o esquema (DT no GPIO 4, SCK no GPIO 5).
  3) Deixe a plataforma de pesagem VAZIA (sem nenhum peso em cima).
  4) Compile e faça upload (ícone de seta no rodapé do VSCode/PlatformIO).
  5) Abra o Monitor Serial do PlatformIO (ícone de tomada, ou Ctrl+Alt+S).
  6) Aguarde a leitura estabilizar e a tara ser feita automaticamente.
  7) Coloque um peso conhecido em cima da plataforma.
  8) Digite no Monitor Serial o peso conhecido EM GRAMAS e aperte enter
     (ex: se usou um peso de 5kg, digite: 5000)
  9) O sketch vai calcular e mostrar o FATOR DE CALIBRACAO.
  10) Anote esse número - vamos usar no main_leitura_definitiva.cpp.
*/

#include <Arduino.h>
#include <HX711.h>
#include "config_balancas.h"   // <--- nosso arquivo de config

HX711 balanca;

int  canalAtual        = 0;
bool canalInicializado = false;
bool calibTerminado    = false;

float fatoresCalibracao[NUM_LOAD_CELLS];
unsigned long ultimoPrint = 0;
const unsigned long INTERVALO_PRINT = 500;

void iniciarCalibracaoCanal(int idx) {
    const auto &cfg = LOAD_CELLS[idx];

    Serial.println();
    Serial.println("================================");
    Serial.print("Iniciando CALIBRACAO do ");
    Serial.println(cfg.nome);
    Serial.print("Pinos: DT=");
    Serial.print(cfg.pinoDT);
    Serial.print(" SCK=");
    Serial.println(cfg.pinoSCK);
    Serial.println("================================");

    balanca.begin(cfg.pinoDT, cfg.pinoSCK);

    Serial.println("Aguarde, estabilizando leitura com a plataforma VAZIA...");
    delay(2000);

    balanca.set_scale();    // sem fator
    balanca.tare(20);       // média de 20 leituras

    Serial.println("Tara concluida. Plataforma zerada.");
    Serial.println();
    Serial.println("Agora coloque o PESO CONHECIDO sobre a plataforma deste canal.");
    Serial.println("Digite o peso EM GRAMAS no Monitor Serial e aperte Enter.");
    Serial.println("Exemplo: se o peso e de 5kg, digite: 5000");
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=== CALIBRACAO MULTIPLAS CELULAS DE CARGA ===");
    Serial.print("Numero de canais configurados: ");
    Serial.println(NUM_LOAD_CELLS);
    Serial.println();

    iniciarCalibracaoCanal(canalAtual);
    canalInicializado = true;
}

void loop() {
    if (calibTerminado) {
        delay(1000);
        return;
    }

    if (canalAtual >= NUM_LOAD_CELLS) {
        Serial.println();
        Serial.println("=== TODAS AS CELULAS FORAM CALIBRADAS ===");
        Serial.println("RESUMO DOS FATORES DE CALIBRACAO:");
        Serial.println();

        for (int i = 0; i < NUM_LOAD_CELLS; i++) {
            const auto &cfg = LOAD_CELLS[i];
            Serial.print(cfg.nome);
            Serial.print(" (DT=");
            Serial.print(cfg.pinoDT);
            Serial.print(", SCK=");
            Serial.print(cfg.pinoSCK);
            Serial.print(") -> FATOR = ");
            Serial.println(fatoresCalibracao[i], 4);
        }

        Serial.println("==========================================");
        Serial.println("Copie estes fatores para config_balancas.h em fatorCalibracao.");
        calibTerminado = true;
        return;
    }

    if (!canalInicializado) {
        iniciarCalibracaoCanal(canalAtual);
        canalInicializado = true;
    }

    unsigned long agora = millis();
    if (agora - ultimoPrint >= INTERVALO_PRINT) {
        ultimoPrint = agora;
        if (balanca.is_ready()) {
            long leitura_bruta = balanca.get_units(5);
            Serial.print("[");
            Serial.print(LOAD_CELLS[canalAtual].nome);
            Serial.print("] Leitura bruta atual: ");
            Serial.println(leitura_bruta);
        } else {
            Serial.println("HX711 nao encontrado. Verifique a fiacao.");
        }
    }

    if (Serial.available() > 0) {
        float peso_conhecido_g = Serial.parseFloat();
        while (Serial.available() > 0) Serial.read();

        if (peso_conhecido_g > 0) {
            long leitura_com_peso = balanca.get_units(10);
            float fator = (float)leitura_com_peso / peso_conhecido_g;
            fatoresCalibracao[canalAtual] = fator;

            Serial.println();
            Serial.println("=== RESULTADO DA CALIBRACAO ===");
            Serial.print("Canal: ");
            Serial.println(LOAD_CELLS[canalAtual].nome);
            Serial.print("Peso conhecido (g): ");
            Serial.println(peso_conhecido_g);
            Serial.print("Leitura bruta: ");
            Serial.println(leitura_com_peso);
            Serial.print(">>> FATOR DE CALIBRACAO: ");
            Serial.println(fator, 4);
            Serial.println("================================");
            Serial.println();

            canalAtual++;
            canalInicializado = false;

            if (canalAtual < NUM_LOAD_CELLS) {
                Serial.println("Remova o peso da celula atual e coloque na PROXIMA.");
            }
        } else {
            Serial.println("Valor invalido. Digite apenas o numero em gramas.");
        }
    }
}