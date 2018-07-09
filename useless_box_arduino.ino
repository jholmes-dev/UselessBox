#include <Servo.h>

// Define some constants
const int dirPin = 12;
const int stepPin = 11;
const int sleepPin = 8;
const int homePin = 2;
const int ms1Pin = 5;
const int ms2Pin = 6;
const int ms3Pin = 7;

// Servo Stuff
Servo armServo;
const int armServoPin = 9;
const int restPos = 100;
const int readyPos = 30;
const int hitPos = 1;

Servo doorServo;
const int doorServoPin = 10;
const int dOpenPoint = 160;
const int dClosePoint = 90;

// Stepping constants
const int maxSteps = 360; // Max steps from home

// Some variables to track distance
float stepCount = 0;
bool hitHome = false;
int platformDir = 0;
float stepAmount = 1.0;

// Some switch tracking things
const int switchNums[3] = {0, 1, 3};
int switchPos[3] = {150, 240, 320};

// Linked list for switch tracking
struct swTracker {
  int swNum;
  volatile struct swTracker *next;
};

typedef struct swTracker *node;
volatile node swListHead = NULL;

// Some State checks
bool isReady = true;
bool isOpen = true;
bool isSleep = false;
volatile byte curState[3] = {0, 0, 0};


// Pin interrupt setup
void pciSetup(byte pin) {
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}


// Interrupt function for D0-D7 (switch pins)
ISR (PCINT2_vect) {
  int hPin[3] = {-1, -1, -1};
  byte hIndex = 0;
  
  // Check pins against current state to see if any just went high
  for (byte i = 0; i < 3; i++) {
    byte pState = digitalRead(switchNums[i]);

    // Compare states
    if (pState != curState[i]) {

      // Save only if just went high
      if (pState == HIGH && curState[i] == LOW) {
        hPin[hIndex] = i;
        hIndex++;
      }
      // Update curstate
      curState[i] = pState;
      
    }
    
  } // end for

  // Add the newly switched nodes to the tracker
  for (byte x = 0; x < hIndex; x++) {

    // Break if we hit NULL
    if (hPin[x] == -1) {
      break;
    }

    // Create node for current pin & add to the list
    node tempNode;
    tempNode = (node)malloc(sizeof(struct swTracker));
    tempNode->swNum = hPin[x];
    tempNode->next = NULL;

    if (swListHead == NULL) {
      swListHead = tempNode;
      //armServo.write(readyPos);
      
    } else {
      //armServo.write(hitPos);
      
      node tempHead = swListHead;

      while (tempHead->next != NULL) {
        tempHead = tempHead->next;
      }

      tempHead->next = tempNode;
    }
     
  } // end for
     
} // end ISR


void setup() {

  // Motor Pins
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(sleepPin, OUTPUT);
  pinMode(ms1Pin, OUTPUT);
  pinMode(ms2Pin, OUTPUT);
  pinMode(ms3Pin, OUTPUT);

  // Switch pins
  pinMode(homePin, INPUT);
  pinMode(switchNums[0], INPUT);
  pinMode(switchNums[1], INPUT);
  pinMode(switchNums[2], INPUT);

  // Switch pin interrupts
  pciSetup(switchNums[0]);
  pciSetup(switchNums[1]);
  pciSetup(switchNums[2]);
   
  // Prep pins
  digitalWrite(dirPin, LOW);
  digitalWrite(sleepPin, HIGH);
  digitalWrite(dirPin, LOW);

  // Setup servos
  armServo.attach(armServoPin);
  doorServo.attach(doorServoPin);

  // Set the step amount, and reset platform position
  changeStep(2);
  goHome();

}


void loop() {

   // Monitor the switch list
  while (swListHead != NULL) {

     // Turn off the switch if needed
     if (digitalRead(switchNums[swListHead->swNum]) == HIGH) {
        hitSwitch(swListHead->swNum);
     }

     // Save the old switch head
     node oldSwHead = swListHead;

     // Assign the next switch list head
     swListHead = swListHead->next;

     // Free the old head's memory
     free(oldSwHead);
    
  } // end while


  // Fallback method
  if (swListHead == NULL && (digitalRead(switchNums[0]) == HIGH || digitalRead(switchNums[1]) == HIGH || digitalRead(switchNums[2]) == HIGH)) {

    // Loop through the pins.
    for (int i = 0; i < 3; i++) {
      if (digitalRead(switchNums[i]) == HIGH) {

        // Hit it
        hitSwitch(i);
      
      }
    } // end for
    
  } // end fallback method if

  // Reset on idle
  if (digitalRead(homePin) != HIGH) {
    goHome();
  }
  
} // end loop


