#ifndef ultra_sonic_x
#define ultra_sonic_x

#define trigPin D3 // Trig pin on the HC-SR04
#define echoPin D4 // Echo pin on the HC-SR04

 void ultra_sonic_setup();
 uint32_t ultra_sonic_ping();
 uint32_t uCentimeters(uint32_t microseconds);
 uint32_t uMilli(uint32_t microseconds);
 uint32_t uInches(uint32_t microseconds);

 #endif
