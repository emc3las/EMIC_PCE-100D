#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <HX711.h>
  
const int RELAIS = 2; // Porta digital D2 usada pelo relé

// Define as conexões da balança e cria o objeto para acesso
const int CELULA_DADO = 11;
const int CELULA_CLOCK = 13;
HX711 celulaCarga;
  
// Define as conexões do display e cria o objeto para acesso
const int RS = 8, EN = 9, D4 = 4, D5 = 5, D6 = 6, D7 = 7;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
  
// Indices para as teclas
const int TEC_DIREITA = 0;
const int TEC_CIMA = 1;
const int TEC_BAIXO = 2;
const int TEC_ESQUERDA = 3;
const int TEC_SELECT = 4;
const int TEC_NENHUMA = 5;
   
// Endereços na EEProm
const int ENDER_FLAG = 0;
const int ENDER_ESCALA = 1;
  
// Indicação de balança calibrada
const byte FLAG_CALIBRADA = 0x55;
  
// Inicialização
float cargaMax = 0;
void setup() {
  Serial.begin(115200);
  pinMode(RELAIS, OUTPUT);
  digitalWrite(RELAIS, HIGH);
  lcd.begin(16, 2);
  // Iniciação do HX711
  celulaCarga.begin(CELULA_DADO, CELULA_CLOCK);
  ajustaTara();
  mostraVersao();
  if (EEPROM.read(ENDER_FLAG) != FLAG_CALIBRADA) {
    calibra();
  } else {
    // Usa escala salva na EEPROM
    float escala;
    EEPROM.get (ENDER_ESCALA, escala);
    celulaCarga.set_scale(escala);
  }
}
  
// Laço principal
void loop() {
  // Lê e mostra a carga
  lcd.clear();
  float carga = celulaCarga.get_units();
  cargaMax = max(carga, cargaMax);
  char medida[17];
  char texto[17] = "Atual  ";
  dtostrf (carga, 5, 0, medida);
  Serial.println(medida);
  strcat(medida, " kN");
  strcat(texto, medida);     
  lcd.setCursor(0,0);
  lcd.print(texto);
  dtostrf (cargaMax, 5, 0, medida);
  strcat(medida, " kN"); 
  sprintf(texto,"Maxima ");
  strcat(texto, medida);   
  lcd.setCursor(0,1);
  lcd.print(texto);  
  
  // Dá um tempo entre as leituras
  delay (500);
   
  // Trata as teclas
  int tecla = leTecla();
  switch(tecla) {
    case TEC_DIREITA:
      lcd.clear();
      lcd.print("P/ tarar  SEL." );
      lcd.setCursor(0,1);
      lcd.print("Outra qq cancela");
      while (leTecla() == TEC_NENHUMA) {
        delay(100);
      }
      if (tecla == TEC_SELECT) {
        ajustaTara();
      }
      break;
    case TEC_CIMA:
      digitalWrite(RELAIS, LOW);
      break;
    case TEC_BAIXO:
      digitalWrite(RELAIS, HIGH);
      break;
    case TEC_ESQUERDA:
      lcd.clear();
      lcd.print("P/ calibrar SEL." );
      lcd.setCursor(0,1);
      lcd.print("Outra qq cancela");
      while (leTecla() == TEC_NENHUMA) {
        delay(100);
      }
      if (tecla == TEC_SELECT) {
        ajustaTara();
        calibra();
      }
      break;
    default:
      // comando(s)
      break;
  }
}

// Apresenta a identificação e versão do programa
void mostraVersao() {
  lcd.clear();
  lcd.print("PH. EMIC PCE-100D");
  delay(1000);
}
  
// Ajusta o offset para o valor da balança vazia
void ajustaTara() {
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Registrando TARA");
  delay(500);
  celulaCarga.tare(50);
}

// Efetua a calibração da balança
void calibra() {
  // Mostra as instruções
  lcd.clear();
  lcd.print ("CALIBRACAO 200KN");
  delay (3000);
  lcd.clear();
  lcd.print("Comprimir 1.6 mm");
  lcd.setCursor(0,1);
  lcd.print("ANEL DINATST 278");
  delay (3000);
  lcd.clear();
  lcd.print ("Aperte SELECT");
  while (leTecla() != TEC_SELECT) {
    delay(100);
  }
  lcd.setCursor(0,1);
  lcd.print("Solte  SELECT");
  while (leTecla() != TEC_NENHUMA) {
    delay(100);
  }
  lcd.clear();
  lcd.print("Aguarde......");
  delay(1000); 
  // Faz a leitura e calcula a escala
  long leitura = celulaCarga.read_average(50);
  float escala = (leitura - celulaCarga.get_offset())/200.00f;
  // Salva na EEProm
  EEPROM.put(ENDER_ESCALA, escala);
  EEPROM.write(ENDER_FLAG, FLAG_CALIBRADA);
  // Usa a escala calculada
  celulaCarga.set_scale(escala);
}

// Le uma tecla (cada tecla do shield causa uma tensão diferente em A0)
int leTecla() {
  static const int limiteTecla[] = {50, 195, 380, 550, 800, 1500}; // Limites para detecção das teclas
  int leitura = analogRead(A0);
  for (int tecla = 0; tecla < 6; tecla++) {
    if (leitura < limiteTecla[tecla]) {
      return tecla;
    }
  }
  return TEC_NENHUMA;
}