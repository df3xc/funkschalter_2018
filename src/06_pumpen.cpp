/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "d:/funkschalter_2018/src/06_pumpen.ino"

#include "01_funk_schalter.h"
#include "08_database.h"
#include "04_rsl_switch.h"



/*---------------------------------------------------------------------
Pumpe des Wassertanks einschalten bzw. ausschalten
---------------------------------------------------------------------*/
int switch_pumpe_main(int new_state, int laufzeit);
int switch_pumpe_reserve(int new_state, int laufzeit);
int switch_pumpe_funk(int newState, int laufzeit);
void CountDown();
void BlumenGiessen(int now, int ts);
void TankFuellen(int critical_level);
#line 11 "d:/funkschalter_2018/src/06_pumpen.ino"
int switch_pumpe_main(int new_state, int laufzeit) // pumpe ein- oder aus
{
    Serial.printlnf(" --------------------------------------------- ");
    Serial.printlnf(" MAIN Pumpe schalten : %d Zeit %d", new_state, laufzeit);

    pinMode(DO_PUMPE_MAIN, OUTPUT);
    st_main_pumpe = digitalRead(DO_PUMPE_MAIN);

    if ((new_state == ON) & (st_main_pumpe == OFF))
    {
        WriteToDatabase("WASSER", "BLUMEN GIESSEN BEGINNT  ");
        old_level = ultra_sonic_measure();
        WriteToDatabase("WASSER", "WASSERSTAND before pumpe ON : ", old_level);

        if (old_level < TROCKENLAUF_SCHUTZ_LEVEL)
        {
            WriteToDatabase("WASSER", "WASSERSTAND TROCKEN-LAUF-SCHUTZ. PUMPE NICHT EINSCHALTEN");
            return (st_main_pumpe);
        }

        tnow = getTime();
        tmain_stop = tsec + laufzeit;
        digitalWrite(DO_PUMPE_MAIN, 1);
        WriteToDatabase("WASSER", "HAUPT-PUMPE EINGESCHALTET : ", main_countDown);
    }

    if ((new_state == OFF) & (st_main_pumpe == ON))
    {
        digitalWrite(DO_PUMPE_MAIN, 0);
        main_countDown = 0;
        WriteToDatabase("WASSER", "HAUPT-PUMPE AUSGESCHALTET ");
        new_level = ultra_sonic_measure();
        WriteToDatabase("WASSER", "WASSERSTAND after pumpe OFF : ", new_level);
        WriteToDatabase("WASSER", "WASSERSTAND gefallen um [mm] : ", old_level - new_level);

        if ((old_level - new_level) > 3)
        {
            WriteToDatabase("WASSER", "WASSERSTAND BLUMEN GIESSEN ERFOLGREICH BEENDET");
        }
    }

    delay(250);
    st_main_pumpe = digitalRead(DO_PUMPE_MAIN);
    Serial.printlnf(" DoPumpeMain    : %d ", st_main_pumpe);
    readAdcChannels();
    Serial.printlnf(" AiPumpeMain    : %d [mV]", AiPumpeMain);

    WriteToDatabase("ADC", "AiPumpeMain [mV] : ", AiPumpeMain);
    Serial.printlnf(" --------------------------------------------- ");

    return (st_main_pumpe);
}

/*---------------------------------------------------------------------
Pumpe des zweiten Wassertanks einschalten bzw. ausschalten
---------------------------------------------------------------------*/
int switch_pumpe_reserve(int new_state, int laufzeit)

{
    Serial.printlnf(" --------------------------------------------- ");
    Serial.printlnf(" RESERVE Pumpe schalten : %d Zeit %d", new_state, laufzeit);

    pinMode(DO_PUMPE_RESERVE, OUTPUT);
    st_reserve_pumpe = digitalRead(DO_PUMPE_RESERVE);

    if ((new_state == ON) & (st_reserve_pumpe == OFF))
    {
        old_level = ultra_sonic_measure();
        WriteToDatabase("WASSER", "WASSERSTAND before res-pumpe ON : ", old_level);
        digitalWrite(DO_PUMPE_RESERVE, 1);
        WriteToDatabase("WASSER", "RESERVE-PUMPE EINGESCHALTET ");

        tnow = getTime();
        tres_stop = tsec + laufzeit;
     }

    if ((new_state == OFF) & (st_reserve_pumpe == ON))
    {
        digitalWrite(DO_PUMPE_RESERVE, 0);
        WriteToDatabase("WASSER", "RESERVE-PUMPE AUSGESCHALTET. FUELLUNG NR. ",control.reserve_repetitions);
        new_level = ultra_sonic_measure();
        WriteToDatabase("WASSER", "WASSERSTAND after res-pumpe OFF : ", new_level);

        if ((new_level - old_level) > 5)
        {
            WriteToDatabase("WASSER", " WASSERSTAND TANK FUELLEN ERFOLGREICH BEENDET");
        }
        else
        {
            WriteToDatabase("WASSER", " WASSERSTAND ERROR: TANK FUELLEN fehlgeschlagen");
        }
    }

    delay(250);
    st_reserve_pumpe = digitalRead(DO_PUMPE_RESERVE);
    Serial.printlnf(" DoPumpeReserve    : %d ", st_reserve_pumpe);
    readAdcChannels();
    Serial.printlnf(" AiPumpeReserve    : %d [mV]", AiPumpeReserve);

    WriteToDatabase("ADC", "AiPumpeReserve [mV] : ", AiPumpeReserve);
    Serial.printlnf(" --------------------------------------------- ");

    return (st_reserve_pumpe);
}

