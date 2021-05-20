
#include "application.h"
#include "05_ultrasonic.h"
#include "08_database.h"
#include "01_funk_schalter.h"

#define TankHoehe 360  // Höhe Sensor über dem Grund des Behälters in mm
#define MessFehler 15  // distanz ist um 15 mm zu groß

#define ULTRA_LOOP 3
// array used to store up ultrasonic measurements
uint32_t hs[ULTRA_LOOP];

/*---------------------------------------------------------------------
Ultrasonic distance measurement (tested29.07.2017)
Returns water level in millimeter
---------------------------------------------------------------------*/
int  ultra_sonic_measure()

{
  uint32_t time;
  uint32_t dist = 0;
  uint32_t avg = 0;

  int level = 0;
  int error = 0;
  int k = 0;

  Serial.printlnf ( " ultra sonic distance ");

  for (k=0; k<ULTRA_LOOP; k++)
  {
      time = ultra_sonic_ping();
      dist = uMilli(time);
      hs[k] = dist;
      Serial.printlnf ( " ultra distance : %d mm", hs[k]);
      run_blynk();
      delay(150);
  }

  avg = 0;
  for (k=0; k<ULTRA_LOOP; k++)
  {
    avg = avg + hs[k];
  }
  avg = avg / ULTRA_LOOP;

  for (k=0; k<ULTRA_LOOP; k++)
  {
    error = error + (avg-hs[k]);
  }

  if (avg==0)
  {
    level = 1000; // prevent that the second pump is switched on
    WriteToDatabase ( "WASSER", "WARNING: Failed to read ultrasonic sensor");  
    return(1000);
  }
  
  avg = avg - MessFehler;

  Serial.printlnf ( " ultra error : %d ", error);
  Serial.printlnf ( " ultra distance average : %d mm", avg);

  println("distance : ",avg);

  level = TankHoehe - avg;

  if (level < 0 ) level = -1;

  Serial.printlnf ( " ultra level average : %d mm", level);
  run_blynk();

  //WriteToDatabase("WASSER","WASSERSTAND : ",level);

  return(level);

}