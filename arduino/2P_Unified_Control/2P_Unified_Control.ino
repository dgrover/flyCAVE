  /*
  Unified Control for 2P
  (IR Laser, IR LED, Air Puffer, Record Trigger)
  
  Serial Inputs:
  IR LED:
      red pin: pin 5 on arduino
      black pin: ground on arduino (same side)
      0 - Turn off LED
      1 - Turn On LED (Full Intensity)
      + - increase Intensity (PWM)
      - - Descrease Intensity (PWM)

          LED_STEP - Step size for intensity inc/decrement
          LED_MAS  - Full Scale (Always on)

  AIR PUFFER:
      2 - Send a puff of air
          Duration set by macro PUFF_TIME

  LASER PUNISHMENT:
      3 - Send laser pules
          DAC output (Current) set by LASER_POWER macro 
          Pulse duration set by LASER_TIME macros (milliseconds)

  TRIGGER RECORDING:
      4 - Send single pulse to trigger Low-High-Low
          Active time high set by macro TRIGGER_TIME
          
*/

// Libraries
#include <SPI.h>
#include <RCSwitch.h>

// Macro
#define LED_STEP      15   // PWM step size per increment LED instensity
#define LED_MAX       255  // LED max instensity
#define PUFF_TIME     150  //300  // Duration of time to puff air
#define LASER_POWER   2350 // value corresponding to power to reach 35C
#define LASER_TIME    500  // Laser pulse width in ms
#define TRIGGER_TIME  500  // Time trigger pin high on trigger

// Constants for RC Switch 
char* code_word = "010110011101001100110010";

// GPIO PINS
int Chip_Select = 10;   // CS Pin for DAC
int Load_DAC    = 9;    // Load Pin for DAC
int led_pin     = 5;    // IR LED PWM 
int switch_pin  = 6;   // RC Switch Input
int trigger_pin = 7;   // Trigger pin

// Globals
unsigned int serialInLASER, serialInLED; // Current for Laser/LED
int serial_read_byte = 0;                // Incoming Serial Byte from PC
RCSwitch mySwitch = RCSwitch();          // RC Switch Object                     
int intensity_val = 255;                 // Initial IR LED intensity

void setup() {

    // Serial Console (Virtual COM)
    Serial.begin(19200);
  
    // Set up interface pins
    pinMode(Load_DAC, OUTPUT);
    pinMode(Chip_Select, OUTPUT); 
    pinMode(led_pin, OUTPUT);  
    pinMode(trigger_pin,OUTPUT);
    mySwitch.enableTransmit(switch_pin);
    
    // initialize SPI:
    SPI.begin(); 
    SPI.beginTransaction(SPISettings(SPI_CLOCK_DIV128, MSBFIRST, SPI_MODE3));

    // Init Sequence for DAC?
    digitalWrite(Load_DAC,LOW);
    delay(200);
    digitalWrite(Load_DAC,HIGH);
    delay(10);

    // Initialize pins
    analogWrite(led_pin,0);
    digitalWrite(trigger_pin,LOW);
    digitalWrite(Chip_Select, HIGH); // CS active low
    digitalWrite(Load_DAC, HIGH);    // Load falling?
    
    //Initalize DAC outputs to 0
    delay(100);
    serialInLED   = 0;
    serialInLASER = 0; 
    setVoltages(serialInLED,serialInLASER);  //LED, LASER
  
    // Setup RC Switch 
    mySwitch.setPulseLength(256);
    mySwitch.setProtocol(2);
}

void loop() {

   if(Serial.available() > 0) {
        
        serial_read_byte = Serial.read();
        
        switch(serial_read_byte) {

            //ASCII 0
            case 48:
                Serial.println("LED Off");
                analogWrite(led_pin, 0);
            break;
            
            //ACSII 1
            case 49:
                Serial.println("LED On");
                analogWrite(led_pin, intensity_val);
            break;
            
            //ASCII 2
            case 50:
                Serial.println("Air puff activated");
                mySwitch.send(code_word);
                delay(PUFF_TIME);
                mySwitch.send(code_word);
            break;

            // ASCII 3
            case 51:
            {
                //serialInLASER = 1000; // this value used for aligning laser
                
                // code for final use case of laser heat punishment
                serialInLASER = LASER_POWER; // this value used for current setting when final temperature of 35C is achieved
                
                set_V_LASER(serialInLASER); // set current
                delay(LASER_TIME); //delay of 0.5 sec US
                serialInLASER = 0; // set laser to zero
                set_V_LASER(serialInLASER);
            break;
            }

            // ASCII 4
            case 52:
                digitalWrite(trigger_pin,HIGH);
                delay(TRIGGER_TIME);
                digitalWrite(trigger_pin,LOW);
            break;
                        
            // ACSII +
            case 43:
                intensity_val = (intensity_val + LED_STEP) > LED_MAX ? intensity_val : intensity_val + LED_STEP;
                Serial.print("LED intensity increased VALUE: ");
                Serial.print(intensity_val);
                Serial.println();
                analogWrite(led_pin, intensity_val);
            break;
            
            // ACSII -
            case 45:
               intensity_val = (intensity_val - LED_STEP) < 0 ? intensity_val : intensity_val - LED_STEP;
                Serial.print("LED intensity decreased VALUE: ");
                Serial.print(intensity_val);
                Serial.println();
                analogWrite(led_pin, intensity_val);
            break;
            
        }
    }
}



// FUNCTION FOR ADC

/*
 * setVoltages assumes the values laser and led are unsigned values
 * between 0 and 4095
 */
void setVoltages(unsigned int led, unsigned int laser){
  unsigned int temp_LED =     0b0010000000000000;   //output to reg A
  unsigned int temp_LASER =   0b0011000000000000;   //output to reg B

  // oreq value to register address
  temp_LED   |= led;
  temp_LASER |= laser;
  
  //write LED value to output
  digitalWrite(Load_DAC, HIGH);
  digitalWrite(Chip_Select, LOW);   //load
  SPI.transfer16(temp_LED);
  digitalWrite(Chip_Select, HIGH);
  //load value into output reg
  digitalWrite(Load_DAC, LOW);
  

  //write LASER value to output
  digitalWrite(Load_DAC, HIGH);
  digitalWrite(Chip_Select, LOW);
  SPI.transfer16(temp_LASER);
  digitalWrite(Chip_Select, HIGH);
  //load value into output reg  
  digitalWrite(Load_DAC, LOW);
    
  
}// end setCurrent

/*
 * set_V_LASER assumes the value laser is an unsigned value
 * between 0 and 4095
 */
void set_V_LASER(unsigned int laser){
  unsigned int temp_LASER = 0b0011000000000000;   //output to reg B

  // oreq DAC value to register address
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
