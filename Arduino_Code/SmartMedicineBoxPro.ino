#include <LiquidCrystal.h>
#include "HX711.h"

//================ LCD ==================
LiquidCrystal lcd(12, 11, 4, 3, 2, A0);

//================ HX711 ================
#define DT 7
#define SCK 8
HX711 scale;

//================ Pins =================
#define RED_LED     9
#define GREEN_LED   10
#define YELLOW_LED  A3
#define BUZZER      13

#define BTN_9AM      5
#define BTN_12PM     6
#define BTN_4PM      A1
#define BTN_8PM      A2

//================ Clock =================
byte hour = 8;
byte minute = 59;
byte second = 50;

unsigned long lastClock = 0;

//================ Weight ================
float weight = 0.0;
int pillsLeft = 0;
float previousWeight = 0.0;

const float CAL_FACTOR = 4500000.0;

//================ Reminder ==============
bool reminderActive = false;
bool dose9AMDone = false;
bool dose12PMDone = false;
bool dose4PMDone = false;
bool dose8PMDone = false;

float reminderWeight = 0;

bool medicineTaken = false;
bool waitingSMS = false;
bool refillSent = false;

//========= Daily Report =========
int takenCount = 0;
int missedCount = 0;
int wrongDoseCount = 0;
int correctDoseCount = 0;
int underDoseCount = 0;
int doubleDoseCount = 0;
int tooManyPillsCount = 0;

String doseStatus = "";

unsigned long reminderStart = 0;

//Medication History
String history[10];
int historyIndex = 0;

//================ Buffer ================
char buffer[17];

//========================================
void updateClock()
{
    if (millis() - lastClock >= 1000)
    {
        lastClock = millis();

        second++;

        if (second >= 60)
        {
            second = 0;
            minute++;
        }

        if (minute >= 60)
        {
            minute = 0;
            hour++;
        }

        if (hour >= 24)
        {
            hour = 0;
        }
    }
}

//======================================== History
void addHistory(String event)
{
    String timeStamp = "";

    if(hour < 10) timeStamp += "0";
    timeStamp += String(hour);
    timeStamp += ":";

    if(minute < 10) timeStamp += "0";
    timeStamp += String(minute);
    timeStamp += ":";

    if(second < 10) timeStamp += "0";
    timeStamp += String(second);

    history[historyIndex] = timeStamp + " - " + event;

    historyIndex++;

    if(historyIndex >= 10)
    {
        historyIndex = 0;
    }
}

void showHistory()
{
    Serial.println();
    Serial.println("===== MEDICATION HISTORY =====");

    for(int i = 0; i < historyIndex; i++)
    {
        Serial.print(i + 1);
        Serial.print(". ");
        Serial.println(history[i]);
    }

    Serial.println("==============================");
}

void showDailyReport()
{
    Serial.println();
    Serial.println("========== DAILY REPORT ==========");

    Serial.print("Correct Dose    : ");
    Serial.println(correctDoseCount);

    Serial.print("Under Dose      : ");
    Serial.println(underDoseCount);

    Serial.print("Double Dose     : ");
    Serial.println(doubleDoseCount);

    Serial.print("Too Many Pills  : ");
    Serial.println(tooManyPillsCount);

    Serial.print("Missed Dose     : ");
    Serial.println(missedCount);        

    Serial.print("Remaining Pills: ");
    Serial.println(pillsLeft);

    int totalDoses = correctDoseCount +
                 underDoseCount +
                 doubleDoseCount +
                 tooManyPillsCount +
                 missedCount;

    Serial.print("Adherence      : ");

    if(totalDoses > 0)
    {
        Serial.print((correctDoseCount * 100) / totalDoses);
        Serial.println("%");
    }
    else
    {
        Serial.println("0%");
    }

    Serial.println("==================================");
}

