
#pragma GCC diagnostic ignored "-Wwrite-strings"

#include "application.h"
#include "01_funk_schalter.h"
#include "04_rsl_switch.h"
#include "ds18x20_temperature.h"
#include "08_database.h"

/*---------------------------------------------------------------------
Dispatch command
---------------------------------------------------------------------*/

void dispatchCommand(char c)
{

  switch (c)
  {
  case 'h':
    termEnabled = 1;
    termCounter = 10;
    help();
    break;

    // case '0':
    //   digitalWrite(DO_PUMPE_RESERVE, 0);
    //   digitalWrite(DO_PUMPE_MAIN, 0);
    //   println(" MAIN und RESERVE Pumpe ausgeschaltet  ");
    //   break;

    // case '1':
    //   digitalWrite(DO_PUMPE_RESERVE, 1);
    //   digitalWrite(DO_PUMPE_MAIN, 1);
    //   println(" MAIN und RESERVE Pumpe eingeschaltet  ");
    //   break;

  case 'i':
    if (control.pumpe_count_down < 40)
    {
      control.pumpe_count_down = control.pumpe_count_down + 4;
    }
    else
    {
      control.pumpe_count_down = 40;
    }

    EEPROM.put(0, control);
    WriteToDatabase("control", "PUMPE MAIN COUNTDOWN NOW ", control.pumpe_count_down);
    break;

  case 'd':
    if (control.pumpe_count_down > 6)
    {
      control.pumpe_count_down = control.pumpe_count_down - 4;
    }
    else
    {
      control.pumpe_count_down = 6;
    }
    EEPROM.put(0, control);
    WriteToDatabase("control", "PUMPE MAIN COUNTDOWN NOW ", control.pumpe_count_down);
    break;

  case 'a':
    printSlowStatus();
    break;

  case 'b':
    BlumenGiessen(1, ts_giessen);
    break;

  case 's':
    control.dontSleepSW = 0;
    println(" Sleep Mode enabled ");
    EEPROM.put(0, control);
    break;

  case 'u':
    println(" Clear reserve repetition counter "); 
    control.reserve_repetitions=0;
    EEPROM.put(0, control);  
    break;

  case 'p':
    control.dontSleepSW = 1;
    println(" Sleep Mode disabled ");
    EEPROM.put(0, control);
    break;

  case 't':
    get_Temperature();
    sprintf(timebuffer, "TEMP IN:%d OUT:%d", temp_in, temp_out);
    println(timebuffer);
    break;

  case 'w':
    println(" Store CONTROL in EEPROM ");
    EEPROM.put(0, control);
    break;

  case 'x':
    println(" Ultrasonic ");
    ultra_sonic_measure();
    break;

    case 'y':
    println(" Prepare Sleep ");
    sleep(getSleepTime(55));
    break;  

  case 'z':
    println(" Photon HW ID ", System.deviceID() );
    Particle.publish("particle/device/name");
    delay(500); 
  break;  
  }
}

/*---------------------------------------------------------------------
Dispatch characters from serial port
---------------------------------------------------------------------*/
void serialEvent()
{
  char c = Serial.read();
  dispatchCommand(c);
}
