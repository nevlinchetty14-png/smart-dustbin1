/*
  Smart Dustbin - Automatic Touchless Lid Opener
  ------------------------------------------------
  Uses an ultrasonic sensor to detect when a hand/object is near the
  dustbin, then automatically opens the lid using a servo motor.
  The lid stays open briefly and then closes automatically.

  Components:
  - Arduino Uno/Nano
  - HC-SR04 Ultrasonic Sensor
  - SG90 Servo Motor
  - Jumper wires, breadboard, 5V power supply

  Author: Nevlin Chetty
*/

#include <Servo.h>

// Pin definitions
const int trigPin = 9;
const int echoPin = 10;
const int servoPin = 6;

// Distance threshold (in cm) to trigger lid opening
const int triggerDistance = 15;

// Servo angles
const int closedAngle = 0;
const int openAngle = 90;

// How long the lid stays open (ms)
const int lidOpenDuration = 3000;

Servo lidServo;

long duration;
int distance;

void setup() {
  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  lidServo.attach(servoPin);
  lidServo.write(closedAngle); // Start with lid closed
}

void loop() {
  distance = getDistanceCM();

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance > 0 && distance <= triggerDistance) {
    openLid();
    delay(lidOpenDuration);
    closeLid();
  }

  delay(200); // Small delay before next reading
}

// Measures distance using the HC-SR04 ultrasonic sensor
int getDistanceCM() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout

  if (duration == 0) {
    return -1; // No echo received
  }

  // Speed of sound = 0.0343 cm/microsecond
  return duration * 0.0343 / 2;
}

void openLid() {
  lidServo.write(openAngle);
  Serial.println("Lid opened");
}

void closeLid() {
  lidServo.write(closedAngle);
  Serial.println("Lid closed");
}