//========================================
void setup()
{
   Serial.begin(9600);

    lcd.begin(16,2);

    scale.begin(DT,SCK);

    pinMode(RED_LED,OUTPUT);
    pinMode(GREEN_LED,OUTPUT);
    pinMode(YELLOW_LED,OUTPUT);
    pinMode(BUZZER,OUTPUT);

    pinMode(BTN_9AM, INPUT_PULLUP);
    pinMode(BTN_12PM,INPUT_PULLUP);
    pinMode(BTN_4PM, INPUT_PULLUP);
    pinMode(BTN_8PM, INPUT_PULLUP);

    digitalWrite(RED_LED,LOW);
    digitalWrite(GREEN_LED,LOW);
    digitalWrite(YELLOW_LED,LOW);
    medicineTaken = false;
    digitalWrite(BUZZER,LOW);

    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("Smart Med Box");

    lcd.setCursor(0,1);
    lcd.print("Initializing");

    Serial.println("================================");
    Serial.println(" SMART MEDICINE BOX STARTED");
    Serial.println(" GSM SIMULATION READY");

    Serial.println("================================");
    Serial.println("--------------------------------");
    Serial.println("Event Log Started");
    Serial.println("--------------------------------");

    delay(2000);
    lcd.clear();
    previousWeight = scale.read() / CAL_FACTOR;
}
//================ Read Weight =====================
void readWeight()
{
    long raw = scale.read();

    weight = (raw / CAL_FACTOR) * 620.0;   // Convert to grams
    // One tablet = 5 grams
    pillsLeft = (int)(weight / 5.0);

if(pillsLeft < 0)
    pillsLeft = 0;

    if(weight < 0)
        weight = 0;
   // Serial.print("Weight : ");
   // Serial.print(weight);
   // Serial.print(" g");

   // Serial.print("   Pills : ");
   // Serial.println(pillsLeft);
}

//================ Display LCD =====================
void displayLCD()
{
 if(medicineTaken)
{
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("Medicine");

    lcd.setCursor(0,1);
    lcd.print("Taken");

    return;
}

if(waitingSMS)
{
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("Reminder");

    lcd.setCursor(0,1);
    lcd.print("Sent");

    return;
}

if(reminderActive)
{
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("Take Medicine");

    lcd.setCursor(0,1);
    lcd.print("Reminder ");

    return;
}
    lcd.setCursor(0,0);
    lcd.print("                ");   // Clear first row
    lcd.setCursor(0,0);

    if(hour < 10) lcd.print("0");
    lcd.print(hour);
    lcd.print(":");

    if(minute < 10) lcd.print("0");
    lcd.print(minute);
    lcd.print(":");

    if(second < 10) lcd.print("0");
    lcd.print(second);

    lcd.setCursor(0,1);
    lcd.print("                ");   // Clear second row
    lcd.setCursor(0,1);

    lcd.print("Pills:");
    lcd.print(pillsLeft);
    lcd.print("  ");
}

//================ Demo Buttons ====================
void checkButtons()
{
    if(digitalRead(BTN_9AM)==LOW)
    {
        hour=9;
        minute=0;
        second=0;
        delay(300);
    }

   else if(digitalRead(BTN_12PM)==LOW)
    {
        hour=12;
        minute=0;
        second=0;
        delay(300);
    }
   else if(digitalRead(BTN_4PM)==LOW)
{
    hour = 16;
    minute = 0;
    second = 0;
    delay(300);
}

   else if(digitalRead(BTN_8PM)==LOW)
{
    hour = 20;
    minute = 0;
    second = 0;
    delay(300);
}
}
void logEvent(String event)
{
    Serial.print(hour);
    Serial.print(":");

    if(minute < 10) Serial.print("0");
    Serial.print(minute);
    Serial.print(":");

    if(second < 10) Serial.print("0");
    Serial.print(second);

    Serial.print("  ");

    Serial.println(event);
}
void medicineReminder()
{
//============== 9:00 AM ==============
if(hour==9 && minute==0 && second==0 && !dose9AMDone)
{
    reminderActive = true;
    medicineTaken = false;

    reminderWeight = weight;
    reminderStart = millis();

    digitalWrite(RED_LED,HIGH);
    digitalWrite(BUZZER,HIGH);
    Serial.println();
    Serial.println("[09:00]  Reminder Started");
    logEvent("Reminder Started");

    dose9AMDone = true;
}

//============== 12:00 PM ==============
if(hour==12 && minute==0 && second==0 && !dose12PMDone)
{
    reminderActive = true;
    medicineTaken = false;

    reminderWeight = weight;
    reminderStart = millis();

    digitalWrite(RED_LED,HIGH);
    digitalWrite(BUZZER,HIGH);
     Serial.println("[12:00]  Reminder Started");
    logEvent("Reminder Started");

    dose12PMDone = true;
}

//============== 4:00 PM ==============
if(hour==16 && minute==0 && second==0 && !dose4PMDone)
{
    reminderActive = true;
    medicineTaken = false;

    reminderWeight = weight;
    reminderStart = millis();

    digitalWrite(RED_LED,HIGH);
    digitalWrite(BUZZER,HIGH);
    Serial.println("[16:00]  Reminder Started");
    logEvent("Reminder Started");

    dose4PMDone = true;
}

//============== 8:00 PM ==============
if(hour==20 && minute==0 && second==0 && !dose8PMDone)
{
    reminderActive = true;
    medicineTaken = false;

    reminderWeight = weight;
    reminderStart = millis();

    digitalWrite(RED_LED,HIGH);
    digitalWrite(BUZZER,HIGH);
    Serial.println("[20:00]   Reminder Started");
    logEvent("Reminder Started");

    dose8PMDone = true;
}
    // Medicine Taken
    if(reminderActive)
    {
     float removedWeight = reminderWeight - weight;

// Under Dose
if(removedWeight > 0 && removedWeight < 4)
{
    reminderActive = false;

    underDoseCount++;

    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER, HIGH);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Under Dose");

    lcd.setCursor(0,1);
    lcd.print("Take Full Dose");

    addHistory("Under Dose");
    showHistory();
    showDailyReport();

    Serial.println();
    Serial.println("WARNING: Under Dose");
    Serial.print("Removed : ");
    Serial.print(removedWeight);
    Serial.println(" g");

    logEvent("Under Dose");

    delay(3000);

    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);

    lcd.clear();
}

