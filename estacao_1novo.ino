// Arquivo:    estacao_1novo.ino
// Tipo:       Codigo-fonte para NodeMCU atraves da IDE do Arduino
// Autor:      Pedro Otávio Sampaio Torres
// Descricao:  Projeto Estação Meteorológica com Servidor Web e Controle de Telhado (Botão pulsante e Fim de Curso)

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h> 
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h> 
#include "ThingSpeak.h"
#include <DHT.h>
#include <ESP8266WebServer.h> 
#include <ArduinoJson.h>      

// Configurações Wi-Fi
const char* wifi = "ALGAR_Torres2023";
const char* senha = "t0rres!001";

// Seção configuração WIFI e IP estático
WiFiClient client; // Para ThingSpeak
ESP8266WebServer server(80); 

IPAddress ip(192, 168, 10, 50);      // IP definido ao NodeMCU
IPAddress gateway(192, 168, 10, 114);    // IP do roteador
IPAddress subnet(255, 255, 255, 0);     // Mascara da rede

// Configurações canal 1 ThingSpeak
const char* writeApiKey1 = "HM9V6JB10EKLMUZ8";
const char* readApiKey1 = "62NAEIAGZDOUMAX8";
unsigned long Channel1 = 2555542;

// Configurações canal 2 ThingSpeak
const char* writeApiKey2 = "5YFLW8F695AYTVZR";
const char* readApiKey2 = "VEFPKAID3NATF5P7";
unsigned long Channel2 = 2987110;

// Pinos
#define DHT_SENSOR_PIN   13   // GPIO13 (D7 no NodeMCU) conectado ao DHT11
#define DHT_SENSOR_TYPE DHT11
#define PPA_PULSE_PIN      14  // GPIO14 (D5 no NodeMCU) - Pino para enviar o pulso para a placa PPA
#define FIM_CURSO_PIN      12  // GPIO12 (D6 no NodeMCU) - Pino para ler o status do fim de curso
#define CHUVA_SENSOR_AO_PIN A0    // Pino Analógico A0 para o sensor de chuva (saida analógica)
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);  //Cria objeto DHT
Adafruit_BMP280 bme; // protocolo de comunicação I2C


// VARIÁVEL PARA O ESTADO DO TELHADO 
// 0 = Fechado, 1 = Aberto. fim de curso HIGH = Aberto, LOW = Fechado
int estadoTelhado = 0; 

int valorChuvaAnalogico = 0; //Armazena o valor bruto da leitura analógica da chuva (0-1023)

// Variáveis dos sensores (globais para acesso em diferentes funções)
float temperatura1 = 0;
float pressao = 0;
float altitude = 0;
float humi = 0;
float temperature_C = 0;

// Variável para controlar o tempo da última leitura para ThingSpeak e web
unsigned long UltimaLeitura = 0;
const long IntervaloLeitura = 15000; // 15 segundos

