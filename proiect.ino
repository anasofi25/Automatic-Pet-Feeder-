#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <Arduino.h>
#include <time.h>
#include <LiquidCrystal_I2C.h> // Use this library for I2C LCD

#define SDA_PIN 21
#define SCL_PIN 22
// MESSAGE STRINGS
const String SETUP_INIT = "SETUP: Initializing ESP32 dev board";
const String SETUP_ERROR = "!!ERROR!! SETUP: Unable to start SoftAP mode";
const String SETUP_SERVER_START = "SETUP: HTTP server started --> IP addr: ";
const String SETUP_SERVER_PORT = " on port: ";
const String INFO_NEW_CLIENT = "New client connected";
const String INFO_DISCONNECT_CLIENT = "Client disconnected";

// HTTP headers and HTML
const String HTTP_HEADER = "HTTP/1.1 200 OK\r\nContent-type:text/html\r\n\r\n";
const String HTML_PAGE = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Automatic Pet Feeder</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
    button { padding: 20px; font-size: 18px; cursor: pointer; }
  </style>
</head>
<body>
  <h1>Automatic Pet Feeder</h1>
  <h1>Feeding occurs every 12 hours </h1>
  <button onclick="rotateMotor()">Tap to Manual Feed!</button>
  <h2 id="foodStatus">Checking food level...</h2>
  <h2 id="timeRemaining">Calculating time until next feeding...</h2>
  <script>
    function rotateMotor() {
      fetch('/rotate')
        .then(response => response.text())
        .then(data => {
          alert(data);
        });
    }

    function updateFoodStatus(status) {
      document.getElementById("foodStatus").innerHTML = status;
    }

    function updateTimeRemaining(time) {
      document.getElementById("timeRemaining").innerHTML = time;
    }

    // Poll server every 5 seconds to update food status and time remaining
    setInterval(() => {
  fetch('/status')
    .then(response => response.json())
    .then(data => {
      updateFoodStatus(data.foodStatus);
      updateTimeRemaining(data.timeRemaining);

      // Show notification if motor rotated
      if (data.notification) {
        alert(data.notification);
      }
    });
}, 5000);

  </script>
</body>
</html>
)rawliteral";

// WIFI CONFIGURATION
const char *SSID = "Ana_1";
const char *PASS = "12345678";
const int HTTP_PORT_NO = 80;

// GLOBAL VARIABLES
WiFiServer HttpServer(HTTP_PORT_NO);

// Stepper Motor Pin Configuration
#define STEPPER_IN1 27
#define STEPPER_IN2 26
#define STEPPER_IN3 25
#define STEPPER_IN4 33

#define REVOLUTION_STEP 2048 // Number of steps for a full revolution

volatile bool motorMoving = false; // Motor movement flag
int stepperStep = 0; // Stepper motor step

// Motor timing
const int intervalStepper = 4;
long prevMillisStepper = 0;

// IR Sensor Pin
#define IR_SENSOR_PIN 14

// Time configuration: Set your desired activation time
const int desiredHour = 17;   // Set desired hour (2:00 PM)
const int desiredMinute = 13;  // Set desired minute (00)

unsigned long startMillis = 0;

// LCD configuration (I2C with address 0x27, 16x2 size)
LiquidCrystal_I2C lcd(0x27, 16, 2); // Use the correct address for your LCD

void setup() {
  // Set up serial communication
  Serial.begin(9600);

  // Stepper Motor setup
  pinMode(STEPPER_IN1, OUTPUT);
  pinMode(STEPPER_IN2, OUTPUT);
  pinMode(STEPPER_IN3, OUTPUT);
  pinMode(STEPPER_IN4, OUTPUT);

  // IR sensor setup
  pinMode(IR_SENSOR_PIN, INPUT);

  // WiFi setup
  if (!WiFi.softAP(SSID, PASS)) {
    Serial.println(SETUP_ERROR);
    while (1);
  }

  IPAddress accessPointIP = WiFi.softAPIP();
  String webServerInfoMessage = SETUP_SERVER_START + accessPointIP.toString() + SETUP_SERVER_PORT + HTTP_PORT_NO;
  HttpServer.begin();
  Serial.println(webServerInfoMessage);
  
  // Initialize LCD
  Wire.begin(SDA_PIN, SCL_PIN); 
  lcd.init(); // Initialize the LCD with 16 columns and 2 rows
  lcd.backlight();  // Ensure the backlight is on
  lcd.clear();      // Clear the screen
  lcd.print("Initializing...");
  delay(2000); // Wait for 2 seconds to display the initialization message

  startMillis = millis();
}
volatile bool motorNotification = false; 
unsigned long lastLCDUpdate = 0;  // Track the last time the LCD was updated
String timeRemainingStr;         // Store the formatted time string