/*---------------------------------------------------------------------
Funk-Pumpe eines weiteren Wassertanks einschalten bzw. ausschalten
---------------------------------------------------------------------*/
int switch_pumpe_funk(int newState, int laufzeit) // pumpe ein- oder aus

{
    if (newState == ON)
    {
        conrad_rsl_switch_code(4, EIN); // Dosen-Label RSL2
        st_funk_pumpe = ON;
        WriteToDatabase("WASSER", "FUNK-PUMPE RSL2 EINGESCHALTET ");
        tfunk_stop = tsec + laufzeit;
    }
    else
    {
        conrad_rsl_switch_code(4, AUS); // Dosen-Label RSL2
        st_funk_pumpe = OFF;
        tfunk_stop = 4000;
        WriteToDatabase("WASSER", "FUNK-PUMPE RSL2 AUSGESCHALTET ");
    }
    return (newState);
}

/*---------------------------------------------------------------------
CountDown Pumpen timer and switch off the pumpen if countDown = 0:
---------------------------------------------------------------------*/
void CountDown()
{
    tnow = getTime();

    st_main_pumpe = digitalRead(DO_PUMPE_MAIN);
    
    if (st_main_pumpe == ON)
    {
        println("tsec", tsec);
        println("tmain stop", tmain_stop);
    }

    if ((st_main_pumpe == ON) & (tsec > tmain_stop ))
    {
        tmain_stop = 4000;
        switch_pumpe_main(OFF, 0);
        TankFuellen(LOW_LEVEL_TANKFUELLEN);
    }

     st_reserve_pumpe = digitalRead(DO_PUMPE_RESERVE);   

    if ((st_reserve_pumpe == ON) & (tsec > tres_stop))
    {
        tres_stop = 4000;
        control.reserve_repetitions++;
        EEPROM.put(0, control);            
        switch_pumpe_reserve(OFF, 0);
    }

    if ((st_funk_pumpe == ON) & (tsec > tfunk_stop ))
    {
        tfunk_stop = 4000;
        //WriteToDatabase("COUNTDOWN","Pumpe Funk CountDown : ", funk_countDown);
        switch_pumpe_funk(OFF, 0);
    }
}

/*---------------------------------------------------------------------
Blumengiessen
now = 1 : jetzt sofort Giessen
now = 0 : giesse wenn tnow = ts zur geplanten Zeit
done_giessen muß 0 sein
---------------------------------------------------------------------*/
void BlumenGiessen(int now, int ts)
{
   
    if (done_giessen == 1) return;
    
    if (tnow == ts || now == 1)
    {
        if (control.dontGiessen == 1)
        {
            WriteToDatabase("WASSER", "WASSERSTAND : BLUMEN GIESSEN IST DEAKTIVIERT ");
            done_giessen = 1;
            return;
        }

        st_main_pumpe = switch_pumpe_main(ON, control.pumpe_count_down);

        // Blaue COMET Pumpe, 3 dünne Schläuche , Verteiler Typ "3"
        // 150 sekunden laufzeit ergibt 0.5L

        // BLaue Comet Pumpe, 4mm Schlauch, ohne Verteiler
        // 10 Sekunden ergibt 0.5L

        st_funk_pumpe = switch_pumpe_funk(OFF,0);
        delay(1000);
        st_funk_pumpe = switch_pumpe_funk(ON, 10);
        done_giessen = 1;
    }
}

/*---------------------------------------------------------------------
Tank fuellen -> schalte reserve pumpe ein wenn der Wasserstand 
kleiner als der critical level ist
---------------------------------------------------------------------*/
void TankFuellen(int critical_level)
{
    println(" Tankfuellen() ");

    waterlevel = ultra_sonic_measure();

    if (waterlevel > critical_level)
    {
            WriteToDatabase("WASSER", " TANKFUELLEN : NICHT NOTWENDIG ");
            return;        
    }

    if(control.reserve_repetitions > MAX_NACHFUELL_REPETITIONS)
    {
        WriteToDatabase("WASSER", "ERROR - ABORT: Wiederholungen des Tankfuellens ueberschritten = ",control.reserve_repetitions);    
        return;                   
    }   

    if ((waterlevel > 0) & (waterlevel < critical_level) & (st_reserve_pumpe == OFF))
    {

        if (control.dontGiessen == 1)
        {
            WriteToDatabase("WASSER", "WASSERSTAND TANKFUELLEN : BLUMEN GIESSEN IST DEAKTIVIERT ");
            return;
        }

        WriteToDatabase("WASSER", "WASSERSTAND TANK FUELLEN gestartet ");
        st_reserve_pumpe = switch_pumpe_reserve(1, RESERVE_PUMPE_LAUFZEIT);
    }
}
