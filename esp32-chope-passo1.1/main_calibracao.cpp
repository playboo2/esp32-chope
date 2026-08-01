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

// Pinos de conexão do HX711 no ESP32
const int PINO_DT  = 4;
const int PINO_SCK = 5;

HX711 balanca;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== CALIBRACAO DO CANAL 1 ===");

  balanca.begin(PINO_DT, PINO_SCK);

  Serial.println("Aguarde, estabilizando leitura com a plataforma vazia...");
  delay(2000);

  // Zera a leitura com a plataforma vazia (tara)
  balanca.set_scale();       // sem fator ainda, só leitura bruta
  balanca.tare(20);          // faz a média de 20 leituras para tarar

  Serial.println("Tara concluida. Plataforma zerada.");
  Serial.println();
  Serial.println("Agora coloque o peso conhecido sobre a plataforma.");
  Serial.println("Digite o peso EM GRAMAS no Monitor Serial e aperte Enter.");
  Serial.println("Exemplo: se o peso e de 5kg, digite 5000");
}

void loop() {
  // Mostra a leitura bruta (raw) continuamente, para acompanhar
  if (balanca.is_ready()) {
    long leitura_bruta = balanca.get_units(5); // média de 5 leituras
    Serial.print("Leitura bruta atual: ");
    Serial.println(leitura_bruta);
  } else {
    Serial.println("HX711 nao encontrado. Verifique a fiacao (DT/SCK/VCC/GND).");
  }

  // Verifica se o usuário digitou um peso conhecido no Monitor Serial
  if (Serial.available() > 0) {
    float peso_conhecido_g = Serial.parseFloat();

    // Limpa o buffer serial
    while (Serial.available() > 0) Serial.read();

    if (peso_conhecido_g > 0) {
      long leitura_com_peso = balanca.get_units(10); // média de 10 leituras
      float fator_calibracao = (float) leitura_com_peso / peso_conhecido_g;

      Serial.println();
      Serial.println("=== RESULTADO DA CALIBRACAO ===");
      Serial.print("Peso conhecido informado (g): ");
      Serial.println(peso_conhecido_g);
      Serial.print("Leitura bruta com o peso: ");
      Serial.println(leitura_com_peso);
      Serial.print(">>> FATOR DE CALIBRACAO: ");
      Serial.println(fator_calibracao, 4);
      Serial.println("Anote esse valor - vamos usar no proximo arquivo.");
      Serial.println("================================");
      Serial.println();
    }
  }

  delay(500);
}