// Conteúdo HTML da página web
const char* HTML_CONTENT = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Estação Meteorológica</title>
    <meta charset="UTF-8">
    <style>
        body { font-family: Arial, Helvetica, sans-serif; text-align: center; margin: 0; padding: 0; background-color: #f4f4f4;}
        .container { max-width: 600px; margin: 20px auto; padding: 20px; border: 1px solid #ccc; border-radius: 8px; box-shadow: 0 0 10px rgba(0,0,0,0.1); background-color: #fff;}
        h1 { color: #333; margin-bottom: 25px; }
        .sensor-data { margin-top: 20px; text-align: left; padding: 0 20px;}
        .sensor-data p { margin: 10px 0; font-size: 1.1em; color: #555;}
        .sensor-data span { font-weight: bold; color: #007bff;}
        .controls { margin-top: 35px; border-top: 1px solid #eee; padding-top: 25px;}
        .btn { background-color: #007bff; color: white; padding: 12px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 1.1em; margin: 8px; min-width: 200px; transition: background-color 0.3s ease;}
        .btn:hover { opacity: 0.9; background-color: #0056b3;}
        .btn:active { transform: translateY(1px); }
        .status-section { margin-top: 25px; font-size: 1em; color: #666; border-top: 1px solid #eee; padding-top: 15px;}
        .status-section p { margin: 5px 0; }
        .status-section span { font-weight: bold; }
        .status-open { color: #28a745; } /* Verde para aberto */
        .status-closed { color: #dc3545; } /* Vermelho para fechado */
        .status-unknown { color: #ffc107; } /* Amarelo para desconhecido/em movimento */
        .last-update { font-size: 0.85em; color: #999; margin-top: 10px; }
        .log-messages { text-align: left; margin-top: 20px; font-size: 0.8em; color: #888; border-top: 1px solid #eee; padding-top: 15px; max-height: 100px; overflow-y: auto; background-color: #f9f9f9; padding: 10px; border-radius: 4px;}
        .log-messages p { margin: 2px 0; }
        /* NOVO: Estilos para status de chuva (baseado na intensidade) */
        .ChuvaInt0 { color: #555; } /* Seco */
        .ChuvaInt1 { color: #5dade2; } /* Leve */
        .ChuvaInt2 { color: #3498db; } /* Moderada */
        .ChuvaInt3 { color: #2874a6; } /* Forte */
    </style>
</head>
<body>
    <div class="container">
        <h1>Estação Meteorológica</h1>
        <div class="sensor-data">
            <p>Temperatura (BMP280): <span id="temp_bmp">--</span> °C</p>
            <p>Pressão: <span id="pressao">--</span> hPa</p>
            <p>Altitude: <span id="altitude">--</span> m</p>
            <p>humi (DHT11): <span id="humi">--</span> %</p>
            <p>Temperatura (DHT11): <span id="temp_dht">--</span> °C</p>
            <p>Chuva: <span id="valor_chuva" class="ChuvaInt0">--</span></p>
        </div>

        <div class="controls">
            <button class="btn" onclick="sendCommand('/aciona_telhado')">Aciona Telhado</button>
        </div>
        
        <div class="status-section">
            <p>Estado do Telhado: <span id="telhado_status" class="status-unknown">Desconhecido</span></p>
            <p class="last-update" id="last_update">Última atualização: --</p>
        </div>

        <div class="log-messages" id="log">
            <p>Logs:</p>
        </div>
    </div>

    <script>
        const logDiv = document.getElementById('log');

        function appendLog(message) {
            const p = document.createElement('p');
            p.innerText = `[${new Date().toLocaleTimeString()}] ${message}`;
            logDiv.appendChild(p);
            logDiv.scrollTop = logDiv.scrollHeight; // Scroll para o final
        }

        // Função para buscar e atualizar os dados dos sensores
        function updateSensorData() {
            var xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function() {
                if (this.readyState == 4) { // Requisição completa
                    if (this.status == 200) { // Resposta OK
                        try {
                            var data = JSON.parse(this.responseText);
                            document.getElementById('temp_bmp').innerText = data.temperatura1.toFixed(2);
                            document.getElementById('pressao').innerText = data.pressao.toFixed(2);
                            document.getElementById('altitude').innerText = data.altitude.toFixed(2);
                            document.getElementById('humi').innerText = data.humi.toFixed(2);
                            document.getElementById('temp_dht').innerText = data.temperature_C.toFixed(2);
                            document.getElementById('last_update').innerText = 'Última atualização: ' + new Date().toLocaleTimeString();
                            
                            // Atualiza o status do telhado na página
                            const telhadoStatusSpan = document.getElementById('telhado_status');
                            telhadoStatusSpan.classList.remove('status-open', 'status-closed', 'status-unknown');

                            if (data.estado_telhado !== undefined) {
                                if (data.estado_telhado == 1) { // 1 = Aberto
                                    telhadoStatusSpan.innerText = "ABERTO";
                                    telhadoStatusSpan.classList.add('status-open');
                                } else if (data.estado_telhado == 0) { // 0 = Fechado
                                    telhadoStatusSpan.innerText = "FECHADO";
                                    telhadoStatusSpan.classList.add('status-closed');
                                } else { // Outros valores (ex: -1 para erro, ou 2 para em movimento se for implementado)
                                    telhadoStatusSpan.innerText = "DESCONHECIDO"; // Ou "EM MOVIMENTO"
                                    telhadoStatusSpan.classList.add('status-unknown');
                                }
                            } else {
                                telhadoStatusSpan.innerText = "DESCONHECIDO (Sem dado)";
                                telhadoStatusSpan.classList.add('status-unknown');
                            }

                            // NOVO: Atualiza o status da chuva (com base no valor analógico)
                            const chuvaValorSpan = document.getElementById('valor_chuva');
                            chuvaValorSpan.classList.remove('ChuvaInt0', 'ChuvaInt1', 'ChuvaInt2', 'ChuvaInt3');
                            
                            if (data.valor_chuva_analogico !== undefined) {
                                let intensidadeChuva = "";
                                if (data.valor_chuva_analogico < 100) { // Muito molhado, chuva forte
                                    intensidadeChuva = "FORTE (" + data.valor_chuva_analogico + ")";
                                    chuvaValorSpan.classList.add('ChuvaInt3');
                                } else if (data.valor_chuva_analogico < 400) { // Molhado, chuva moderada
                                    intensidadeChuva = "MODERADA (" + data.valor_chuva_analogico + ")";
                                    chuvaValorSpan.classList.add('ChuvaInt2');
                                } else if (data.valor_chuva_analogico < 700) { // Pouco molhado, chuva leve
                                    intensidadeChuva = "LEVE (" + data.valor_chuva_analogico + ")";
                                    chuvaValorSpan.classList.add('ChuvaInt1');
                                } else { // Seco
                                    intensidadeChuva = "SECO (" + data.valor_chuva_analogico + ")";
                                    chuvaValorSpan.classList.add('ChuvaInt0');
                                }
                                chuvaValorSpan.innerText = intensidadeChuva;
                            } else {
                                chuvaValorSpan.innerText = "DESCONHECIDO";
                                chuvaValorSpan.classList.add('ChuvaInt0');
                            }

                            appendLog("Dados dos sensores atualizados com sucesso.");
                        } catch (e) {
                            appendLog("Erro ao parsear JSON dos sensores: " + e.message);
                            appendLog("Resposta recebida: " + this.responseText.substring(0, 100) + "..."); // Limita o log
                        }
                    } else {
                        appendLog("Erro HTTP ao buscar dados dos sensores: " + this.status);
                    }
                }
            };
            xhr.open("GET", "/data", true); // Endpoint para os dados dos sensores
            xhr.send();
        }

        // Função para enviar comandos de telhado
        function sendCommand(command) {
            appendLog("Enviando comando: " + command);
            var xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function() {
                if (this.readyState == 4) {
                    if (this.status == 200) {
                        appendLog("Comando '" + command + "' enviado com sucesso!");
                        updateSensorData(); // Atualiza dados após o comando para pegar o novo estado
                    } else {
                        appendLog("Erro ao enviar comando '" + command + "': " + this.status);
                    }
                }
            };
            xhr.open("GET", command, true); // Ex: /aciona_telhado
            xhr.send();
        }

        // Atualiza os dados a cada 10 segundos
        setInterval(updateSensorData, 5000); 
        // Chama a primeira atualização imediatamente ao carregar a página
        window.onload = updateSensorData; 
    </script>
</body>
</html>
)rawliteral";

// Protótipos de função
void EscreveCanal1(float tempAtual, float pressaoAtual, float altitudeAtual, float humi, float temperature_C, int estadoTelhado, int valorChuvaAnalogico);
//void EscreveCanal2(int estadoTelhado);
void lerSensores(); 
void EnviaHTML();   
void EnviaDADOS();   
void AcionaTelhado(); 

void setup() {
  Serial.begin(115200); // velocidade do monitor serial
  delay(100);

  // **Configuração dos pinos do telhado**
  pinMode(PPA_PULSE_PIN, OUTPUT);
  digitalWrite(PPA_PULSE_PIN, LOW); 

  // **Configuração do pino do fim de curso**
  pinMode(FIM_CURSO_PIN, INPUT_PULLUP); 

  // Configuracao do pino do sensor de chuva
  // NAO EH NECESSARIO pinMode() para pino analogico A0
  // pino A0 nao tem pull-up/pull-down configuravel via software como os digitais.

  dht_sensor.begin(); // initialize the DHT sensor
  
  ThingSpeak.begin(client);

  // Configuração de IP estático
  WiFi.config(ip, gateway, subnet); 
  
  WiFi.begin(wifi, senha);   // Conecta ao WIFI
  Serial.println("Conectando ao Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Conectado ao Wi-Fi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  if (!bme.begin(0x76)) {   // Tentar 0x77 se não funcionar - Se o begin for false o ! torna em true e entra no if
    Serial.println("BMP280 não localizado! Verifique as conexões.");
    while (1); //cria um loop infinito. Se o sensor não for encontrado, o programa ficará preso aqui indefinidamente.
  }
 
  // Lê o estado inicial do telhado pelo fim de curso
  // Ajustar a lógica aqui de acordo com o comportamento do fim de curso:
  if (digitalRead(FIM_CURSO_PIN) == LOW) { 
      estadoTelhado = 1; // 1 para ABERTO
      Serial.println("Telhado inicialmente detectado como ABERTO (Fim de Curso LOW).");
  } else { // Se o sensor não está acionado (HIGH), telhado fechado
      estadoTelhado = 0; // 0 para FECHADO
      Serial.println("Telhado inicialmente detectado como FECHADO (Fim de Curso HIGH/Pulled Up).");
  }

  // Configuração das rotas do servidor web
  server.on("/", HTTP_GET, EnviaHTML); 
  server.on("/data", HTTP_GET, EnviaDADOS); 
  server.on("/aciona_telhado", HTTP_GET, AcionaTelhado); 
  
  server.begin(); 
  Serial.println("Servidor web HTTP iniciado.");
}

// Função para enviar informações para o Canal 1 (dados dos sensores)
void EscreveCanal1(float temperatura1, float pressao, float altitude, float humi, float temperature_C, int estadoTelhado, int valorChuva) {
  ThingSpeak.setField(1, temperatura1);     // Field 1 - Temperatura atual BMP280
  ThingSpeak.setField(2, pressao);          // Field 2 - pressão atual BMP280
  ThingSpeak.setField(3, altitude);         // Field 3 - altitude BMP280
  ThingSpeak.setField(4, humi);             // Field 4 - humi DHT11
  ThingSpeak.setField(5, temperature_C);    // Field 5 - Temperatura DHT11
  ThingSpeak.setField(6, valorChuva);       // Field 6 para o valor analogico da chuva
  ThingSpeak.setField(7, estadoTelhado);    // Field 7 para o estado do telhado
  int status = ThingSpeak.writeFields(Channel1, writeApiKey1);
  if (status == 200) {
    Serial.println("Dados dos sensores enviados ao Canal 1 com sucesso!");
  } else {
    Serial.println("Erro ao enviar dados dos sensores ao Canal 1. Código: " + String(status));
  }
}

// Função para leitura dos sensores
void lerSensores() {
  temperatura1 = bme.readTemperature();
  pressao = bme.readPressure() / 100.0F; 
  altitude = bme.readAltitude(1013.25); 

  humi = dht_sensor.readHumidity();
  temperature_C = dht_sensor.readTemperature();

  // Tratamento de leituras inválidas do DHT
  if (isnan(humi) || humi < 0 || humi > 100) {
    Serial.println("Erro na leitura da umidade (DHT11)!");
    humi = -1.0; 
  }
  if (isnan(temperature_C) || temperature_C < -50 || temperature_C > 100) {
    Serial.println("Erro na leitura da temperatura (DHT11)!");
    temperature_C = -1.0; 
  }
  // Tratamento de leituras inválidas do BMP280
  if (isnan(temperatura1)) {
      Serial.println("Erro na leitura da temperatura (BMP280)!");
      temperatura1 = -1.0;
  }
  if (isnan(pressao)) {
      Serial.println("Erro na leitura da pressão (BMP280)!");
      pressao = -1.0;
  }
  if (isnan(altitude)) {
      Serial.println("Erro na leitura da altitude (BMP280)!");
      altitude = -1.0;
  }

  // Leitura do sensor de chuva (analógica)**
  valorChuvaAnalogico = analogRead(CHUVA_SENSOR_AO_PIN); // Le o valor de 0-1023
  Serial.printf("Valor Chuva (Analogico): %d (0-1023)\n", valorChuvaAnalogico);

  // Informa no monitor serial os dados coletados pelos sensores
  Serial.println("\n--- Leituras Atuais ---");
  Serial.printf("Temperatura BMP280: %.2f °C", temperatura1);
  Serial.printf("  Pressão: %.2f hPa", pressao);
  Serial.printf("  Altitude: %.2f m\n", altitude);
  Serial.printf("humi: %.2f %%", humi);
  Serial.printf("  Temperatura DHT11: %.2f °C\n", temperature_C);
  Serial.printf("Chuva: %d (Analógico)\n", valorChuvaAnalogico); 
  Serial.printf("Posicao do telhado: %d\n", estadoTelhado);

  // Atualiza o estado do telhado lendo o fim de curso

  if (digitalRead(FIM_CURSO_PIN) == LOW) { 
      estadoTelhado = 1; // 1 para ABERTO
      Serial.println("Fim de curso detecta: ABERTO");
  } else { // Se o sensor não está acionado (HIGH), telhado fechado
      estadoTelhado = 0; // 0 para FECHADO
      Serial.println("Fim de curso detecta: FECHADO");
  }
}

// Lógica para servir a página HTML
void EnviaHTML() {
  server.send(200, "text/html", HTML_CONTENT);
}

// Lógica para servir os dados dos sensores em JSON
void EnviaDADOS() {
  StaticJsonDocument<256> doc; 
  doc["temperatura1"] = temperatura1;
  doc["pressao"] = pressao;
  doc["altitude"] = altitude;
  doc["humi"] = humi;
  doc["temperature_C"] = temperature_C;
  doc["estado_telhado"] = estadoTelhado; 
  doc["valor_chuva_analogico"] = valorChuvaAnalogico; 

  String jsonResponse;
  serializeJson(doc, jsonResponse);
  server.send(200, "application/json", jsonResponse);
  Serial.println("Dados JSON enviados ao cliente web.");
}

// Acionar o telhado (pulso PPA)
void AcionaTelhado() {
  Serial.println("Comando: Acionar Telhado (pulso PPA)");
  digitalWrite(PPA_PULSE_PIN, HIGH); // Liga o pino
  delay(500); // Mantém o pulso por 500ms (ajustar conforme necessidade da placa PPA)
  digitalWrite(PPA_PULSE_PIN, LOW);  // Desliga o pino

  server.send(200, "text/plain", "Comando de acionamento do telhado enviado."); //Informa ao navegador que o comando foi executado
  
  // Após enviar o pulso, esperamos que o telhado se mova e o fim de curso mude de estado.
  // Lemos novamente os sensores para capturar o novo estado do telhado (via fim de curso).
  // Adicione um pequeno delay para a placa PPA ter tempo de iniciar o movimento e o fim de curso mudar.
  delay(100); // Pequeno delay antes de reler o fim de curso
  lerSensores(); 
  //EscreveCanal2(estadoTelhado); // Envia o novo estado do telhado (do fim de curso) para ThingSpeak
}

void loop() {
  // Verifica se o WIFI continua conectado e reconecta se necessário
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado. Tentando reconectar...");
    WiFi.begin(wifi, senha);
    delay(5000);
    return; 
  }

  server.handleClient(); 

  // Condição para leitura e envio de dados a cada 15 segundos
  if (millis() - UltimaLeitura >= IntervaloLeitura) {
    UltimaLeitura = millis(); 
    lerSensores(); // Chama a função para ler os sensores
    
    // Chama função que escreve a temperatura e o nível no Thingspeak (Canal 1)
    EscreveCanal1(temperatura1, pressao, altitude, humi, temperature_C, estadoTelhado, valorChuvaAnalogico);
  }
}
