/*
 * MIP3_AVR_minutka_casovac_Vavrinak.cpp
 *
 * Created: 25.04.2025 8:19:57
 * Author : admin
 */ 
#define F_CPU 16000000UL	// nastavení frekvence cpu, nutné pro fugování zpoždìní

#include <avr/io.h>			// knihovna pro práci s I/O porty
#include <util/delay.h>		// knihovna pro zpožìní
#include <avr/interrupt.h>	// knihovna pro ovládání pøerušení
#include <stdint.h>			// knihovna pro celoèíselné datové typy s pevnou délkou

volatile uint8_t priznak_sekundy = 0;	// nastaví se na 1 pøi uplynutí 1s
uint8_t i_poz = 0;	// aktuální pozice segmentu pro zobrazení
uint8_t minuty = 0;	// uchovává aktuální stav minut	
uint8_t sekundy = 0; // uchovává aktuální stav sekund
bool nastavovani = false;	// stav nastavovaní odpoètu
bool bezi_odpocet = false;	// utrèuje zda-li bìží odpoèet
bool blik = false;	// aktivuje se pokud skonèí odpoèet, spouští blíkání LED

// pole obsahuje zápis znakù pro zobrazení na 7-seg displeji
const uint8_t mapa_znaku[] = {
	0b11000000,		// 0
	0b11111001,		// 1
	0b10100100,		// 2
	0b10110000,		// 3
	0b10011001,		// 4
	0b10010010,		// 5
	0b10000010,		// 6
	0b11111000,		// 7
	0b10000000,		// 8
	0b10010000,		// 9
	0b11111111		// nic
};
// uchovává možné poèet 7-segemntù
const uint8_t pozice[] = {1, 2, 4, 8};

// Zobrazí aktuální èíslo z mapy na požadované pozici
void zobraz_cislo(uint8_t poz, uint8_t cis)
{
	PORTD = ~(pozice[poz]);
	PORTA = mapa_znaku[cis];
}

// Uchovává hodnoty, který jsou možné pro vrátit po stisku klávesy
const uint8_t mapa_klaves[4][4] =
{
	{1, 2, 3, 10},		// (S13) 1 2 3 A (S1)
	{4, 5, 6, 11},		//		 4 5 6 B
	{7, 8, 9, 12},		//		 7 8 9 C
	{13, 0, 14, 15}		// (S16) * 0 # D (S4)
};

// Funkce prochází všechny sloupce a ktivuje na nich 0
// následì prochází všechny øádky a pokud najde schodu
// vrátí stisknutou klávesu podle mapy, pokud žádná není stisknuta vrací 99
int cti_klavesnici() 
{	
	for (int s = 0; s < 4; s++)
	{
		PORTC = ~(1 << s);
		_delay_ms(1);
		for (int r = 0; r < 4; r++)
		{
			if (((~PINC)&(16 << r))>0 )
			{
				_delay_ms(1);
				return mapa_klaves[r][s];
			}
		}
	}
	return 99; // není nic stisknuto
}

// Funkce rozdìlí minuty a sekundy a zobrazí je na jednotlivých segmentech
ISR(TIMER0_OVF_vect) {
	switch (i_poz)
	{
		// pozice nejvíce v pravo
		case 0:
		zobraz_cislo(0, sekundy %10);
		break;
		case 1:
		zobraz_cislo(1, (sekundy/10) %10);
		break;
		case 2:
		zobraz_cislo(2, minuty %10);
		break;
		// pozice nejvíce v levo
		case 3:
		zobraz_cislo(3, (minuty/10) %10);
		break;
	}
	// postupnì prochází všechny pozice 1-4
	i_poz++;
	if (i_poz > 3)
	{
		i_poz = 0;
	}
}

// Jakmile upline 1s nastaví se pžíznak na 1
ISR(TIMER1_COMPA_vect) {
	priznak_sekundy = 1;
}

int main(void)
{
    DDRA = 0b11111111; // segmenty
	DDRD = 0b00001111; // pozice
	DDRC = 0b00001111; // klavesnic vstup/výstup
	PORTC = 0b11110000; // klavesnice pull-up
	DDRB = 0b11111111;	// ledky
	PORTB = 0b11111111;	// ledky pull-up
	
	// nastavení èasovaèe 0
	TCCR0 = (1 << CS02);
	TIMSK |= (1 << TOIE0);
	// nastavení èasovaèe 1
	TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
	OCR1A = 15624;
	TIMSK |= (1 << OCIE1A);
	// povolení pøerušení
	sei();
	
    while (1) 
    {	
		// Povolí nastavovani
		if (cti_klavesnici() == 10) // A
		{
			nastavovani = true;
			while (cti_klavesnici() != 99);	// øeší aby nedocházelo k opìtovnému stisku pøi držení
		}
		
		// Zakaze nastavovani
		if (cti_klavesnici() == 11) // B
		{
			nastavovani = false;
			while (cti_klavesnici() != 99);	// øeší aby nedocházelo k opìtovnému stisku pøi držení
		}

		if (nastavovani == true)
		{
			// inkrementace minut - 1
			if (cti_klavesnici() == 1)
			{
				minuty++;
				while (cti_klavesnici() != 99);	// øeší aby nedocházelo k opìtovnému stisku pøi držení
			}
			// inkrementace sekund - 3
			if (cti_klavesnici() == 3)
			{
				sekundy++;
				while (cti_klavesnici() != 99);	// øeší aby nedocházelo k opìtovnému stisku pøi držení
			}
			// nastavení nul
			if (cti_klavesnici() == 0)
			{
				minuty = 0;
				sekundy = 0;
				while (cti_klavesnici() != 99);	// øeší aby nedocházelo k opìtovnému stisku pøi držení
			}
		}
		
		// povolí spuštìní odpoètu pokud není v režimu nastavování
		if (nastavovani == false)
		{
			if (cti_klavesnici() == 15) //D - SUPSTENI
			{
				bezi_odpocet = true;

				while (cti_klavesnici() != 99);	// øeší aby nedocházelo k opìtovnému stisku pøi držení
			}

			if (cti_klavesnici() == 12) //C - POZASTAVENI
			{
				bezi_odpocet = false;
				while (cti_klavesnici() != 99);	// øeší aby nedocházelo k opìtovnému stisku pøi držení
			}
		}
		
		// logika odpoètu
		if (bezi_odpocet == true)
		{
			while(priznak_sekundy)
			{
				sekundy--;
				priznak_sekundy = 0;
			}
			if ((sekundy > 60) & (minuty > 0))
			{
				minuty--;
			}
			if ((sekundy > 60))
			{
				sekundy = 59;
			}
			if ((sekundy == 0) & (minuty == 0))
			{
				bezi_odpocet = false;
				blik = true;
			}
		}
		
		// signalizuje odpoèet blikáním LEDek
		while (blik == true)
		{
			PORTB = 0b00000000;
			_delay_ms(100);
			PORTB = 0b11111111;
			_delay_ms(100);

			if (cti_klavesnici() < 20)	// bliká dokud nebude cokoliv stisknuto
			{
				blik = false;
			}
		}	
		

    }
}

