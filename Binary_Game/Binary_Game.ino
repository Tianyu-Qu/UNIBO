// Pin Definitions for LEDs
#define   L1   13
#define   L2   12
#define   L3   11
#define   L4   10
#define   LS   9  // Red LED

// Pin Definitions for Buttons
#define   B1   2
#define   B2   3
#define   B3   4
#define   B4   5

// Pin Definition for Potentiometer (Analog input)
#define   Pot   A2  // Potentiometer connected to A0


int fadeAmount = 15;
int currIntensity = 0;
// Define states
enum GameState { INIT, GAME, GAME_OVER };
GameState gameState = INIT;

unsigned long startTime = 0;  // For time tracking
unsigned long T1;         // Maximum time to respond
int F;                    // Difficulty factor
int score = 0;
int playerInput = 0;
int displayedNumber = 0;
const unsigned long DEBOUNCE_DELAY = 50; // 消抖延迟时间（毫秒）
unsigned long lastButtonPressTime[4] = {0, 0, 0, 0}; // 记录每个按钮上次按下的时间
unsigned long lastB1PressTime = 0;
volatile bool sleepMode = false;
// LCD (I2C communication uses A4 for SDA and A5 for SCL by default)
#include <Wire.h>
#include <TimerOne.h>
#include <LiquidCrystal_I2C.h>
#include <avr/sleep.h>
#include <PinChangeInterrupt.h>
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27,20,4);  // LCD address 0x27, 20 columns, 4 rows

void Init()  {
  // Initialize LEDs as outputs
  pinMode(L1, OUTPUT);
  pinMode(L2, OUTPUT);
  pinMode(L3, OUTPUT);
  pinMode(L4, OUTPUT);
  pinMode(LS, OUTPUT);
  
  // Initialize buttons as inputs (with internal pull-up resistors enabled)
  pinMode(B1, INPUT_PULLUP);
  pinMode(B2, INPUT_PULLUP);
  pinMode(B3, INPUT_PULLUP);
  pinMode(B4, INPUT_PULLUP);

  // Initialize potentiometer as an analog input
  //pinMode(Pot, INPUT);

  // Start the LCD and print a message
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Welcome to GMB!");
  lcd.setCursor(0, 1);
  lcd.print("Press B1 to Start");


  lastB1PressTime = millis();
}

void setup() {
  Serial.begin(9600);
  Init();  // Initialize hardware
  delay(100); // 添加短暂延迟
  Serial.println("Setup completed"); // 调试信息
}

void loop() {
  switch (gameState) {
    case INIT:
      handleInitialState();
      break;
    case GAME:
      handleGameState();
      break;
    case GAME_OVER:
      handleGameOverState();
      break;
  }
}

void LS_PULSE() {
  analogWrite(LS, currIntensity); 
  currIntensity = currIntensity + fadeAmount;
  if (currIntensity == 0 || currIntensity == 255) {
   fadeAmount = -fadeAmount ; 
  } 
  delay(20);
}

// Handle the initial state
void handleInitialState() {
  lcd.setCursor(0, 0);
  lcd.print("Welcome to GMB!");
  lcd.setCursor(0, 1);
  lcd.print("Press B1 to Start");

  LS_PULSE();  // Red LED pulses while waiting for game start
  // If B1 pressed, start the game
  unsigned long currentTime = millis(); // 获取当前时间
  // 检查 B1 按钮的状态
  if (digitalRead(B1) == LOW) {
    // 如果当前时间与上次按下的时间差大于消抖延迟
    if (currentTime - lastB1PressTime > DEBOUNCE_DELAY) {
      lastB1PressTime = currentTime; // 更新上次按下的时间
      startGame(); // 开始游戏
    }
  }
  if (millis() - lastB1PressTime > 10000) {
    goToSleep();
    delay(1000);
    gameState = INIT;
    lastB1PressTime = millis();
  }
}

void goToSleep(){
  // go to sleep
  Serial.println("Going to sleep");
  attachInterrupt(digitalPinToInterrupt(B1),WakeUp,RISING);
  attachInterrupt(digitalPinToInterrupt(B2),WakeUp,RISING);
  //attachInterrupt(digitalPinToInterrupt(B3),WakeUp,RISING);
  //attachInterrupt(digitalPinToInterrupt(B4),WakeUp,RISING);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(B3), WakeUp, RISING);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(B4), WakeUp, RISING);
  digitalWrite(LS, LOW);
  lcd.clear();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); 
  sleep_enable();
  sleep_mode();

  //sleeping

  //wake up
  Serial.println("WAKE UP");
  sleep_disable(); 

}
void WakeUp(){

}

// Start the game
void startGame() {
  gameState = GAME;
  startTime = millis();
  score = 0;
  T1 = 10000;  // Starting max time (10 seconds)
  setDifficulty();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Go!");
  lcd.setCursor(0, 1);
  lcd.print("Score: " + String(score));
  delay(1000);
  newRound();
}

