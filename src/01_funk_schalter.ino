/*---------------------------------------------------------------------
24.12.2016  : added serial terminal commands
6.July 2020 : added Reseve Pumpe. Platine mit Target erstellt und
             in Betrieb genommen.
---------------------------------------------------------------------*/
#pragma GCC diagnostic ignored "-Wwrite-strings"

#include <time.h>

//#define BLYNK_DEBUG // Uncomment this to see debug prints
#define BLYNK_PRINT Serial

#include "BlynkSimpleParticle.h" // connot be included in more that one INO/CPP File !!!

#include "03_elro_switch.h"
#include "04_rsl_switch.h"
#include "05_ultrasonic.h"
#include "06_pumpen.h"
#include "07_wasserstand.h"
#include "08_database.h"
#include "01_funk_schalter.h"
#include "ds18x20_temperature.h"
#include "console.h"

// Ersetze "... das Gedoens ..." mit dem Token aus der Email von Blynk
char auth[] = "2a5e74b8eebd444b8261b5d928ab77e6";
String hwID;

int temp_in;
int temp_out;

char buffer[250];    // used by webhook / webrequest
char timebuffer[50]; // used by webhook / webrequest
int waterlevel = 1000;

int old_level = 0;
int new_level = 0;

int fast_counter = 0; // used by LOOP
//int slow_counter = 0;     // used by LOOP to trigger slow actions

int tnow; // Uhrzeit in Minuten (24*hour+minute)#

int ts_giessen = 0; // Uhrzeit Wasserpumpe einschalten
int done_giessen = 0;

int st_main_pumpe = 0;    // status Haupt-Wasserpumpe
int st_funk_pumpe = 0;    // status Funk-Wasserpumpe
int st_reserve_pumpe = 0; // status der resereve tank st_funk_pumpe

int main_countDown = 0;
int reserve_countDown = 0;
int funk_countDown = 0;

unsigned int AiPumpeMain = 0;
unsigned int AiPumpeReserve = 0;
unsigned int Ai12V = 0;

//int level = 0;

int termEnabled = 0;
int termCounter = 0;

uint dontSleepHW; // HW Pin =  1 -> do not enter sleep mode

struct control_struct control;

// Ein BLYNK APP Terminal an virtual pin V22
WidgetTerminal terminal(V22);

/*---------------------------------------------------------------------
-> runs one time after power on or HW reset.
-> is not called on timer wake-up
---------------------------------------------------------------------*/
void setup()
{
  Serial.begin(115200);
  delay(2000); // Allow board to settle

  // Time.zone(+1); // Winterzeit
  Time.zone(+2); // Sommerzeit

  Serial.printlnf(" ");

  Serial.printlnf(" ");
  Serial.printlnf("----------------------");
  Serial.printlnf("      Welcome");
  Serial.printlnf("----------------------");
  hwID = System.deviceID();
  Serial.printlnf(hwID);

  Particle.subscribe("particle/device/name", deviceNameHandler);
  Particle.publish("particle/device/name");

  // Subscribe to the webHook integration response event
  Particle.subscribe("hook-response/waterControl", myWebHookHandler, MY_DEVICES);
  Particle.publish("waterControl", buffer, PRIVATE);

  Blynk.begin(auth);

// TODO: use SW version as define 

  WriteToDatabase("RESET", "#### SETUP/RESET Version ",SW_VERSION);
  hwInit();

  EEPROM.get(0, control);

  if ((control.pumpe_count_down < 4) | (control.pumpe_count_down > 40))
  {
    control.pumpe_count_down = 16;
    EEPROM.put(0, control);
  }

  WriteToDatabase("CONTROL", "PUMPE MAIN COUNTDOWN IS ", control.pumpe_count_down);

  if (control.dontGiessen == 1)
  {
    WriteToDatabase("WASSER", "WASSERSTAND : BLUMEN GIESSEN IST DEAKTIVIERT ");
  }

  help();

  tnow = getTime();

  fast_counter = 36000;
  termEnabled = 0;
}

