#include <SPI.h>

#include <SD.h>

#include <RTCZero.h>

const int chipSelect = SDCARD_SS_PIN;

RTCZero rtc;



/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 0;
const byte hours = 16;

/* Change these values to set the current initial date */
const byte day = 15;
const byte month = 6;
const byte year = 15;

/*
//einstellungen
bool parrallel = false;
double R_par = 33000;
int C = 100;
*/

void setup() {



  // Open serial communications and wait for port to open:

  Serial.begin(9600);

  while (!Serial) {

    ;  // wait for serial port to connect. Needed for native USB port only
  }

  Serial.begin(9600);

  /*
  // Interupts der digitalen Input Pins des TTD
  attachInterrupt(digitalPinToInterrupt(4), U_0_low, FALLING);
  attachInterrupt(digitalPinToInterrupt(3), U_ref_low, FALLING);
  attachInterrupt(digitalPinToInterrupt(2), U_s_low, FALLING);
  */

  //attachInterrupt(digitalPinToInterrupt(4), u_ttd_off, RISING);
  //attachInterrupt(digitalPinToInterrupt(4), u_ttd_on, FALLING);

  rtc.begin();  // initialize RTC
  // Set the time
  rtc.setHours(hours);
  rtc.setMinutes(minutes);
  rtc.setSeconds(seconds);
  // Set the date
  rtc.setDay(day);
  rtc.setMonth(month);
  rtc.setYear(year);


  Serial.print("Initializing SD card...");


  // see if the card is present and can be initialized:

  if (!SD.begin(chipSelect)) {

    Serial.println("Card failed, or not present");

    // don't do anything more:

    while (1)
      ;
  }

  Serial.println("card initialized.");

  //resolution
  analogReadResolution(12);
  //Pins
  pinMode(5, OUTPUT);  //Quelle TTD
  digitalWrite(5, HIGH);
  pinMode(4, INPUT);  //In R_0
  pinMode(3, INPUT_PULLUP);  //In R_ref
  pinMode(2, INPUT);  //In R_Sensor

  //Pins Temperaturregelung
  pinMode(A4, OUTPUT); //Regelt die Stromstärke
  pinMode(A2, INPUT); //Ausgang Differenzverstärker (Spannung am Heizwiderstand)

  //startet TTD
  load_cap();
}

//Widerstandswerte TTD in Ohm
double R_s = -1;  // noch unbekannt
double R_0 = 10000;
double R_ref = 100000;

//Zeitwerte
volatile double T_0 = 0;
volatile double T_ref = 0;
volatile double T_sensor = 0;
//Absolute Zeitwerte
volatile double T_0_last = 0;
volatile double T_ref_last = 0;
volatile double T_sensor_last = 0;
//Anfang Messung
volatile double time_last_charge = 0;

volatile int state = 0; //legt fest welcher der Pins zum messen / Widerstände zum entladen verwendet wird

// Interuptfunktionen für den TTD
void U_0_low() {
  if (state == 0){
    pinMode(4, INPUT);  //In R_0 high impedance
    int now = millis();
    T_0 = now - time_last_charge;
    Serial.println("T_0 ="+String(T_0)+"ms"); //Serial.println verursacht interrupt => problematisch!
    state++;
    load_cap();
  }
}

void U_ref_low() {
  if (state == 1){
    pinMode(3, INPUT);  //In R_ref high impedance
    int now = millis();
    T_ref = now - time_last_charge;
    Serial.println("T_ref ="+String(T_ref)+"ms");
    state++;
    load_cap();
  }
}

void U_s_low() {
  if (state == 2) {
    pinMode(2, INPUT);  //In R_s high impedance
    int now = millis();
    T_sensor = now - time_last_charge;
    Serial.println("T_sensor ="+String(T_sensor)+"ms");
    calc_R_s();
    log_R_s();
    state = 0;
    load_cap();
  }
}

void load_cap() {
  pinMode(5, OUTPUT);  //Quelle TTD
  digitalWrite(5, HIGH); //ladestrom an
  Serial.println("warte auf geladenen Kondensator...");
  delay(2000);
  /*
  double u_c = 1.5;
  while (u_c <= 3.2){
    u_c = double(analogRead(A1)) / 4095. * 3.3;
    Serial.println("u_0 = " + String(u_c) + "V");
  }
  */
  pinMode(5, INPUT);  //Quelle TTD high impedance
  time_last_charge = millis();
  //Serial.println("Kondensator mit u_0 = " + String(u_c) + "V am Zeitpunkt " + String(time_last_charge) + "ms geladen!");
  switch(state) {
    case 0:
      pinMode(4, OUTPUT);  //In R_0 low impedance
      digitalWrite(4, LOW);
      Serial.println("Messung von T_0 am Zeitpunkt " + String(time_last_charge) + "ms gestartet!");
      break;
    case 1:
      pinMode(3, OUTPUT);  //In R_ref low impedance
      digitalWrite(3, LOW);
      Serial.println("Messung von T_ref am Zeitpunkt " + String(time_last_charge) + "ms gestartet!");
      break;
    case 2:
      pinMode(2, OUTPUT);  //In R_s low impedance
      digitalWrite(2, LOW);
      Serial.println("Messung von T_s am Zeitpunkt " + String(time_last_charge) + "ms gestartet!");
      break;
    default:
      Serial.println("ERROR state invalid!");
      break;
  }
}


