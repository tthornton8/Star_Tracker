#include <math.h>      
#include <Arduino.h>
#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD
#include "MountStepper.h"
#include <string.h>

const byte interruptPin = 2;
const byte dirPinRA = 4;
const byte stepPinRA = 3;
const byte dirPinDec = 8;
const byte stepPinDec = 7;
const int gearRatioRA = 24;
const int gearRatioDec = 4;
const int analogPin = A0;
const signed long baseControlSpeed = 512*1e3;
const signed long automaticSpeed = 1121927; // Steps in a Sideral Day
const char *modeText[6] = {"Auto", "RA", "DEC", "GoTo", "Zero", "Move"};

volatile signed long inputVal = 0;
volatile byte mode = 2;
volatile signed long controlSpeed = 1000*1e3;
volatile byte modeFlag = 0;
volatile float degrees = 0;
volatile signed long lastStep = 0;
volatile signed long lastScreenUpdate = 0;
volatile signed long currTimeStep = 0;
volatile signed long currTimeScreen = 0;
volatile byte halfStep = 1;
volatile signed long timeDiff = 0;
volatile byte charSelected = 0;
volatile int targetPosArrRA[5] = {0, 0, 0, 0};
volatile int targetPosArrDec[5] = {0, 0, 0, 0};
volatile int menuPosArr[5] = {3, 11, 3, 9, 3};
volatile byte inMenu = 1;
volatile byte stepperSelected = 0;

volatile float targetPosRA;
volatile float targetPosDec;
volatile char toPrint;
volatile int inputChar;
volatile int cursorPos;
volatile byte interruptFlag;
String EqDeg;
String DecDeg;

// Wiring: SDA pin is connected to A4 and SCL pin to A5.
// Connect to LCD via I2C, default address 0x27 (A0-A2 not jumpered)
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2); //
MountStepper stepperEq = MountStepper(stepPinRA, dirPinRA, gearRatioRA, automaticSpeed, 1e3, 360);
MountStepper stepperDec = MountStepper(stepPinDec, dirPinDec, gearRatioDec, 0, 6e3, 180);

void printMenu() {
  inputVal = floor(analogRead(analogPin)/(1024/5));
  Serial.println(inputVal);
  Serial.println(analogRead(analogPin));
  
  lcd.clear();

  if (inputVal < 4) {
      lcd.setCursor(3, 0);
      lcd.print("Auto    RA");
      lcd.setCursor(3, 1);
      lcd.print("DEC   GoTo");
  } else {
      lcd.setCursor(3, 0);
      lcd.print("DEC   GoTo");
      lcd.setCursor(3, 1);
      lcd.print("Zero");
  }

}

void printStepperPos() {
  lcd.setCursor(0, 0);
          lcd.print("RA  ");
          EqDeg = String(stepperEq.degrees, 1);
          while (EqDeg.length() < 5) {
            EqDeg = "0" + EqDeg;
          }
          lcd.print(EqDeg);
          lcd.write(B01111110);
          
          for (int i = 0; i < 6; i++) {
            if (i < 3) {
              toPrint = targetPosArrRA[i]+48;
            } else if (i == 3) {
              toPrint = 46;
            } else {
              toPrint = targetPosArrRA[i-1]+48;
            }
  
            lcd.print(toPrint);
          }

          lcd.setCursor(0, 1);
          lcd.print("DEC ");
          DecDeg = String(stepperDec.degrees, 1);
          while (DecDeg.length() < 5) {
            DecDeg = "0" + DecDeg;
          }
          lcd.print(DecDeg);
          lcd.write(B01111110);
          
          for (int i = 0; i < 6; i++) {
            if (i < 3) {
              toPrint = targetPosArrDec[i]+48;
            } else if (i == 3) {
              toPrint = 46;
            } else {
              toPrint = targetPosArrDec[i-1]+48;
            }
  
            lcd.print(toPrint);
          }
}

