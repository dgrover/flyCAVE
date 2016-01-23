 #include <SPI.h>
unsigned int val_A = 0b0010000000000000;
unsigned int val_B = 0b0011000000000000;
int Chip_Select=10;
int Load_DAC=9;

unsigned int a=0;
unsigned int b=0;

unsigned int serialLed;
unsigned int serialLaser;

void setup() {
  
  pinMode(Load_DAC, OUTPUT);
  digitalWrite(Load_DAC,LOW);
  delay(200);
  digitalWrite(Load_DAC,HIGH);
  // initialize SPI:
  SPI.begin(); 
  SPI.beginTransaction(SPISettings(SPI_CLOCK_DIV128, MSBFIRST, SPI_MODE3));
  // initalize the chip select pin:

  //Init control pins
  delay(10);
  pinMode(Chip_Select, OUTPUT);
  digitalWrite(Chip_Select, HIGH);
  pinMode(Load_DAC, OUTPUT);
  digitalWrite(Load_DAC, HIGH);
  
  //Init outputs to zero
  delay(100);
  digitalWrite(Chip_Select, LOW);
  SPI.transfer16(val_B);
  digitalWrite(Chip_Select, HIGH);
  digitalWrite(Load_DAC, LOW);
  digitalWrite(Load_DAC, HIGH);
  digitalWrite(Chip_Select, LOW);
  SPI.transfer16(val_A);
  digitalWrite(Chip_Select, HIGH);  
  digitalWrite(Load_DAC, LOW);
  digitalWrite(Load_DAC, HIGH);

  b = 0;
  a = 0;
  setVoltages(a,b);  //LED, LASER
  //set_V_LASER(a);

  //init serial
  Serial.begin(19200);
  
}

void loop() {

 if (Serial.available() > 0)
 {
    serialLed = Serial.parseInt();
    serialLaser = Serial.parseInt();
    
    setVoltages(serialLed,serialLaser);     //LED, LASER 
    
    //set_V_LED(serialLed);
    
 }// end if serial


//while (Serial.available() > 0) {
//    int inChar = Serial.read();
//    if (isDigit(inChar)) {
//      // convert the incoming byte to a char
//      // and add it to the string:
//      inString += (char)inChar;
//    }
//    
//    // if you get a newline, print the string,
//    // then the string's value:
//    
//    if (inChar == '\n') {
//      Serial.print("Value:");
//      Serial.println(inString.toInt());
//      Serial.print("String: ");
//      Serial.println(inString);
//      // clear the string for new input:
//     
//      set_V_LED(inString.toInt());
//      inString = "";
//
//    }
//  }




}

/*
 * setVoltages assumes the values laser and led are unsigned values
 * between 0 and 4095
 */
void setVoltages(unsigned int led, unsigned int laser){
  unsigned int temp_LED = 0b0010000000000000;   //output to reg A
  unsigned int temp_LASER =   0b0011000000000000;   //output to reg B

  //bitwise OR
  temp_LED |= led;
  temp_LASER |= laser;
  
  //write LED value to output
  digitalWrite(Chip_Select, LOW);   //load
  SPI.transfer16(temp_LED);
  digitalWrite(Chip_Select, HIGH);
  //load value into output reg
  digitalWrite(Load_DAC, LOW);
  digitalWrite(Load_DAC, HIGH);

  //write LASER value to output
  digitalWrite(Chip_Select, LOW);
  SPI.transfer16(temp_LASER);
  digitalWrite(Chip_Select, HIGH);
  //load value into output reg  
  digitalWrite(Load_DAC, LOW);
  digitalWrite(Load_DAC, HIGH);  
  
}// end setCurrent

/*
 * set_V_LASER assumes the value laser is an unsigned value
 * between 0 and 4095
 */
void set_V_LASER(unsigned int laser){
  unsigned int temp_LASER = 0b0011000000000000;   //output to reg B

  //bitwise OR
  temp_LASER |= laser;
  
  //write LASER value to output
  digitalWrite(Chip_Select, LOW);
  SPI.transfer16(temp_LASER);
  digitalWrite(Chip_Select, HIGH);
  //load value into output reg  
  digitalWrite(Load_DAC, LOW);
  digitalWrite(Load_DAC, HIGH);

} // end set_V_LASER


/*
 * set_V_LED assumes the value LED is an unsigned value
 * between 0 and 4095
 */
void set_V_LED(unsigned int led){
    unsigned int temp_LED =   0b0010000000000000;   //output to reg A

  //bitwise OR
  temp_LED |= led;
 
  //write LED value to output
  digitalWrite(Chip_Select, LOW);   //load
  SPI.transfer16(temp_LED);
  digitalWrite(Chip_Select, HIGH);
  //load value into output reg
  digitalWrite(Load_DAC, LOW);
  digitalWrite(Load_DAC, HIGH);

} // end set_V_LED

