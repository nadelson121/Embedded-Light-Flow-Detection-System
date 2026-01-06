#include <Arduino.h>
#include <Servo.h>

/* Pin initialization */
const int servoPin   = 9;      // PWM pin for servo
const int rescanPin  = 2;      // pushbutton for manual rescan
const int blueLED    = 12;     // Blue LED to indicate that the system is ready and if the scan finished successfully
const int yellowLED  = 10;     // Yellow LED to indicate that the system is scanning
const int redLED     = 7;      // Red LED to indicate that there is a rescan about to be initiated from the pushbutton trigger or change in light sensitivity at the source's angle location

/* Servo object */
Servo myServo;

/* Rescan flags */
volatile bool rescanRequested = false;
volatile unsigned long lastInterruptTime = 0; // debounce for button

/* Automatic scan timing */
unsigned long lastScan = 0;     // stores when the last scan ended
unsigned long scanInterval = 5000; // default = 5 seconds

/* Light sensitivity */
int lightThreshold = 80;         // user-adjustable sensitivity threshold

/* Post-scan monitoring */
int originalLightValue = 0;      // max intensity from last scan
int bestAngle = 0;               // angle with max intensity from last scan
unsigned long lastRecheck = 0;   // stores when the last recheck ended
const unsigned long recheckInterval = 2000; // recheck interval in ms

/* ISR for pushbutton */
void requestRescan() {
    unsigned long currentTime = millis();
    if (currentTime - lastInterruptTime > 200) { // debounce
        rescanRequested = true;
        lastInterruptTime = currentTime;
        Serial.println("Button pressed! Interrupt detected.");
    }
}

/* Read light value with averaging to prevent noise */
int readLight(int samples = 5) {
    long total = 0;
    for (int i = 0; i < samples; i++) {
        total += analogRead(A0);
        delay(5);
    }
    return total / samples;
}

/* Light scanning function */
void scanLight() {
    digitalWrite(blueLED, LOW);
    digitalWrite(yellowLED, HIGH); // LED indicator for scan-in-progress
    digitalWrite(redLED, LOW);
    Serial.println("Scanning...");

    int selected_angle = 0;
    int selected_intensity = -1;

    for (int angle = 0; angle <= 180; angle += 5) {
        digitalWrite(redLED, LOW);
        digitalWrite(yellowLED, HIGH);
        digitalWrite(blueLED, LOW);  
        // Check for pushbutton rescan
        if (rescanRequested) {
            Serial.println("Scan interrupted! Restarting sweep.");
            digitalWrite(redLED, HIGH); // LED indicator for rescan
            digitalWrite(yellowLED, LOW);
            digitalWrite(blueLED, LOW);
            rescanRequested = false;
            myServo.write(0);
            delay(300); // delay to stabilize Servo
            angle = -5; // restart sweep
            selected_angle = 0; // reset angle of max intensity
            selected_intensity = -1; // reset value of max intensity
            continue; // proceed with the loop for next scan
        }
      
        myServo.write(angle); // set the angle of Servo as it sweeps
        delay(200);

        int lightValue = readLight(); // read light intensity
        Serial.print("Angle: ");
        Serial.print(angle);
        Serial.print(" - Avg Intensity: ");
        Serial.println(lightValue);

        if (lightValue > selected_intensity) { 
            selected_intensity = lightValue; // update maximum intensity with the largest intensity in the sweep
            selected_angle = angle; // update the angle with the location of the strongest source
        }
    }

    myServo.write(selected_angle); // move the servo to the angle with maximum intensity
    bestAngle = selected_angle; // update all-time best angle in terms of intensity for new scan
    originalLightValue = selected_intensity; // update all-time maximum intensity

    Serial.print("Max intensity at angle: ");
    Serial.println(selected_angle);

    digitalWrite(blueLED, HIGH); // LED indicator for successful scan
    digitalWrite(redLED, LOW);

    digitalWrite(yellowLED, LOW);
    Serial.println("Scan done!");
    delay(1000);
    lastScan = millis(); // keeping track of when scan finished
}

/* Setup */
void setup() {
    Serial.begin(9600);
    /* Serial setup */
    while (!Serial) { ; } // waits for scan interval or sensitivity threshold from user; eventually, proceeds with defaults
    Serial.println("System ready. Enter scan interval in ms (default = 5000) and light threshold (default = 80).");

    /* Servo setup */
    myServo.attach(servoPin);


    /* LED setup */
    pinMode(blueLED, OUTPUT);
    pinMode(yellowLED, OUTPUT);
    pinMode(redLED, OUTPUT);

    pinMode(rescanPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(rescanPin), requestRescan, FALLING);


    /* Initialize LEDs */
    digitalWrite(blueLED, HIGH);
    digitalWrite(yellowLED, LOW);
    digitalWrite(redLED, LOW);

    /* Initializes time variables */
    lastScan = millis();
    lastRecheck = millis();
}

/* Main loop */
void loop() {
    // --- Handle user input for scan interval or sensitivity ---
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim(); // removes trailing white space
        if (input.length() > 0) {
            if (input.charAt(0) == 'S' || input.charAt(0) == 's') { // check if there is user input for sensitivity
                int newSens = input.substring(1).toInt();
                if (newSens > 0) {
                    lightThreshold = newSens; // updates sensitivity threshold to user-defined value
                    Serial.print("Sensitivity updated to: ");
                    Serial.println(lightThreshold);
                }
            } else { // if there is input but nothing to indicate that the user inputted a sensitivity value, assume it is related to scan interval
                unsigned long newInterval = input.toInt();
                if (newInterval > 0) {
                    scanInterval = newInterval; // updates scan interval to user-defined value (in ms)
                    Serial.print("Scan interval updated to: ");
                    Serial.print(scanInterval);
                    Serial.println(" ms");
                }
            }
        }
    }

    // --- Trigger scan if button pressed or automatic interval elapsed ---
    if (rescanRequested || (millis() - lastScan >= scanInterval)) {
        bool manualTrigger = rescanRequested;
        rescanRequested = false; // clear before scanning

        if (manualTrigger) { // checks for trigger from light sensitivity change
            Serial.println(">>> Manual/light-triggered scan starting...");
        } else {
            Serial.println(">>> Automatic scan triggered...");
        }

        //lastScan = millis();
        digitalWrite(blueLED, HIGH); // LED indicator that system is ready
        digitalWrite(yellowLED, LOW);
        digitalWrite(redLED, LOW);

        delay(1500); // stabilizing LEDs
        scanLight(); // starts the scan
    }

    // --- Post-scan monitoring at best angle ---
    if (!rescanRequested && (millis() - lastRecheck >= recheckInterval)) {
        lastRecheck = millis();

        myServo.write(bestAngle); // ove servo to last position of maximum light intensity
        delay(100);

        int currentLight = readLight(); // gets light reading from current position
        Serial.print("Post-scan light check at angle ");
        Serial.print(bestAngle);
        Serial.print(": ");
        Serial.println(currentLight);

        // If the difference between the current and original light intensity is greater than the defined threshold, new scan is triggered
        Serial.println(">>> Light changed significantly! Triggering new scan.");
        if (abs(currentLight - originalLightValue) >= lightThreshold) { 
            rescanRequested = true;
        }
    }
}