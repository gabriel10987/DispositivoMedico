
// CONNECTIONS:
// DS1302 CLK/SCLK --> 5
// DS1302 DAT/IO --> 4
// DS1302 RST/CE --> 2
// DS1302 VCC --> 3.3v - 5v
// DS1302 GND --> GND

#include <RtcDS1302.h>

#define BUZZER_PIN 13 // Pin del buzzer
#define BOTON_1 12
#define BOTON_2 14
#define LED 18
#define MOTOR 34

ThreeWire myWire(4,5,2); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

void setup () 
{
    Serial.begin(57600);

    Serial.print("compiled: ");
    Serial.print(__DATE__);
    Serial.println(__TIME__);

    Rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
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

    ledcSetup(0, 5000, 8); // Canal 0, frecuencia de 5000 Hz, resolución de 8 bits
    ledcAttachPin(BUZZER_PIN, 0); // Asignar el pin del buzzer al canal 0
    pinMode(BOTON_1, INPUT_PULLUP);
    pinMode(BOTON_2, INPUT_PULLUP);
    pinMode(LED, OUTPUT);
    ledcWriteTone(0, 0);
}

void loop () {
    int melody[] = {262, 262, 294, 262, 349, 330, 262, 262, 294, 262, 392, 349, 262, 262, 523, 440, 349, 330, 294, 466, 466, 440, 349, 392, 349, 262};
    int noteDurations[] = {4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 2, 4};

    digitalWrite(LED, HIGH); // Enciende el LED
  
    if (digitalRead(BOTON_1) == LOW) {
      digitalWrite(LED, LOW); // Apaga el LED
      for (int i = 0; i < sizeof(melody) / sizeof(melody[0]); i++) {
        int noteDuration = 1000 / noteDurations[i];
        ledcWriteTone(0, melody[i]);
        delay(noteDuration);
        ledcWriteTone(0, 0);
        delay(50); // Pausa entre notas
      }
      digitalWrite(LED, HIGH); // Enciende el LED
    }

    if (digitalRead(BOTON_2) == LOW) {
      
      Serial.println("Emergencia activada");

    }


    // medir el tiempo RTC
    RtcDateTime now = Rtc.GetDateTime();

    printDateTime(now);
    Serial.println();

    if (!now.IsValid())
    {
        // Common Causes:
        //    1) the battery on the device is low or even missing and the power line was disconnected
        Serial.println("RTC lost confidence in the DateTime!");
    }

    //delay(10000); // ten seconds
    delay(5000); //5seg
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime& dt) {
    char datestring[26];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}