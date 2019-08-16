// This code base originally came from:
//https://www.brainy-bits.com/stepper-motor-with-joystick-and-limit-switches/

// Description of two channel lift table controller
// With this code and the corresponding hardware two controllers can control one lift table stepper driver.
// this allows the table to be controlled without having the smoothie connected to a PC. 
// when the joystick is activated the mux switches to Arduino control
// when the joystic is not activated the stepper controls are from the smoothie.
// the tables threaded lift rod is M6 x 1.0 mm pitch. This means that one revolution is a 1mm change in height. 
// Switch settings on TB660 driver
// 16 micro steps per step
// 360 degrees / 1.8 degrees/step = 200 steps/rev
// 200 steps/rev * 16 microsteps/step = 3200 micro steps/rev
// SW1=OFF
// SW2=OFF
// SW3=ON
// SW4=ON
// SW5=OFF
// SW6=OFF

//V1.0  
  //tested virgin two channel control system system
  // This code modified from the arduino only code [Joystick_Stepper_Control_2.0]
  // Channel 1= Arduino
  // Channel 2 = smoothie
  // Stepper driver = TB6600
  // 74157 mux http://www.ti.com/lit/ds/symlink/sn54157.pdf
  // changed the stepper controls to grnd the - signals to match: https://cohesion3d.freshdesk.com/support/solutions/articles/5000744633-wiring-a-z-table-and-rotary-step-by-step-instructions
// V1.2
  // -Enable on the stepper driver needs to be HIGH to allow the stepper controller to work.
  // all functions working from the arduino channel.
  // only the arduino channel is tested.
  // changed num = num^=1 to num = num^1 to elliminate compiler warning.
  // after setting the steppers for micro steps the loop time is too long.
  // necessitates driving the stepper with fast subroutines. Plan to run 1 step, 5 steps, 10 steps increments to solve this problem.
//V3.2  
  // for speed redesigned the way the stepper is driven.  
  // step_speed now acts as a multiplier. The stepperIncrement() uses this value to control how many microsteps the controller is given for each joystick move. 
//V4.0
  // resedigned step control to keep the loop short
  // fixed value for the step pulse (low part)
  // variable value for the off (high) part of the pulse determines speed (time between pulses)
// 4.1
  // added home functions. Joystick left is home bottom, right is home top
  // bug: table runs faster in high speed home-mode than in manual up/down mode 


//DEBUG 
#define DEBUG       //V1.2 Comment this out for normal compile
#define VERSION 4.1 //
// CONTROL PANEL I/O
#define slowLed 5   // Pin 5 connected to slow LED on panel
#define medLed 6  // Pin 6 connected to med LED on panel
#define fastLed 19   // Pin 19 [A5] connected to fasd LED on panel [pin 7 was dead on the board I had]
#define X_pin A0    // Pin A0 connected to joystick x axis
#define Y_pin A1    // Pin A1 connected to joystick y axis (up down)
#define Joy_switch 4  // Pin 4 connected to joystick switch

// Stepper Driver I/O
// The + stepper controls are connected to 5v. This supplies the internal optocouplers with a 5V source.
// The - stepper controls are connected to the arduino. This means that stepper signals are LOW true.

#define step_pin 9  // Pin 9 connected to PUL- pin through the mux to stepper driver
#define dir_pin 8   // Pin 8 connected to DIR- pin through the mux to the stepper driver
#define stepper_en 12     // Pin 12 connected through mux to enable pin on stepper driver

// Mux control
#define chan_select 10      //Pin 10 connected to the select pin on the mux [74157]. 
                            //Low selects Arduino stepper controls
                            //High selects smoothie stepper controls
//Debug
#define test_pin 11       // used for scope triggering

//Limit Switches
#define Limit01 2  // Pin 2 connected to UP Limit switch 
#define Limit02 3  // Pin 3 connected to DWN Limit switch

// stepper variables

