#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Configuration Settings ---
const int LOOP_DELAY_MS = 20;        // ~50Hz sample rate
const int DISPLAY_INTERVAL_MS = 200; // ~5Hz screen updates
const int FLUSH_INTERVAL_MS = 500;   // Flush SD card twice a second
const int CHIP_SELECT_PIN = D10; 
const int BATTERY_PIN = A0;      

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32    
#define OLED_RESET    -1 
#define SCREEN_ADDRESS 0x3C 

Adafruit_MPU6050 mpu;
Adafruit_BMP3XX bmp;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Keep the file object globally accessible
File dataFile;
bool sdHealthy = false;

float currentApogee = 0.0;
float groundPressureHPa = 1013.25; 

// --- Timers ---
unsigned long lastDisplayUpdateTime = 0; 
unsigned long lastFlushTime = 0;

// --- Battery Smoothing Variables ---
float smoothedBatteryVoltage = 0.0;
const float BATTERY_ALPHA = 0.05;           // Takes 5% new reading, 95% old history
const float ADC_CALIBRATION_OFFSET = 0.30; // Tweak this offset to match your multimeter
bool isFirstBatteryRead = true;

void setup() {
    Serial.begin(115200); 
    Wire.begin(); 

    pinMode(BATTERY_PIN, INPUT);

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) { while (1); }
    display.clearDisplay();
    display.setTextSize(1);      
    display.setTextColor(SSD1306_WHITE); 
    display.setCursor(0, 0); 
    display.println("BOOTING LOGGERS..."); 
    display.display(); 
    delay(500);

    if (!mpu.begin() || !bmp.begin_I2C()) { while (1); }

    mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);

    // Warm up the BMP sensor
    for (int i = 0; i < 30; i++) { bmp.performReading(); delay(20); }

    // Calibrate ground pressure for accurate AGL altitude
    float pressureSum = 0;
    int samples = 50;
    for (int i = 0; i < samples; i++) {
        if (bmp.performReading()) { pressureSum += bmp.pressure / 100.0; }
        delay(20);
    }
    groundPressureHPa = pressureSum / samples; 

    // --- SD CARD SETUP ---
    if (SD.begin(CHIP_SELECT_PIN)) {
        dataFile = SD.open("/payload_log.csv", FILE_WRITE);
        if (dataFile) {
            sdHealthy = true;
            if (dataFile.size() == 0) {
                // Header includes the new Pressure and Temp columns
                dataFile.println("TimeMS,Altitude,Pressure,Temp,AccX,AccY,AccZ,GyroX,GyroY,GyroZ,BatteryV");
                dataFile.flush();
            }
        }
    }

    display.clearDisplay();
}

void loop() {
    unsigned long currentTime = millis();

    // Read MPU6050
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Read BMP390 (No blocking return, so MPU data logs even if BMP skips a beat)
    bmp.performReading(); 
    
    float relativeAltitude = bmp.readAltitude(groundPressureHPa);
    if (relativeAltitude < 0.0) relativeAltitude = 0.0;

    if (relativeAltitude > currentApogee) {
        currentApogee = relativeAltitude;
    }

    // --- READ AND SMOOTH BATTERY VOLTAGE ---
    int rawADC = analogRead(BATTERY_PIN);
    float measuredVoltage = (rawADC * 3.3) / 4095.0;
    float rawBatteryVoltage = (measuredVoltage * ((10000.0 + 5000.0) / 5000.0)) + ADC_CALIBRATION_OFFSET;

    if (isFirstBatteryRead) {
        smoothedBatteryVoltage = rawBatteryVoltage;
        isFirstBatteryRead = false;
    } else {
        // Exponential Moving Average Filter
        smoothedBatteryVoltage = (BATTERY_ALPHA * rawBatteryVoltage) + ((1.0 - BATTERY_ALPHA) * smoothedBatteryVoltage);
    }

    // --- SD CARD LOGGING ---
    if (sdHealthy && dataFile) {
        dataFile.print(currentTime);            dataFile.print(",");
        dataFile.print(relativeAltitude, 2);    dataFile.print(",");
        
        // Log Pressure (in hPa) and Temperature (in Celsius)
        dataFile.print(bmp.pressure / 100.0, 2);dataFile.print(","); 
        dataFile.print(bmp.temperature, 2);     dataFile.print(",");
        
        dataFile.print(a.acceleration.x, 2);    dataFile.print(",");
        dataFile.print(a.acceleration.y, 2);    dataFile.print(",");
        dataFile.print(a.acceleration.z, 2);    dataFile.print(",");
        dataFile.print(g.gyro.x, 2);            dataFile.print(",");
        dataFile.print(g.gyro.y, 2);            dataFile.print(",");
        dataFile.print(g.gyro.z, 2);            dataFile.print(",");
        dataFile.println(smoothedBatteryVoltage, 2);     
        
        // Non-Blocking SD Flush (runs twice a second)
        if (currentTime - lastFlushTime >= FLUSH_INTERVAL_MS) {
            dataFile.flush();
            lastFlushTime = currentTime;
        }
    }

    // --- OLED DISPLAY ---
    if (currentTime - lastDisplayUpdateTime >= DISPLAY_INTERVAL_MS) {
        lastDisplayUpdateTime = currentTime;

        display.clearDisplay(); 
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("MAX APOGEE");

        // Display the perfectly smooth battery voltage
        display.setCursor(90, 0);
        display.print(smoothedBatteryVoltage, 1);
        display.print("V");

        display.setTextSize(2); 
        display.setCursor(0, 15);
        display.print(currentApogee, 1);
        display.print(" m");

        display.display(); 
    }

    delay(LOOP_DELAY_MS);
}