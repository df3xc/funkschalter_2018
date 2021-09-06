/*--------------------------------------------------------------------
Sammlung von Funktionen um einen Conrad RSL FunkSchalter zu schalten

Getestet mit einem Particle Photon am 21.12.2015

Der Datenpin des 433MHz Senders wird mit dpin verbunden

Author : Dein grosser Bruder in Stuttgart

--------------------------------------------------------------------*/

/*--------------------------------------------------------------------

Die Tabelle (ein zwei-dimensionales array) codes:

Erste Spalte  : Codes zum ausschalten
Zweite Spalte : Codes zum einschalten
--------------------------------------------------------------------*/
// unsigned int codes[2][6] = {
//   { 0,
//     352633047,    // ID 1 : Wandschalter links aus
//     218415319,    // ID 2 : Wandschalter rechts aus
//     29369856,     // ID 3 : RSL1 : Fernbedienung  #1520000979 Gruppe 1 Taster 1 aus
//     784344576,    // ID 4 : RSL2 : Fernbedienung  #1520000979 Gruppe 1 Taster 2 aus
//     515909120 },  // ID 5 : RSL3 : Fernbedienung  #1520000979 Gruppe 1 Taster 3 aus
//   { 0,
//     419741911,    // ID 1 : Wandschalter links ein
//     84197591,     // ID 2 : Wandschalter rechts ein
//     247473664,    // ID 3 : RSL1 : Fernbedienung  #1520000979 Gruppe 1 Taster 1 ein
//     650126848,    // ID 4 : RSL2 :  Fernbedienung #1520000979 Gruppe 1 Taster 2 ein
//     381691392 }   // ID 5 : RSL3 : Fernbedienung  #1520000979 Gruppe 1 Taster 3 ein
// };

unsigned int codes[2][10] = {
	{ 0,          // erste Spalte
  2176853504,   // RSL1 Fernbedienung  #1520000979 Gruppe 1 Taster 1 aus
	2931828224,   // RSL2 Fernbedienung  #1520000979 Gruppe 1 Taster 2 aus
	2663392768,   // RSL3 Fernbedienung  #1520000979 Gruppe 1 Taster 3 aus
	2378180096,   // RSL4 Fernbedienung  #1520000979 Gruppe 2 Taster 1 aus
	2780833280,   // RSL5 Fernbedienung  #1520000979 Gruppe 2 Taster 2 aus
	2512397824,   // RSL6 Fernbedienung  #1520000979 Gruppe 2 Taster 3 aus	
	2747278848,   // RSL4,5,6 Fernbedienung  #1520000979 alle Dosen aller Gruppen aus
	2167330816,   // Luefter Dusche Fernbedienung  #1909000183 Gruppe 1 Taster 1 aus LUEFTER DUSCHE
  2922305536    // Licht Dusche Fernbedienung  #1909000183 Gruppe 1 Taster 2 aus LICHT DUSCHE
	},  
	{ 0,          // zweite Spalte
	2394957312,   // RSL1 Fernbedienung  #1520000979 Gruppe 1 Taster 1 ein
	2797610496,   // RSL2 Fernbedienung  #1520000979 Gruppe 1 Taster 2 ein
	2529175040,   // RSL3 Fernbedienung  #1520000979 Gruppe 1 Taster 3 ein 
	2243962368,   // RSL4 Fernbedienung  #1520000979 Gruppe 2 Taster 1 ein
	2847942144,   // RSL5 Fernbedienung  #1520000979 Gruppe 2 Taster 2 ein
	2579506688,   // RSL6 Fernbedienung  #1520000979 Gruppe 2 Taster 3 ein 
  2478843392,   // RSL 4,5,6 Fernbedienung  #1520000979 alle Dosen aller Gruppen ein
	2385434624,   // Luefter Dusche Fernbedienung  #1909000183 Gruppe 1 Taster 1 ein LUEFTER DUSCHE
	2788087808    // Licht Dusche Fernbedienung  #1909000183 Gruppe 1 Taster 2 ein LICHT DUSCHE
	}   
};


