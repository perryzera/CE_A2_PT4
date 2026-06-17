#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int pinLM35 = A0;
const int pinMotor = 9;
const int pinBuzzer = 3;
const int ledR = 10;
const int ledG = 11;
const int ledB = 12;

LiquidCrystal_I2C lcd(0x27, 16, 2);

float setpoint = 25.0;
float Kp = 50.0;
float Ki = 10.0;
float integral = 0;
float tempAtual = 0;
unsigned long tempoAnterior = 0;
unsigned long tempoAnteriorBuzzer = 0;
bool estadoBuzzer = false;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  pinMode(pinMotor, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);
  Serial.println("PID Iniciado");
}

void lerComandosSerial() {
  if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    comando.toUpperCase();
    if (comando.startsWith("SETPOINT ")) setpoint = comando.substring(9).toFloat();
    else if (comando.startsWith("KP ")) Kp = comando.substring(3).toFloat();
    else if (comando.startsWith("KI ")) Ki = comando.substring(3).toFloat();
    else if (comando == "STATUS") {
      Serial.print("Status -> SP:"); Serial.print(setpoint);
      Serial.print(" Kp:"); Serial.print(Kp);
      Serial.print(" Ki:"); Serial.println(Ki);
    }
  }
}

void loop() {
  lerComandosSerial();

  if (millis() - tempoAnterior >= 100) {
    float dt = (millis() - tempoAnterior) / 1000.0;
    tempoAnterior = millis();

    tempAtual = (analogRead(pinLM35) * 5.0 / 1024.0) * 100.0;
    float erro = tempAtual - setpoint;
    int pwm = 0;

    if (erro > 0) {
      integral += (erro * dt);
      float calculo = (Kp * erro) + (Ki * integral);

      if (calculo > 255) {
        pwm = 255;
        integral -= (erro * dt);
      } else if (calculo < 0) {
        pwm = 0;
      } else {
        pwm = (int)calculo;
      }
    } else {
      integral = 0;
      pwm = 0;
    }

    analogWrite(pinMotor, pwm);

    if (erro > 2.0) {
      digitalWrite(ledR, HIGH);
      digitalWrite(ledG, LOW);
      digitalWrite(ledB, LOW);
      if (millis() - tempoAnteriorBuzzer > 300) {
        tempoAnteriorBuzzer = millis();
        estadoBuzzer = !estadoBuzzer;
        if (estadoBuzzer) tone(pinBuzzer, 1000);
        else noTone(pinBuzzer);
      }
    } else if (erro < -2.0) {
      digitalWrite(ledR, LOW);
      digitalWrite(ledG, LOW);
      digitalWrite(ledB, HIGH);
      noTone(pinBuzzer);
    } else {
      digitalWrite(ledR, LOW);
      digitalWrite(ledG, HIGH);
      digitalWrite(ledB, LOW);
      noTone(pinBuzzer);
    }

    lcd.setCursor(0, 0);
    lcd.print("Temp: "); lcd.print(tempAtual, 1); lcd.print(" C  ");
    lcd.setCursor(0, 1);
    lcd.print("SP: "); lcd.print(setpoint, 1);
    lcd.print(" PWM:"); lcd.print(pwm); lcd.print("  ");

    Serial.print(tempAtual);
    Serial.print(",");
    Serial.print(setpoint);
    Serial.print(",");
    Serial.println(map(pwm, 0, 255, 0, 50));
  }
}