//Needed Libraries
//Core library for code-sense
#if defined(ENERGIA) // LaunchPad MSP430, Stellaris and Tiva, Experimeter Board FR5739 specific
#include "Energia.h"
#else // error
#error Platform not defined
#endif
//Include application, user and local libraries
#include <SPI.h>

#include <LCD_screen.h>
#include <LCD_screen_font.h>
#include <LCD_utilities.h>
#include <Screen_HX8353E.h>
#include <Terminal12e.h>
#include <Terminal6e.h>
#include <Terminal8e.h>
Screen_HX8353E myScreen;

const int xpin = 23;                  // x-axis of the accelerometer
const int ypin = 24;                  // y-axis
const int zpin = 25;                  // z-axis (only on 3-axis models)
int xpin_current = 0;  //holds the current value of the x-pin
int ypin_current = 0;  //holds the current value of the y-pin
int zpin_current = 0;  //holds the current value of the z-pin
int xpin_previous = 0; //holds the previous value of the x-pin
int ypin_previous = 0; //holds the previous value of the y-pin
int zpin_previous = 0; //holds the previous value of the z-pin
int xpin_value_eqn = 0; //holds the result of the y-value from the equation 

//used to keep track of the time
int second = 0;
int minute = 0;
int hour = 0; 

int next_freq = 0; //increments the frequency values for the buzzer

long previousMillis = 0;  //used to keep track of the elaspsed two to increment the seconds
long previousMillis_Two = 0; //used to keep track of the elapsed time to clear the screen
long previousMillis_500 = 0; //used to keep track of the elapsed time 
long readingDelay = 100;
long lastDebounceTime1 = 0;   //the last time the output pin was toggled
long lastDebounceTime2 = 0;   //the last time the output pin was toggled
long Delay = 5000;  //interval at which to blink (milliseconds) 
int next_freq1 = 0;   //used for the tone function
int next_freq2 = 0;   //used for the tone function
int clock_flag = 0;   //used to stop the time from resetting during the first loop 
bool accel_change = false;  //used to figure out if there was a change in the accelerometer values
int ypin_change = false; //monitors if the ypin_change has occured
int debounce_flag1 = LOW; //used to prevent debouncing in the state_S1 interrupt
int debounce_flag2 = LOW; //used to prevent debouncing in the state_S2 interrupt
int ledState = LOW;   //used to blink the red LED


uint16_t x, y, x00, y00;
String xpin_str;  //the string representation of the x-pin
String ypin_str;  //the string representation of the y-pin
String zpin_str;  //the string representation of the z-pin
String sec;
String min_t;
String h_t;
int buzzerPin = 40; //used to set up the buzzer pin
int display_flag = false;  //flag that is used to clear and display the screen
int reading_flag = false;

//variables used for the push buttons and red LED
const int redLED = 39; //the pin number of the red LED
const int S1 = 33; //the pin number for the S1 push button
const int S2 = 32; //the pin number for the S2 push button
volatile byte state_S1 = LOW;  //used for the interrupt pin
volatile byte state_S2 = LOW;  //used for the interrupt pin 

void setup()
{
  analogReadResolution(12);
  //initialize the serial communications:
  Serial.begin(9600);
  myScreen.begin();
  x00 = 0;
  y00 = 0;
  //initializes the led
  pinMode(redLED, OUTPUT);
  //initializes the buzzer
  pinMode(buzzerPin, OUTPUT);
  //initialize the push buttons
  pinMode(S1, INPUT_PULLUP);
  pinMode(S2, INPUT_PULLUP);
  //if the push buttons are pressed the corresponding functions are executed
  attachInterrupt(S1, change_S1, FALLING);
  attachInterrupt(S2, change_S2, FALLING);
}

