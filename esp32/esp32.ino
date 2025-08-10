#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DHT.h>

// ======================= WiFi Config =======================
#define WIFI_SSID       "Dak"
#define WIFI_PASSWORD   "12345678"

// ==================== Firebase Config ======================
#define FIREBASE_HOST   "https://tttt-1e85d-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH   "yqvjUPNvb7v8yh6BRvIJRWXwWiUtgqL1kFZ9hSpy"

// ==================== DHT22 Sensor =========================
#define DHTPIN          15
#define DHTTYPE         DHT22
DHT dht(DHTPIN, DHTTYPE);

// ==================== LED Pins =============================
#define LED_RED         25
#define LED_GREEN       26
#define LED_YELLOW      27

// ==================== Sound Sensor =========================
#define SOUND_PIN       35

// ==================== LDR Sensor ===========================
#define LDR_PIN         34

// ==================== Firebase Objects =====================
FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

// ==================== LED States ===========================
int previousStates[3] = {0, 0, 0}; // Red, Green, Yellow

// ============================================================
// Setup
// ============================================================
void setup() {
    Serial.begin(115200);

    // Initialize sensors
    dht.begin();

    // Configure LED pins
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("Attempting to connect to WiFi...");
        delay(1000);
    }
    Serial.println("WiFi connected successfully");

    // Firebase configuration
    firebaseConfig.database_url = FIREBASE_HOST;
    firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;

    Firebase.begin(&firebaseConfig, &firebaseAuth);
    Firebase.reconnectWiFi(true);
}

// ============================================================
// Read Sensors
// ============================================================
float readLDR() {
    return convertADCToLux(analogRead(LDR_PIN));
}

float convertADCToLux(int adcValue) {
    // Map ADC value (0–4095) to a lux range (0–1000)
    return map(adcValue, 0, 4095, 0, 1000);
}

float readSoundLevel() {
    return analogRead(SOUND_PIN);
}

// ============================================================
// Loop
// ============================================================
void loop() {
    float temperature = dht.readTemperature();
    float lightLevel  = readLDR();
    float soundLevel  = readSoundLevel();

    uploadSensorData(temperature, lightLevel, soundLevel);
    updateLEDState();

    delay(5000);
}

// ============================================================
// Upload Sensor Data to Firebase
// ============================================================
void uploadSensorData(float temperature, float lightLevel, float soundLevel) {
    if (Firebase.ready()) {
        if (Firebase.setFloat(firebaseData, "/phong1/Nhietdo", temperature)) {
            Serial.println("Temperature uploaded");
        } else {
            Serial.printf("Error uploading temperature: %s\n", firebaseData.errorReason().c_str());
        }

        if (Firebase.setFloat(firebaseData, "/phong1/Cdas", lightLevel)) {
            Serial.println("Light intensity uploaded");
        } else {
            Serial.printf("Error uploading light intensity: %s\n", firebaseData.errorReason().c_str());
        }

        if (Firebase.setFloat(firebaseData, "/phong1/Volume", soundLevel)) {
            Serial.println("Sound level uploaded");
        } else {
            Serial.printf("Error uploading sound level: %s\n", firebaseData.errorReason().c_str());
        }
    } else {
        Serial.println("Firebase not ready");
    }
}

// ============================================================
// Update LED State from Firebase
// ============================================================
void updateLEDState() {
    const char* firebasePaths[] = {
        "/thietbi2/den1",
        "/thietbi3/loa",
        "/thietbi1/maylanh"
    };

    const int ledPins[] = {
        LED_RED,
        LED_GREEN,
        LED_YELLOW
    };

    const char* ledNames[] = {
        "Red LED",
        "Green LED",
        "Yellow LED"
    };

    for (int i = 0; i < 3; i++) {
        controlLED(firebasePaths[i], ledPins[i], ledNames[i], i);
    }
}

// ============================================================
// Control Individual LED
// ============================================================
void controlLED(const char* firebasePath, int ledPin, const char* ledName, int index) {
    if (Firebase.getString(firebaseData, firebasePath)) {
        String newState = firebaseData.stringData();

        if (newState == "1" && previousStates[index] != 1) {
            digitalWrite(ledPin, HIGH);
            previousStates[index] = 1;
            Serial.printf("%s state changed to: ON\n", ledName);
        } 
        else if (newState == "0" && previousStates[index] != 0) {
            digitalWrite(ledPin, LOW);
            previousStates[index] = 0;
            Serial.printf("%s state changed to: OFF\n", ledName);
        }
    } else {
        Serial.printf("Error reading %s: %s\n", ledName, firebaseData.errorReason().c_str());
    }
}
