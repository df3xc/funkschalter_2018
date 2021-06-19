/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "d:/funkschalter_2018/src/05_ultrasonic.ino"
/**
 ******************************************************************************
 * @file     photon-desk.cpp
 * @authors  TJ Hunter
 * @version  V1.0.0
 * @date     22-Oct-2015
 * @brief    Adjust the height of your geekdesk over WiFi using a Particle Photon
 ******************************************************************************
  Copyright (c) 2015 TJ Hunter.  All rights reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */


#include "05_ultrasonic.h"

 void ultra_sonic_setup();
uint32_t ultra_sonic_ping();
uint32_t uInches(uint32_t microseconds);
uint32_t uCentimeters(uint32_t microseconds);
uint32_t uMilli(uint32_t microseconds);
#line 26 "d:/funkschalter_2018/src/05_ultrasonic.ino"
void ultra_sonic_setup()
 {
   	// Initialize pins for sensor
   	pinMode(trigPin, OUTPUT);
   	digitalWriteFast(trigPin, LOW);
   	delay(50);
 }


 uint32_t ultra_sonic_ping()
 {
   	uint32_t duration;
   	pinMode(echoPin, INPUT);
   	pinMode(trigPin, OUTPUT);

   	// The sensor is triggered by a HIGH pulse of 10 or more microseconds.
   	digitalWriteFast(trigPin, HIGH);
   	delayMicroseconds(10);
   	digitalWriteFast(trigPin, LOW);

   	duration = pulseIn(echoPin, HIGH); // Time in microseconds to recieve a ping back on the echo pin

   	return duration;
 }

 uint32_t uInches(uint32_t microseconds)
 {
     // According to Parallax's datasheet for the PING))), there are
     // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
     // second).  This gives the distance travelled by the ping, outbound
     // and return, so we divide by 2 to get the distance of the obstacle.
     // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
     return ( microseconds / 74 / 2);
 }

 uint32_t uCentimeters(uint32_t microseconds)
 {
     // The speed of sound is 340 m/s or 29 microseconds per centimeter.
     // The ping travels out and back, so to find the distance of the
     // object we take half of the distance travelled.
     return (microseconds / 29 / 2);
 }

uint32_t uMilli(uint32_t microseconds)
{
     // The speed of sound is 340 m/s or 29 microseconds per centimeter.
     // The ping travels out and back, so to find the distance of the
     // object we take half of the distance travelled.
     return ( 10 * microseconds / 29 / 2);
}