void loop()
{
  // put your main code here, to run repeatedly:
  //reading the sensor values
  xpin_current = ((int) analogRead(xpin)) - 2048;
  ypin_current = ((int) analogRead(ypin)) - 2048;
  zpin_current = ((int) analogRead(zpin)) - 2048;
  //code executes if the interrupt pin is set to HIGH and the debounce flag is set to high, the last debouncetime is set to the elapsed time
  if((state_S1 == HIGH) && (debounce_flag1 == LOW))
  {
     lastDebounceTime1 = millis();  //sets the lastDebounceTime to the elapsed time
     debounce_flag1 = HIGH;   //a flag that prevents the lastDebounceTime from resetting every time 
  }
  //code executes if the interrupt pin is set to HIGH and the difference between the current time and the lastDebounceTime is greater than 250 milliseconds
  if((state_S1 == HIGH) && ((millis() - lastDebounceTime1)> 250))  //code executed when the S1 interrupt pin is pressed
  {
    debounce_flag1 = LOW;  //flag is used to prevent the reseting of the lastDebounceTime1 variable prematurely 
    tone(buzzerPin, 100 + 10*next_freq1, 200);  //turn on the tone function
    hour++;   //increment the hours counter, displayed on the LCD display
    state_S1 = LOW;  //toggles the interrupt S1 variable
    next_freq1++;  //increments the counter used in the tone function
  }
  
  if((state_S2 == HIGH) && (debounce_flag2 == LOW))
  {
     lastDebounceTime2 = millis();  //sets the lastDebounceTime to the elapsed time
     debounce_flag2 = HIGH;   //a flag that prevents the lastDebounceTime from resetting every time
  }
  //code executes if the interrupt pin is set to HIGH and the difference between the current time and the lastDebounceTime is greater than 250 milliseconds
  if((state_S2 == HIGH)&& ((millis() - lastDebounceTime2) > 250))  //code executed when the S2 interrupt pin is pressed
  {
    debounce_flag2 = LOW;  //flag is used to prevent the reseting of the lastDebounceTime1 variable prematurely
    tone(buzzerPin, 100 + 10*next_freq2, 200);  //turn on the tone function
     minute++;  //increment the minutes counter, displayed on the LCD display
     state_S2 = LOW;  //toggles the interrupt S1 variable  
     next_freq2++;  //increments the counter used in the tone function 
  }
  
  if(hour == 24)  //if the hours counter is equal to 24, it should roll over to 00
  {
      hour = 0;
  }
  if(minute == 60)  //if the hours counter is equal to 60, it should roll over to 00
  {
     minute = 0;
  }
  
  accel_change = false; //sets the flag to false for better execution of the program
  if(((abs(ypin_current - ypin_previous) <= 20) || (abs(xpin_current - xpin_previous)<= 20) || (abs(zpin_current - zpin_previous) <= 20)) && !(ypin_change) && !(state_S1 || state_S2))
  { //sets a flag only when there is a change in the accelerometer values including the error range
         accel_change = true;
  }
   
  if(((millis() - previousMillis) > readingDelay))
  {//update the previous accelerometer values
     xpin_previous = xpin_current;
     ypin_previous = ypin_current;
     zpin_previous = zpin_current;
  }
  
  if((millis() - previousMillis) >= 1000 && !(state_S1 || state_S2))
  {  //updates the military clock counter values
      second++;
      if(second == 60)
      {
        second = 00;
        minute++; 
        if(minute == 60)
        {
           minute = 00;
           hour++;
          if(hour == 24)
          {
            hour = 0;
          } 
        }
      } 
      previousMillis = millis();  //update the value saved in previous millis
  }
  //used to blink the red LEDs (toggle them on and off) every 1 second
  if(millis() - previousMillis_500 > 500){
      previousMillis_500 = millis();
      if(ledState == LOW)
      {
         ledState = HIGH;
      }
      else
      {
         ledState = LOW; 
      }
      digitalWrite(redLED, ledState);
  }
  
  if(((millis() - previousMillis_Two) >= 5000) && (accel_change == true))//((abs(ypin_current - ypin_previous) <= 20) || (abs(xpin_current - xpin_previous)<= 20)||(abs(zpin_current - zpin_previous) <= 20)))
  {
     //clear the LCD screen
     //myScreen.clear(blackColour);
     //myScreen.invert(true);
     myScreen.clear();
     display_flag = true;   //toggles the flag
     reading_flag = false;
     accel_change = false;  
  }

  if((accel_change == false )&& (clock_flag > 0))
  {//resets the old time value 
    previousMillis_Two  = millis();
  }
  clock_flag++; //arbitary variable used for the clock 
  //calculates if there is a change in the y value   //if(abs(ypin_current - ypin_previous) >= 30 && (display_flag == true))
  xpin_value_eqn = (0.11* xpin_current)  + 0.23;  //equation used to find the angle of the board with respect to the board placed on the table/at origin
  //Serial.println(xpin_value_eqn);  //used for debugging
  if(((xpin_value_eqn >= 30) || (xpin_value_eqn <= -30)) && (display_flag == true))
  {//display the screen again 
     display_flag = false;  //enables the screen to display
     ypin_change = true;  //sets the flag that denotes there was a change in the y-values
     reading_flag = true;  //sets the flag denoting there is a change in the placement of the board
  }
  else
  {
     ypin_change = false; 
  }
   if((minute == 30) && (second == 0) && (hour == 7))  //generate a tone when the digital clock is equal to 7:30:00
   {
     tone(buzzerPin, 100 + 90*next_freq, 200);  //turn on the tone function 
     next_freq++;
   }
   
    myScreen.setFontSize(0);
    if(hour <= 9) //adds an extra digit when then the number is less than 9, to fit the military format 
    {
      h_t = "0" + String(hour);
    }
    else
    {
      h_t = String(hour);
    }
    if(minute <= 9)  //adds an extra digit when then the number is less than 9, to fit the military format
    {
      min_t = "0" + String(minute);
    }
    else
    {
      min_t = String(minute);
    }
    
    if(second <= 9)  //adds an extra digit when then the number is less than 9, to fit the military format
    {
      sec = "0" + String(second);
    }
    else
    {
      sec = String(second);
    }
 
    if(display_flag == false)
    {  //displays the updated values on the LCD screen
        myScreen.gText(30,64, h_t + " :" + min_t + " : " + sec);// " Y:" + ypin_str);
        myScreen.setFontSize(1);
        //myScreen.gText(30, 76," CC. Anuri"); 
    }
}
void change_S1(){ //toggles the interrupt variable
 state_S1 = !state_S1; 
}
void change_S2(){//toggles the interrupt variable
  state_S2 = !state_S2;
}
