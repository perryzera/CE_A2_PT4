#include <Arduino.h>

#define LM35_PIN A0
#define COOLER_PIN 9

float setpoint = 28.0;

float Kp = 50.0;
float Ki = 10.0;

float integral = 0.0;

unsigned long ultimoControle = 0;
const unsigned long periodoControle = 500;

float historico[10];
int indiceHistorico = 0;
bool bufferCheio = false;

float lerTemperatura()
{
    int leitura = analogRead(LM35_PIN);
    float tensao = leitura * (5.0 / 1023.0);
    float temperatura = tensao * 100.0;
    return temperatura;
}

void salvarHistorico(float temp)
{
    historico[indiceHistorico] = temp;
    indiceHistorico++;
    if (indiceHistorico >= 10)
    {
        indiceHistorico = 0;
        bufferCheio = true;
    }
}

void mostrarStatus()
{
    Serial.println("\n===== STATUS =====");
    Serial.print("Temperatura: ");
    Serial.println(lerTemperatura());
    Serial.print("Setpoint: ");
    Serial.println(setpoint);
    Serial.print("Kp: ");
    Serial.println(Kp);
    Serial.print("Ki: ");
    Serial.println(Ki);
    Serial.println("==================");
}

void mostrarLog()
{
    Serial.println("\n===== LOG =====");
    int total = bufferCheio ? 10 : indiceHistorico;
    for (int i = 0; i < total; i++)
    {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(historico[i]);
    }
    Serial.println("===============");
}

void processarSerial()
{
    if (!Serial.available())
        return;

    String comando = Serial.readStringUntil('\n');
    comando.trim();

    if (comando.startsWith("SETPOINT"))
    {
        setpoint = comando.substring(9).toFloat();
        Serial.print("Novo Setpoint = ");
        Serial.println(setpoint);
    }
    else if (comando.startsWith("KP"))
    {
        Kp = comando.substring(2).toFloat();
        Serial.print("Novo Kp = ");
        Serial.println(Kp);
    }
    else if (comando.startsWith("KI"))
    {
        Ki = comando.substring(2).toFloat();
        Serial.print("Novo Ki = ");
        Serial.println(Ki);
    }
    else if (comando.equals("STATUS"))
    {
        mostrarStatus();
    }
    else if (comando.equals("LOG"))
    {
        mostrarLog();
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(COOLER_PIN, OUTPUT);
    for (int i = 0; i < 10; i++)
        historico[i] = 0;
    Serial.println("Sistema iniciado");
}

void loop()
{
    processarSerial();

    unsigned long agora = millis();

    if (agora - ultimoControle >= periodoControle)
    {
        ultimoControle = agora;

        float temperatura = lerTemperatura();
        salvarHistorico(temperatura);

        float erro = setpoint - temperatura;
        float dt = periodoControle / 1000.0;

        integral += erro * Ki * dt;

        if (integral > 255)
            integral = 255;
        if (integral < 0)
            integral = 0;

        float saida = (Kp * erro) + integral;

        if (saida > 255)
            saida = 255;
        if (saida < 0)
            saida = 0;

        analogWrite(COOLER_PIN, (int)saida);

        Serial.print(temperatura);
        Serial.print(",");
        Serial.print(setpoint);
        Serial.print(",");
        Serial.println(saida);
    }
}