/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "d:/funkschalter_2018/src/01_funk_schalter.ino"
/*---------------------------------------------------------------------
24.12.2016  : added serial terminal commands
6.July 2020 : added Reseve Pumpe. Platine mit Target erstellt und
             in Betrieb genommen.
---------------------------------------------------------------------*/
void setupWifi();
void init_control();
void setup();
void sleep(int minutes);
void loop();
void run_blynk();
void myDelay ( int seconds);
int getTime();
void printTnow(int tnow);
int getSleepTime(int target);
unsigned int convertAdcValue(unsigned int adc);
void readAdcChannels();
void printStatus();
void printSlowStatus();
void hwInit();
void deviceNameHandler(const char *topic, const char *data);
void myWebHookHandler(const char *event, const char *data);
int reportDontSleepPin();
int checkDontSleepPin();
int ledToggle(String command);
void help();
void timeStamp();
void println(char *text);
void println(String text);
void println(char *text, int data);
void println(char *text, String data);
#line 6 "d:/funkschalter_2018/src/01_funk_schalter.ino"
#pragma GCC diagnostic ignored "-Wwrite-strings"

#include <time.h>

//#define BLYNK_DEBUG // Uncomment this to see debug prints
#define BLYNK_PRINT Serial

//#include "BlynkSimpleParticle.h" // connot be included in more that one INO/CPP File !!!

#include "03_elro_switch.h"
#include "04_rsl_switch.h"
#include "05_ultrasonic.h"
#include "06_pumpen.h"
#include "07_wasserstand.h"
#include "08_database.h"
#include "01_funk_schalter.h"
#include "ds18x20_temperature.h"
#include "console.h"


SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

// Ersetze "... das Gedoens ..." mit dem Token aus der Email von Blynk
//char auth[] = "2a5e74b8eebd444b8261b5d928ab77e6";  // old Blynk before June 2022 (Blynk v.0.6.1)
//char auth[] = "Yb9r5XgNM5JWUJ69ga4plY81X8qLcjit";  // new Blynk since June 2022
String hwID;

int temp_in;
int temp_out;

char buffer[250];    // used by webhook / webrequest
char timebuffer[50]; // used by webhook / webrequest
int waterlevel = 1000;

int old_level = 0;
int new_level = 0;

int fast_counter = 0;  // used by LOOP
int slow_counter = 0;  // used by LOOP to trigger slow actions

int tnow;        // Uhrzeit in Minuten (24*hour+minute)
int tsec;        // sekunden
int tmain_stop;  // sekunden bis hauptpumpe abschaltet wird
int tres_stop;   // sekunden bis reserve pumpe abschaltet wird
int tfunk_stop;  // sekunden bis funk pumpe abgeschaltet wird

int ts_giessen = 8 * 60 + 1; // Uhrzeit Wasserpumpe einschalten
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


int termEnabled = 0;
int termCounter = 0;

uint dontSleepHW; // HW Pin =  1 -> do not enter sleep mode

struct control_struct control;


/*---------------------------------------------------------------------
change WiFi settings
---------------------------------------------------------------------*/

void setupWifi()
{
WiFi.on();

if(WiFi.hasCredentials())
  {
  WiFi.clearCredentials();
  }

WiFi.setCredentials("Stitzenburg", "BC$_clu$_4277");
Particle.connect();
}

/*---------------------------------------------------------------------
-> init EEPROM the first time
---------------------------------------------------------------------*/

void init_control()

{
 EEPROM.get(0, control);

 if ((control.pumpe_count_down < 30) | (control.pumpe_count_down > 240))
  {
    control.pumpe_count_down = 90;
    control.reserve_repetitions = 0;
    control.version = 1;
    EEPROM.put(0, control);
  }

  WriteToDatabase("RESET", "PUMPE MAIN COUNTDOWN IS ", control.pumpe_count_down);

}

