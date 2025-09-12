7#include <ESP32Servo.h>
#include <ESP32PWM.h>

//    FILE: TCRT5000_demo.ino
//  AUTHOR: Rob Tillaart
// PURPOSE: test basic behaviour and performance
//     URL: https://github.com/RobTillaart/TCRT5000

#include "TCRT5000.h"

// #include <Arduino.h>
// #include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>

// PIN 
#define TRIG   12  // HCSR04 1 & 2
#define ECHO1  3   // HCSR04 1 
#define ECHO2  16  // HCSR04 2
#define PIN_SERVO  14  // SERVO PWM
#define LED_R      13     // RGB-CC: R via 220Ω
#define LED_G      1     // RGB-CC: G via 220Ω
#define IR_LEFT    15
#define IR_RIGHT   4
#define BUZZER     2
6
// Classification
int waste_type = -1; // 0=organik, 1=anorganik, -1=belum ada

// SERVO via ESP32Servo
Servo myservo;
void servoWriteDeg(int deg){
  myservo.write(constrain(deg, 0, 180));
}

// HC-SR04
long readDistanceCM1(){
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long duration1 = pulseIn(ECHO1, HIGH, 30000UL);
  if (duration1 == 0) return -1;
  return duration1 * 0.034 / 2; // cm
}

long readDistanceCM2(){
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long duration2 = pulseIn(ECHO2, HIGH, 30000UL);
  if (duration2 == 0) return -1;
  return duration2 * 0.034 / 2; // cm
}

int runAI() {

}
const char* WIFI_SSID = "SMAN6YK Hotspot";
const char* WIFI_PASSWORD = "0274513335";

const char* UBIDOTS_TOKEN = "BBUS-FXL1UGenkoxjxjWrHUpFa19jyhDWcF"; 
const char* UBIDOTS_DEVICE = "adit-tolongin-dit"; // label device yang udah kamu buat

void setup(){
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi tersambung!");
  delay(500);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(ECHO2, INPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(IR_LEFT, INPUT);
  pinMode(IR_RIGHT, INPUT);
  pinMode(BUZZER, OUTPUT);

  // Servo
  myservo.setPeriodHertz(50);
  myservo.attach(PIN_SERVO, 500, 2500); // 0..180 => 500..2500us
  servoWriteDeg(0);

  // // Indikasi awal
  // digitalWrite(LED_R, HIGH);  // merah = "salah" (default)
  // digitalWrite(LED_G, LOW);

  Serial.println("Ready. Tekan 'c' utk kalibrasi 200g (simulasi).");
}

void sendToUbidots(int ultrasonic1, int ultrasonic2, int ir_left, int ir_right, int waste_type, int led_status, int buzzer_status) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://industrial.api.ubidots.com/api/v1.6/devices/adit-tolongin-dit");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Auth-Token", UBIDOTS_TOKEN);

    String payload = "{";
    payload += "\"ultrasonic1\": " + String(ultrasonic1) + ",";
    payload += "\"ultrasonic2\": " + String(ultrasonic2) + ",";
    payload += "\"ir_left\": " + String(ir_left) + ",";
    payload += "\"ir_right\": " + String(ir_right) + ",";
    payload += "\"waste_type\": " + String(waste_type) + ",";
    payload += "\"led_status\": " + String(led_status) + ",";
    payload += "\"buzzer_status\": " + String(buzzer_status);
    payload += "}";

    int httpResponseCode = http.POST(payload);
    Serial.println("Response: " + String(httpResponseCode));

    http.end();

  } else {
    Serial.println("WiFi not connected!");
  }
}

uint32_t t0=0;

void loop(){
  // Baca jarak HCSR04
  int ultrasonic1 = readDistanceCM1();  // sensor utama di dalam tempat sampah
  int ultrasonic2 = readDistanceCM2();  // optional, kalau pakai dua sensor

  // Baca IR
  int ir_left  = digitalRead(IR_LEFT);
  int ir_right = digitalRead(IR_RIGHT);

  // Baca tipe sampah
  waste_type = runAI(); // ini harusnya dimasukin fungsi isinya AI modul yg formatnya .tflite, tp belum ada

  if (waste_type == 0 && IR_LEFT == HIGH) {
      // Organik & ditaruh di kiri
      myservo.write(45);
      Serial.println("Organik BENAR -> Servo buka");
  }
  else if (waste_type == 1 && IR_RIGHT == HIGH) {
      // Anorganik & ditaruh di kanan
      myservo.write(45);
      Serial.println("Anorganik BENAR -> Servo buka");
  }
  else {
      myservo.write(0);
      Serial.println("Sampah SALAH -> Servo tidak buka");
  }

   // Tinggi tempat sampah
  int led_status, buzzer_status;
  if ((ultrasonic1 > 0 && ultrasonic1 < 150) || (ultrasonic2 > 0 && ultrasonic2 < 150)) {   // jarak < 15 cm = sampah penuh
    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_G, LOW);
    digitalWrite(BUZZER, HIGH);
    led_status = 1;     // merah
    buzzer_status = 1;  // bunyi
  } else {
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_G, HIGH);
    digitalWrite(BUZZER, LOW);
    led_status = 2;     // hijau
    buzzer_status = 0;  // mati
  }

  // Print ke serial
  Serial.printf("U1=%d cm | U2=%d cm | IR_L=%d | IR_R=%d\n", ultrasonic1, ultrasonic2, ir_left, ir_right);

  // int ultrasonic1 = readDistanceMM(TRIG1, ECHO1);
  // int ultrasonic2 = readDistanceMM(TRIG1, ECHO1);
  // int ir_left = random(0, 2);
  // int ir_right = random(0, 2);
  // int waste_type = random(0, 2);
  // int servo_angle = 45;
  // int led_status = random(0, 3);
  // int buzzer_status = random(0, 2);
  // ### ini tadinya hanya dummy data buat koneksi ke ubidots.

  sendToUbidots(ultrasonic1, ultrasonic2, ir_left, ir_right, waste_type,
                led_status, buzzer_status);

  delay(1000);
}