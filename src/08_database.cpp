

#include "application.h"
#include "01_funk_schalter.h"

/*---------------------------------------------------------------------
Write to my database at www.carstenlueck.de
---------------------------------------------------------------------*/
void WriteToDatabase ( char* status,  char* option)
{
    if (Particle.connected() == false)
    {
        Serial.printlnf("### WARNING : not connected to cloud. cannot write to database");
        return;
    }    
    sprintf(buffer,"{\"status\":\"%s\",\"time\":\"%.2d:%.2d:%.2d\",\"date\":\"%.2d.%.2d.%.2d\",\"luefter\":\"PHOTON\",\"option\":\"%s\"}", \
                    status,Time.hour(),Time.minute(),Time.second(),Time.day(),Time.month(),Time.year(),option);

#ifdef debug_db   
    Serial.printlnf(buffer);
#endif

    Particle.publish("wasserstand", buffer, PRIVATE);
    println(option);
    delay(2000);
}


/*---------------------------------------------------------------------
Write to my database at www.carstenlueck.de
---------------------------------------------------------------------*/
void WriteToDatabase ( char* status,  char* option, int data)
{
    if (Particle.connected() == false)
    {
        Serial.printlnf("### WARNING : not connected to cloud. cannot write to database");
        return;
    }

    

    sprintf(buffer,"{\"status\":\"%s\",\"time\":\"%.2d:%.2d:%.2d\",\"date\":\"%.2d.%.2d.%.2d\",\"luefter\":\"PHOTON\",\"option\":\"%s %2d\"}", \
                    status,Time.hour(),Time.minute(),Time.second(),Time.day(),Time.month(),Time.year(),option,data);

#ifdef debug_db   
    Serial.printlnf(buffer);
#endif

    Particle.publish("wasserstand", buffer, PRIVATE);
    sprintf(buffer," %s %d ", option, data);
    println(option,data);
    delay(2000);
}


/*---------------------------------------------------------------------
send text to WCW Luefter using TCP Client
---------------------------------------------------------------------*/
void reportToLuefter(char* text)

{
//   int done = 0;
//   int count = 0;
//
//     terminal.println(text);
//     terminal.flush();
//
//     while(done==0)
//     {
//       client.connect(server,40300);
//       count++;
//       delay(200);
//
//       if (client.connected() == true)
//       {
//         client.println(text);
//         println(text);
//         done=1;
//         delay(500);
//       }
//       client.stop();
//
//       if (count>5)
//       {
//         println(" ERR: no Luefter ");
//         done = 1;
//       }
//     }
}