double calc_R_s() {
  //Berechnung R_sensor
  R_s = (T_sensor - T_0) / (T_ref - T_0) * R_ref;
  Serial.println("R_sensor von " + String(R_s) + "Ohm gemessen");
  return R_s;
}

String dataString_R = "";

void log_R_s() {
  dataString_R += (String(R_s) + "," + String(rtc.getHours()) + ":" + String(rtc.getMinutes()) + ":" + String(rtc.getSeconds()) + "\n");
}

// switchen der Spannungsquelle des TTD
void u_ttd_off() {
  digitalWrite(5, LOW);
  Serial.println("u_ttd_off");
}

void u_ttd_on() {
  digitalWrite(5, HIGH);
  Serial.println("u_ttd_on");
}

/*
int time_charged = 0;
int T_C = 0;
int time_discharged = 0;
int T_D = 0;
*/

/*
String dataString_T_C = "";
String times = "";
*/



void loop() {

  //Schleife TTD
  if (digitalRead(5) == LOW) {
    switch(state) {
      case 0:
        //Serial.println("pin_0:" + String(digitalRead(4)));
        U_0_low();
        Serial.println("U_0_low triggerd");
        break;
      case 1:
        //Serial.println("pin_ref:" + String(digitalRead(3)));
        U_ref_low();
        Serial.println("U_ref_low triggerd");
        break;
      case 2:
        //Serial.println("pin_s:" + String(digitalRead(2)));
        U_s_low();
        Serial.println("U_s_low triggerd");
        break;
      default:
        Serial.println("ERROR state invalid!");
        break;
    }
  }

  //Testsignal Stromquelle
  double U_h = analogRead(A2) / 4095 * 3.3;
  Serial.println("U_h = " + String(U_h) + "V");
  int quelle = millis() /100 % 255;
  analogWrite(A4, quelle);
  Serial.println("Output = " + String(double(quelle)/255.*100.) + "%");

  //Serial.println("A0 = "+String(double(analogRead(A0)) / 4095 * 3.3)+"V, ");
  //Serial.print("A2 = "+String(double(analogRead(A2)) / 4095 * 3.3)+"V, ");
  //Serial.println("A3 = "+String(double(analogRead(A3)) / 4095 * 3.3)+"V");


  /*
  if (millis()%10000 == 0) {
    Serial.println("Es lebt! State = " + String(state) );
  }
  */

  //int sensor = analogRead(1);
  
  /*
  if(digitalRead(4) == HIGH) {
    u_ttd_off();
  }
  else if(digitalRead(4) == LOW) {
    u_ttd_on();
  }
  */

  //Serial.println("U_0 = "+String(double(sensor) / 4095 * 3.3)+"V");

  // make a string for assembling the data to log:




  // read three sensors and append to the string:

  /*
  
  for (int analogPin = 1; analogPin < 2; analogPin++) {

    int sensor = analogRead(analogPin);
    double voltage = double(sensor) / 4095 * 3.3;
    if(millis()%100 == 0) {
      Serial.println(String(voltage)+"V");
    }
    
    //dataString += String(voltage);

    if (analogPin == 1) {
      if (voltage >= 3.28) {
        digitalWrite(A0, LOW);
        digitalWrite(LED_BUILTIN, LOW);
        //Serial.println("Source off at: ");
        //Serial.println(millis());
        time_charged = millis();
        T_C = time_charged - time_discharged;
        Serial.println("T_C = " + String(T_C) + "ms");
        double R = calc_R_s();
        Serial.println("===> R = " + String(R) + "Ohm");
        //dataString_T_C += (String(T_C)+",");
        //dataString_R += (String(R)+","+String(rtc.getHours())+":"+String(rtc.getMinutes())+":"+String(rtc.getSeconds())+":"+String(time_charged%1000)+"\n");

      }
      else if (voltage <= 0.2) {
        digitalWrite(A0, HIGH);
        digitalWrite(LED_BUILTIN, HIGH);
        //Serial.println("Source on at: ");
        //Serial.println(millis());
        time_discharged = millis();
        //T_D = time_discharged - time_charged;
        //Serial.println("T_D:" + T_D);
        //dataString_T_D += (String(T_D)+",\n");
      }
    }

    /*
    if (analogPin < 5) {

      dataString += ",";

    }
    */
  //} ????

  /*
  if (millis() % 500 == 0) {
    Serial.println("T_0 = "+String(T_0)+"ms");
    Serial.println("T_ref = "+String(T_ref)+"ms");
    Serial.println("T_sensor = "+String(T_sensor)+"ms");
  }
  */

  // open the file. note that only one file can be open at a time,

  // so you have to close this one before opening another.
  if (millis() % 3000 == 0) {
    //File dataFile_Voltages = SD.open("datalog.txt", FILE_WRITE);
    File dataFile = SD.open("datalog.txt", FILE_WRITE);

    // if the file is available, write to it:

    if (dataFile) {
      //if (true) {
      //dataString += "," + str(now());
      if (dataString_R != "") {
        dataFile.println(dataString_R);

        dataFile.close();

        // print to the serial port too:

        Serial.println("Saved:\n" + dataString_R);
        dataString_R = "";  //macht das alles kaputt?????
      }


    }

    // if the file isn't open, pop up an error:

    else if (!dataFile) {

      Serial.println("error opening datalog.txt");
    }
  }
}