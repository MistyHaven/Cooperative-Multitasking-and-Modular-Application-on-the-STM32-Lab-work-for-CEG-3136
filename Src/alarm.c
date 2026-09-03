// Alarm system app

#include <stdio.h>

#include "alarm.h"

#include "systick.h"

#include "gpio.h"

#include "display.h"





// GPIO pins

static const Pin_t RedLED = {GPIOA, 9}; // Pin PA9 -> User LD3

static const Pin_t BlueLED = {GPIOB, 7}; // Pin PB7 -> User LD2

static const Pin_t GreenLED = {GPIOC, 7}; // Pin PC7 -> User LD1

static const Pin_t Buzzer = {GPIOA, 0}; // Pin PA0 -> Buzzer





static const Pin_t EStop = {GPIOB, 2}; //Pin PB2 <- E-Stop

static const Pin_t MotionSensor = {GPIOB, 9}; // Pin PB9 <- User Motion Sensor



static enum {DISARMED, ARMED, TRIGGERED} state;



// Constants

#define LONG_TIME_PRESSED 3000

#define LED_ON_TIME 1000

#define DEBONCE_TIME 50




// Variables

static Time_t lastToggle; // for blinking every 1s without blocking.

static Time_t pressTime; // to detect short or long press



static int motionFlag;



static int shortPress;







// Declare interrupt callback functions

static void CallbackMotionDetect();

static void CallbackButtonPress();

static void CallbackButtonRelease();





// Initialization code

void Init_Alarm (void) {



    GPIO_Callback(MotionSensor, CallbackMotionDetect, RISE);

    GPIO_Callback(EStop, CallbackButtonPress, RISE);

    GPIO_Callback(EStop, CallbackButtonRelease, FALL);





    GPIO_Enable(RedLED);

    GPIO_Enable(BlueLED);

    GPIO_Enable(GreenLED);

    GPIO_Enable(Buzzer);



    GPIO_Output(RedLED, LOW);

    GPIO_Output(BlueLED, LOW);

    GPIO_Output(GreenLED, LOW);

    GPIO_Output(Buzzer, LOW);



    GPIO_Mode (RedLED, OUTPUT);

    GPIO_Mode (BlueLED, OUTPUT);

    GPIO_Mode (GreenLED, OUTPUT);

    GPIO_Mode (Buzzer, OUTPUT);





    GPIO_Enable(EStop);

    GPIO_Mode(EStop, INPUT);



    GPIO_Enable(MotionSensor);

    GPIO_Mode(MotionSensor, INPUT);



    state = DISARMED;

    motionFlag =0;

    shortPress =0;


    DisplayEnable();
    DisplayColor(ALARM, WHITE);
    DisplayPrint(ALARM, 0, "DISARMED");


}





// Task code (state machine)

void Task_Alarm (void) {

 switch (state) {



    case DISARMED:



   GPIO_Output(RedLED, LOW);

   GPIO_Output(BlueLED, LOW);

   GPIO_Output(GreenLED, LOW);

   GPIO_Output(Buzzer, LOW);

   DisplayEnable();
   DisplayColor(ALARM, WHITE);
   DisplayPrint(ALARM, 0, "DISARMED");



  if (shortPress) {

         shortPress=0;

         state = ARMED;

         printf("Armed at time %u", TimeNow());

        GPIO_Output(RedLED, LOW);

        GPIO_Output(Buzzer, LOW);


       GPIO_Output(BlueLED, HIGH);

       GPIO_Output(GreenLED, LOW);

       lastToggle = TimeNow();

   }



    motionFlag = 0;

    break;





    case ARMED:
    	DisplayEnable();
        DisplayColor(ALARM, YELLOW);
        DisplayPrint(ALARM, 0, "ARMED");

    if(GPIO_Input(EStop) == HIGH && TimePassed(pressTime) >= LONG_TIME_PRESSED){



         state = DISARMED;

         printf("Disarmed at time %u", TimeNow());

         GPIO_Output(RedLED, LOW);

         GPIO_Output(BlueLED, LOW);

         GPIO_Output(GreenLED, LOW);

         GPIO_Output(Buzzer, LOW);

     }

     else if(motionFlag){

         motionFlag = 0;



         state= TRIGGERED;

         printf("Alarm Triggered at time %u", TimeNow());

         GPIO_Output(BlueLED, LOW);

        GPIO_Output(GreenLED, LOW);
     }

     else if(TimePassed(lastToggle) >= LED_ON_TIME){



       GPIO_Toggle(BlueLED);

       GPIO_Toggle(GreenLED);

       lastToggle = TimeNow();

    }



    shortPress =0;

    break;





    case TRIGGERED:

    GPIO_Output(RedLED, HIGH);

    GPIO_Output(BlueLED, LOW);

    GPIO_Output(GreenLED, LOW);

    DisplayEnable();
    DisplayColor(ALARM, RED);
    DisplayPrint(ALARM, 0, "TRIGGERED");


    if(shortPress){  // OR USE SH

           shortPress = 0;



          state = ARMED;

         printf("Armed at time %u", TimeNow());

         GPIO_Output(RedLED, LOW);

         GPIO_Output(Buzzer, LOW);


          GPIO_Output(BlueLED, HIGH);

          GPIO_Output(GreenLED, LOW);

          lastToggle = TimeNow();

     }

     else if(GPIO_Input(EStop) == HIGH && TimePassed(pressTime) >= LONG_TIME_PRESSED){



          state = DISARMED;

          printf("Disarmed at time %u", TimeNow());

          GPIO_Output(RedLED, LOW);

          GPIO_Output(BlueLED, LOW);

          GPIO_Output(GreenLED, LOW);

          GPIO_Output(Buzzer, LOW);


     }





     motionFlag = 0;

     break;

  }


}



void CallbackMotionDetect (void) {
       motionFlag = 1;
}

void CallbackButtonPress (void) {
    pressTime = TimeNow(); // remember when it starts

}

void CallbackButtonRelease (void) {
    if (TimePassed(pressTime) > 50 && TimePassed(pressTime) < LONG_TIME_PRESSED ) {
         shortPress = 1;
    }

}