//constants for the joystick switch speed selection
#define HIGHSPEED 10 // high speed 
#define MEDSPEED 5 // med speed
#define LOWSPEED 1 // low speed
// Constants for the step pulse dead time which determines speed
#define HIGHDEADTIME 10 // off time (usec) for high speed
#define MEDDEADTIME 100 // off time (usec) for med speed
#define LOWDEADTIME 1000 // off time (usec) for slow speed


int step_speed = 1;  // step speeds are 10, 5, 1 whereas 10 is high and 1 is low
                      // these values set the number of steps that the driver will take each time the joystick is seen above its threshold postion 
int stepPulse = 10; // low pulse width of the stepper PUL input
int stepLowTime = 10; //low time (usec) for the -PUL on stepper driver
int stepHighTime = LOWDEADTIME; 
int laststepCounter = 0; //keeps track of when the counter changes
int stepCounter = 0;  // step counter

//limit switch variables
bool upperLimitReached = false;
bool lowerLimitReached = false;

int num;               // temp variable for blinking LED
int stickPosition;    // the Y joystick value
float mmPosition;      // table postion in mm
//Testing
bool testMode = false; // test mode 

void setup() 
{
 #ifdef DEBUG
  Serial.begin(57600);   //V1.0
 #endif
// Stepper Driver Controls
   pinMode(dir_pin, OUTPUT);
   pinMode(step_pin, OUTPUT);
   pinMode(stepper_en, OUTPUT);
   pinMode(chan_select,OUTPUT);
   pinMode(test_pin,OUTPUT);
//Control Panel 
   pinMode(slowLed, OUTPUT);
   pinMode(medLed, OUTPUT);
   pinMode(fastLed, OUTPUT);
   pinMode(Joy_switch, INPUT_PULLUP);
//Limit Switches   
   pinMode(Limit01, INPUT_PULLUP);
   pinMode(Limit02, INPUT_PULLUP);
// Stepper Control  
   digitalWrite(stepper_en, HIGH);  // enable the stepper driver
   //digitalWrite(chan_select, HIGH); //select the smoothie channel
   digitalWrite(chan_select, LOW);  //enable the arduino's stepper signals
   delay(5);  // Wait for Driver to wake up
   digitalWrite(slowLed,HIGH);
   digitalWrite(medLed, LOW);
   digitalWrite(fastLed,LOW);  
   Serial.print("K40 Dual Channel Lift Controller V");
   Serial.println(VERSION); 
  if (!digitalRead(Joy_switch)) 
  {
    testMode=true;            // if switch held during reset set test mode
    Serial.println("Test mode initiated");
  }  
 // homeTable();
}