/*---------------------------------------------------------------------
Sleep and Wakeup
---------------------------------------------------------------------*/
void sleep(int minutes)
{
  if (minutes < 1)
  {
    minutes = 5;
  }

  if (minutes > 120)
  {
    minutes = 120;
  }

  if ((dontSleepHW == 0) & (control.dontSleepSW == 0) & (st_funk_pumpe == 0))
  {
    //System.sleep(SLEEP_MODE_DEEP,2*60); // will reset device after wakeup

    println(" *** PREPARE SLEEP *** ");

    // sollte nicht vorkommen, dass Pumpen noch laufen.
    // Besser schalten wir die Pumpen hier trotzdem aus
    st_main_pumpe = switch_pumpe_main(OFF, 0);
    st_reserve_pumpe = switch_pumpe_reserve(OFF, 0);
    st_funk_pumpe = switch_pumpe_funk(OFF, 0);

    delay(1000);
    WriteToDatabase("WASSER", "#### SLEEP Minutes #### : ", minutes);
    delay(1000);

    //rsl_disable_receive();

    System.sleep(D1, RISING, 60 * minutes); // no reset of device after wakeup

    // After WAKE UP to SW continues execution here :

    println(" *** WAKE UP *** ");

    WriteToDatabase("WASSER", "#### WAKE UP ####");
    WriteToDatabase("control", "PUMPE MAIN COUNTDOWN IS ", control.pumpe_count_down);

    // Subscribe to the integration response event
    Particle.subscribe("particle/device/name", deviceNameHandler);
    Particle.subscribe("hook-response/waterControl", myWebHookHandler, MY_DEVICES);
    delay(500);
    Particle.publish("particle/device/name");
    delay(500);
    Particle.publish("waterControl", buffer, PRIVATE);

    tnow = getTime();
    //rsl_disable_receive();
    hwInit();

    EEPROM.get(0, control);
    if ((control.pumpe_count_down < 4) | (control.pumpe_count_down > 40))
    {
      control.pumpe_count_down = 16;
      EEPROM.put(0, control);
    }

    if (control.dontGiessen == 1)
    {
      WriteToDatabase("WASSER", "WASSERSTAND : BLUMEN GIESSEN IST DEAKTIVIERT ");
    }

    get_Temperature(); // Temperature sensors not yet installed
    Serial.printlnf("temp in : %d  temp out : %d ", temp_in, temp_out);
    sprintf(timebuffer, "TEMP IN:%d OUT:%d", temp_in, temp_out);
    WriteToDatabase("WASSER", timebuffer);

    if ((Time.hour()>8) & (Time.hour()<11))
    {
      TankFuellen(LOW_LEVEL_TANKFUELLEN);
    }

    fast_counter = 36000;
    termEnabled = 0;
  }
}

/*---------------------------------------------------------------------
The main loop runs forever
---------------------------------------------------------------------*/
void loop()
{
  unsigned long rf_code;
  int minutes;

  Blynk.run();

  fast_counter++;

  // this is just to test a huge amout of sleep cycles

  //if ((Time.minute() % 5) == 0)
  // {
  //   sleep(1);
  // }

  if ((fast_counter % 1600) == 0) // every 5 seconds
  {
    CountDown();

    // wir schlafen bis zur nächsten Stunde
    if (Time.minute() == 9)
    {
      sleep(getSleepTime(57));
    }
  }

  if ((fast_counter % 5100) == 0) // every 15 seconds
  {
    tnow = getTime();
    //WriteToDatabase ( "CONTROL", "FASTCOUNTER");
    printStatus();
    dontSleepHW = checkDontSleepPin();
    BlumenGiessen(0, ts_giessen);

  } // if fast_counter

  if ((fast_counter % 80000) == 0) // once per 4 minutes
  {
    printSlowStatus();

    if (termCounter > 0)
    {
      termCounter--;
      if (termCounter == 0)
      { 
        println("Terminal disabled");      
        termEnabled == 0;
        WriteToDatabase("CONTROL","TERMINAL disabled by timeout");
      }
    }
  }


} // loop

/*
* run blynk
*/
void run_blynk()
{
  Blynk.run();
}

