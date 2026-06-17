#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

const int pinLM35 = A0;
const int btnEnter = 6;
const int pinBuzzer = 3;
const int pinMotor = 9;
const int ledR = 10;
const int ledG = 11;
const int ledB = 12;

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int numLeituras = 5;
float leituras[numLeituras];
int indiceLeitura = 0;
float total = 0;
float tempMedia = 0;

float tempMax = -100.0;
float tempMin = 100.0;

bool telaExtremos = false;
bool lastEnter = HIGH;

unsigned long tempoAnteriorBuzzer = 0;
bool estadoBuzzer = false;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  pinMode(btnEnter, INPUT_PULLUP);
  pinMode(pinMotor, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  for (int i = 0; i < numLeituras; i++) leituras[i] = 0;

  EEPROM.get(0, tempMin);
  EEPROM.get(4, tempMax);
  if (isnan(tempMin) || tempMin < -50 || tempMin > 150) tempMin = 100.0;
  if (isnan(tempMax) || tempMax < -50 || tempMax > 150) tempMax = -100.0;
}

void loop() {
  total = total - leituras[indiceLeitura];
  leituras[indiceLeitura] = (analogRead(pinLM35) * 5.0 / 1024.0) * 100.0;
  total = total + leituras[indiceLeitura];
  indiceLeitura = (indiceLeitura + 1) % numLeituras;
  tempMedia = total / numLeituras;

  bool gravou = false;
  if (tempMedia > tempMax && tempMedia < 150) {
    tempMax = tempMedia;
    EEPROM.put(4, tempMax);
    gravou = true;
  }
  if (tempMedia < tempMin && tempMedia > -50) {
    tempMin = tempMedia;
    EEPROM.put(0, tempMin);
    gravou = true;
  }
  if (gravou) Serial.println("Gravação EEPROM");

  bool atualEnter = digitalRead(btnEnter);
  if (atualEnter == LOW && lastEnter == HIGH) {
    telaExtremos = !telaExtremos;
    lcd.clear();
    delay(50);
  }
  lastEnter = atualEnter;

  if (tempMedia >= 30.0) {
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
  } else if (tempMedia <= 20.0) {
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

  lcd.setCursor(0, 0);
  if (!telaExtremos) {
    lcd.print("Temp: ");
    lcd.print(tempMedia, 1);
    lcd.print(" C   ");
    lcd.setCursor(0, 1);
    lcd.print("Status: Normal  ");
  } else {
    lcd.print("Max: ");
    lcd.print(tempMax, 1);
    lcd.print(" C   ");
    lcd.setCursor(0, 1);
    lcd.print("Min: ");
    lcd.print(tempMin, 1);
    lcd.print(" C   ");
  }
  delay(200);
}