void loop() 
{
  digitalWrite(test_pin,HIGH); // for measuring the loop time
  
  if(testMode)
  {// check if test mode was set in the startup
    test_Mode();        // go and run tests first
  }
  
  if (!digitalRead(Joy_switch)) 
    {  //  If Joystick switch is clicked
      while(!digitalRead(Joy_switch))
      {//wait here for Joy switch to go high. The Joy stick bounces alot
      //Serial.println("Joyswitch = LOW");
      }
      switch (step_speed) 
      {  // check current value of step_speed and change it
        case HIGHSPEED: //Fast speed
          stepHighTime = LOWDEADTIME;
          step_speed=LOWSPEED;  // If fast speed go to slow speed
          #ifdef DEBUG //V1.2
            Serial.println("-------Slow Speed-------");
          #endif
          digitalWrite(medLed, LOW);
          digitalWrite(fastLed,LOW);
          digitalWrite(slowLed, HIGH);
        break;
        case MEDSPEED: //med speed
          stepHighTime = HIGHDEADTIME;
          step_speed=HIGHSPEED;  // if med speed go to fast speed
          #ifdef DEBUG
            Serial.println("-------Fast Speed-------");
          #endif
          digitalWrite(slowLed, LOW);
          digitalWrite(medLed, LOW);
          digitalWrite(fastLed,HIGH);
        break;
        case LOWSPEED:  //slow speed
          stepHighTime = MEDDEADTIME; 
          step_speed=MEDSPEED;  // if slow speed go to medium speed
          #ifdef DEBUG
            Serial.println("-------Medium Speed------");
          #endif
          digitalWrite(slowLed, LOW);
          digitalWrite(fastLed,LOW);
          digitalWrite(medLed, HIGH);
        break;
      }
    }    
  
 // -------------- CHECK IF JOYSTICK IS PUSHED UP ----------- 
  if(digitalRead(Limit01))
    {  //if the upper limit switch is not activated check for joystick UP movement 
     digitalWrite(dir_pin, HIGH);  // (HIGH = anti-clockwise / LOW = clockwise)
     while(analogRead(Y_pin) >712 && digitalRead(Limit01))
      { // send step pulses as long as the joystick is held up and the limit switch is not reached
        // this section needs to stay minimal to enable high enough speed
        digitalWrite(step_pin, LOW);
        delayMicroseconds(stepPulse);
        digitalWrite(step_pin, HIGH);
        delayMicroseconds(stepHighTime); // is this needed
      }
     //put blink here...
    if (!digitalRead(Limit01)) 
      {  // check if up limit switch was activated with the up movement
      Serial.println("Lift at top");
      for (int i = 0; i <= 10; i++)
        {//blink the fast (upper) led as a warning
         digitalWrite (fastLed, num = num ^1);
         delay(500);
        }
      }
      restore_LED();     // put the led back to it previous state 
    }


//----------- CHECK TO SEE IF THE JOYSTICK IS PUSHED DOWN ----------------- 
  if(digitalRead(Limit02))
    {  //if the lower limit switch is not activated check for joystick down movement 
    digitalWrite(dir_pin, LOW);  // (HIGH = anti-clockwise / LOW = clockwise)
    while(analogRead(Y_pin) <312 && digitalRead(Limit02))
    //while(analogRead(Y_pin) >712)
      {
        digitalWrite(step_pin, LOW);
        delayMicroseconds(stepPulse);
        digitalWrite(step_pin, HIGH);
        delayMicroseconds(stepHighTime); // is this needed
      } 
   
    if (!digitalRead(Limit02)) 
      {  // check if lower limit switch was activated during down movement
      Serial.println("Lift at bottom");
      for (int i = 0; i <= 10; i++)
        {//blink the slow (lower) led as a warning
         digitalWrite (slowLed, num = num ^1);
         delay(500);
        }     
      }
      restore_LED();     // put the led back to it previous state   
    }
  // Pulse an I/O pin to use as a scope loop sync
  digitalWrite(test_pin, LOW);
  delayMicroseconds(20);
  digitalWrite(test_pin, HIGH);

// ----- Manage home positions ----

  if(analogRead(X_pin) >712 && digitalRead(Limit01))
  { 
   homeTop(); 
  }

 if(analogRead(X_pin) <312 && digitalRead(Limit02))
  { 
   homeBottom(); 
  }

}//end Loop



// ============================ FUNCTIONS ===========================



/// ------------------ HOME ------
void homeTop()
 { 
  digitalWrite(dir_pin, HIGH);  // (HIGH = anti-clockwise / LOW = clockwise)
  while (digitalRead(Limit01) && digitalRead(Joy_switch))
     { // step until the upper limit is reached (Limit = 0)or the Joystick is pushed in.
      digitalWrite(step_pin, LOW);
      delayMicroseconds(stepPulse);
      digitalWrite(step_pin, HIGH);
      delayMicroseconds(stepHighTime); // is this needed??? 
     }
  while(!digitalRead(Joy_switch))
    {//wait here for Joy switch to go high. The Joy stick bounces alot
    //Serial.println("Joyswitch = LOW");
    }
  stepCounter = 0; // reset the position counter
  Serial.println();
  Serial.println("Table @ TOP or Stopped");
  return;
 }