// Correct dose
else if(removedWeight >= 4 && removedWeight <= 6)
{
    reminderActive = false;
    medicineTaken = true;

    takenCount++;

    addHistory("Medicine Taken");
    showHistory();
    showDailyReport();

    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);
    digitalWrite(GREEN_LED, HIGH);

    Serial.println();
    Serial.println("Medicine Taken");
    Serial.print("Weight : ");
    Serial.print(weight);
    Serial.println(" g");

    Serial.print("Remaining Pills : ");
    Serial.println(pillsLeft);

    Serial.println();
    Serial.println("Dose Status : Correct");
    logEvent("Medicine Taken");
    Serial.print("Removed : ");
    Serial.print(removedWeight);
    Serial.println(" g");

    displayLCD();

    delay(3000);

    digitalWrite(GREEN_LED, LOW);
    medicineTaken = false;
    lcd.clear();
}

// Double dose
else if(removedWeight > 6 && removedWeight <= 12)
{
    reminderActive = false;
    doubleDoseCount++;

    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER, HIGH);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Double Dose");
    logEvent("Double Dose");

    lcd.setCursor(0,1);
    lcd.print("Be Careful");

    Serial.println("WARNING: Double dose detected");

    delay(3000);

    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);
    lcd.clear();
}

// Too many pills
else if(removedWeight > 12)
{
    reminderActive = false;
    tooManyPillsCount++;

    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER, HIGH);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Too Many");

    lcd.setCursor(0,1);
    lcd.print("Pills Removed");

    Serial.println("ALERT! Too many pills removed");
    logEvent("Overdose");

    delay(3000);

    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);
    lcd.clear();
}

        // 30 seconds timeout
        else if(millis()-reminderStart>30000)
        {
            reminderActive=false;

    digitalWrite(RED_LED,LOW);
    digitalWrite(BUZZER,LOW);

    waitingSMS=true;

    missedCount++;

    addHistory("Missed Dose");
    showHistory();
    showDailyReport();
    displayLCD();
    delay(3000);
    waitingSMS = false;
    lcd.clear();

    Serial.println();
    Serial.println("========== GSM ==========");
    Serial.println();
    Serial.println("Dose Missed");
    Serial.println("SMS Alert Sent");
    Serial.println("=========================");
    logEvent("Missed Dose");
            
        }
    }

    // Daily reset
 if(hour==0 && minute==0 && second==0)
{
    dose9AMDone = false;
    dose12PMDone = false;
    dose4PMDone = false;
    dose8PMDone = false;
}
}
void refillCheck()
{
    // Don't show refill while reminder or medicine taken is active
    if(reminderActive || medicineTaken || waitingSMS)
    {
        return;
    }

    // Show refill warning only once
    if(weight <= 75 && !refillSent)
    {
        refillSent = true;

    addHistory("Medicine Low");
    showHistory();
    showDailyReport();

    digitalWrite(YELLOW_LED,HIGH);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Refill Needed");

    lcd.setCursor(0,1);
    lcd.print("Medicine Low");

    Serial.println();
    Serial.println("========== GSM ==========");
    Serial.println();
    Serial.println("Medicine Stock Low");
    Serial.print("Remaining Pills : ");
    Serial.println(pillsLeft);
    Serial.println("Refill Required");
    Serial.println("SMS Alert Sent");
        Serial.println("=========================");

        delay(3000);

        lcd.clear();
    }

    // Reset refill flag after medicine is refilled
    if(weight > 75)
    {
        refillSent = false;
        digitalWrite(YELLOW_LED,LOW);
    }
}
void loop()
{
    updateClock();

    readWeight();

    checkButtons();

    medicineReminder();

    refillCheck();

    displayLCD();

    delay(300);
}