/*---------------------------------------------------------------------
-> runs one time after power on or HW reset.
-> is not called on timer wake-up
---------------------------------------------------------------------*/
void setup()
{
  int day = 0;

  Serial.begin(115200);
  delay(2000); // Allow board to settle

  setupWifi();

  // Time.zone(+1); // Winterzeit
  Time.zone(+2); // Sommerzeit

  Serial.printlnf(" ");

  Serial.printlnf(" ");
  Serial.printlnf("----------------------");
  Serial.printlnf("      Welcome");
  Serial.printlnf("----------------------");
  hwID = System.deviceID();
  Serial.printlnf(hwID);
  

  delay(3000);

  WriteToDatabase("RESET", "#### SETUP/RESET Version ",SW_VERSION);

  init_control();

  if (Particle.connected() == true)
    {
    Serial.printlnf("Connected to Particle Cloud");

    Particle.subscribe("particle/device/name", deviceNameHandler);
    Particle.publish("particle/device/name");

    // Subscribe to the webHook integration response event
    Particle.subscribe("hook-response/waterControl", myWebHookHandler, MY_DEVICES);
    Particle.publish("waterControl", buffer, PRIVATE);
    }
    else
    {
     Serial.printlnf("WARNING : NOT Connected to Particle Cloud");     
    }

  hwInit();

  EEPROM.get(0, control);

  if ((control.pumpe_count_down < 30) | (control.pumpe_count_down > 240))
  {
    control.pumpe_count_down = 90;
    EEPROM.put(0, control);
  }

  WriteToDatabase("RESET", "PUMPE MAIN COUNTDOWN IS ", control.pumpe_count_down);

  if (control.dontGiessen == 1)
  {
    WriteToDatabase("RESET", "WASSERSTAND : BLUMEN GIESSEN IST DEAKTIVIERT ");
  }

  day = Time.weekday(); // North American implementation : Sunday is day number one, Monday is day numer two
  Serial.printlnf("Day of week : %d",day);

  printSlowStatus();

  help();

  tnow = getTime();

  fast_counter = 60000;
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

    digitalWrite(DO_PUMPE_MAIN, 0);
    digitalWrite(DO_PUMPE_RESERVE, 0);
    WriteToDatabase("WASSER", "#### SLEEP Minutes #### : ", minutes);
    delay(1000);

    //rsl_disable_receive();

    System.sleep(D1, RISING, 60 * minutes); // no reset of device after wakeup

    // After WAKE UP to SW continues execution here :

    println(" *** WAKE UP *** ");

    WiFi.on();
    delay(5000);
    Particle.connect(); 
    delay(5000);

    if (Particle.connected() == true)
    {
    WriteToDatabase("WAKE UP", "#### WAKE UP ####");
    WriteToDatabase("WAKE UP", "PUMPE MAIN COUNTDOWN IS ", control.pumpe_count_down);

    // Subscribe to the integration response event
    Particle.subscribe("particle/device/name", deviceNameHandler);
    Particle.subscribe("hook-response/waterControl", myWebHookHandler, MY_DEVICES);
    delay(500);
    Particle.publish("particle/device/name");
    delay(500);
    Particle.publish("waterControl", buffer, PRIVATE);
    }

    tnow = getTime();
    //rsl_disable_receive();
    hwInit();

    EEPROM.get(0, control);
    if ((control.pumpe_count_down < 30) | (control.pumpe_count_down > 240))
    {
      control.pumpe_count_down = 90;
      EEPROM.put(0, control);
    }

    if (control.dontGiessen == 1)
    {
      WriteToDatabase("WAKE UP", "WASSERSTAND : BLUMEN GIESSEN IST DEAKTIVIERT ");
    }

    get_Temperature(); 
    Serial.printlnf("temp in : %d  temp out : %d ", temp_in, temp_out);
    sprintf(timebuffer, "TEMP IN:%d OUT:%d", temp_in, temp_out);
    WriteToDatabase("WAKE UP", timebuffer);

    printSlowStatus();

    if ((Time.hour()>8) & (Time.hour()<11))
    {
      TankFuellen(LOW_LEVEL_TANKFUELLEN);
    }

    slow_counter = 0;
    fast_counter = 0;
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
  int utime;

  fast_counter++;
  delay(10);
  tnow = getTime();
  utime = Time.now();

  // this is just to test a huge amout of sleep cycles

  //if ((Time.minute() % 5) == 0)
  // {
  //   sleep(1);
  // }

  if ((utime % 2) == 0) // every 2 seconds
  {
    slow_counter++; 
    BlumenGiessen(0, ts_giessen);    
    CountDown();

    if (Time.minute() == 10)     // wir schlafen bis zur nächsten Stunde
    {
      if ((Time.hour() > 9) | (Time.hour() < 5))
      {
        sleep(60+getSleepTime(55));
      }
      else
      {
        sleep(getSleepTime(55));
      }
    }
    myDelay(1);
  }

  if ((slow_counter % 10) == 0) // every 20 seconds
  {
    //WriteToDatabase ( "CONTROL", "counter1 ",slow_counter);  
    printStatus();
    dontSleepHW = checkDontSleepPin();

    if (tnow == (5*60 + 1))
    {
      conrad_rsl_switch_code(RSL4,0);
      WriteToDatabase ( "CONTROL", "RSL4 abgeschaltet");      
    }

    if (tnow == (5*60 + 2))
    {
      conrad_rsl_switch_code(RSL4,1);
      WriteToDatabase ( "CONTROL", "RSL4 eingeschaltet");     
    }

    slow_counter++;
  } 

  if (slow_counter > 300) // once per 10 minutes
  {

    slow_counter = 0;
    printSlowStatus();

    if (tnow > (22*60) + 30)
    {
      done_giessen = 0; // armed for the next day
    }

  }
 
} // loop

