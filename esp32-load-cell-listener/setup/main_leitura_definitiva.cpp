/*
  PASSO 1 - LEITURA DEFINITIVA DO CANAL (1 célula de carga + 1 HX711 + 1 ESP32)
  -----------------------------------------------------------------------------
  Use este arquivo DEPOIS de ter descoberto o FATOR DE CALIBRACAO com o
  main_calibracao.cpp.

  Substitua o valor de FATOR_CALIBRACAO abaixo pelo número que você
  anotou (ex: 420.55).

  Para usar: renomeie este arquivo para "main.cpp" dentro da pasta src/
  (só pode existir um main.cpp por vez no projeto).
*/

#include <Arduino.h>
#include <HX711.h>

// Pinos de conexão do HX711 no ESP32
const int PINO_DT  = 4;
const int PINO_SCK = 5;

// >>> SUBSTITUA pelo valor encontrado na calibração <<<
float FATOR_CALIBRACAO = 420.55;

HX711 balanca;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== LEITURA DE PESO - CANAL 1 ===");

  balanca.begin(PINO_DT, PINO_SCK);

  balanca.set_scale(FATOR_CALIBRACAO); // aplica o fator encontrado
  balanca.tare(20);                    // zera com a plataforma vazia

  Serial.println("Pronto. Balanca zerada e calibrada.");
  Serial.println("Coloque o barril (ou peso de teste) para ver a leitura.");
  Serial.println();
}

void loop() {
  if (balanca.is_ready()) {
    // get_units já aplica o fator de calibração e devolve em kg
    // (porque calibramos usando gramas / fator -> aqui dividimos por 1000)
    float peso_kg = balanca.get_units(5) / 1000.0;

    Serial.print("Peso: ");
    Serial.print(peso_kg, 2);
    Serial.println(" kg");
  } else {
    Serial.println("HX711 nao encontrado. Verifique a fiacao.");
  }

  delay(500);
}