void updateScreen() {
  currTimeScreen = millis();
  
  if ((currTimeScreen - lastScreenUpdate) > 1000) {
    lcd.clear();

    if (inMenu) {
      stepperEq.stop();
      stepperDec.stop();
      mode = 7;
      printMenu();
      inputVal = floor(analogRead(analogPin)/(1024/5));
      lcd.setCursor(menuPosArr[inputVal], inputVal > 1 ? 1 : 0);
      lcd.cursor();
    } else {

      if (mode != 3) {
        lcd.setCursor(2, 0);
        lcd.print(modeText[mode]);
    
    
        size_t lenText = sizeof(modeText[mode]) / sizeof(modeText[mode][0]);
        lcd.setCursor(lenText+7, 0);
      }
      lcd.noCursor();
  
      switch (mode) {
        case 4:
          stepperEq.setPos(0);
          stepperDec.setPos(0);

          lcd.clear();
          
          lcd.print("RA  ");
          EqDeg = String(stepperEq.degrees, 1);
          while (EqDeg.length() < 5) {
            EqDeg = "0" + EqDeg;
          }
          lcd.print(EqDeg);
          lcd.print((char)223);

          lcd.setCursor(0, 1);
          lcd.print("DEC ");
          DecDeg = String(stepperDec.degrees, 1);
          while (DecDeg.length() < 5) {
            DecDeg = "0" + DecDeg;
          }
          lcd.print(DecDeg);
          lcd.print((char)223);
          
          break;
        case 5:
          printStepperPos();
          break;
        case 3:
          printStepperPos();
          cursorPos = charSelected > 2 ? charSelected + 11 : charSelected + 10;
          lcd.setCursor(cursorPos, stepperSelected);
          lcd.cursor();
          break;
        case 2:
          lcd.print(stepperDec.degrees);
          lcd.print((char)223);
          lcd.setCursor(5, 1);
          lcd.print(stepperDec.speed);
          break;
        case 1:
          lcd.print(stepperEq.degrees);
          lcd.print((char)223);
          lcd.setCursor(5, 1);
          lcd.print(stepperEq.speed);
          break;
        case 0:
          lcd.print(stepperEq.degrees);
          lcd.print((char)223);
          lcd.setCursor(5, 1);
          lcd.print(stepperEq.speed);
          break;
      }
    }

    lastScreenUpdate = currTimeScreen;
  }
}

float clip( float n, float lower, float upper )
{
    n = ( n > lower ) * n + !( n > lower ) * lower;
    return ( n < upper ) * n + !( n < upper ) * upper;
}

void changeMode(){
  // Interrupt handler for mode switch button
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) 
  { 
    interruptFlag = 1;
  }
  last_interrupt_time = interrupt_time;
}

void setup()
{
  // Declare pins
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), changeMode, CHANGE);
  pinMode(LED_BUILTIN, OUTPUT);

  // Initiate the LCD:
  lcd.init();
  lcd.backlight();
  lcd.setCursor(2, 0);
  lcd.print(modeText[mode]);

  stepperDec.setSpeed(1e3);
  stepperDec.setDir(LOW);

  Serial.begin(9600);
}

void loop(){   
  if (interruptFlag) {
    if (inMenu) {
      mode = inputVal;
      modeFlag = 1;
      inMenu = 0;
    } else if (mode == 3) {
      if (charSelected == 4) {
        if (stepperSelected == 0) {
          stepperSelected = 1;
        } else {
          stepperSelected = 0;
          mode = 5;
          modeFlag = 1;
          stepperEq.GoTo(targetPosRA);
          stepperDec.GoTo(targetPosDec);
        }
        charSelected = 0;
        
      } else {
        charSelected = charSelected + 1;
      }
    } else {
      inMenu = 1;
    }

    interruptFlag = 0;
  }
  // Print LCD text
  updateScreen();

  switch (mode) {
    case 3:
      // GoTo Set
      inputVal = analogRead(analogPin)/102.4;
      inputChar = floor(inputVal);
      
      switch (charSelected) {
        case 0:
          inputChar = clip(inputChar, 0, 3);
          break;
        case 1:
          if (targetPosRA > 300) {
            inputChar = clip(inputChar, 0, 6);
          }
          break;
        case 2:
          if (targetPosRA > 360) {
            inputChar = clip(inputChar, 0, 5);
          }
          break;
        default:
          if (targetPosRA > 365) {
            inputChar = 0;
          }
          break;
        
      }

      if (stepperSelected == 0) {
        targetPosArrRA[charSelected] = inputChar;
        targetPosRA = 0;
        for (int i = 0; i < 6; i++) {
          targetPosRA = targetPosRA + targetPosArrRA[i]*pow(10, 2-i);
        }
      } else {
        targetPosArrDec[charSelected] = inputChar;
        targetPosDec = 0;
        for (int i = 0; i < 6; i++) {
          targetPosDec = targetPosDec + targetPosArrDec[i]*pow(10, 2-i);
        }
      }
      
      
      break;
    case 2:
      // read potentiometer value
      inputVal = analogRead(analogPin)-512;
      
      // Set motor speed
      controlSpeed = (abs(inputVal) > 0.5) ? abs(baseControlSpeed/inputVal) : baseControlSpeed;
  
      // Set direction
      stepperDec.setSpeed(controlSpeed*6);
      if (inputVal < 0) {
        stepperDec.setDir(HIGH);
      } else { 
        stepperDec.setDir(LOW);
      }
      break;
    case 1:
      // Manual Control

      // read potentiometer value
      inputVal = analogRead(analogPin)-512;
      
      // Set motor speed
      controlSpeed = (abs(inputVal) > 0.5) ? abs(baseControlSpeed/inputVal) : baseControlSpeed;
  
      // Set direction
      stepperEq.setSpeed(controlSpeed);
      if (inputVal < 0) {
        stepperEq.setDir(HIGH);
      } else { 
        stepperEq.setDir(LOW);
      }
      break;
    case 0:
      // Automatic Control
      stepperEq.setSpeed(automaticSpeed);
      stepperEq.setDir(HIGH);
      break;
  }

  stepperEq.run();
  stepperDec.run();
}