/*
* run blynk
*/
void run_blynk()
{

}

void myDelay ( int seconds)
{
for (int i = 0; i<(20*seconds); i++)
  {
    delay(50);
  }
}


/*
* Return time as "minutes of day" = 60*hour + minutes
*/
int getTime()
{
  tsec = 60 * Time.minute() + Time.second();  
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

  timeStamp();


  println(" tnow: ", tnow);

  readAdcChannels();
  println("Main    [mV] : ", AiPumpeMain);
  println("Reserve [mV] : ", AiPumpeReserve);
  println("12V     [mV] : ", Ai12V);

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

  st_main_pumpe = digitalRead(DO_PUMPE_MAIN);

  if (st_main_pumpe == HIGH)
  {
    println("MAIN Pumpe is ON");
    WriteToDatabase("STATUS", "MAIN Pumpe is ON ");
  }

  st_reserve_pumpe = digitalRead(DO_PUMPE_RESERVE);

  if (st_reserve_pumpe == HIGH)
  {
    println("RESERVE Pumpe ist ON");
    WriteToDatabase("STATUS", "RESERVE Pumpe is ON ");
  }

  if (st_funk_pumpe == ON)
  {
    println("FUNK Pumpe ist ON");
    WriteToDatabase("STATUS", "FUNK Pumpe is ON ");
  }

  Serial.printlnf(" waterlevel: %d ", waterlevel);
  Serial.printlnf(" wifi=%s cloud=%s fast_counter=%d ", (wifiReady ? "on" : "off"), (cloudReady ? "on" : "off"), fast_counter);

}

/*---------------------------------------------------------------------
print status infos
---------------------------------------------------------------------*/
void printSlowStatus()

