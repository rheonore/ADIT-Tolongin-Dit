#include <ESP32Servo.h>
#include <ESP32PWM.h>
#include "TCRT5000.h"
#include <WiFi.h>
#include <HTTPClient.h>

// PIN 
#define PIN_SERVO  14  // SERVO PWM
#define LED_R      1     // RGB-CC: R via 220Ω
#define LED_G      2     // RGB-CC: G via 220Ω
#define IR_LEFT    13
#define IR_RIGHT   3
#define BUZZER     16

// Classification
int waste_type = -1; // 0=organik, 1=anorganik, -1=belum ada

// SERVO via ESP32Servo
Servo myservo;
void servoWriteDeg(int deg){
  myservo.write(constrain(deg, 0, 180));
}

int runAI() { // INI MASIH DUMMY!!!!
  int result = random(0, 2);
  Serial.print("AI classify: ");
  Serial.println(result);
  return result;
}

const char* WIFI_SSID = "WANDA";
const char* WIFI_PASSWORD = "vincentia04032011";

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
  Serial.println(WiFi.localIP());
  delay(500);
  
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(IR_LEFT, INPUT);
  pinMode(IR_RIGHT, INPUT);
  pinMode(BUZZER, OUTPUT);

  // Servo
  myservo.setPeriodHertz(50);
  myservo.attach(PIN_SERVO, 500, 2500); // 0..180 => 500..2500us
  servoWriteDeg(0);

  randomSeed(analogRead(0));
}

void sendToUbidots(int ir_left, int ir_right, int waste_type, int led_status, int buzzer_status) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://industrial.api.ubidots.com/api/v1.6/devices/adit-tolongin-dit");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Auth-Token", UBIDOTS_TOKEN);

    String payload = "{";
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

void loop(){
  // Baca IR
  int ir_left  = digitalRead(IR_LEFT);
  int ir_right = digitalRead(IR_RIGHT);

  // Baca tipe sampah
  waste_type = runAI(); // ini harusnya dimasukin fungsi isinya AI modul yg formatnya .h, tp belum ada

  int led_status = 0;
  int buzzer_status = 0;

  if (waste_type == 0 && ir_left == HIGH) {
      // Organik & ditaruh di kiri
      myservo.write(45);
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_R, LOW);
      digitalWrite(BUZZER, LOW);
      led_status = 2; buzzer_status = 0;
      Serial.println("Organik BENAR -> Servo buka");
  }
  else if (waste_type == 1 && ir_right == HIGH) {
      // Anorganik & ditaruh di kanan
      myservo.write(45);
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_R, LOW);
      digitalWrite(BUZZER, LOW);
      led_status = 2; buzzer_status = 0;
      Serial.println("Anorganik BENAR -> Servo buka");
  }
  else {
      myservo.write(0);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_R, HIGH);
      digitalWrite(BUZZER, HIGH);
      led_status = 1; buzzer_status = 1;
      Serial.println("Sampah SALAH -> Servo tidak buka");
  }

  // Print ke serial
  Serial.printf("IR_L=%d | IR_R=%d\n", ir_left, ir_right);

  // int ultrasonic1 = readDistanceMM(TRIG1, ECHO1);
  // int ultrasonic2 = readDistanceMM(TRIG1, ECHO1);
  // int ir_left = random(0, 2);
  // int ir_right = random(0, 2);
  // int waste_type = random(0, 2);
  // int servo_angle = 45;
  // int led_status = random(0, 3);
  // int buzzer_status = random(0, 2);
  // ### ini tadinya hanya dummy data buat koneksi ke ubidots.

  sendToUbidots(ir_left, ir_right, waste_type, led_status, buzzer_status);

  delay(1000);
}