void loop() {
  // Get current time
  unsigned long currentMillis = millis();

  // Check the IR sensor (whether food is detected in the bowl)
  bool foodDetected = digitalRead(IR_SENSOR_PIN) == LOW;

  // Update food status
  String foodStatus = foodDetected ? "Enough food in the bowl!" : "No food detected, refill!";
  Serial.println(foodStatus);

  // Calculate time remaining
  unsigned long timeRemaining = 60000 - (currentMillis - startMillis);
  if (timeRemaining <= 0) timeRemaining = 0;  // Prevent negative time

  unsigned long hoursRemaining = timeRemaining / 3600000;
  unsigned long minutesRemaining = (timeRemaining % 3600000) / 60000;
  unsigned long secondsRemaining = (timeRemaining % 60000) / 1000;

  // Format the time string
  timeRemainingStr = String(hoursRemaining) + ":" +
                     (minutesRemaining < 10 ? "0" : "") + String(minutesRemaining) + ":" +
                     (secondsRemaining < 10 ? "0" : "") + String(secondsRemaining);

  // Rotate motor and set notification if 1 minute has passed
  if (currentMillis - startMillis >= 60000 && !motorMoving) {
    Serial.println("1 minute has passed. Rotating motor for 2 seconds.");

    // Display "Feeding Time" on the LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Feeding Time!");

    // Rotate motor for 2 seconds
    rotateMotorForDuration();

    // Set notification flag
    motorNotification = true;

    // Reset the timer (to simulate periodic feeding if desired)
    startMillis = currentMillis;
  }

  // Update LCD display every 5 seconds
  if (currentMillis - lastLCDUpdate >= 5000) {
    lastLCDUpdate = currentMillis;

    // Display time remaining on the LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Time left:");
    lcd.setCursor(0, 1);
    lcd.print(timeRemainingStr); // Use the same formatted time string
  }

  // Serve client requests
  WiFiClient client = HttpServer.available();
  if (client) {
    String currentLine = "";
    String requestPath = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        if (c == '\n') {
          if (currentLine.length() == 0) {
            if (requestPath.startsWith("GET /rotate")) {
              rotateMotorForDuration();
              client.println(HTTP_HEADER);
              client.println("Manual feeding successfull!!");
            } else if (requestPath.startsWith("GET /status")) {
              client.println(HTTP_HEADER);
              client.printf(
                "{\"foodStatus\": \"%s\", \"timeRemaining\": \"%s\", \"notification\": \"%s\"}",
                foodStatus.c_str(),
                timeRemainingStr.c_str(), // Send the same time string
                motorNotification ? "Successfull feeding!" : ""
              );

              motorNotification = false;  // Reset the flag after notifying
            } else {
              sendWebPage(client);
            }
            break;
          } else if (currentLine.startsWith("GET ")) {
            requestPath = currentLine;
          }
          currentLine = "";
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
  }
}

void rotateMotorForDuration() {
  unsigned long startTime = millis();
  motorMoving = true;

  while (millis() - startTime < 2000) {
    stepMotor();
  }

  motorMoving = false;
  stopMotor();
}

void stepMotor() {
  if (millis() - prevMillisStepper >= intervalStepper) {
    prevMillisStepper = millis();

    switch (stepperStep) {
      case 0:
        digitalWrite(STEPPER_IN1, HIGH);
        digitalWrite(STEPPER_IN2, LOW);
        digitalWrite(STEPPER_IN3, LOW);
        digitalWrite(STEPPER_IN4, LOW);
        break;
      case 1:
        digitalWrite(STEPPER_IN1, LOW);
        digitalWrite(STEPPER_IN2, HIGH);
        digitalWrite(STEPPER_IN3, LOW);
        digitalWrite(STEPPER_IN4, LOW);
        break;
      case 2:
        digitalWrite(STEPPER_IN1, LOW);
        digitalWrite(STEPPER_IN2, LOW);
        digitalWrite(STEPPER_IN3, HIGH);
        digitalWrite(STEPPER_IN4, LOW);
        break;
      case 3:
        digitalWrite(STEPPER_IN1, LOW);
        digitalWrite(STEPPER_IN2, LOW);
        digitalWrite(STEPPER_IN3, LOW);
        digitalWrite(STEPPER_IN4, HIGH);
        break;
    }

    stepperStep++;
    if (stepperStep > 3) stepperStep = 0;
  }
}

void stopMotor() {
  digitalWrite(STEPPER_IN1, LOW);
  digitalWrite(STEPPER_IN2, LOW);
  digitalWrite(STEPPER_IN3, LOW);
  digitalWrite(STEPPER_IN4, LOW);
}

void sendWebPage(WiFiClient client) {
  client.println(HTTP_HEADER);
  client.print(HTML_PAGE);
}
