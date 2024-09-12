// Include the Servo library to control servo motor
#include <Servo.h>
// Include SoftwareSerial library for serial communication with ESP8266
#include <SoftwareSerial.h>

// Define pins 2 (RX) and 3 (TX) for software serial communication
SoftwareSerial ESP8266(2, 3); 

// ThingSpeak API key
String myAPIkey = "9HR9RBP2JEV9QAO1";

// Create a servo object to control the servo motor
Servo servo;

// Variables for LDR (Light Dependent Resistor) values
int eastLDR = 0;
int westLDR = 0;
int difference = 0;
int error = 10;  // Sensitivity for LDR difference
int servoSet = 130;  // Initial position of servo

// Upload data factors
long writingTimer = 17;  // Time interval (in seconds) to upload data to ThingSpeak
long startTime = 0;
long waitTime = 0;

// Variables for actuators and status checks
boolean relay1_st = false;
boolean relay2_st = false;
unsigned char check_connection = 0;
unsigned char times_check = 0;  // To track reconnection attempts

void setup() { 
  // Attach servo motor to pin 9
  servo.attach(9);
  
  // Initialize the serial monitor for debugging
  Serial.begin(9600);
  
  // Set servo to initial position
  servo.write(130);

  // Start ESP8266 communication at 9600 baud rate
  ESP8266.begin(9600);
  
  // Record start time for data upload timing
  startTime = millis();

  // Reset ESP8266 module
  ESP8266.println("AT+RST");
  delay(2000);  // Wait for reset
  
  Serial.println("Connecting to Wifi");

  // Connection to Wifi
  while(check_connection == 0) {
    Serial.print(".");
    // Send AT command to connect to Wi-Fi (replace with your Wi-Fi credentials)
    ESP8266.print("AT+CWJAP=\"Paradise\",\"$Harumia312\"\r\n");
    ESP8266.setTimeout(5000);

    // Check if the module connects to Wi-Fi successfully
    if(ESP8266.find("WIFI CONNECTED")) {
      Serial.println("WIFI Connected");
      check_connection = 1;
    } else {
      times_check++;
      if(times_check > 3) {
        times_check = 0;
        Serial.println("Trying to reconnect ...");
      }
    }
  }
}

void loop() { 
  // Read values from LDRs connected to analog pins A0 (east) and A1 (west)
  eastLDR = analogRead(A0);
  westLDR = analogRead(A1);
  
  // Calculate elapsed time for data upload
  waitTime = millis() - startTime;
  
  // Check if it's time to upload data to ThingSpeak
  if (waitTime > (writingTimer * 1000)) {
    writeThingSpeak();
    startTime = millis();  // Reset start time for the next interval
  }

  // Check light intensity; if both LDRs report low light, rotate servo towards east
  if (eastLDR < 400 && westLDR < 400) {  
    while (servoSet <= 140 && servoSet >= 15) {
      servoSet++;  // Move servo incrementally
      servo.write(servoSet);
      delay(100);
    }
  }
  
  // Calculate the difference between east and west LDRs
  difference = eastLDR - westLDR;

  // Adjust the panel based on the difference in light intensity
  if (difference > error) {  // East is brighter
    if (servoSet <= 140) {
      servoSet++;  // Move servo to the west side
      servo.write(servoSet);
    }
  } else if (difference < -error) {  // West is brighter
    if (servoSet >= 15) {
      servoSet--;  // Move servo to the east side
      servo.write(servoSet);
    }
  }

  // Print debugging information to Serial Monitor
  Serial.print(eastLDR);
  Serial.print("   -   ");
  Serial.print(westLDR);
  Serial.print("   -   ");
  Serial.print(difference);
  Serial.print("   -   ");
  Serial.print(servoSet);
  Serial.println("   -   .");
  
  // Add delay to avoid rapid readings
  delay(100);
}

// Function to send data to ThingSpeak
void writeThingSpeak() {
  // Initialize ThingSpeak connection
  startThingSpeakCmd();

  // Prepare the GET request string for ThingSpeak
  String getStr = "GET /update?api_key=";
  getStr += myAPIkey;
  getStr += "&field1=" + String(eastLDR);
  getStr += "&field2=" + String(westLDR);
  getStr += "\r\n\r\n";

  // Send the GET request
  GetThingspeakcmd(getStr);
}

// Function to initialize connection to ThingSpeak server
void startThingSpeakCmd() {
  ESP8266.flush();  // Clear previous data from ESP buffer
  String cmd = "AT+CIPSTART=\"TCP\",\"184.106.153.149\",80";  // Connect to ThingSpeak IP
  ESP8266.println(cmd);
  Serial.print("Start Command: ");
  Serial.println(cmd);

  if (ESP8266.find("Error")) {  // Check if there was an error in starting the connection
    Serial.println("AT+CIPSTART error");
    return;
  }
}

// Function to send a GET request to ThingSpeak
String GetThingspeakcmd(String getStr) {
  String cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  ESP8266.println(cmd);  // Send the length of the GET request
  Serial.println(cmd);

  // Wait for ">" prompt from ESP8266 to send data
  if (ESP8266.find(">")) {
    ESP8266.print(getStr);  // Send the actual GET request
    Serial.println(getStr);
    delay(500);  // Wait for server response

    // Read the server response
    String messageBody = "";
    while (ESP8266.available()) {
      String line = ESP8266.readStringUntil('\n');
      if (line.length() == 1) {  // Empty line indicates start of message body
        messageBody = ESP8266.readStringUntil("\n");
      }
    }

    // Print the message body (response from ThingSpeak)
    Serial.print("Message Body Received: ");
    Serial.println(messageBody);
    return messageBody;
  } else {
    // If no response from ESP8266, close connection
    ESP8266.println("AT+CIPCLOSE");
    Serial.println("AT+CIPCLOSE");
  }
}