/*
* Return time as "minutes of day" = 60*hour + minutes
*/
int getTime()
{
  return (Time.hour() * 60 + Time.minute());
}

/*
* Print "minutes of day " as hour:minutes
 */
void printTnow(int tnow)
{
  int hour = 0;
  int min = 0;

  hour = tnow / 60;
  min = tnow - (hour * 60);
  Serial.printlnf(" (%02d:%02d)", hour, min);
}

int getSleepTime(int target)
{
  target = target - Time.minute();
  if (target < 0)
    target = target + 60;
  Serial.printlnf(" proposed sleep duration: %d minutes", target);
  return (target);
}

#pragma region ADC

/*---------------------------------------------------------------------
read and convert ADC channels
---------------------------------------------------------------------*/

unsigned int convertAdcValue(unsigned int adc)
{
  unsigned int val = 0;
  val = 3300 * adc;
  val = val * 43 / 40960;
  return (val);
}

void readAdcChannels()
{
  AiPumpeMain = convertAdcValue(analogRead(A0));
  AiPumpeReserve = convertAdcValue(analogRead(A2));
  Ai12V = convertAdcValue(analogRead(A1));
}

#pragma endregion

/*---------------------------------------------------------------------
print status infos
---------------------------------------------------------------------*/
void printStatus()
{
  bool wifiReady;
  bool cloudReady;

  wifiReady = WiFi.ready();
  cloudReady = Particle.connected();

  println(" tnow: ", tnow);
  printTnow(tnow);
  Serial.printf(" ts_giessen: %d ", ts_giessen);
  printTnow(ts_giessen);

  checkDontSleepPin();

  readAdcChannels();
  println("Main    [mV] : ", AiPumpeMain);
  println("Reserve [mV] : ", AiPumpeReserve);
  println("12V     [mV] : ", Ai12V);

  st_main_pumpe = digitalRead(DO_PUMPE_MAIN);

  if (st_main_pumpe == HIGH)
  {
    println("MAIN Pumpe is ON");
  }

  st_reserve_pumpe = digitalRead(DO_PUMPE_RESERVE);

  if (st_reserve_pumpe == HIGH)
  {
    println("RESERVE Pumpe ist ON");
  }

  Serial.printlnf(" waterlevel: %d ", waterlevel);
  Serial.printlnf(" wifi=%s cloud=%s fast_counter=%d ", (wifiReady ? "on" : "off"), (cloudReady ? "on" : "off"), fast_counter);


}

/*---------------------------------------------------------------------
print status infos
---------------------------------------------------------------------*/
void printSlowStatus()

{
  timeStamp();

  if (termEnabled == 1)
    terminal.println(timebuffer);

  getSleepTime(55); // time to wake up at minute = 55

  println(" Photon HW ID ", hwID);
  Particle.publish("particle/device/name");
  delay(500);

  Particle.publish("waterControl", buffer, PRIVATE);

  EEPROM.get(0, control);

  println("version         :  ", control.version);
  println("dontSleep       :  ", control.dontSleepSW);
  println("dontGiessen     :  ", control.dontGiessen);
  println("pumpe_count_down:  ", control.pumpe_count_down);

  reportDontSleepPin();

  waterlevel = ultra_sonic_measure();

  readAdcChannels();

  if (AiPumpeMain > 1000)
  {
    Serial.printlnf(" AiPumpeMain    : %d [mV]", AiPumpeMain);
    WriteToDatabase("ADC", "AiPumpeMain    [mV] : ", AiPumpeMain);
  }

  if (AiPumpeReserve > 1000)
  {
    Serial.printlnf(" AiPumpeReserve : %d [mV]", AiPumpeReserve);
    WriteToDatabase("ADC", "AiPumpeReserve [mV] : ", AiPumpeReserve);
  }

  if (control.dontGiessen == 1)
  {
    WriteToDatabase("WASSER", "WASSERSTAND : BLUMEN GIESSEN IST DEAKTIVIERT ");
  }

  get_Temperature(); 

  println("temp in  : ", temp_in);
  println("temp out : ", temp_out);

  sprintf(timebuffer, "TEMP IN:%d OUT:%d", temp_in, temp_out);
  WriteToDatabase("CONTROL", timebuffer);
  println(" slow actions done ");
}

