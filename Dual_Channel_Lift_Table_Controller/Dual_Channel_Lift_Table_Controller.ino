// This code base originally came from:
//https://www.brainy-bits.com/stepper-motor-with-joystick-and-limit-switches/

// Description of two channel lift table controller
// With this code and the corresponding hardware two controllers can control one lift table stepper driver.
// this allows the table to be controlled without having the smoothie connected to a PC. 
// when the joystick is activated the mux switches to Arduino control
// when the joystic is not activated the stepper controls are from the smoothie. 
//V1.0  
  //tested virgin two channel control system system
  // This code modified from the arduino only code [Joystick_Stepper_Control_2.0]
  // Channel 1= Arduino
  // Channel 2 = smoothie
  // Stepper driver = TB6600
  // 74157 mux http://www.ti.com/lit/ds/symlink/sn54157.pdf
  // changed the stepper controls to grnd the - signals to match: https://cohesion3d.freshdesk.com/support/solutions/articles/5000744633-wiring-a-z-table-and-rotary-step-by-step-instructions

//DEBUG 
#define DEBUG       //V1.2 Comment this out for normal compile

// CONTROL PANEL I/O
#define slowLed 5   // Pin 5 connected to slow LED on panel
#define medLed 6  // Pin 6 connected to med LED on panel
#define fastLed 19   // Pin 19 [A5] connected to fasd LED on panel [pin 7 was dead on the board I had]
#define X_pin A0    // Pin A0 connected to joystick x axis
#define Y_pin A1    // Pin A1 connected to joystick y axis (up down)
#define Joy_switch 4  // Pin 4 connected to joystick switch

// Stepper Driver I/O
// The + stepper controls are connected to 5v. This supplies the internal optocouplers with 5V.
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

int step_speed = 3000;  // Speed of Stepper motor (higher = slower)
int num;               // temp variable for blinking LED
int stickPosition;    // the Y joystick value

bool testMode = false; // test mode 

void setup() 
{
 #ifdef DEBUG
  Serial.begin(19200);   //V1.0
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
   
   digitalWrite(stepper_en, LOW);  // enable the stepper driver
   //digitalWrite(chan_select, HIGH); //select the smoothie channel

   digitalWrite(chan_select, LOW);  //enable the arduino's stepper signals
   delay(5);  // Wait for Driver to wake up
   digitalWrite(slowLed,HIGH);
   digitalWrite(medLed, LOW);
   digitalWrite(fastLed,LOW);
   
/* Configure settings on TB660 driver
// SW1=ON
// SW2=ON
// SW3=OFF
// SW4=OFF
// SW5=ON
// SW6=OFF

*/

 
 Serial.println("K40 Dual Channel Lift Controller V1.0");
  
  if (!digitalRead(Joy_switch)) 
  {
    testMode=true;            // if switch held during reset set test mode
    Serial.println("Test mode initiated");
  }
  
}

void loop() 
{
  digitalWrite(test_pin,HIGH);
  
  if(testMode)
  {
    test_Mode();        // go and run tests first
  }
  //Serial.println(step_speed);
  digitalWrite(step_pin,HIGH);
  if (!digitalRead(Joy_switch)) 
    {  //  If Joystick switch is clicked
      delay(500);  // delay for deboucing
    switch (step_speed) 
    {  // check current value of step_speed and change it
      case 300:
        step_speed=3000;  // If fast speed go to slow speed
        #ifdef DEBUG //V1.2
          Serial.println("-------Slow Speed-------");
        #endif
        digitalWrite(medLed, LOW);
        digitalWrite(fastLed,LOW);
        digitalWrite(slowLed, HIGH);
      break;
      case 800:
        step_speed=300;  // if med speed go to fast speed
        #ifdef DEBUG
          Serial.println("-------Fast Speed-------");
        #endif
        digitalWrite(slowLed, LOW);
        digitalWrite(medLed, LOW);
        digitalWrite(fastLed,HIGH);
      break;
      case 3000:
        step_speed=800;  // if slow speed go to medium speed
        #ifdef DEBUG
          Serial.println("-------Medium Speed------");
        #endif
        digitalWrite(slowLed, LOW);
        digitalWrite(fastLed,LOW);
        digitalWrite(medLed, HIGH);
      break;
    }
  }    
  stickPosition = analogRead(Y_pin);  
  if (analogRead(Y_pin) > 712) 
  {  //  If joystick is moved UP
    if (!digitalRead(Limit01)) 
      {  // check if up limit switch is activated
      Serial.println("Lift at top");
      for (int i = 0; i <= 10; i++)
        {//blink the fast led as a warning
         digitalWrite (fastLed, num = num ^= 1);
         delay(500);
        }
      restore_LED();     // put the led back to it previous state  
    }
    else 
      {  //  if limit switch is not activated, move motor clockwise  
        //digitalWrite(test_pin,HIGH);
        //digitalWrite(chan_select, LOW);// Enable the Arduino channel
        digitalWrite(dir_pin, LOW);  // (HIGH = anti-clockwise / LOW = clockwise)
        digitalWrite(step_pin, LOW);
        delayMicroseconds(step_speed);
        digitalWrite(step_pin, HIGH);
        delayMicroseconds(step_speed);
        //digitalWrite(chan_select, HIGH); //reset the channel to smoothie
       // digitalWrite(test_pin, LOW);
      }      
   
  }
  
    if (analogRead(Y_pin) < 312) 
    {  // If joystick is moved DOWN
      if (!digitalRead(Limit02)) 
        {// check if lower limit switch is activated
          Serial.println("Lift at bottom");  
          for (int i = 0; i <= 10; i++)
            {//blink the slow led as a warning
              digitalWrite (slowLed, num = num^=1);
              delay(500);
            } 
          restore_LED();     // put the led back to it previous state  
        }  
      else 
      {  //  if limit switch is not activated, move motor counter clockwise     
/*
        Serial.print("Moving Down @");
        Serial.println(stickPosition);
*/     
        digitalWrite(dir_pin, HIGH);  // (HIGH = anti-clockwise / LOW = clockwise)
        digitalWrite(step_pin, LOW);
        delayMicroseconds(step_speed);
        digitalWrite(step_pin, HIGH);
        delayMicroseconds(step_speed);
      }      
    }
digitalWrite(test_pin, LOW);
}//end Loop

// .....................
// Used to put the panel LED's back to their previous state ...
// ... after blinking them on achieveing the top or bottom
// ... with a joystick move in the direction of the closed endstop 
void restore_LED()
{
  Serial.println (step_speed); 
  switch (step_speed) 
      {  // check current value of step_speed update the LED's
        case 300:
         digitalWrite(slowLed,LOW);
         digitalWrite(medLed,LOW);
         digitalWrite(fastLed,HIGH);
        break;
        case 800:
         digitalWrite(slowLed,LOW);
         digitalWrite(medLed,HIGH);
         digitalWrite(fastLed,LOW);
        break;
        case 3000:
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
      digitalWrite (fastLed, num = num^=1); //toggle num with XOR
      delay(500);
    }
  // blink the middle led
  for(int i=1; i<=10;i++)
    {
      digitalWrite (medLed, num = num^=1);
      delay(500);
    }
  //blink the bottom led
  for(int i=1; i<=10;i++)
    {
      digitalWrite (slowLed, num = num^=1);
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
        digitalWrite (fastLed, num = num^=1);
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
      digitalWrite(slowLed, num = num^=1);
      delay(500);
    }
  digitalWrite(slowLed,LOW);
  return;
}
