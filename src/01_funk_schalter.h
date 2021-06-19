
#ifndef _funk_x
#define _funk_x

#define SW_VERSION 20

#define ON 1
#define OFF 0

#define DO_PUMPE_MAIN D1     // die Haupt-Wasserpumpe
#define DO_PUMPE_RESERVE D2  // die Pumpe im Reservetank
#define DONT_SLEEP_PIN D6    // GPIO D6
#define BLYNK_LED D7

#define TROCKENLAUF_SCHUTZ_LEVEL 70
#define LOW_LEVEL_TANKFUELLEN 95
#define RESERVE_PUMPE_LAUFZEIT  90
#define MAX_NACHFUELL_REPETITIONS    5

struct control_struct {
  unsigned int version;
  unsigned int dontSleepSW;         // 1 = do not enter sleep Mode
  unsigned int dontGiessen;         // 1 = do not giessen
  unsigned int pumpe_count_down;    // zeitdauer in sekunden
  unsigned int reserve_repetitions; // Anzahl Tank nachfüllen
};

extern struct control_struct control;
extern int waterlevel;

extern int st_main_pumpe;
extern int st_reserve_pumpe;
extern int st_funk_pumpe;
extern int old_level;
extern int new_level;
extern int main_countDown;
extern int reserve_countDown;
extern int funk_countDown;

extern unsigned int AiPumpeMain;
extern unsigned int AiPumpeReserve;
extern unsigned int Ai12V;

extern char buffer[];     // used by webhook / webrequest
extern char timebuffer[];  // used by webhook / webrequest

extern int tnow;
extern int tsec;
extern int tmain_stop;
extern int tres_stop; 
extern int tfunk_stop;
extern int termEnabled;
extern int termCounter;

extern int temp_in;
extern int temp_out;

extern int ts_giessen;
extern int done_giessen;
extern int slow_counter;

 void run_blynk();

 void help();
 void timeStamp(void);
 void println( char* text);
 void println( char* text, int data);
 void println(char *text, String data);
 int getTime();
 void reportToLuefter(char* text);
 void BlumenGiessen(int now, int ts);
 int  ultra_sonic_measure(void);
 void sleep(int minutes);
 int getSleepTime(int target);
 void readAdcChannels();
 void printSlowStatus();

#endif