/*---------------------------------------------------------------------
Hardware Initialisation
---------------------------------------------------------------------*/
void hwInit()
{
  conrad_rsl_init(); // 433 MHz Sender abschalten

  ultra_sonic_setup();

  pinMode(DO_PUMPE_MAIN, OUTPUT);
  pinMode(DO_PUMPE_RESERVE, OUTPUT);

  digitalWrite(DO_PUMPE_RESERVE, 0);
  digitalWrite(DO_PUMPE_MAIN, 0);

  st_funk_pumpe = switch_pumpe_main(OFF, 0);
  st_reserve_pumpe = switch_pumpe_reserve(OFF, 0);

  pinMode(BLYNK_LED, OUTPUT);
  pinMode(DONT_SLEEP_PIN, INPUT);

  EEPROM.get(0, control);

 ts_giessen = 7 * 60 + 1; // zu dieser Zeit wird die Wasserpumpe eingeschaltet

  done_giessen = 0;
}

// Open a serial terminal and see the device name printed out
void deviceNameHandler(const char *topic, const char *data)
{
  println("received  : " + String(topic));
  println("received  : " + String(data));
}

// --------------------------------------------------------------
//  called if response from table "control" is recieved
// --------------------------------------------------------------
void myWebHookHandler(const char *event, const char *data)
{
  // Handle the integration response
  println("received  : " + String(data));

  if ((String(data).startsWith("off")) & (control.dontGiessen == 0))
  {
    control.dontGiessen = 1;
    EEPROM.put(0, control);
    WriteToDatabase("CONTROL", "WASSERSTAND : GIESSEN JETZT DEAKTIVIERT");
  }

  if ((String(data).startsWith("on")) & (control.dontGiessen == 1))
  {
    control.dontGiessen = 0;
    EEPROM.put(0, control);
    WriteToDatabase("CONTROL", "WASSERSTAND : GIESSEN JETZT AKTIVIERT");
  }
}
/*---------------------------------------------------------------------
report to database if DontSleepPin is set
---------------------------------------------------------------------*/
int reportDontSleepPin()

{
  if (digitalRead(DONT_SLEEP_PIN) == HIGH)
  {
    dontSleepHW = 1;
    WriteToDatabase("CONTROL", "SLEEP disabled by HW pin ");
  }

  if (control.dontSleepSW == 1)
  {
    WriteToDatabase("CONTROL", "SLEEP disabled by SW ");
  }
  return (dontSleepHW);
}

/*---------------------------------------------------------------------
check if DontSleepPin is set
---------------------------------------------------------------------*/
int checkDontSleepPin()
{
  if (digitalRead(DONT_SLEEP_PIN) == HIGH)
  {
    dontSleepHW = 1;
  }
  else
  {
    dontSleepHW = 0;
  }

  Serial.printlnf(" dontSleep pin : %d ", dontSleepHW);
  return (dontSleepHW);
}

/*---------------------------------------------------------------------
---------------------------------------------------------------------*/

int ledToggle(String command)
{

  /* Spark.functions always take a string as an argument and return an integer.
    Since we can pass a string, it means that we can give the program commands on how the function should be used.
    */

  int com;

  com = atoi(command);

  switch (com)
  {
  case 1:
    conrad_rsl_switch_code(3, EIN);
    digitalWrite(BLYNK_LED, HIGH);
    return 1;
    break;

  case 2:
    conrad_rsl_switch_code(3, AUS);
    digitalWrite(BLYNK_LED, LOW);
    return 1;
    break;

  case 3:
    digitalWrite(BLYNK_LED, HIGH);
    return 1;
    break;

  case 4:
    digitalWrite(BLYNK_LED, LOW);
    return 1;
    break;

  case 5:
    BlumenGiessen(1, ts_giessen);
    return 1;
    break;

  default:
    return -1;
  }
}

#pragma region print_to_serial_port

