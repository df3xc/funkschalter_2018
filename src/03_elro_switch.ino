/*--------------------------------------------------------------------
Sammlung von Funktionen um einen ELRO FunkSchalter zu schalten

Getestet mit einem Particle Photon am 21.12.2015

Der Datenpin des 433MHz Senders wird mit datapin verbunden

Author : Dein grosser Bruder in Stuttgart

--------------------------------------------------------------------*/

#define EIN 1
#define AUS 0

// this pin is used to control the RF transmitter

int datapin = D0;

/*--------------------------------------------------------------------
Diese Funktion muss einmal von setup() aufgerufen werden
call this function once from setup()
--------------------------------------------------------------------*/
void elro_init()
{
  pinMode(datapin,OUTPUT);
  digitalWrite(datapin,LOW);
}

/*--------------------------------------------------------------------
toggle transmitter data using the given data array
--------------------------------------------------------------------*/
  void elro_transmit (int* data)

  {
	   int rep;
	   int i;

 	   digitalWrite(datapin,LOW);
	   delayMicroseconds(5000);

	   digitalWrite(datapin,LOW);
	   delayMicroseconds(5000);

	   digitalWrite(datapin,LOW);
	   delayMicroseconds(5000);

	   // repeat sending the entire control data sequence

	  for (rep=0; rep<8; rep++)
		 {
			 // send the bits from data array

			 for (i=0; i < data[0]; i = i+2)
			 {
			   digitalWrite(datapin,HIGH);
			   delayMicroseconds(data[i+1]);
			   digitalWrite(datapin,LOW);
			   delayMicroseconds(data[i+2]);
			 }

	    delayMicroseconds(9900);
  		}
    digitalWrite(datapin,LOW);
  }

/*--------------------------------------------------------------------
create code sequenz from string and send code sequenz
--------------------------------------------------------------------*/
void elro_send(char* code, int SHORT, int LONG)
{
	int i = 0;
	int k = 1;
  int ed[90];  // tx code sequenz

	ed[0]=50;

	for(i=0; i<24; i++)
	{
		if (code[i]==0x30)
		{
			ed[k] = SHORT;
			ed[k+1] = LONG;
		}
		else
		{
			ed[k] = LONG;
			ed[k+1] = SHORT;
		}
		k=k+2;
	}
	ed[k] = SHORT;
	ed[k+1] = LONG;
  elro_transmit(&ed[0]);
}


/*--------------------------------------------------------------------
Control ELRO rc switch
--------------------------------------------------------------------*/
void elro_switch ( int which, int state )
{
	char es[34];

	if (which==1)
	{
		if (state==AUS)  strncpy(es,"000100010000010101010010",24);
		if (state==EIN)  strncpy(es,"000100010000010101010001",24);
   	elro_send(es,220,940);
	}

	if (which==2)
	{
		if (state==AUS)  strncpy(es,"000100010001000101010100",24);
		if (state==EIN)  strncpy(es,"000100010001000101010001",24);
   	elro_send(es,220,940);
	}

	if (which==3)
	{
		if (state==AUS)  strncpy(es,"000100010001010001010010",24);
		if (state==EIN)  strncpy(es,"000100010001010001010001",24);
   	elro_send(es,220,940);
	}

}

/*--------------------------------------------------------------------
Control intertechno rc switch
--------------------------------------------------------------------*/
void intertechno_switch ( int which, int state )
{
	char es[34];

	if (which==1)
	{
		if (state==EIN)  strncpy(es,"010100010000000000010101",24);
		if (state==AUS)  strncpy(es,"010100010000000000010100",24);
	  elro_send(es,310,1150);
	}

	if (which==2)
	{
		if (state==EIN)  strncpy(es,"010100010100000000010101",24);
		if (state==AUS)  strncpy(es,"010100010100000000010100",24);
	  elro_send(es,310,1150);
	}

}
