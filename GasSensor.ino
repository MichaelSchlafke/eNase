#include <SPI.h>

#include <SD.h>

#include <RTCZero.h>

const int chipSelect = SDCARD_SS_PIN;

RTCZero rtc;



/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 27;
const byte hours = 17;

/* Change these values to set the current initial date */
const byte day = 5;
const byte month = 3;
const byte year = 22;

void setup() {



  // Open serial communications and wait for port to open:

  Serial.begin(9600);

  while (!Serial) {

    ;  // wait for serial port to connect. Needed for native USB port only
  }

  Serial.begin(9600);

  /*
  // Interupts der digitalen Input Pins des TTD (alternative mothode zu Loop)
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
  pinMode(A0, OUTPUT); //Regelt die Stromstärke
  pinMode(A6, INPUT); //Ausgang Differenzverstärker (Spannung am Heizwiderstand)

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
  delay(2000); //warte bis Kondensator geladen
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

double Temperatur = -1;
double temp_target = 300; //Zieltemperatur in Kelvin

double R_h = -1;
double U_h = -1;
double U_reg = -1;

void log_R_s() {
  dataString_R += (String(rtc.getYear()) + "/" + String(rtc.getMonth()) + "/" + String(rtc.getDay()) + "," + String(rtc.getHours()) + ":" + String(rtc.getMinutes()) + ":" + String(rtc.getSeconds()) + "," + String(millis()/1000./60./60./24.) + "," + String(R_s) + "," + String(R_h) + "," + String(temp_target) + "," + String(U_h) + "," + String(U_reg) + "\n");
}

double R_load = 66.; //Effekiver Widerstand Stromregelung

double calc_R_h(double U_h, double U_reg) {
  //Bestimmt aktuellen Heizwiderstand
  //U_h: Spannung am Heizelement; U_reg: Spannung am eingang der Stromregelung
  double I = U_reg / R_load;
  double R_h = U_h / I;
  return R_h;
}

double calc_U_reg(double I) {
  //berrechnet für gegebenen Strom benötigte Spannung
  double U_reg = I * R_load;
}

double calc_temp(double R_h) {
  Temperatur = 290.99 * R_h - 1416.7 - 273.15; //per linearen Fit bestimmt
  return Temperatur;
}


void loop() {

  int mode = -1;
  //Welchselt Zieltemperatur
  if(millis()%30000 <= 15000) {
    temp_target = 300; //Raumtemp
    mode = 3;
  }
  else {
    temp_target = 623; //350°C
    mode = 2;
  }

  
  U_h = analogRead(A6) / 4095.0 * 3.3 * 0.5;
  int quelle = 150;
  //Testsignale Stromquelle
  switch(mode) {
    case 1:
      quelle = millis() / 100 % 255; //Testsignal Sägezahn
      break;
    case 2:
      quelle = 2.97 / 3.3 * 255; //entspricht 350°C
      break;
    case 3:
      quelle = 0.1 / 3.3 * 255; //Raumtemperatur
      break;
    default:
      break;
  }
  analogWrite(A0, quelle);
  U_reg = (double(quelle)/255.*3.3);
  //Serial.println("U_reg = " + String(U_reg) + "V");
  R_h = calc_R_h(U_h, U_reg);
  Temperatur = calc_temp(R_h);

  //Schleife TTD
  if (digitalRead(5) == LOW) {
    //Output Temperaturregelung. In schleife für weniger Spam
    Serial.print("Output = " + String(double(quelle)/255.*100.) + "%, ");
    Serial.print("U_h = " + String(U_h) + "V, ");
    //Serial.print("Temperatur = " + String(Temperatur) + "°C, ");
    Serial.println("R_h = " + String(R_h) + "Ohm");
    //Ende
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

  
  if (millis()%10000 == 0) {
    Serial.println("Es lebt! State = " + String(state) );
  }
 

  if (millis() % 3000 == 0) {
    File dataFile = SD.open("datalog.txt", FILE_WRITE);

    // if the file is available, write to it:

    if (dataFile) {
      if (dataString_R != "") {
        dataFile.print(dataString_R);

        dataFile.close();

        // print to the serial port too:

        Serial.println("Saved:\n" + dataString_R);
        dataString_R = "";
      }


    }

    // if the file isn't open, pop up an error:

    else if (!dataFile) {

      Serial.println("error opening datalog.txt");
    }
  }
}