void help()
{
  println(" Hello ");
  println(" b : Blumen giessen");
  println(" d : Zeitdauer Giessen verringern");
  println(" i : Zeitdauer Giessen erhoehen");
  println(" s : sleep enabled ");
  println(" p : sleep disabled ");
  println(" u : clear reserve repetition counter ");  
  println(" w : store config to EEPROM ");
  println(" y : goto sleep ");  
  println(" x : ultra sonic measurement ");
}

/*---------------------------------------------------------------------
print timestamp to serial port
---------------------------------------------------------------------*/
void timeStamp()
{
  sprintf(timebuffer, " %.2d-%.2d-%d %.2d:%.2d:%.2d ",
          Time.day(),
          Time.month(),
          Time.year(),
          Time.hour(),
          Time.minute(),
          Time.second());

  Serial.print(timebuffer);

/*   if (termEnabled == 1)
  {
    terminal.print(timebuffer); // Ausgabe an BLYNK APP terminal
    terminal.flush();
  }  */
}

/*---------------------------------------------------------------------
print to serial port and to Blynk terminal
---------------------------------------------------------------------*/
void println(char *text)
{
  timeStamp();
  Serial.println(text);

  if (termEnabled == 1)
  {
    terminal.println(text); // Ausgabe an BLYNK APP terminal
    terminal.flush();
  }
}

/*---------------------------------------------------------------------
print to serial port and to Blynk terminal
---------------------------------------------------------------------*/

void println(String text)
{
  timeStamp();
  Serial.println(text);

  if (termEnabled == 1)
  {
    terminal.println(text); // Ausgabe an BLYNK APP terminal
    terminal.flush();
  }
}

/*---------------------------------------------------------------------
print to serial port and to Blynk terminal
---------------------------------------------------------------------*/
void println(char *text, int data)
{
  timeStamp();
  sprintf(buffer, "%s %d", text, data);

  Serial.println(buffer);

  if (termEnabled == 1)
  {
    terminal.println(buffer); // Ausgabe an BLYNK APP terminal
    terminal.flush();
  }
}

void println(char *text, String data)
{
  timeStamp();
  sprintf(buffer, "%s %d", text, data.c_str());

  Serial.println(buffer);

  if (termEnabled == 1)
  {
    terminal.println(buffer); // Ausgabe an BLYNK APP terminal
    terminal.flush();
  }
}

#pragma endregion

#pragma region blynk_buttons

/*---------------------------------------------------------------------
BLYNK Terminal
---------------------------------------------------------------------*/

BLYNK_WRITE(V22)
{
  // send it back
  Blynk.virtualWrite(V22, "\nYou said:", param.asStr());
  char c;
  c = *param.asStr();
  dispatchCommand(c);
}

/*---------------------------------------------------------------------
BLYNK Buttons Vx.
---------------------------------------------------------------------*/

BLYNK_WRITE(V1)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    println(" rsl 1 ein ");
    conrad_rsl_switch_code(1, EIN);
    Blynk.virtualWrite(V1, 255);
  }
}

BLYNK_WRITE(V2)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    println(" rsl 1 aus ");
    conrad_rsl_switch_code(1, AUS);
    Blynk.virtualWrite(V2, 255);
  }
}

BLYNK_WRITE(V3)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    println(" rsl 2 ein ");
    conrad_rsl_switch_code(2, EIN);
    Blynk.virtualWrite(V20, 255);
  }
}

BLYNK_WRITE(V4)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    timeStamp();
    println(" rsl 2 aus ");
    conrad_rsl_switch_code(2, AUS);
    Blynk.virtualWrite(V20, 0);
  }
}

BLYNK_WRITE(V5)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    println(" rsl 3 ein ");
    conrad_rsl_switch_code(3, EIN);
    Blynk.virtualWrite(V20, 255);
  }
}

BLYNK_WRITE(V6)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    println(" rsl 3 aus ");
    conrad_rsl_switch_code(3, AUS);
    Blynk.virtualWrite(V20, 0);
  }
}

