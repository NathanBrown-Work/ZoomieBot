#include <Arduino.h>
#include <QuadDecoder.h>
#include <QTRSensors.h>

// ---------------------- MOTOR DEFINES -----------------------
#define MOTOR1_PWM 9     // LEFT
#define MOTOR1_DIR 8
#define MOTOR2_PWM 10    // RIGHT
#define MOTOR2_DIR 11
#define MOTOR1_ENCODER_A 1
#define MOTOR1_ENCODER_B 2
#define MOTOR2_ENCODER_A 3
#define MOTOR2_ENCODER_B 5

#define TICKS_PER_REV 2249

QuadDecoder<1> Enc1;

// ---------------------- ULTRASONIC -------------------------
#define ULTRA_PIN 22
const float speedOfSound = 0.0343;

float latestDistance = 9999;
bool emergencyStop = false;

// ---------------------- COLOR LOGIC -------------------------
int blue_cnt = 0;
int red_cnt = 0;
int cur_blue_cnt = 0;
int cur_red_cnt = 0;
float pwmOut = 63.5;

// ---------------------- LINE SENSORS ------------------------
#define NUM_SENSORS 3
QTRSensors qtr;
uint16_t sensorValues[NUM_SENSORS];
float Kp = 15;   // proportional gain for steering
int leftSpeed = 0;
int rightSpeed = 0;

// ============================================================
//                ULTRASONIC MEASUREMENT
// ============================================================
void readUltrasonic() {

    pinMode(ULTRA_PIN, OUTPUT);
    digitalWrite(ULTRA_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(ULTRA_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRA_PIN, LOW);

    pinMode(ULTRA_PIN, INPUT);
    unsigned long duration = pulseIn(ULTRA_PIN, HIGH, 25000);

    if (duration == 0) {
        latestDistance = 9999;
        emergencyStop = false;
        return;
    }
    latestDistance = (duration * speedOfSound) / 2.0;
    emergencyStop = (latestDistance < 12.0);
}

// ============================================================
//                          SETUP
// ============================================================
void setup() {

    Enc1.begin(TICKS_PER_REV);

    pinMode(MOTOR1_PWM, OUTPUT);
    pinMode(MOTOR1_DIR, OUTPUT);
    pinMode(MOTOR2_PWM, OUTPUT);
    pinMode(MOTOR2_DIR, OUTPUT);
    digitalWrite(MOTOR1_DIR, LOW);
    digitalWrite(MOTOR2_DIR, LOW);

    analogWrite(MOTOR1_PWM, 0);
    analogWrite(MOTOR2_PWM, 0);

    Serial.begin(9600);
    Serial5.begin(9600);

    // ----------- SETUP QTR SENSORS -----------
    qtr.setTypeAnalog();
    qtr.setSensorPins((const uint8_t[]){A0, A1, A2}, NUM_SENSORS);
    //qtr.setEmitterPin(2);

    // ----------- CALIBRATE -----------
    delay(500);
    for (int i = 0; i < 400; i++) {
        qtr.calibrate();
        delay(5);
    }
}

// ============================================================
//                          MAIN LOOP
// ============================================================
void loop() {

    /*while(true)
    {
        qtr.read(sensorValues);
        Serial.print(sensorValues[0]);
        Serial.print(",");
        Serial.print(sensorValues[1]);
        Serial.print(",");
        Serial.print(sensorValues[2]);
        Serial.print("\r\n");

    }*/
        // ---- ULTRASONIC STOP ----
    readUltrasonic();
    if (emergencyStop) {
        analogWrite(MOTOR1_PWM, 0);
        analogWrite(MOTOR2_PWM, 0);
        Serial.println("STOP — Object detected!");
        delay(50);
        return;
    }

    // ---- COLOR LOGIC (unchanged) ----
    if (Serial5.available() > 0) {

        String data = Serial5.readStringUntil('\n');
        int commaIndex = data.indexOf(',');

        if (commaIndex > 0) {
            cur_blue_cnt = data.substring(0, commaIndex).toInt();
            cur_red_cnt  = data.substring(commaIndex + 1).toInt();
        }

        if (cur_blue_cnt != blue_cnt || cur_red_cnt != red_cnt) {

            int blue_diff = cur_blue_cnt - blue_cnt;
            int red_diff  = cur_red_cnt - red_cnt;

            if (blue_diff > 0) pwmOut *= (1 << blue_diff);
            else if (blue_diff < 0) pwmOut /= (1 << (-blue_diff));

            if (red_diff > 0) pwmOut /= (1 << red_diff);
            else if (red_diff < 0) pwmOut *= (1 << (-red_diff));

            pwmOut = constrain(pwmOut, 10, 255);

            blue_cnt = cur_blue_cnt;
            red_cnt  = cur_red_cnt;
        }
    }

    // ---- LINE FOLLOWING ----
    qtr.read(sensorValues);

    long position = qtr.readLineBlack(sensorValues); // 0–5000
    long center = 1500;
    Serial.print("Position: ");
    Serial.println(position);
    long error = position - center;
    if (error < -200) {
        leftSpeed = pwmOut;
        rightSpeed = 0;
        analogWrite(MOTOR1_PWM, pwmOut);
        analogWrite(MOTOR2_PWM, 0);

    }
    else if (error > 200) {
        leftSpeed = 0;
        rightSpeed = pwmOut;
        analogWrite(MOTOR1_PWM, 0);
        analogWrite(MOTOR2_PWM, pwmOut);
    }
    else {
        leftSpeed = pwmOut;
        rightSpeed = pwmOut;
        analogWrite(MOTOR1_PWM, pwmOut);
        analogWrite(MOTOR2_PWM, pwmOut);
    }

    //long steering = error / Kp;

    //int leftSpeed  = pwmOut + steering;
    //int rightSpeed = pwmOut + steering;
    //leftSpeed  = constrain(leftSpeed, 0, 255);
    //rightSpeed = constrain(rightSpeed, 0, 255);

    // ---- DRIVE ----
    //analogWrite(MOTOR1_PWM, pwmOut);
    //analogWrite(MOTOR2_PWM, pwmOut);
    Serial.print("Left Speed: ");
    Serial.println(leftSpeed);
    Serial.print("Right Speed: ");
    Serial.println(rightSpeed);
    //delay(200);
}
