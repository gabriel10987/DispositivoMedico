#include <Arduino.h>
#include <Ticker.h> // importante para buzzer
#include <RtcDS1302.h> // para RTC
#include <WiFi.h> // wifi
#include <PubSubClient.h> // para mqtt

// Configuracion WIFI
const char* ssid = "Redmi 12C kanna";
const char* password = "kana1234xd";

// RTC definicion
// CONNECTIONS:
// DS1302 CLK/SCLK --> 5
// DS1302 DAT/IO --> 4
// DS1302 RST/CE --> 2
// DS1302 VCC --> 3.3v - 5v
// DS1302 GND --> GND
ThreeWire myWire(4,5,2); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);



// funcion milis para medir cada 5 seg
unsigned long previousMillis = 0;
const unsigned long interval = 5000;
unsigned long previousMillis_60s = 0; //sonar

// LED RGB Definicion
#define LED_BLUE 19
#define LED_GREEN 21
#define LED_RED 18
const int redChannel = 0;
const int greenChannel = 1;
const int blueChannel = 2;
const int freq = 5000;
const int resolution = 8;

//BUZZER definicion
#define BUZZER_PIN 15
#define BUZZER_CHANNEL 3
#define BUZZER_RESOLUTION 8
const int notaDuracion = 200;
const int melodia[] = {
    262, 294, 330, 349, 392, 440, 494, 523
};
const int duracionNota[] = {
  4, 4, 4, 4, 4, 4, 4, 4
};
Ticker ticker;
int indiceNota = 0;
bool reproduciendo = false;

//motor definicion
#define MOTORVIB 14
const int motor_channel = 4;
const int freq_mot = 5000;
const int resolution_mot = 8;

// Botones definicion
#define BOTON_PRINCIPAL 12
#define BOTON_EMERGENCIA 13
// Variables para almacenar el estado actual y el estado previo del botón
int btnEmAct = HIGH; // Estado inicial: no presionado
int btnEmPrev = HIGH; // Estado previo: no presionado

// Estados
enum EstadoBrazalete{ESPERANDO, ALARMA, EMERGENCIA};

EstadoBrazalete est_brazalete = ESPERANDO;

// Configuración del servidor MQTT
const char* ID_PULSERA = "100100"; //sonar
const char* mqtt_server = "161.132.49.157"; // Por ejemplo, puedes usar un servidor MQTT público para pruebas
const int mqtt_port = 1883; // Puerto predeterminado para MQTT
const char* mqtt_topic = ID_PULSERA; // El tema al que deseas publicar
// Cliente WiFi y cliente MQTT
WiFiClient espClient;
PubSubClient client(espClient);
// Variable para almacenar el mensaje recibido del tópico MQTT
String mensajeRecibido = "";
String ultimoMensaje = "";

//pruebas
bool sonar = true;

void setup(){
  Serial.begin(115200); // Inicializar la comunicación serial
  // =================== setup led ===================
  ledcSetup(redChannel, freq, resolution);
  ledcSetup(greenChannel, freq, resolution);
  ledcSetup(blueChannel, freq, resolution);
  ledcAttachPin(LED_BLUE, blueChannel);
  ledcAttachPin(LED_GREEN, greenChannel);
  ledcAttachPin(LED_RED, redChannel);

  
  // =================== setup BUZZER ===================
  ledcSetup(BUZZER_CHANNEL, 1000, BUZZER_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);

  // ========================== setup RTC ==========================
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  setup_RTC(compiled);

  // ========================== setup WIFI ==========================
  Serial.println("Conectando a la red WiFi...");
  WiFi.begin(ssid, password);

  // ========================== Botones setup ==========================
  pinMode(BOTON_PRINCIPAL, INPUT_PULLUP);
  pinMode(BOTON_EMERGENCIA, INPUT_PULLUP);

  // Conexión al servidor MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect(); // Conectar al broker MQTT al inicio

  // ========================== motor vibraccion ==========================
  ledcSetup(motor_channel, freq_mot, resolution_mot);
  ledcAttachPin(MOTORVIB, motor_channel);
}