BLYNK_WRITE(V7)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    BlumenGiessen(1, ts_giessen);
    //st_funk_pumpe = switch_pumpe_funk(ON,3);
    //conrad_rsl_switch_code(4,EIN);
    Blynk.virtualWrite(V20, 255);
  }
}

BLYNK_WRITE(V8)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    st_funk_pumpe = switch_pumpe_funk(OFF, 0);
    st_main_pumpe = switch_pumpe_main(OFF, 0);
    Blynk.virtualWrite(V20, 0);
  }
}

BLYNK_WRITE(V9)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    println(" rsl 5 ein ");
    conrad_rsl_switch_code(5, EIN);
    Blynk.virtualWrite(V20, 255);
  }
}

BLYNK_WRITE(V10)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    println(" rsl 5 aus ");
    conrad_rsl_switch_code(5, AUS);
    Blynk.virtualWrite(V20, 0);
  }
}

BLYNK_WRITE(V11)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    println(" weihnachten ein  ");
    elro_switch(1, EIN);
    elro_switch(2, EIN);
    elro_switch(3, EIN);
    Blynk.virtualWrite(V20, 255);
  }
}

BLYNK_WRITE(V12)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    println(" weihnachten aus  ");
    elro_switch(1, AUS);
    elro_switch(2, AUS);
    elro_switch(3, AUS);
    Blynk.virtualWrite(V20, 0);
  }
}

BLYNK_WRITE(V13)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    println(" Terminal disabled  ");
    termEnabled = 0;
    termCounter = 0;
    WriteToDatabase("CONTROL","TERMINAL disabled by Blynk Button");
  }
}

BLYNK_WRITE(V14)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    termEnabled = 1;
    println(" Terminal enabled  ");
    termCounter = 10;
    WriteToDatabase("CONTROL","TERMINAL enabled by Blynk Button");   
  }
}

BLYNK_WRITE(V25)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    control.dontSleepSW = 1;
    EEPROM.put(0, control);
    WriteToDatabase("CONTROL", "SLEEP DISABLED BY BLYNK BUTTON ");
    println(" Sleep Mode Disabled by blynk button ");
  }
}

BLYNK_WRITE(V26)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    control.dontSleepSW = 0;
    EEPROM.put(0, control);
    WriteToDatabase("CONTROL", "SLEEP ENABLED BY BLYNK BUTTON ");
    println(" Sleep Mode Enabled by blynk button ");
  }
}

BLYNK_WRITE(V27)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    TankFuellen(250);
    WriteToDatabase("CONTROL", "Tankfuellen by BLYNK button ");
    println(" Reserve Pumpe eingeschaltet by button  ");
  }
}

BLYNK_WRITE(V28)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    st_reserve_pumpe = switch_pumpe_reserve(OFF, 0);
    WriteToDatabase("CONTROL", "Reserve Pumpe off by BLYNK button ");
    println(" Reserve Pumpe ausgeschaltet by button ");
  }
}

BLYNK_WRITE(V29)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    control.dontGiessen = 1;
    EEPROM.put(0, control);
    WriteToDatabase("CONTROL", "Blumen giessen deaktiviert by Blynk Button ");
    println("Blumen giessen deaktiviert ");
  }
}

BLYNK_WRITE(V30)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    control.dontGiessen = 0;
    EEPROM.put(0, control);
    WriteToDatabase("CONTROL", "Blumen giessen aktiviert by Blynk Button ");
    println("Blumen giessen aktiviert ");
  }
}

BLYNK_WRITE(V31)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    st_main_pumpe = switch_pumpe_main(ON, control.pumpe_count_down);
    WriteToDatabase("CONTROL", "Main Pumpe by button ");
    println(" Main Pumpe eingeschaltet by button  ");
  }
}

BLYNK_WRITE(V32)
{
  if (param.asInt() == 1) // Schalter nieder gedrückt ?
  {
    st_main_pumpe = switch_pumpe_main(OFF, 0);
    WriteToDatabase("CONTROL", "Main Pumpe ausgeschaltet by button ");
    println(" Main Pumpe ausgeschaltet by button ");
  }
}

#pragma endregion