// Switches a switch off, returns false on failure
bool hitSwitch(int sn) {

  // Turn on the motor
  digitalWrite(sleepPin, HIGH);

  // Move into position
  int armOffset = switchPos[sn] - stepCount;

  // Change direction
  if (armOffset < 0) {
    changeDir(0);
  } else {
    changeDir(1);
  }

  // Move the arm
  while (stepCount != switchPos[sn]) {
    oneStep();
    delay(1);
  }

  // Open door if currently closed
  if (!isOpen) {
    toggleDoor(true, 100);
  }

  // Ready the arm
  if (!isReady) {
    toggleReady(true, 300);
  }
  
  // Hit the switch
  armServo.write(hitPos);
  delay(200);
  
  // Re-Ready the arm
  toggleReady(true, 200);
  
} // end hitSwitch


// Sends the platform back home
void goHome() {

  // Turn on the motor
  digitalWrite(sleepPin, HIGH);

  // Put the arm down
  if (isReady) {
    toggleReady(false, 300);
  }

  // Put the lid down
  if (isOpen) {
    toggleDoor(false, 200);
  }

  // Set the direction towards home
  changeDir(0);
  
  while (digitalRead(homePin) == LOW) {
    oneStep();
    delay(1);
  }

  // Reset step count
  stepCount = 0;

  // Turn off the motor
  digitalWrite(sleepPin, LOW);
  
} // end goHome


// Handles direction changes (from front)
// LOW == LEFT
// HIGH == RIGHT
void changeDir(int dir) {

  digitalWrite(dirPin, dir);
  platformDir = dir;
  
} // end changeDir


// Pushes the motor one step and records the step number
void oneStep() {

  // Check step max
  if (platformDir == 1 && stepCount >= maxSteps) {
    goHome();
    return;
  } else if (platformDir == 0 && digitalRead(homePin)) {
    return;
  }
  
  digitalWrite(stepPin, HIGH);
  delay(.01);
  digitalWrite(stepPin, LOW);
  
  if (platformDir == 1) {
    stepCount += stepAmount;
  } else {
    stepCount -= stepAmount;
  }
}


// Changes the step amount
void changeStep(int newAmount) {

  switch (newAmount) {
    case 1: // Full
      digitalWrite(ms1Pin, LOW);
      digitalWrite(ms2Pin, LOW);
      digitalWrite(ms3Pin, LOW);
      stepAmount = 1.0;
      break;
    case 2: // Half
      digitalWrite(ms1Pin, HIGH);
      digitalWrite(ms2Pin, LOW);
      digitalWrite(ms3Pin, LOW);
      stepAmount = 0.5;
      break;
    case 3: // Quarter
      digitalWrite(ms1Pin, LOW);
      digitalWrite(ms2Pin, HIGH);
      digitalWrite(ms3Pin, LOW);
      stepAmount = 0.25;
      break;
    case 4: // Eighth
      digitalWrite(ms1Pin, HIGH);
      digitalWrite(ms2Pin, HIGH);
      digitalWrite(ms3Pin, LOW);
      stepAmount = 0.125;
      break;
    case 5: // Sixteenth
      digitalWrite(ms1Pin, HIGH);
      digitalWrite(ms2Pin, HIGH);
      digitalWrite(ms3Pin, HIGH);
      stepAmount = 0.0625;
      break; 
  }

} // end changeStep


// Toggles the arm to and from ready
void toggleReady(bool state, int aniDelay) {

  if (state) {
    armServo.write(readyPos);
    isReady = true;
  } else {
    armServo.write(restPos);
    isReady = false;
  }

  delay(aniDelay);
  
} // end toggleReady


// Toggles door state
void toggleDoor(bool state, int aniDelay) {
  
  if (state) {
    doorServo.write(dOpenPoint);
    isOpen = true;
  } else {
    doorServo.write(dClosePoint);
    isOpen = false;
  }

  delay(aniDelay);
  
} // end toggleDoor

