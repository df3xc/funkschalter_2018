/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "d:/funkschalter_2018/src/ds18x20_temperature.ino"
/*
Use this sketch to read the temperature from 1-Wire devices
you have attached to your Particle device (core, p0, p1, photon, electron)

Temperature is read from: DS18S20, DS18B20, DS1822, DS2438

A pull-up resistor is required on the signal line. The spec calls for a 4.7K.
I have used 1K-10K depending on the bus configuration and what I had out on the
bench. If you are powering the device, they all work. If you are using parasitic
power it gets more picky about the value.

*/

#include "ds18.h"
#include "ds18x20_temperature.h"
#include "01_funk_schalter.h"

int get_Temperature();
void printDebugInfo();
#line 18 "d:/funkschalter_2018/src/ds18x20_temperature.ino"
#define SENSOR_PIN D5  // data line of sensor

uint8_t get_ID();

DS18 sensor(SENSOR_PIN);  // create sensor instance

/*----------------------------------------------------------------
read all temperature sensors
-----------------------------------------------------------------*/
int get_Temperature()
{

  int k = 0;
  int id = 0;

Serial.println(" Get Temperature ");

do
  {
    if (sensor.read()) {
      // Do something cool with the temperature
      id = get_ID();
      Serial.printf("Sensor ID %d num=%d Temperature %.2f C  \n\r", id,k,sensor.celsius());
      println("sensor id",id);
      
      k++;
      //Particle.publish("temperature", String(sensor.celsius()), PRIVATE);
      delay(250);

      if(id == 37) {
        temp_in = sensor.celsius();
      }

      if(id == 151){
        temp_out = sensor.celsius();
      }

    }

  } while(sensor.searchDone() == false);

return(sensor.celsius());

}


uint8_t get_ID()
{
  if (sensor.crcError()) {
    Serial.print(" CRC Error ");
    return(0);
  }

  // Use ROM address 7 as ID
  uint8_t addr[8];
  uint8_t id;
  sensor.addr(addr);
  id = addr[7];
  //Serial.printf(" ID=%02X",id);
  return(id);
}

void printDebugInfo() {
  // If there's an electrical error on the 1-Wire bus you'll get a CRC error
  // Just ignore the temperature measurement and try again
  if (sensor.crcError()) {
    Serial.print("CRC Error ");
  }

  // // Print the sensor type
  // const char *type;
  // switch(sensor.type()) {
  //   case WIRE_DS1820: type = "DS1820"; break;
  //   case WIRE_DS18B20: type = "DS18B20"; break;
  //   case WIRE_DS1822: type = "DS1822"; break;
  //   case WIRE_DS2438: type = "DS2438"; break;
  //   default: type = "UNKNOWN"; break;
  // }
  // Serial.print(type);

  // Print the ROM (sensor type and unique ID)
  uint8_t addr[8];
  sensor.addr(addr);
  Serial.printf(
    " ROM=%02X%02X%02X%02X%02X%02X%02X%02X",
    addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]
  );

  // Use ROM address 7 as ID
  //uint8_t addr[8];
  uint8_t id=0;
  sensor.addr(addr);
  id = addr[7];
  Serial.printf(" ID=%02X",addr[7]);

  // // Print the raw sensor data
  // uint8_t data[9];
  // sensor.data(data);
  // Serial.printf(
  //   " data=%02X%02X%02X%02X%02X%02X%02X%02X%02X",
  //   data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]
  // );
}
