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

  // Interupts der digitalen Input Pins des TTD
  attachInterrupt(digitalPinToInterrupt(4), U_0_low, FALLING);
  attachInterrupt(digitalPinToInterrupt(3), U_ref_low, FALLING);
  attachInterrupt(digitalPinToInterrupt(2), U_s_low, FALLING);

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
  pinMode(3, INPUT);  //In R_ref
  pinMode(2, INPUT);  //In R_Sensor
  //Überprüfung TTD Pins
  pinMode(A1, INPUT);  //In R_Sensor

  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A0, OUTPUT);
  digitalWrite(A0, HIGH);
}

//Widerstandswerte TTD in Ohm
double R_s = -1;  // noch unbekannt
double R_0 = 10000;
double R_ref = 33000;

//Zeitwerte
volatile double T_0 = 0;
volatile double T_ref = 0;
volatile double T_sensor = 0;
//Absolute Zeitwerte
volatile double T_0_last = 0;
volatile double T_ref_last = 0;
volatile double T_sensor_last = 0;

// Interuptfunktionen für den TTD
void U_0_low() {
  int now = millis();
  T_0 = now - T_0_last;
  T_0_last = now;
  //Serial.println("T_0 ="+String(T_0)+"ms"); //Serial.println verursacht interrupt => problematisch!
  Serial.println("R_O INTERRUPT!!!!!!!");
}

void U_ref_low() {
  int now = millis();
  T_ref = now - T_ref_last;
  T_ref_last = now;
  //Serial.println("T_ref ="+String(T_ref)+"ms");
}

void U_s_low() {
  int now = millis();
  T_sensor = now - T_sensor_last;
  T_sensor_last = now;
  //Serial.println("T_sensor ="+String(T_sensor)+"ms");
}

double calc_R_s() {
  //Berechnung R_sensor
  R_s = (T_sensor - T_0) / (T_ref - T_0) * R_ref;
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

  Serial.print("A2 = "+String(double(analogRead(A2)) / 4095 * 3.3)+"V, ");
  Serial.println("A3 = "+String(double(analogRead(A3)) / 4095 * 3.3)+"V");

  if (millis()%10000 == 0) {
    Serial.println("Es lebt!");
  }

  int sensor = analogRead(1);
  
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