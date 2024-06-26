//Librerias Utilizadas
#include <Arduino.h>
#include <Ticker.h> //importante para buzzer
#include <RtcDS1302.h> //para RTC
#include <WiFi.h> //wifi
#include <PubSubClient.h> //para mqtt

// Configuracion WIFI 
const char* ssid = "Redmi 12C kanna";
const char* password = "kana1234xd";

// Sensor RTC definicion
// CONNECTIONS:
// DS1302 CLK/SCLK --> 5 pin
// DS1302 DAT/IO --> 4 pin
// DS1302 RST/CE --> 2 pin
// DS1302 VCC --> 3.3v - 5v
// DS1302 GND --> GND
ThreeWire myWire(4,5,2); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

// funcion milis para medir cada 5 seg
unsigned long currentMillis;
unsigned long previousMillis = 0;
const unsigned long interval = 5000;

// LED RGB Definicion
#define LED_BLUE 19
#define LED_GREEN 21
#define LED_RED 18
// canales del LED
const int redChannel = 0;
const int greenChannel = 1;
const int blueChannel = 2;
const int freq = 5000; //frecuencia
const int resolution = 8; //resolucion

//BUZZER definicion
#define BUZZER_PIN 15
#define BUZZER_CHANNEL 3
#define BUZZER_RESOLUTION 8
const int notaDuracion = 200;
// Arreglo que contiene frecuencias de una escala musical
const int melodia[] = {
    262, 294, 330, 349, 392, 440, 494, 523
}; 
// Duracion de las notas
const int duracionNota[] = {
  4, 4, 4, 4, 4, 4, 4, 4
};
Ticker ticker;
int indiceNota = 0; //apunta a la nota actual
bool reproduciendo = false;

//MOTOR VIBRACION definicion
#define MOTORVIB 14
const int motor_channel = 4; //canal del motor
const int freq_mot = 5000; //frecuencia del motor
const int resolution_mot = 8; //resolucion del motor

// Botones definicion
#define BOTON_PRINCIPAL 12
#define BOTON_EMERGENCIA 13
// boton emergencia
bool btnEmAnt = false;
bool btnPrinAnt = false;
bool btnPrinAct;
// definicion de Estados
//Estados brazaletes
enum EstadoBrazalete{ESPERANDO, ALARMA, EMERGENCIA}; 
EstadoBrazalete est_brazalete = ESPERANDO;
// Estados led en emergencia
enum EstadoLed{ESPERAR, ENCENDER_ROJO, ENCENDER_AZUL};
EstadoLed estLed = ESPERAR; 

//iniciliazacion de variables para el uso de Millis()
unsigned long prevMill3min = 0;
unsigned long prevLedMillis = 0;

// Configuración del servidor MQTT
const char* mqtt_server = "161.132.48.12"; //Servidor broker
const int mqtt_port = 1883; //Puerto predeterminado para MQTT
const char* topic_hora = "proyecto"; //Topic para recibir la hora
const char* topic_conf = "proyecto/confirmed"; //Topic a publicar respuesta de recordatorio
const char* topic_eme = "proyecto/emergency"; //Topic a publicar estado emergencia

// Cliente WiFi y cliente MQTT
//Declaración de objeto WiFiClient para gestionar la conexión Wi-Fi
WiFiClient espClient; 
//Declaración de objeto PubSubClient para gestionar las comunicaciones MQTT
PubSubClient client(espClient); 

// Variable para almacenar el mensaje recibido del tópico hora de MQTT
String mensajeRecibido = ""; //ultima hora recibida
String ultimoMensaje = ""; //ultimo mensaje(cualquiera) recibido