void homeBottom()
  {
  digitalWrite(dir_pin, LOW);  // (HIGH = anti-clockwise / LOW = clockwise)
  
  while (digitalRead(Limit02) && digitalRead(Joy_switch))
     { // step until the lower limit is reached (Limit = 0), or the Joystick is pushed in.
      digitalWrite(step_pin, LOW);
      delayMicroseconds(stepPulse);
      digitalWrite(step_pin, HIGH);
      delayMicroseconds(stepHighTime); // is this needed??? 
     }
    while(!digitalRead(Joy_switch))
    {//wait here for Joy switch to go high. The Joy stick bounces alot
    //Serial.println("Joyswitch = LOW");
    }
    stepCounter = 0; // reset the position counter
    Serial.println();
    Serial.println("Table @ BOTTOM or Stopped");
  return;
  }



// .....................
// Used to put the panel LED's back to their previous state ...
// ... after blinking them on achieveing the top or bottom
// ... with a joystick move in the direction of the closed endstop 

void restore_LED()
{
  //Serial.println (step_speed); 
  switch (step_speed) 
      {  // check current value of step_speed update the LED's
        case 10:
         digitalWrite(slowLed,LOW);
         digitalWrite(medLed,LOW);
         digitalWrite(fastLed,HIGH);
        break;
        case 5:
         digitalWrite(slowLed,LOW);
         digitalWrite(medLed,HIGH);
         digitalWrite(fastLed,LOW);
        break;
        case 1:
          digitalWrite(fastLed, LOW);
          digitalWrite(slowLed, HIGH);
          digitalWrite(medLed,LOW); 
        break;
      }
      return;
}



//////////////////Test Mode /////////////////

void test_Mode()
{
  int testNo = 1;
  bool notTestEnd = true;
  digitalWrite(slowLed,LOW);
  Serial.println(digitalRead(Joy_switch));
  Serial.println(notTestEnd);
  while (!digitalRead(Joy_switch))
    {//hang out in this loop until the Joy switch is deactivated after holding it during power on
      Serial.println("release the Joy Switch"); 
    }
  while (notTestEnd) 
    {//if the tests have not run 1 time stay in this loop
  
        //digitalWrite (medLed,HIGH); // turn on middle led as indication of test mode
      switch (testNo)
          {// run test based on current test #
            case(1):
              Serial.println("Start test 1");
              test_Leds();                //run test on Led's
              testNo = 2;
            break;
            case(2):
              Serial.println("Start test 2");
             test_for_UpSw();
              testNo = 3;
            break;
            case(3):
              Serial.println("Start test 3");
              test_for_DwnSw();
              testNo = 1;
              notTestEnd = false; // we have completed all tests
            break;
          }
    }
  digitalWrite(medLed,LOW);   // turn off the test mode led
  digitalWrite(fastLed,LOW); // 
  step_speed = 3000;          // set slow speed
  digitalWrite(slowLed,HIGH); // set slow speed indicator
  testMode=false;
  Serial.println ("Test Mode Ended");
  return;
}

///////Testing mode functions///////
//Test the leds
void test_Leds()
{
 bool num=1;
 //blink the top led
  for(int i=1; i<=10;i++)
    {
      digitalWrite (fastLed, num = num^1); //toggle num with XOR
      delay(500);
    }
  // blink the middle led
  for(int i=1; i<=10;i++)
    {
      digitalWrite (medLed, num = num^1);
      delay(500);
    }
  //blink the bottom led
  for(int i=1; i<=10;i++)
    {
      digitalWrite (slowLed, num = num^1);
      delay(500); 
    }
  return;
}

//// Test the up limit switch
void test_for_UpSw()
{ 
  digitalWrite(fastLed,HIGH);
  while(digitalRead(Limit01))
      {
        Serial.println("activate upper limit switch");
      }
  while(!digitalRead(Limit01)) 
      {
      //blink the fast led while holding switch
        digitalWrite (fastLed, num = num^1);
        delay(500);
      }
  digitalWrite(fastLed,LOW);
  return;
}

void test_for_DwnSw()
{
  digitalWrite(slowLed,HIGH);
  while(digitalRead(Limit02))
    {
  Serial.println("acitvate lower limit switch");
    }
  while(!digitalRead(Limit02)) 
    {
    //blink the slow led while holding switch
      digitalWrite(slowLed, num = num^1);
      delay(500);
    }
  digitalWrite(slowLed,LOW);
  return;
}