void loop(){
  // codigo tiempo y wifi
  unsigned long currentMillis = millis();

  switch(est_brazalete){
    case ESPERANDO:{
      RtcDateTime now = Rtc.GetDateTime();
      String comp_actual = getDateTimeStringComparativo(now);
      if (currentMillis - previousMillis >= interval) {
        String datetime = getDateTimeString(now);
        Serial.println(datetime);
        wifi_estado();
        apagar_led();
        checkMQTT();
        previousMillis = currentMillis;
      }
      if (currentMillis - previousMillis_60s >= 60000) {
        sonar = true;
        previousMillis_60s = currentMillis;
      }
      //est_brazalete = btnEmergencia_pulsado();
      //est_brazalete = comparar_tiempo("23/05/2024 11:07", comp_actual);
      if (sonar){
        est_brazalete = revisar_eventos(mensajeRecibido, comp_actual);
      }
      
    }
    break;
    case ALARMA:
      if (!reproduciendo) {
        alarma_sonar();
      }
      est_brazalete = btnPrincipal_pulsado();
      //cuando no lo pulsa
    break;
    case EMERGENCIA:
      // Reconectar al servidor MQTT si es necesario
      if (!client.connected()) {
          reconnect();
      }
      // Publicar un mensaje en el tema MQTT
      client.publish(mqtt_topic, "Emergencia");
      mensajeRecibido = "";
      ultimoMensaje = "";
      est_brazalete = ESPERANDO;
    break;
  }
  
}

// ============================== LED RGB metodos ==============================
void encender_verde(){
  ledcWrite(redChannel, 0);
  ledcWrite(greenChannel, 255);
  ledcWrite(blueChannel, 0);
}

void encender_rojo(){
  ledcWrite(redChannel, 255);
  ledcWrite(greenChannel, 0);
  ledcWrite(blueChannel, 0);
}
void encender_amarillo(){
  ledcWrite(redChannel, 255);
  ledcWrite(greenChannel, 150);
  ledcWrite(blueChannel, 0);
}
void apagar_led(){
  ledcWrite(redChannel, 0);
  ledcWrite(greenChannel, 0);
  ledcWrite(blueChannel, 0);
}

// ============================== Buzzer metodos ==============================
void reproducirNota() {
  if (!reproduciendo) {
    ledcWriteTone(BUZZER_CHANNEL, 0); // Detener la melodía si no se está reproduciendo
    return;
  }
  
  ledcWriteTone(BUZZER_CHANNEL, melodia[indiceNota]);
  
  int duracion = notaDuracion * 4 / duracionNota[indiceNota];
  indiceNota = (indiceNota + 1) % (sizeof(melodia) / sizeof(melodia[0])); // Avanzar al siguiente índice de la nota
  
  // Si hemos reproducido todas las notas, volver al principio
  if (indiceNota == 0) {
    indiceNota = 0;
  }

  ticker.once_ms(duracion, reproducirNota); // Reproducir la siguiente nota después de la duración
}

void detenerMelodia() {
  reproduciendo = false;
  ledcWriteTone(BUZZER_CHANNEL, 0); // Detener el sonido
}

//======================================= motor metodos =====================================
void vibrar(){
  ledcWrite(motor_channel, 255);
}
void no_vibrar(){
  ledcWrite(motor_channel, 0);
}

//======================================= metodos RTC =====================================

void setup_RTC(RtcDateTime compiled){
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid()) 
  {
      // Common Causes:
      //    1) first time you ran and the device wasn't running yet
      //    2) the battery on the device is low or even missing

      Serial.println("RTC lost confidence in the DateTime!");
      Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected())
  {
      Serial.println("RTC was write protected, enabling writing now");
      Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning())
  {
      Serial.println("RTC was not actively running, starting now");
      Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) 
  {
      Serial.println("RTC is older than compile time!  (Updating DateTime)");
      Rtc.SetDateTime(compiled);
  }
  else if (now > compiled) 
  {
      Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled) 
  {
      Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }
}

