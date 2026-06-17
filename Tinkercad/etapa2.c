#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

const int pinLM35 = A0;
const int pinMotor = 9;
const int pinBuzzer = 3;
const int ledR = 10;
const int ledG = 11;
const int ledB = 12;

LiquidCrystal_I2C lcd(0x27, 16, 2);

enum EstadoFSM { EXIBICAO, SET_MAX, SET_MIN };
EstadoFSM estadoAtual = EXIBICAO;

float limiteMax = 30.0;
float limiteMin = 20.0;
float tempAtual = 0;

volatile bool flagUp = false;
volatile bool flagDown = false;
volatile bool flagEnter = false;

unsigned long tempoUltimoClique = 0;
unsigned long tempoPiscar = 0;
unsigned long tempoAnteriorBuzzer = 0;
bool estadoPiscar = true;
bool estadoBuzzer = false;

ISR(PCINT2_vect) {
  if (!(PIND & (1 << PD4))) flagUp = true;
  if (!(PIND & (1 << PD5))) flagDown = true;
  if (!(PIND & (1 << PD6))) flagEnter = true;
}

void setup() {
  lcd.init();
  lcd.backlight();
  pinMode(pinMotor, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  PORTD |= (1 << PD4) | (1 << PD5) | (1 << PD6);

  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT20) | (1 << PCINT21) | (1 << PCINT22);

  EEPROM.get(10, limiteMax);
  EEPROM.get(14, limiteMin);
  if (isnan(limiteMax)) limiteMax = 30.0;
  if (isnan(limiteMin)) limiteMin = 20.0;
}

void loop() {
  tempAtual = (analogRead(pinLM35) * 5.0 / 1024.0) * 100.0;

  if (tempAtual > limiteMax) {
    digitalWrite(ledR, HIGH);
    digitalWrite(ledG, LOW);
    digitalWrite(ledB, LOW);
    digitalWrite(pinMotor, HIGH);
    if (millis() - tempoAnteriorBuzzer > 300) {
      tempoAnteriorBuzzer = millis();
      estadoBuzzer = !estadoBuzzer;
      if (estadoBuzzer) tone(pinBuzzer, 1000);
      else noTone(pinBuzzer);
    }
  } else if (tempAtual < limiteMin) {
    digitalWrite(ledR, LOW);
    digitalWrite(ledG, LOW);
    digitalWrite(ledB, HIGH);
    digitalWrite(pinMotor, LOW);
    noTone(pinBuzzer);
  } else {
    digitalWrite(ledR, LOW);
    digitalWrite(ledG, HIGH);
    digitalWrite(ledB, LOW);
    digitalWrite(pinMotor, LOW);
    noTone(pinBuzzer);
  }

  bool up = false, down = false, enter = false;
  if (millis() - tempoUltimoClique > 200) {
    if (flagUp) { up = true; tempoUltimoClique = millis(); }
    if (flagDown) { down = true; tempoUltimoClique = millis(); }
    if (flagEnter) { enter = true; tempoUltimoClique = millis(); }
  }
  flagUp = false; flagDown = false; flagEnter = false;

  if (millis() - tempoPiscar > 500) {
    estadoPiscar = !estadoPiscar;
    tempoPiscar = millis();
  }

  switch (estadoAtual) {
    case EXIBICAO:
      lcd.setCursor(0, 0);
      lcd.print("Temp: "); lcd.print(tempAtual, 1); lcd.print(" C   ");
      lcd.setCursor(0, 1);
      lcd.print("L:"); lcd.print(limiteMin, 1); lcd.print(" H:"); lcd.print(limiteMax, 1);

      if (enter) { estadoAtual = SET_MAX; lcd.clear(); }
      break;

    case SET_MAX:
      lcd.setCursor(0, 0); lcd.print("Ajuste SET_MAX  ");
      lcd.setCursor(0, 1);
      if (estadoPiscar) { lcd.print("Val: "); lcd.print(limiteMax, 1); lcd.print(" C   "); }
      else { lcd.print("                "); }

      if (up) limiteMax += 0.5;
      if (down) limiteMax -= 0.5;
      if (enter) {
        EEPROM.put(10, limiteMax);
        estadoAtual = SET_MIN;
        lcd.clear();
      }
      break;

    case SET_MIN:
      lcd.setCursor(0, 0); lcd.print("Ajuste SET_MIN  ");
      lcd.setCursor(0, 1);
      if (estadoPiscar) { lcd.print("Val: "); lcd.print(limiteMin, 1); lcd.print(" C   "); }
      else { lcd.print("                "); }

      if (up) limiteMin += 0.5;
      if (down) limiteMin -= 0.5;
      if (enter) {
        EEPROM.put(14, limiteMin);
        estadoAtual = EXIBICAO;
        lcd.clear();
      }
      break;
  }
}