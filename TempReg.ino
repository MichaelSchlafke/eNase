void setup() {
  // put your setup code here, to run once:

  //resolution
  analogReadResolution(12);
  //Pins
  pinMode(A4, OUTPUT); //Regelt die Stromstärke
  pinMode(A2, INPUT_PULLUP); //Ausgang Differenzverstärker (Spannung am Heizwiderstand)
}

void loop() {
  // put your main code here, to run repeatedly:
  double U_h = analogRead(A2) / 4095 * 3.3;
  Serial.println("U_h = " + String(U_h) + "V");
  int quelle = millis() /100 % 255;
  //int quelle = 100;
  analogWrite(A4, quelle);
  Serial.println("Output = " + String(double(quelle)/255.*100.) + "%");
}