/* 
*  Configuracion de todos los componenetes
*
*/
void setup(){
  Serial.begin(115200); // Inicializar la comunicación serial
  // Configuración de los canales PWM para el control de un LED RGB

  // =================== setup led ===================
  ledcSetup(redChannel, freq, resolution);    // Configura el canal PWM para el LED rojo
  ledcSetup(greenChannel, freq, resolution);  // Configura el canal PWM para el LED verde
  ledcSetup(blueChannel, freq, resolution);   // Configura el canal PWM para el LED azul

  // Adjunta los pines del LED RGB a sus respectivos canales PWM
  ledcAttachPin(LED_BLUE, blueChannel);       // Adjunta el pin del LED azul al canal azul
  ledcAttachPin(LED_GREEN, greenChannel);     // Adjunta el pin del LED verde al canal verde
  ledcAttachPin(LED_RED, redChannel);         // Adjunta el pin del LED rojo al canal rojo

  // ======================= setup BUZZER =============================
  // Configura el canal PWM para el buzzer
  ledcSetup(BUZZER_CHANNEL, 1000, BUZZER_RESOLUTION); // Configura el canal PWM con frecuencia de 1000 Hz y resolución especificada
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL); // Asocia el pin del buzzer al canal PWM configurado

  // ========================== setup RTC =============================
  // Configura el módulo RTC (Real Time Clock)
  Rtc.Begin(); // Inicializa el RTC
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__); // Obtiene la fecha y hora de compilación
  setup_RTC(compiled); // Configura el RTC con la fecha y hora de compilación

  // =========================== setup WIFI ===========================
  // Configura la conexión Wi-Fi
  Serial.println("Conectando a la red WiFi...");
  WiFi.begin(ssid, password); // Inicia la conexión a la red Wi-Fi con el SSID y la contraseña especificados

  // ========================== Botones setup =========================
  // Configura los pines de los botones
  pinMode(BOTON_PRINCIPAL, INPUT_PULLUP); // Configura el pin del botón principal como entrada con pull-up interno
  pinMode(BOTON_EMERGENCIA, INPUT_PULLUP); // Configura el pin del botón de emergencia como entrada con pull-up interno

  // =================== Conexión al servidor MQTT ====================
  // Configura la conexión al broker MQTT
  client.setServer(mqtt_server, mqtt_port); // Configura el servidor MQTT con la dirección y el puerto especificados
  client.setCallback(callback); // Configura la función de callback para gestionar los mensajes entrantes
  reconnect(); // Conecta al broker MQTT al inicio

  // ======================== motor vibracion =========================
  // Configura el canal PWM para el motor de vibración
  ledcSetup(motor_channel, freq_mot, resolution_mot); // Configura el canal PWM con la frecuencia y resolución especificadas para el motor de vibración
  ledcAttachPin(MOTORVIB, motor_channel); // Asocia el pin del motor de vibración al canal PWM configurado
}