#define PHOTON

#ifdef PHOTON
  #include "application.h"
#else
  #include "Arduino.h"
  #include "Print.h"
#endif

#include "04_rsl_switch.h"

// Timings for Conrad Electronic Radio Wall Switch
#define ST    430   // short duration 500 usec
#define LO    1150  // long duration 1200 usec


// mit diesem Pin wird der sender gesteuert.
// this pin is used tx_code to control the RF transmitter

#ifdef PHOTON
int dpin = D0;
#else
int dpin = 10;
#endif


/*--------------------------------------------------------------------
Diese Funktion muss einmal von setup() aufgerufen werden
call this function once from setup()
--------------------------------------------------------------------*/
void conrad_rsl_init()  
{
  pinMode(dpin,OUTPUT);
  digitalWrite(dpin,LOW);
  delay(500);
}

/*------------------------------------------------------------------------------
CONRAD RSL toggle transmitter control using the given data array

param int* code:
The data array is a sequence of pulse timings.
The first value is the entire length of the sequenz

The next value sets transmitter pin HIGH
------------------------------------------------------------------------------*/
  void conrad_rsl_transmit (int* code)

  {
  int rep;
  int i;

   digitalWrite(dpin,LOW);
   delayMicroseconds(5000);

   digitalWrite(dpin,LOW);
   delayMicroseconds(5000);

   digitalWrite(dpin,LOW);
   delayMicroseconds(5000);

     // repeat sending the entire control data sequence

    for (rep=0; rep<8; rep++)
     {
       // send the bits from data array

     for (i=0; i < code[0]; i = i+4)
     {
         digitalWrite(dpin,HIGH);
         delayMicroseconds(code[i+1]);
         digitalWrite(dpin,LOW);
         delayMicroseconds(code[i+2]);

         digitalWrite(dpin,HIGH);
         delayMicroseconds(code[i+3]);
         digitalWrite(dpin,LOW);
         delayMicroseconds(code[i+4]);
     }

        // for RSL switch
    digitalWrite(dpin,HIGH);
    delayMicroseconds(ST);

    // repeat transmitting after given gap

    digitalWrite(dpin,LOW);
    delayMicroseconds(8000);
    }
  digitalWrite(dpin,LOW); // IMPORTANT : Switch transmitter OFF
  }


  /*--------------------------------------------------------------------
  create code sequence and send code sequenz

  code : code as long integer (4 byte)

  SHORT : dauer des kurzen Pulses in milliseconds
  LONG : dauer des langen Pulses in milliseconds
  --------------------------------------------------------------------*/
  void conrad_rsl_send(unsigned int code, int SHORT, int LONG)
  {
    int i = 0;
    int k = 1;
    int tx_code[70];  // speicher tx code sequenz

    Serial.println(code);

    memset(tx_code,0,64);
    tx_code[0]=64;   // die laenge des tx codes

    for(i=0; i<32; i++)
    {
      if ( (code & 0x80000000) == 0x80000000)
      {
        tx_code[k] = LONG;
        tx_code[k+1] = SHORT;
        Serial.print("1");
      }
      else
      {
        tx_code[k] = SHORT;
        tx_code[k+1] = LONG;
        Serial.print("0");
      }
      code = code << 1;
      k=k+2;
    }
    Serial.println(" ");
    conrad_rsl_transmit(&tx_code[0]);
  }

/*------------------------------------------------------------------------------
 Einen Funkschalter ein- oder ausschalten
------------------------------------------------------------------------------*/
  void conrad_rsl_switch_code ( int which, int state )
  {

    unsigned int code = 0;

    if (which>11)
    {
      Serial.println(" ERROR which not valid ");
    }

    if (state>1)
    {
      Serial.println(" ERROR state not valid ");
    }

    code = codes[state][which];

    if (code != 0)
    {
      Serial.print("Sende Code : ");
      Serial.println(code);
      conrad_rsl_send(code,430,1150);
    }
    else
    {
      Serial.print(" Fehler : konnte Code nicht finden ");
    }

  }
