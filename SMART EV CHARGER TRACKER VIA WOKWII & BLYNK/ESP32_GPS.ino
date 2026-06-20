/***************************************************
 * BLYNK CONFIGURATION
 ***************************************************/
#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "TMPL6fOObLAAr"
#define BLYNK_TEMPLATE_NAME "assignment 2"
#define BLYNK_AUTH_TOKEN "KipxhD_j6ZvsRg1Vq8kOI5z3wDGerRN8"
#define BLYNK_TEMPLATE_NAME "Assignment 2"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

/***************************************************
 * ORIGINAL INCLUDES
 ***************************************************/
#include <TinyGPS++.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

/***************************************************
 * WIFI (WOKWI)
 ***************************************************/
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

/***************************************************
 * OBJECTS
 ***************************************************/
TinyGPSPlus gps;
Adafruit_MPU6050 mpu;

/***************************************************
 * GLOBAL VARIABLES
 ***************************************************/
const int BATT_PIN = 36; 
float currentSpeed = 18.0;     // Initial baseline speed in km/h
unsigned long prevTime = 0;    // For delta time calculation
unsigned long lastLoopTime = 0;
float loopLatency = 0;
int wifiSignalStrength = 0;

// Nearest charger (predefined)
const float chargerLat = -23.466417; 
const float chargerLng = -51.840167;

bool systemEnabled = false; // Default to off
/***************************************************
 * SETUP
 ***************************************************/
void setup() {
  Serial.begin(115200);

  // GPS on UART2
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
  }

  // Blynk connection
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  prevTime = millis();
  Serial.println("EV System with Blynk Connected.");
}
//Toggle the V6 switch in Blynk
BLYNK_WRITE(V6) {
  systemEnabled = param.asInt(); // 1 for ON, 0 for OFF
  if (!systemEnabled) {
    Serial.println("\n[SYSTEM] Monitoring PAUSED by user.");
  } else {
    Serial.println("\n[SYSTEM] Monitoring RESUMED.");
  }
}
/***************************************************
 * LOOP
 ***************************************************/
