
// ------------ PACKAGES ------------ //
#include <Arduino.h>

// Radio
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
RF24 radio(9, 10); // CE, CSN
const byte addresses[][6] = {"00001", "00002"};

// ------------ STRUCTURES ------------ //

struct monitorPack
{
    int id = 1;
    int forceShutdown = 0;
    float thrust = 0.0;
    float pitch = 0.0;
    float roll = 0.0;
    float v_curr = 0.0;
    float v_bat_1 = 0.0;
    float v_bat_2 = 0.0;
    float v_bat_3 = 0.0;
};
monitorPack monitorData;

struct controlPack
{
    int id = 1;
    int isActive = 0;
    float thrust = 0.0;
};
controlPack controlData;

// ------------ PINS ------------ //

const int ledPin = 2;
const int btnPin = 3;
const int minusPin = 4;
const int plusPin = 5;

const int potPin = A0;

// ------------ VARIABLES ------------ //

// Timing
float now = 0;

// Handle Buttons
float thrust = 0.0;
int isActive = 0;
int oldActiveState = 0, newActiveState = 0;
int oldMinusState = 0, newMinusState = 0;
int oldPlusState = 0, newPlusState = 0;
int thrustControlInterval = 2;

// Handle Force Is Active
int prevForceShutdown = 0;

// ------------ SETUP ------------ //

void setup()
{
    Serial.begin(115200);
    delay(100);

    // Initialize LEDs
    pinMode(ledPin, OUTPUT);
    pinMode(btnPin, INPUT);
    pinMode(minusPin, INPUT);
    pinMode(plusPin, INPUT);
    pinMode(potPin, INPUT);

    // Initialize Radio
    radio.begin();
    radio.openWritingPipe(addresses[0]);    // 00002
    radio.openReadingPipe(0, addresses[1]); // 00001
    radio.setDataRate(RF24_2MBPS);
    radio.setChannel(124);
    radio.setPALevel(RF24_PA_MAX);
    radio.startListening();
    Serial.println("setup");
}

// ------------ FUNCTIONS ------------ //

void runComs()
{
    if (radio.available())
    {
        // RECEIVE DATA
        while (radio.available())
        {
            radio.read(&monitorData, sizeof(monitorData));
            // Serial.print(monitorData.id);
            // Serial.println("received...");
        }

        // SEND DATA
        radio.stopListening();
        radio.write(&controlData, sizeof(controlData));
        // Serial.print(controlData.id);
        // Serial.println("sent...");
        controlData.id++;
        radio.startListening();
    }
}

// float calibration_time;

void handleButtons()
{
    // Handle Active Button
    newActiveState = digitalRead(btnPin);

    if (newActiveState == LOW && oldActiveState == HIGH)
    {
        if (isActive == 0)
        {
            isActive = 1;
            //  Serial.println("activated...");
        }
        else
        {
            isActive = 0;
            //   Serial.println("deactivated...");
        }
    };
    oldActiveState = newActiveState;

    if (isActive == 1)
    {
        // Read Pins
        newMinusState = digitalRead(minusPin);
        newPlusState = digitalRead(plusPin);

        // Handle Calibration
        //        if (newMinusState == HIGH && newPlusState == HIGH && (thrust == 0 || thrust == 100))
        //        {
        //            thrust = 100;
        //            calibration_time = now;
        //            Serial.print(thrust);
        //            // Serial.println("FULL THRUST");
        //        }
        //
        //        if (now - calibration_time < 1000)
        //        {
        //            if (newMinusState == LOW && newPlusState == LOW)
        //            {
        //                thrust = 0;
        //                Serial.print(thrust);
        //                // Serial.println("Deactivate thrust");
        //            }
        //        }
        //        else
        //        {
        // Handle Minus Button
        if (newMinusState == LOW && oldMinusState == HIGH)
        {
            thrust = max(0, thrust - thrustControlInterval);
            //            Serial.print(thrust);
            // Serial.println("-2 Thrust");
        };

        // Handle Plus Button
        if (newPlusState == LOW && oldPlusState == HIGH)
        {
            thrust = min(thrust + thrustControlInterval, 100);
            //  Serial.print(thrust);
            // Serial.println("+2 Thrust");
        };
        //        }

        // Reset State
        oldMinusState = newMinusState;
        oldPlusState = newPlusState;
    }
    // Set Control Data
    controlData.isActive = isActive;
    controlData.thrust = thrust;
}

void handleForceShotdown()
{
    if (monitorData.forceShutdown == 1 && prevForceShutdown == 1)
    {
        isActive = 0;
    };
    prevForceShutdown = monitorData.forceShutdown;
}

void handleThrustShutdown()
{
    if (isActive == 0)
    {
        thrust = 0;
    }
}

void handleLEDs()
{
    if (isActive == 1)
    {
        digitalWrite(ledPin, HIGH);
    }
    else
    {
        digitalWrite(ledPin, LOW);
    }
}

float i = 0.0;

void printTelemetry()
{
    Serial.print("loop");
    Serial.println(i);

    Serial.print(">isActive:");
    Serial.println(isActive);

    Serial.print(">forceShutdown:");
    Serial.println(monitorData.forceShutdown);

    Serial.print(">thrust:");
    Serial.println(monitorData.thrust);

    Serial.print(">pitch:");
    Serial.println(monitorData.pitch);

    Serial.print(">roll:");
    Serial.println(monitorData.roll);

    Serial.print(">c_bat:");
    Serial.println(monitorData.v_curr);

    Serial.print(">v_bat_1:");
    Serial.println(monitorData.v_bat_1);

    Serial.print(">v_bat_2:");
    Serial.println(monitorData.v_bat_2);

    Serial.print(">v_bat_3:");
    Serial.println(monitorData.v_bat_3);
}

// ------------ LOOP ------------ //

void loop()
{
    i += 0.1;

    now = millis();

    handleButtons();
    handleForceShotdown();
    handleThrustShutdown();

    float potVal = analogRead(potPin);
    if (isActive == 1 && potVal == 1023)
    {
        thrust = 100;
    }
    if (isActive == 1 && potVal < 1023)
    {
        thrust = map(potVal, 0, 1023, 0, 100);
    }
    handleLEDs();

    runComs();

    printTelemetry();
    delay(100);
}