#define countof(a) (sizeof(a) / sizeof(a[0]))
void printDateTime(const RtcDateTime& dt)
{
    char datestring[26];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Day(),
            dt.Month(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}

String getDateTimeString(const RtcDateTime& dt)
{
    char datestring[26];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Day(),
            dt.Month(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    
    return String(datestring);
}
String getDateTimeStringComparativo(const RtcDateTime& dt)
{
    char datestring[26];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u"),
            dt.Day(),
            dt.Month(),
            dt.Year(),
            dt.Hour(),
            dt.Minute());
    
    return String(datestring);
}

void obtener_tiempo_fecha(RtcDateTime now){
  printDateTime(now);
  Serial.println();
  if (!now.IsValid())
  {
      // Common Causes:
      //    1) the battery on the device is low or even missing and the power line was disconnected
      Serial.println("RTC lost confidence in the DateTime!");
  }
}

//
void wifi_estado(){
  // Guardar el tiempo actual como el tiempo anterior
  if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Conectando...");
  }
  else{
    Serial.println("Conexión exitosa!");
    Serial.print("Dirección IP asignada: ");
    Serial.println(WiFi.localIP());
  }
}

// ======================================== lOGICA general ========================================
void alarma_sonar(){
  reproduciendo = true;
  indiceNota = 0; // Reiniciar la melodía
  reproducirNota();
  encender_rojo();
  vibrar();
}
void alarma_apagar(){
  detenerMelodia();
  apagar_led();
  no_vibrar();
}

EstadoBrazalete comparar_tiempo(String hora_sonar, String hora_recibida){
  if (hora_sonar == hora_recibida){
    return ALARMA;
  } else {
    return ESPERANDO;
  }
}
EstadoBrazalete btnPrincipal_pulsado(){
  if (digitalRead(BOTON_PRINCIPAL) == LOW) {
    alarma_apagar();
    encender_verde();
    sonar = false;
    // Limpiar el contenido anterior del mensaje recibido
    mensajeRecibido = "";
    return ESPERANDO;
  } else{
    return ALARMA;
  }
}

EstadoBrazalete btnEmergencia_pulsado(){
  // Leer el estado actual del botón
  btnEmAct = digitalRead(BOTON_EMERGENCIA);
  if (btnEmAct == LOW && btnEmPrev == HIGH) {
    btnEmPrev = btnEmAct;
    return EMERGENCIA;
  }
  btnEmPrev = btnEmAct;
  return ESPERANDO;
  
}

EstadoBrazalete revisar_eventos(String hora_sonar, String hora_recibida){
  if (digitalRead(BOTON_EMERGENCIA) == LOW) {
    return EMERGENCIA;
  }
  else if (hora_sonar == hora_recibida){
    return ALARMA;
  } else {
    return ESPERANDO;
  }
}




// mqtt metodos
void reconnect() {
    // Intentar conectarse al servidor MQTT
    if (!client.connected()) {
        Serial.print("Conectando al servidor MQTT...");
        if (client.connect("ESP32Client")) {
            Serial.println("Conectado al servidor MQTT");
            // Suscribirse al tópico
            client.subscribe(mqtt_topic);
        } else {
            Serial.print("Error al conectarse al servidor MQTT, rc=");
            Serial.print(client.state());
            Serial.println(" Intentando de nuevo");
            
        }
    }
}

// Función de callback para manejar mensajes entrantes
void callback(char* topic, byte* payload, unsigned int length) {
    String topicStr = String(topic);
    
    Serial.print("Mensaje recibido en tópico: ");
    Serial.print(topicStr);
    Serial.print(". Mensaje: ");   
    // Construir el mensaje recibido a partir del payload
    for (unsigned int i = 0; i < length; i++) {
        mensajeRecibido += (char)payload[i];
    }
    Serial.println(mensajeRecibido);
}
// Función para revisar el tópico manualmente
void checkMQTT() {
    // Manejar la conexión MQTT y revisar mensajes
    if (client.connected()) {
        client.loop();
    } else {
        reconnect();
    }
}