// Set difficulty based on Potentiometer
void setDifficulty() {
  int potValue = analogRead(Pot);
  Serial.println(potValue);
  int level = map(potValue, 0, 1023, 1, 4);
  F = level;  // Example difficulty factor
  Serial.println(F);
}

// Handle the game logic
void handleGameState() {
  if ((millis() - startTime > T1)&&checkCorrectNumber()) {
    score++;
    lcd.clear();
    lcd.print("GOOD! Score: ");
    lcd.print(score);
    if(T1>2000)
    T1 -= F * 1000;  // Reduce time by difficulty factor
    delay(1000);
    newRound();
  }
  if (millis() - startTime > T1) {
    handleGameOverState();
    return;
  }
  // Button press logic and binary number composition
  if (checkCorrectNumber()&&displayedNumber!=0) {
    score++;
    lcd.clear();
    lcd.print("GOOD! Score: ");
    lcd.print(score);
    if(T1>2000)
    T1 -= F * 1000;  // Reduce time by difficulty factor
    delay(1000);
    newRound();
  }
}

// Generate a new random number and display it
void newRound() {
  
  displayedNumber = random(0, 15);
  //displayedNumber = 0;
  lcd.clear();
  lcd.print("Number: ");
  lcd.print(displayedNumber);
  playerInput = 0;
  turnOffLEDs();
  startTime = millis();  // Reset timer for new round
}

void turnOffLEDs() {
  digitalWrite(L1, LOW);
  digitalWrite(L2, LOW);
  digitalWrite(L3, LOW);
  digitalWrite(L4, LOW);
  digitalWrite(LS, LOW);
}
// Check if player has composed the correct binary number using LEDs
bool checkCorrectNumber() {
  int playerNumber = getPlayerBinaryInput();  // Convert LED states to a number
  //Serial.print("玩家输入: ");  // 调试行
  //Serial.println(playerNumber);

  return playerNumber == displayedNumber;

}

// Game over logic
void handleGameOverState() {
  gameState = GAME_OVER;
  turnOffLEDs();
  lcd.clear();
  lcd.print("Game Over");
  lcd.setCursor(0, 1);
  lcd.print("Final Score: ");
  lcd.print(score);
  digitalWrite(LS, HIGH);  // Turn on red LED for 1 second
  delay(1000);
  digitalWrite(LS, LOW);
  delay(10000);  // Show final score for 10 seconds
  resetGame();
}

// Reset game back to the initial state
void resetGame() {
  gameState = INIT;
  lcd.clear();
  LS_PULSE();
  lastB1PressTime = millis();
}

int getPlayerBinaryInput() {


  // 处理 B1 按钮
  unsigned long currentTime = millis(); // 获取当前时间
  if (digitalRead(B1) == LOW) {
    if (currentTime - lastButtonPressTime[0] > DEBOUNCE_DELAY) {
      lastButtonPressTime[0] = currentTime; // 更新按钮按下的时间
      if (digitalRead(L1) == HIGH) { // 如果 L1 亮着
        playerInput -= 8; // 熄灭 L1
        digitalWrite(L1, LOW);
      } else { // 如果 L1 熄灭
        playerInput += 8; // 点亮 L1
        digitalWrite(L1, HIGH);
      }
    }
  }

  // 处理 B2 按钮
  if (digitalRead(B2) == LOW) {
    if (currentTime - lastButtonPressTime[1] > DEBOUNCE_DELAY) {
      lastButtonPressTime[1] = currentTime; // 更新按钮按下的时间
      if (digitalRead(L2) == HIGH) {
        playerInput -= 4; // 熄灭 L2
        digitalWrite(L2, LOW);
      } else {
        playerInput += 4; // 点亮 L2
        digitalWrite(L2, HIGH);
      }
    }
  }

  // 处理 B3 按钮
  if (digitalRead(B3) == LOW) {
    if (currentTime - lastButtonPressTime[2] > DEBOUNCE_DELAY) {
      lastButtonPressTime[2] = currentTime; // 更新按钮按下的时间
      if (digitalRead(L3) == HIGH) {
        playerInput -= 2; // 熄灭 L3
        digitalWrite(L3, LOW);
      } else {
        playerInput += 2; // 点亮 L3
        digitalWrite(L3, HIGH);
      }
    }
  }

  // 处理 B4 按钮
  if (digitalRead(B4) == LOW) {
    if (currentTime - lastButtonPressTime[3] > DEBOUNCE_DELAY) {
      lastButtonPressTime[3] = currentTime; // 更新按钮按下的时间
      if (digitalRead(L4) == HIGH) {
        playerInput -= 1; // 熄灭 L4
        digitalWrite(L4, LOW);
      } else {
        playerInput += 1; // 点亮 L4
        digitalWrite(L4, HIGH);
      }
    }
  }

  return playerInput; // 返回当前用户输入的值
}