{
  int day = 0;

  timeStamp();


  getSleepTime(55); // time to wake up at minute = 55

  println(" Photon HW ID ", hwID);
    Particle.subscribe("particle/device/name", deviceNameHandler);
    Particle.publish("particle/device/name");
  delay(500);

  checkDontSleepPin();
  reportDontSleepPin();

  Particle.publish("waterControl", buffer, PRIVATE);

  EEPROM.get(0, control);

  println("version            : ", control.version);
  println("dontSleep          : ", control.dontSleepSW);
  println("dontGiessen        : ", control.dontGiessen);
  println("pumpe_count_down   : ", control.pumpe_count_down);
  println("reserve_repetitions: ", control.reserve_repetitions);

  day = Time.weekday(); // North American implementation : Sunday is day number one, Monday is day numer two
  println("day of week        : ", day);

  WriteToDatabase("WASSER","dontGiessen:", control.dontGiessen);
  WriteToDatabase("WASSER","pumpe count down:", control.pumpe_count_down);
  WriteToDatabase("WASSER","reserve_repetitions: ", control.reserve_repetitions);
  WriteToDatabase("WASSER","day of week: ", day);

   waterlevel = ultra_sonic_measure();
  WriteToDatabase("WASSER","WASSERSTAND : ",waterlevel);

   if (control.dontGiessen == 1)
  {
    WriteToDatabase("WASSER", "WASSERSTAND : BLUMEN GIESSEN IST DEAKTIVIERT ");
  }

  if(control.reserve_repetitions > MAX_NACHFUELL_REPETITIONS)
    {
        WriteToDatabase("WASSER", "WARNING : Wiederholungen des Tankfuellens ueberschritten = ",control.reserve_repetitions);    
        return;                   
    }  

  get_Temperature(); 

  println("temp in  : ", temp_in);
  println("temp out : ", temp_out);

  sprintf(timebuffer, "TEMP IN:%d OUT:%d", temp_in, temp_out);
  WriteToDatabase("CONTROL", timebuffer);
  println(" ----------------------- ");
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

  ts_giessen = 8 * 60 + 1; // zu dieser Zeit wird die Wasserpumpe eingeschaltet

  done_giessen = 0;
}

// Open a serial terminal and see the device name printed out
void deviceNameHandler(const char *topic, const char *data)
{
  println("received  : " + String(topic));
  println("received  : " + String(data));
  //WriteToDatabase("CONTROL", String(data));
}

// --------------------------------------------------------------
//  called if response from table "control" is recieved
// --------------------------------------------------------------
void myWebHookHandler(const char *event, const char *data)
{
  // Handle the integration response
  println("Webhook received <" + String(data) + ">");

  if (String(data).startsWith("off"))
  {
    control.dontGiessen = 1;
    EEPROM.put(0, control);
    WriteToDatabase("CONTROL", "WASSERSTAND : GIESSEN DEAKTIVIERT by WebHook");
  }

  if (String(data).startsWith("on"))
  {
    control.dontGiessen = 0;
    EEPROM.put(0, control);
    WriteToDatabase("CONTROL", "WASSERSTAND : GIESSEN AKTIVIERT by WebHook");
  }
}
/*---------------------------------------------------------------------
report to database if DontSleepPin is set
---------------------------------------------------------------------*/
int reportDontSleepPin()
{
  int dontSleep = 0;

  if (digitalRead(DONT_SLEEP_PIN) == HIGH)
  {
    dontSleepHW = 1;
    dontSleep = 1;
    WriteToDatabase("CONTROL", "SLEEP disabled by HW pin ");
  }

  if (control.dontSleepSW == 1)
  {
    dontSleep = 1;
    WriteToDatabase("CONTROL", "SLEEP disabled by SW ");
  }
  return (dontSleep);
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
  println(" a : Status");
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


}

/*---------------------------------------------------------------------
print to serial port and to Blynk terminal
---------------------------------------------------------------------*/

void println(String text)
{
  timeStamp();
  Serial.println(text);

}

/*---------------------------------------------------------------------
print to serial port and to Blynk terminal
---------------------------------------------------------------------*/
void println(char *text, int data)
{
  timeStamp();
  sprintf(buffer, "%s %d", text, data);

  Serial.println(buffer);

}

void println(char *text, String data)
{
  timeStamp();
  sprintf(buffer, "%s %d", text, data.c_str());

  Serial.println(buffer);

}

#pragma endregion















#pragma endregion
