#include <Servo.h> // use the Arduino Servo library

#define MIN_VALUE         1000
#define MAX_VALUE         2000
#define NUM_CHANNELS      6

int AUX1 = 1000, AUX2 = 1000; // default PWM duty cycles for AUX1(D10) AUX2(D11)
int increase_aux1 = 0, increase_aux2 = 0;

const int SERVO_PINS[ NUM_CHANNELS ] = { 2,4,5,6,7,8 }; // output pins for Servos
const int DEFAULT_PULSE_WIDTHS[ NUM_CHANNELS ] 
            = { 1500,1500,1500,1500,AUX1,AUX2 }; // default PWM duty cycles
const int ANALOG_PINS[ 6 ] = { A0, A1, A2, A3}; // input analog pins
Servo servos[ NUM_CHANNELS ]; // array of Servo objects

void setup() {
  for ( uint8_t i=0; i < NUM_CHANNELS; i++ ) {
     servos[i].attach( SERVO_PINS[i], MIN_VALUE, MAX_VALUE ); // select output pin
     servos[i].writeMicroseconds( DEFAULT_PULSE_WIDTHS[i] ); // set initial duty cycle
  }
  Serial.begin(9600);          //  setup serial
}

void loop() {
  for ( uint8_t i=0; i < 4; i++ ) {
    int val = analogRead(ANALOG_PINS[i]);
    val = map(val, 0, 1023, MIN_VALUE, MAX_VALUE);
    //Serial.println(val);
    servos[i].writeMicroseconds( val );
  }
  if(digitalRead(10) == LOW) increase_aux1 = 1;
  if(digitalRead(10) == HIGH && increase_aux1){
    AUX1 += 500;
    if(AUX1 > MAX_VALUE) AUX1 = MIN_VALUE;
    increase_aux1 = 0;
  }
  
  if(digitalRead(11) == LOW) increase_aux2 = 1;
  if(digitalRead(11) == HIGH && increase_aux2){
    AUX2 += 500;
    if(AUX2 > MAX_VALUE) AUX2 = MIN_VALUE;
    increase_aux2 = 0;
  }
  //Serial.println(AUX1);
  servos[4].writeMicroseconds(AUX1);
  //Serial.println(AUX2);
  servos[5].writeMicroseconds(AUX2);
  //Serial.println();
}

//////////////////////////////////////////////////////////////////////////

