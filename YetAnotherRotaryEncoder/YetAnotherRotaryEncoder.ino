#include <Arduino.h>

//Pins adjusted for use on a Arduino Nano/Uno

// Rotary encoder pins
#define PIN_A 3 // was pin 32
#define PIN_B 4
#define PUSH_BTN 5 // was pin 16

// A turn counter for the rotary encoder (negative = anti-clockwise)
int rotationCounter = 200;

// Flag from interrupt routine (moved=true)
volatile bool hasRotaryEncoderChanged = false;
volatile uint8_t rotary_state = 0;

#define ROTATE_CLOCKWISE  1
#define NO_MOVEMENT   0
#define ROTATE_ANTICLOCKWISE  -1

int8_t rotation_direction[16];

void init_rotation_direction(){
    //start with no movement
    for(uint8_t i=0; i<16; i++) {
        rotation_direction[i]=NO_MOVEMENT;
    }

    rotation_direction[0b0011] = ROTATE_CLOCKWISE; //0011 => Right after previous Right
   // rotation_direction[0b0101] = ROTATE_CLOCKWISE; //0101 => Right after previous Left
    rotation_direction[0b0110] = ROTATE_ANTICLOCKWISE;  //0110 => Left after previous Left
    rotation_direction[0b1001] = ROTATE_ANTICLOCKWISE;  //1001 => Left after previous Left
    rotation_direction[0b1100] = ROTATE_CLOCKWISE; //1100 => Right after previous Right
 //   rotation_direction[0b1110] = ROTATE_ANTICLOCKWISE;  //1110 => Left after previous Right
}    

// Interrupt routine just sets a flag when rotation_indication is detected
void rotary_ISR()
{
    hasRotaryEncoderChanged = true;

    // both D3 and D4 are on PORTD of a Nano
    rotary_state = PIND;                       //Read the register for port D (pins A and B) directly
    rotary_state = ( rotary_state & 0b0011000); //filter to allow values of D3 and D4 only
    rotary_state = rotary_state>>3;             //make sure the 2 important values are on the LSB
}

void setup()
{
    Serial.begin(115200);
    init_rotation_direction();

    // The module already has pullup resistors on board
    pinMode(PIN_A, INPUT); 
    pinMode(PIN_B, INPUT); 

    // But not for the push switch
    pinMode(PUSH_BTN, INPUT_PULLUP);

    // We need to monitor both pins, rising and falling for all states
    attachInterrupt(digitalPinToInterrupt(PIN_A), rotary_ISR, CHANGE);
 //   attachInterrupt(digitalPinToInterrupt(PIN_B), rotary_ISR, CHANGE);
    Serial.println("Setup completed");
}

uint8_t lookup_index = 0;

void loop()
{
    // Has rotary_ISR encoder moved?
    if (hasRotaryEncoderChanged)
    {
        hasRotaryEncoderChanged = false;
        lookup_index = lookup_index<<2;             // Shift the old value over 2 bits
        lookup_index = lookup_index | rotary_state; // Put the new value on the LSB
        lookup_index = lookup_index & 0b00001111;   // Discard all but the 4 least significant bits
        
        int8_t rotation_increment = rotation_direction[lookup_index]; //Get the rotation_indication from the array   
        
        // If valid movement, do something
        char * rotation_indication=" - "; //rotation_indication is a tri state, L R or invalid
        if (rotation_increment != 0)
        {
            rotationCounter += rotation_increment * 5;
            rotation_indication = (rotation_increment < 1 ? " L " :  " R ");

        }
        //having Serial.print ouside the if makes the glitches/bounces visible
        Serial.print(lookup_index | 128, BIN);
        Serial.print(rotation_indication);
        Serial.println(rotationCounter);
    }

    if (digitalRead(PUSH_BTN) == LOW)
    {
        rotationCounter = 0;
        Serial.print("X");
        Serial.println(rotationCounter);
 
        // Wait until button released (demo only! Blocking call!)
        while (digitalRead(PUSH_BTN) == LOW)
        {
            delay(100);
        }
    }
    
}
