//Arquivo:   estacao_0.ino   
//Tipo:      Codigo-fonte para NodeMCU atraves da IDE do Arduino
//Autor:     Pedro Otávio Sampaio Torres
//Descricao: Projeto Estação Meteorológica 

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h> //Biblioteca BMP280
#include "ThingSpeak.h"
#include <DHT.h>

// Configurações Wi-Fi
const char* ssid = "ALGAR_Torres2023";
const char* password = "t0rres!001";

// Seção configuração WIFI e IP estático
WiFiClient client;

IPAddress ip(192, 168, 10, 50);             // IP definido ao NodeMCU
IPAddress gateway(192, 168, 10, 114);       // IP do roteador
IPAddress subnet(255, 255, 255, 0);         // Mascara da rede
WiFiServer server(80);                     // Cria o servidor web na porta 80

// Configurações canal 1 ThingSpeak
const char* writeApiKey1 = "HM9V6JB10EKLMUZ8";
const char* readApiKey1 = "62NAEIAGZDOUMAX8";
unsigned long Channel1 = 2555542;

// Configurações canal 2 ThingSpeak
const char* writeApiKey2 = "5YFLW8F695AYTVZR";
const char* readApiKey2 = "VEFPKAID3NATF5P7";
unsigned long Channel2 =  2987110;

// Pinos
#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11 
#define BMP_CS 10
#define DHT_SENSOR_PIN  13  // GPIO13 (D7 no NodeMCU) conectado ao DHT11
#define DHT_SENSOR_TYPE DHT11

DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE); //Cria objeto DHT
Adafruit_BMP280 bme; // protocolo de comunicação I2C

// Protótipos de função
void EscreveCanal1(float tempAtual, float pressaoAtual, float altitudeAtual, float humi, float temperature_C);
void EscreveCanal2(int estadoTelhado);

void setup() {
  Serial.begin(115200); // velocidade do monitor serial
  delay(100);

  dht_sensor.begin(); // initialize the DHT sensor
  
  ThingSpeak.begin(client);

  WiFi.begin(ssid, password);   // Conecta ao WIFI
  Serial.println("Conectando ao Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Conectado ao Wi-Fi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  if (!bme.begin(0x76)) {  // Tentar 0x77 se não funcionar - Se o begin for false o ! torna em true e entra no if
    Serial.println("Não foi possível encontrar o sensor BMP280!");
    while (1); //cria um loop infinito. Se o sensor não for encontrado, o programa ficará preso aqui indefinidamente.
  }
 
//  delay(2000); // Aguarda estabilização do DHT11

  //Envia estado inicial para o Canal 2
  EscreveCanal2(0); //apenas no field1

}

// Função para enviar informações para o Canal 2
void EscreveCanal2(int estadoTelhado) {
  ThingSpeak.setField(1, estadoTelhado);    // Field 1 - Estado do telhado 0 fechado ou 1 aberto
  
  int status = ThingSpeak.writeFields(Channel2, writeApiKey2);
  if (status == 200) {
    Serial.println("Estado enviado ao Canal 2 com sucesso!");
  } else {
    Serial.println("Erro ao enviar estados ao Canal 2. Código: " + String(status));
  }
}

// Função para enviar informações para o Canal 1
void EscreveCanal1(float temperatura1, float pressao, float altitude, float humi, float temperature_C) {
  ThingSpeak.setField(1, temperatura1);    // Field 1 - Temperatura atual BMP280
  ThingSpeak.setField(2, pressao);         // Field 2 - pressão atual BMP280
  ThingSpeak.setField(3, altitude);        // Field 3 - altitude BMP280
  ThingSpeak.setField(4, humi);            // Field 4 - Umidade DHT11
  ThingSpeak.setField(5, temperature_C);   // Field 5 - Temperatura DHT11

  int status = ThingSpeak.writeFields(Channel1, writeApiKey1);
  if (status == 200) {
    Serial.println("Dados enviados ao Canal 1 com sucesso!");
  } else {
    Serial.println("Erro ao enviar dados ao Canal 1. Código: " + String(status));
  }
}

void loop() {
  // Verifica se o WIFI continua conectado e reconecta se necessário
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado. Tentando reconectar...");
    WiFi.begin(ssid, password);
    delay(5000);
    return;
  }

  // Seção de leitura dos sensores
  float temperatura1 = bme.readTemperature();
  float pressao = bme.readPressure() / 100.0F; // Convertendo para hPa
  float altitude = bme.readAltitude(1013.25);
  float humi = dht_sensor.readHumidity();
  float temperature_C = dht_sensor.readTemperature();
  float temperature_F = dht_sensor.readTemperature(true);

  // Chama função que escreve a temperatura e o nível no Thingspeak (Canal 1)
  EscreveCanal1(temperatura1, pressao, altitude, humi, temperature_C);
    
  // Informa no monitor serial os dados coletados pelos sensores 
  Serial.print("Temperatura: ");
  Serial.print(temperatura1);
  Serial.println(" °C");
  Serial.print("Pressão: ");
  Serial.print(pressao);
  Serial.println(" hPa");
  Serial.print("Altitude: ");
  Serial.print(altitude);
  Serial.println(" m");

  // Verifica se a leitura do DHT foi bem sucedida
  if (isnan(temperature_C) || isnan(temperature_F) || isnan(humi)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Humidity: ");
    Serial.print(humi);
    Serial.print("%");
    Serial.print("  |  ");
    Serial.print("Temperature: ");
    Serial.print(temperature_C);
    Serial.print("°C  ~  ");
    Serial.print(temperature_F);
    Serial.println("°F");
  }

  delay(15000); // Aguarda 15 segundos
}