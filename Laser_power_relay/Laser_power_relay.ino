int incomingByte = 0;   // for incoming serial data
const int led = 12;

void setup() {
  Serial.begin(9600);     // opens serial port, sets data rate to 9600 bps
  pinMode(led, OUTPUT);
}
void loop() {

  // send data only when you receive data:
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();
    if ( incomingByte > 48) {
      digitalWrite(led, HIGH);
    }
    if ( incomingByte < 49) {
      digitalWrite(led, LOW);
    }

  }
}
