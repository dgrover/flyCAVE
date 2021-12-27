int incomingByte = 0;   // for incoming serial data
const int led = 12;

void setup() {
  Serial.begin(19200);     // opens serial port, sets data rate to 9600 bps
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
}
void loop() {

  // send data only when you receive data:
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();
    if ( incomingByte == 48) { // "0"
      Serial.println("Laser Off");
      digitalWrite(led, LOW);
    }
    if ( incomingByte == 49) { // "1"
      Serial.println("Laser On for 0.5 sec");
      digitalWrite(led, HIGH);
      delay(500);
      digitalWrite(led, LOW);
    }
  }
}