/*
* Programa principal (Main loop)
*
*/
void loop(){
  currentMillis = millis(); //codigo tiempo y wifi
  checkMQTT(); //funcion que verifica
  /*
  * Maquina de estados para el Brazalete
  * 
  */
  switch(est_brazalete){
    case ESPERANDO:{ // caso cuando esta en espera
      RtcDateTime now = Rtc.GetDateTime(); // Obtiene la fecha y hora actual del RTC
      String comp_actual = getDateTimeStringComparativo(now); // guarda la fecha y hora del RTC en una variable String para hacer comprobaciones posteriores
      if (currentMillis - previousMillis >= interval) { // Millis() usado para 5 segundos, cada 5 segundos ejecuta este bloque
        Serial.println(getDateTimeString(now)); //Imprime en Serial la fecha y hora(con segundos) del RTC
        wifi_estado(); // comprueba la conxion del wifi
        previousMillis = currentMillis; // actualiza el millis anterior para reinciar la cuenta de 5 segundos
      }
      est_brazalete = revisar_eventos(ultimoMensaje, comp_actual); // Funcion IMPORTANTE, verifica si se va a cambiar de estado a ALARMA o EMERGENCIA
      /*
      * Maquina de estados del LED RGB
      */
      switch(estLed){ // inicio de los estados del led
        case ESPERAR: // en caso de espera
          prevMill3min = currentMillis; // mantener el tiempo de prevMill3min con millis()
        break;
        case ENCENDER_ROJO: // caso ENCENDER_ROJO, cuando se entra en el caso de emergencia en el brazalete
          encender_rojo(); // enciende el led en ROJO
          if (currentMillis - prevMill3min >= 5000) { // millis de 5 segundos, pasado ese tiempo ejecuta este bloque
            prevMill3min = currentMillis; //reinicia el contador de millis
            apagar_led(); // apaga el led
            estLed = ESPERAR; // cambia el estado del led a esperar
          }
          if (est_brazalete == ALARMA){ // condicional si el brazalete entra en estado de alarma
            prevMill3min = currentMillis; //reinicia el contador de millis
            apagar_led(); // apaga el led
            estLed = ESPERAR; // cambia el estado del led a esperar
          }
        break;
        case ENCENDER_AZUL: // caso ENCENDER_AZUL, es cuando se necesita una confirmacion 
          encender_azul(); // enciende el led en azul
          if (currentMillis - prevMill3min >= 600000) { // millis de 10 minutos, pasado ese tiempo ejecuta este bloque
            prevMill3min = currentMillis; //reiniciar el ontador de milis
            apagar_led(); // apagar el led
            estLed = ESPERAR; // cambiar de estado a ESPERAR
          }
          if (est_brazalete == ALARMA){ // condicional si el brazalete entra en estado de alarma
            prevMill3min = currentMillis; //reinicia el contador de millis
            apagar_led(); // apaga el led
            estLed = ESPERAR; // cambia el estado del led a esperar
          }
          estLed = btnPrincipal_puls(); // IMPORTANTE, cambia el estado del led al confimar la toma de medicamento
        break;
      }
      
    }
    break;
    case ALARMA: // caso alarma, cuando debe sonar una alarma
      if (!reproduciendo) {
        alarma_sonar(); // empieza a sonar la alarma completa, vibrar sonar prenderled
      }
      est_brazalete = btnPrincipal_pulsado(); // verifica si el boton de confirmacion es pulsado para cambiar de estado a ESPERAR en el brazalete
    break;
    case EMERGENCIA: // Estado EMERGENCIA, cuando se presiona el boton de emergecia
      // Reconectar al servidor MQTT si es necesario
      if (!client.connected()) {
          reconnect();
      }
      // Publicar un mensaje en el tema MQTT
      client.publish(topic_eme, "true");
      estLed = ENCENDER_ROJO; // cambia el estado del led a ENCENDER ROJO
      est_brazalete = ESPERANDO; // Cmabia e l estado del brazalete a ESPERANDO
    break;
  }
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX LOOP XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX


// ============================== LED RGB metodos ==============================
void encender_verde(){ // funcion para encender el LED RGB en verde
  ledcWrite(redChannel, 0);
  ledcWrite(greenChannel, 255); // se da solo energia al verde
  ledcWrite(blueChannel, 0);
}
void encender_rojo(){ // funcion para encender el LED RGB en rojo
  ledcWrite(redChannel, 255); // solo se da energia al led rojo
  ledcWrite(greenChannel, 0);
  ledcWrite(blueChannel, 0);
}
void encender_amarillo(){ // funcion para encender el LED RGB en amarillo
  ledcWrite(redChannel, 255); // se combinan rojo y verde dandoles energia
  ledcWrite(greenChannel, 150);
  ledcWrite(blueChannel, 0);
}
void apagar_led(){ // funcion para apagar el LED RGB
  ledcWrite(redChannel, 0);
  ledcWrite(greenChannel, 0); // no se da energia a ningun led
  ledcWrite(blueChannel, 0);
}
void encender_azul(){ // funcion para encender el LED RGB en azul
  ledcWrite(redChannel, 0);
  ledcWrite(greenChannel, 0);
  ledcWrite(blueChannel, 255); // se da solo energia al led azul
}

// ============================== Buzzer metodos ==============================
/*
* Hace sonar el buzzer con la melodia y los tiempos definidos en el inicio
*/
void reproducirNota() {
  if (!reproduciendo) { // condicion para detener la melodia
    ledcWriteTone(BUZZER_CHANNEL, 0); // Detener la melodía si no se está reproduciendo
    return;
  }
  ledcWriteTone(BUZZER_CHANNEL, melodia[indiceNota]); //Escirbe una señal de frecuencia en el buzzer
  int duracion = notaDuracion * 4 / duracionNota[indiceNota]; // define la duracion de la nota
  indiceNota = (indiceNota + 1) % (sizeof(melodia) / sizeof(melodia[0])); // Avanzar al siguiente índice de la nota
  if (indiceNota == 0) {// Si hemos reproducido todas las notas, volver al principio
    indiceNota = 0;
  }
  ticker.once_ms(duracion, reproducirNota); // Reproducir la siguiente nota después de la duración
}

/*
* Detener la reproduccion de sonido
*/
void detenerMelodia() {
  reproduciendo = false; // condicional para detener la melodia
  ledcWriteTone(BUZZER_CHANNEL, 0); // Detener el sonido en el buzzzer con una frecuencia 0
}

//======================================= motor metodos =====================================
// Métodos para controlar el motor de vibración
/*
 * Función para activar la vibración del motor
 */
void vibrar() {
  ledcWrite(motor_channel, 255); // Configura el canal PWM del motor con el valor máximo (255) para activar la vibración
}

/*
 * Función para desactivar la vibración del motor
 */
void no_vibrar() {
  ledcWrite(motor_channel, 0); // Configura el canal PWM del motor con el valor mínimo (0) para desactivar la vibración
}

//======================================= metodos RTC =====================================
/*
 * Función para configurar el RTC con la fecha y hora de compilación
 */
void setup_RTC(RtcDateTime compiled){
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid()) 
  {
      // Causas comunes:
      // 1) Primera vez que se ejecuta y el dispositivo aún no estaba en funcionamiento
      // 2) La batería del dispositivo está baja o incluso falta
      Serial.println("RTC lost confidence in the DateTime!");
      Rtc.SetDateTime(compiled); // Establece la fecha y hora del RTC con la fecha y hora de compilación
  }

  if (Rtc.GetIsWriteProtected())
  {
      Serial.println("RTC was write protected, enabling writing now");
      Rtc.SetIsWriteProtected(false); // Desactiva la protección contra escritura del RTC
  }

  if (!Rtc.GetIsRunning())
  {
      Serial.println("RTC was not actively running, starting now");
      Rtc.SetIsRunning(true); // Inicia el RTC si no estaba en funcionamiento
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) 
  {
      Serial.println("RTC is older than compile time!  (Updating DateTime)");
      Rtc.SetDateTime(compiled); // Actualiza el RTC si la fecha y hora es anterior a la de compilación
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

/*
 * Función para imprimir la fecha y hora en formato legible
 */
#define countof(a) (sizeof(a) / sizeof(a[0]))
void printDateTime(const RtcDateTime& dt){
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

/*
 * Función para obtener la fecha y hora en formato String
 */
String getDateTimeString(const RtcDateTime& dt){
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

/*
 * Función para obtener la fecha y hora en formato String sin los segundos
 */
String getDateTimeStringComparativo(const RtcDateTime& dt){
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

/*
 * Función para obtener y mostrar la fecha y hora actual del RTC
 */
void obtener_tiempo_fecha(RtcDateTime now){
  printDateTime(now);
  Serial.println();
  if (!now.IsValid())
  {
      // Causas comunes:
      // 1) La batería del dispositivo está baja o falta y la línea de alimentación se desconectó
      Serial.println("RTC lost confidence in the DateTime!");
  }
}

//======================================= metodos WiFi =====================================
/*
 * Función para verificar el estado de la conexión Wi-Fi y mostrar información relevante
 */
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

// ======================================== Metodos de LOGICA general ========================================
/*
 * Función para activar la alarma
 */
void alarma_sonar(){
  reproduciendo = true; // Indica que se está reproduciendo la alarma
  indiceNota = 0; // Reiniciar la melodía
  reproducirNota(); // Inicia la reproducción de la nota actual
  encender_amarillo(); // Enciende el LED amarillo
  vibrar(); // Activa la vibración
}

/*
 * Función para apagar la alarma
 */
void alarma_apagar(){
  detenerMelodia(); // Detiene la reproducción de la melodía
  apagar_led(); // Apaga el LED
  no_vibrar(); // Desactiva la vibración
}

/*
 * Función para comparar dos tiempos y determinar si la alarma debe sonar
 */
EstadoBrazalete comparar_tiempo(String hora_sonar, String hora_recibida){ // Estados del brazalete
  if (hora_sonar == hora_recibida){
    return ALARMA; // Retorna estado de alarma si las horas coinciden
  } else {
    return ESPERANDO; // Retorna estado de espera si las horas no coinciden
  }
}

/*
 * Función para gestionar el evento del botón principal pulsado
 */
EstadoBrazalete btnPrincipal_pulsado(){
  btnPrinAct = (digitalRead(BOTON_PRINCIPAL) == LOW); // Lee el estado del botón principal
  if (btnPrinAct && !btnPrinAnt) {
    btnPrinAnt = true; // condicion para que el boton no se pulse varias veces
    currentMillis = millis(); // ajusta el millis
    alarma_apagar(); // Apaga la alarma
    ultimoMensaje = ""; // eliminar la anterior fecha y hora guardada para recibir una nueva
    prevMill3min = currentMillis; // ajustar un millis previo para el estado del led
    estLed = ENCENDER_AZUL; // cambia el estado de led a encender azul
    return ESPERANDO; // retorna el estado del brazalete ESPERANDO
  }
  else if (!btnPrinAct){ // condicion para que el boton no se ejecute varias veces, solo una vez
    btnPrinAnt = false;
  }
  return ALARMA; // Retorna el estado ALARMA si no se realiza la pulsacion del boton 
}

/*
 * Función para revisar eventos y determinar el estado del brazalete
 */
EstadoBrazalete revisar_eventos(String hora_sonar, String hora_recibida){
  bool btnEmAct = (digitalRead(BOTON_EMERGENCIA) == LOW); // Lee el estado del botón de emergencia
  if (btnEmAct && !btnEmAnt) {  
    btnEmAnt = true; // condicional para que el boton de emergencia no se envie varios veces y solo una vez
    return EMERGENCIA; // Retorna estado de emergencia si el botón de emergencia fue pulsado
  }
  else if (!btnEmAct){ // condicional para que el boton de emergencia no se envie varios veces y solo una vez
    btnEmAnt = false;
  }
  
  if (hora_sonar == hora_recibida){ 
    return ALARMA; // Retorna estado de alarma si las horas coinciden
  } else {
    return ESPERANDO; // Retorna estado de espera si las horas no coinciden
  }
}

/*
 * Función para gestionar el evento del botón principal pulsado y el estado del LED
 */
EstadoLed btnPrincipal_puls(){ // cambio de estado para el LED
  btnPrinAct = (digitalRead(BOTON_PRINCIPAL) == LOW); // Lee el estado del botón principal
  if (btnPrinAct && !btnPrinAnt) {
    btnPrinAnt = true; // condicional para evitar pulsacion multiple
    currentMillis = millis(); // ajusta el millis
    apagar_led(); // Apaga el LED
    client.publish(topic_conf, "confirmed"); // Publica un mensaje de confirmación al servidor MQTT

    prevMill3min = currentMillis;
    return ESPERAR; // Retorna estado de esperar después de pulsar el botón
  }
  else if (!btnPrinAct){
    btnPrinAnt = false; // condicional para que el boton principal no se pulse o envie datos varias veces solo una vez
  }
  return ENCENDER_AZUL; // Retorna estado de encender el LED azul si el botón no fue pulsado
}


// ======================================== Metodos de mqtt ========================================
/*
 * Función para reconectar al servidor MQTT si la conexión se ha perdido
 */
void reconnect() {
    // Intentar conectarse al servidor MQTT
    if (!client.connected()) {
        Serial.print("Conectando al servidor MQTT...");
        if (client.connect("ESP32Client")) {
            Serial.println("Conectado al servidor MQTT");
            // Suscribirse al tópico "proyecto"
            client.subscribe(topic_hora);
        } else {
            Serial.print("Error al conectarse al servidor MQTT, rc=");
            Serial.print(client.state());
            Serial.println(" Intentando de nuevo");
        }
    }
}

/*
 * Función de callback para manejar mensajes entrantes
 */
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
    // Almacenar el mensaje recibido, excepto si es "Emergencia" o "confirmed"
    if (mensajeRecibido != "Emergencia" && mensajeRecibido != "confirmed"){
      ultimoMensaje = mensajeRecibido;
      Serial.print("Mensaje guardado: ");
      Serial.println(ultimoMensaje);
    }
    mensajeRecibido = ""; // Reiniciar la variable de mensaje recibido
}

/*
 * Función para revisar manualmente el tópico y manejar la conexión MQTT
 */
 void checkMQTT() {
    // Manejar la conexión MQTT y revisar mensajes
    if (client.connected()) {
        client.loop(); // Mantener la conexión y manejar mensajes entrantes
    } else {
        reconnect(); // Intentar reconectar si la conexión se ha perdido
    }
}
