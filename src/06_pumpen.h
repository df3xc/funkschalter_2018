
#ifndef _pumpen_x
#define _pumpen_x

void poweron_pumpen_test();
int switch_pumpe_main (int new_state, int laufzeit); // pumpe ein- oder aus
int switch_pumpe_reserve (int new_state, int laufzeit);
int switch_pumpe_funk (int state, int laufzeit);  // pumpe ein- oder aus
void CountDown();
void TankFuellen(int level);
void BlumenGiessen(int now, int ts);


#endif
