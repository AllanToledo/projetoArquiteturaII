#include <Arduino.h>
#include <LiquidCrystal.h>
#define RECEIVER PIN_A0
#define measuresSize 10      // number of periods stored
#define sampleSize 4       // number of samples for average
#define riseThreshold 3 // number of rising measuresSize to determine a peak

float readsIR[sampleSize], lastIR = 0, beforeIR = 0, reader = 0;
float irSum = 0;
float irMax = 0;
float irMin = 0;
int measurePointer = 0;
int irPointer = 0;
bool rising = false;
int riseCount = 0;
int T = 20;              // slot milliseconds to read a value from the sensor
int measuresPeriods[measuresSize];
int start;
int lastBeat = 0;

String printMessage = "---";

LiquidCrystal lcd(D8, D7, D3, D2, D1, D0);

#define HEART_LARGER byte(0)
#define HEART_SMALL byte(1)

bool printLargerHeart = true;

byte HeartLarger[] = {
        B11011,
        B11111,
        B01110,
        B00100,
        B00000,
        B00000,
        B00000,
        B00000,
};
byte HeartSmall[] = {
        B01010,
        B01110,
        B00100,
        B00000,
        B00000,
        B00000,
        B00000,
        B00000,
};

void setup() {
    pinMode(RECEIVER, INPUT);
    Serial.begin(9600);
    memset(measuresPeriods, 0, sizeof(measuresPeriods));
    memset(readsIR, 0, sizeof(readsIR));
    lcd.begin(16, 2);
    lcd.createChar(0, HeartLarger);
    lcd.createChar(1, HeartSmall);
    lcd.clear();
}

void loop() {
    // calculate an average of the sensor
    // during a 20 ms (T) period (this will eliminate
    // the 50 Hz noise caused by electric light
    float n = 0.0f;
    start = (int) millis();
    reader = 0.0f;
    do {
        reader += (float) analogRead(RECEIVER);
        n++;
    } while (((int) millis()) < (start + T));
    reader /= n;  // we got an average
    // Add the newest measurement to an array
    // and subtract the oldest measurement from the array
    // to maintain a sum of last measurements
    irSum -= readsIR[irPointer];
    irSum += reader;
    readsIR[irPointer] = reader;
    lastIR = irSum / sampleSize;
    irPointer += (irPointer + 1) % sampleSize;


    int avBPM = 0;
    if (lastIR > beforeIR) {
        printLargerHeart = true;
        riseCount++;  // count the number of samples that are rising
        if (!rising && riseCount > riseThreshold) {
            rising = true;
            measuresPeriods[measurePointer] = (int) millis() - lastBeat;
            lastBeat = (int) millis();
            int averagePeriod = 0;
            int goodMeasures = 0;
            // goodMeasures stores the number of good measuresSize (not floating more than 10%),
            // in the last 10 peaks
            float differenceAcceptable = 1.2;
            for (int i = 1; i < measuresSize; i++) {
                if ((measuresPeriods[i] < measuresPeriods[i - 1] * differenceAcceptable) &&
                    (measuresPeriods[i] > measuresPeriods[i - 1] / differenceAcceptable)) {
                    goodMeasures++;
                    averagePeriod += (int) measuresPeriods[i];
                }
            }
            measurePointer = (measurePointer + 1) % measuresSize;
            // bpm and R shown are calculated as the
            // average of at least 5 good peaks
            avBPM = 0;
            if(goodMeasures > 0)
                avBPM = 60000 / (averagePeriod / goodMeasures);
            // if there are at least 5 good measuresSize...
            if (goodMeasures > 1 && avBPM > 40 && avBPM < 220)
                printMessage = String(avBPM);
        }
    } else {
        // Ok, the curve is falling
        rising = false;
        printLargerHeart = false;
        riseCount = 0;
    }

    lcd.clear();
    lcd.home();
    lcd.print(printMessage + "BPM");
    lcd.write(printLargerHeart ? HEART_LARGER : HEART_SMALL);
    // to compare it with the new value and find peaks
    beforeIR = lastIR;

    // PLOT everything
    Serial.print(lastIR);
    Serial.print(", ");
    Serial.print(irMax);
    Serial.print(", ");
    Serial.print(irMin);
    Serial.print(", ");
    Serial.print(avBPM);
    Serial.println();

}