void loop() {
  // Start timing the loop for Latency Analysis
  unsigned long loopStart = millis();
  Blynk.run(); 
  
  if (systemEnabled) {
  /************* 1. TIME CALCULATION *************/
  unsigned long now = millis();
  float dt = (now - prevTime) / 1000.0; 
  prevTime = now;

  /************* 2. GPS (Motion-Aware Logic) *************/
  static unsigned long lastGPSRead = 0;

  if (currentSpeed > 0.5) {
    while (Serial2.available() > 0) {
      char c = Serial2.read();
      gps.encode(c); 
      lastGPSRead = millis();
    }

    if (millis() - lastGPSRead > 5000 && lastGPSRead != 0) {
      Serial.println("\n[SYSTEM] Warning: GPS signal lost while in motion!");
    }
  } 
  else {
    while (Serial2.available() > 0) Serial2.read();
    lastGPSRead = millis();
  }

  /************* 3. ACCELEROMETER *************/
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  if (abs(a.acceleration.y) > 0.5) { 
    currentSpeed += (a.acceleration.y * dt * 3.6); 
  }
  if (currentSpeed < 0) currentSpeed = 0;

  /************* 4. BATTERY *************/
  int rawValue = analogRead(BATT_PIN);
  float batteryPercent = (rawValue / 4095.0) * 100.0;

  /************* 5. CONSUMPTION & RANGE *************/
  // It increases linearly as speed goes up.
float consumptionRate = 0.14 + (currentSpeed * 0.0012);
  // Limit the consumption 
  if (consumptionRate < 0.1) consumptionRate = 0.1; // Minimum drain
  if (consumptionRate > 2.0) consumptionRate = 2.0; // Maximum drain at very high speed

  // REGEN BRAKING
  // If decelerating hard (negative G-force), consumption drops to almost zero
  if (a.acceleration.y < -1.5) {
    consumptionRate = 0.05; 
  }

  float predictedRange = batteryPercent / consumptionRate;

  /************* 6. DISTANCE TO CHARGER *************/
  float distToCharger = 999.9;
  if (gps.location.isValid()) {
    float rawDLat = gps.location.lat() - chargerLat;
    float rawDLng = gps.location.lng() - chargerLng;

    float amplifiedDLat = rawDLat * 50.0; 
    float amplifiedDLng = rawDLng * 50.0;

    distToCharger = sqrt(pow(amplifiedDLat, 2) + pow(amplifiedDLng, 2)) * 111.0; 
  }

/************* 7. DISPLAY + BLYNK *************/
  static unsigned long displayTimer = 0;
  if (millis() - displayTimer > 1000) {
    
    // Calculate Requirement 5 Metrics
    wifiSignalStrength = WiFi.RSSI(); // Measure Signal Strength (Reliability)
    Serial.println("\n--- EV STATUS REPORT ---");

    if (gps.location.isValid()) {
      Serial.print("LAT: "); Serial.print(gps.location.lat(), 6);
      Serial.print(" | LNG: "); Serial.println(gps.location.lng(), 6);
    } else {
      Serial.println("GPS: Waiting for Fix...");
    }

    Serial.print("BATT: "); Serial.print(batteryPercent, 1); Serial.print("% | ");
    Serial.print("SPD: "); Serial.print(currentSpeed, 1); Serial.println(" km/h");

    Serial.print("EST. RANGE: "); Serial.print(predictedRange, 1); Serial.println(" km");
    Serial.print("DIST TO CHARGER: "); Serial.print(distToCharger, 2); Serial.println(" km");

    bool rangeAlert = predictedRange < (distToCharger + 10.0); //safety buffer so that driver get the warning 10 km earlier
    bool isNearCharger = (distToCharger <= 0.5); // New Proximity Check

    if (rangeAlert) {
      Serial.println("!!! ALERT: RANGE ANXIETY - Proceed to Nearest Charger !!!");
    }
    
    if (isNearCharger) {
      Serial.println(">>> STATUS: EV Charger is near you! <<<");
    }
    // --- REQUIREMENT 5: PERFORMANCE ANALYSIS OUTPUT ---
      Serial.println(">>> REQUIREMENT 5: PERFORMANCE ANALYSIS <<<");
      Serial.print("SYSTEM LATENCY: "); Serial.print(loopLatency); Serial.println(" ms");
      Serial.print("WIFI RELIABILITY (RSSI): "); Serial.print(wifiSignalStrength); Serial.println(" dBm");
      
      if (wifiSignalStrength > -60) Serial.println("SIGNAL: Excellent (High Reliability)");
      else if (wifiSignalStrength > -80) Serial.println("SIGNAL: Good (Stable)");
      else Serial.println("SIGNAL: Critical (Low Reliability)");

      Serial.println("ENERGY EFFICIENCY: Selective Edge Transmission active (1Hz)");
      Serial.println("------------------------");

    // 🔹 BLYNK UPDATES
    Blynk.virtualWrite(V0, batteryPercent);   
    Blynk.virtualWrite(V1, predictedRange);  
    Blynk.virtualWrite(V2, distToCharger);   
    Blynk.virtualWrite(V3, currentSpeed);    
    Blynk.virtualWrite(V4, rangeAlert ? 1 : 0); 
    Blynk.virtualWrite(V5, isNearCharger ? 255 : 0); // V5 Green LED
    Blynk.virtualWrite(V7, loopLatency);

    displayTimer = millis();
  }
} 
  else {
    /************* SYSTEM OFF MODE *************/
    // Periodically clear Blynk dashboard so user knows system is off
    static unsigned long offTimer = 0;
    if (millis() - offTimer > 2000) {
      Blynk.virtualWrite(V0, 0); 
      Blynk.virtualWrite(V1, 0); 
      Blynk.virtualWrite(V2, 0); 
      Blynk.virtualWrite(V3, 0); 
      Blynk.virtualWrite(V4, 0); // Turn off LEDs
      Blynk.virtualWrite(V5, 0);
      offTimer = millis();
    }
    // End timing the loop to calculate latency for the next pass
    loopLatency = millis() - loopStart;
  }
}