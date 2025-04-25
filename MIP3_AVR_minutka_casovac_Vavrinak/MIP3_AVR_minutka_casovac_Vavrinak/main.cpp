/*
 * MIP3_AVR_minutka_casovac_Vavrinak.cpp
 *
 * Created: 25.04.2025 8:19:57
 * Author : admin
 */ 
#define F_CPU 16000000UL	// nastaven� frekvence cpu, nutn� pro fugov�n� zpo�d�n�

#include <avr/io.h>			// knihovna pro pr�ci s I/O porty
#include <util/delay.h>		// knihovna pro zpo��n�
#include <avr/interrupt.h>	// knihovna pro ovl�d�n� p�eru�en�
#include <stdint.h>			// knihovna pro celo��seln� datov� typy s pevnou d�lkou

volatile uint8_t priznak_sekundy = 0;	// nastav� se na 1 p�i uplynut� 1s
uint8_t i_poz = 0;	// aktu�ln� pozice segmentu pro zobrazen�
uint8_t minuty = 0;	// uchov�v� aktu�ln� stav minut	
uint8_t sekundy = 0; // uchov�v� aktu�ln� stav sekund
bool nastavovani = false;	// stav nastavovan� odpo�tu
bool bezi_odpocet = false;	// utr�uje zda-li b�� odpo�et
bool blik = false;	// aktivuje se pokud skon�� odpo�et, spou�t� bl�k�n� LED

// pole obsahuje z�pis znak� pro zobrazen� na 7-seg displeji
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
// uchov�v� mo�n� po�et 7-segemnt�
const uint8_t pozice[] = {1, 2, 4, 8};

// Zobraz� aktu�ln� ��slo z mapy na po�adovan� pozici
void zobraz_cislo(uint8_t poz, uint8_t cis)
{
	PORTD = ~(pozice[poz]);
	PORTA = mapa_znaku[cis];
}

// Uchov�v� hodnoty, kter� jsou mo�n� pro vr�tit po stisku kl�vesy
const uint8_t mapa_klaves[4][4] =
{
	{1, 2, 3, 10},		// (S13) 1 2 3 A (S1)
	{4, 5, 6, 11},		//		 4 5 6 B
	{7, 8, 9, 12},		//		 7 8 9 C
	{13, 0, 14, 15}		// (S16) * 0 # D (S4)
};

// Funkce proch�z� v�echny sloupce a ktivuje na nich 0
// n�sled� proch�z� v�echny ��dky a pokud najde schodu
// vr�t� stisknutou kl�vesu podle mapy, pokud ��dn� nen� stisknuta vrac� 99
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
	return 99; // nen� nic stisknuto
}

// Funkce rozd�l� minuty a sekundy a zobraz� je na jednotliv�ch segmentech
ISR(TIMER0_OVF_vect) {
	switch (i_poz)
	{
		// pozice nejv�ce v pravo
		case 0:
		zobraz_cislo(0, sekundy %10);
		break;
		case 1:
		zobraz_cislo(1, (sekundy/10) %10);
		break;
		case 2:
		zobraz_cislo(2, minuty %10);
		break;
		// pozice nejv�ce v levo
		case 3:
		zobraz_cislo(3, (minuty/10) %10);
		break;
	}
	// postupn� proch�z� v�echny pozice 1-4
	i_poz++;
	if (i_poz > 3)
	{
		i_poz = 0;
	}
}

// Jakmile upline 1s nastav� se p��znak na 1
ISR(TIMER1_COMPA_vect) {
	priznak_sekundy = 1;
}

int main(void)
{
    DDRA = 0b11111111; // segmenty
	DDRD = 0b00001111; // pozice
	DDRC = 0b00001111; // klavesnic vstup/v�stup
	PORTC = 0b11110000; // klavesnice pull-up
	DDRB = 0b11111111;	// ledky
	PORTB = 0b11111111;	// ledky pull-up
	
	// nastaven� �asova�e 0
	TCCR0 = (1 << CS02);
	TIMSK |= (1 << TOIE0);
	// nastaven� �asova�e 1
	TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
	OCR1A = 15624;
	TIMSK |= (1 << OCIE1A);
	// povolen� p�eru�en�
	sei();
	
    while (1) 
    {	
		// Povol� nastavovani
		if (cti_klavesnici() == 10) // A
		{
			nastavovani = true;
			while (cti_klavesnici() != 99);	// �e�� aby nedoch�zelo k op�tovn�mu stisku p�i dr�en�
		}
		
		// Zakaze nastavovani
		if (cti_klavesnici() == 11) // B
		{
			nastavovani = false;
			while (cti_klavesnici() != 99);	// �e�� aby nedoch�zelo k op�tovn�mu stisku p�i dr�en�
		}

		if (nastavovani == true)
		{
			// inkrementace minut - 1
			if (cti_klavesnici() == 1)
			{
				minuty++;
				while (cti_klavesnici() != 99);	// �e�� aby nedoch�zelo k op�tovn�mu stisku p�i dr�en�
			}
			// inkrementace sekund - 3
			if (cti_klavesnici() == 3)
			{
				sekundy++;
				while (cti_klavesnici() != 99);	// �e�� aby nedoch�zelo k op�tovn�mu stisku p�i dr�en�
			}
			// nastaven� nul
			if (cti_klavesnici() == 0)
			{
				minuty = 0;
				sekundy = 0;
				while (cti_klavesnici() != 99);	// �e�� aby nedoch�zelo k op�tovn�mu stisku p�i dr�en�
			}
		}
		
		// povol� spu�t�n� odpo�tu pokud nen� v re�imu nastavov�n�
		if (nastavovani == false)
		{
			if (cti_klavesnici() == 15) //D - SUPSTENI
			{
				bezi_odpocet = true;

				while (cti_klavesnici() != 99);	// �e�� aby nedoch�zelo k op�tovn�mu stisku p�i dr�en�
			}

			if (cti_klavesnici() == 12) //C - POZASTAVENI
			{
				bezi_odpocet = false;
				while (cti_klavesnici() != 99);	// �e�� aby nedoch�zelo k op�tovn�mu stisku p�i dr�en�
			}
		}
		
		// logika odpo�tu
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
		
		// signalizuje odpo�et blik�n�m LEDek
		while (blik == true)
		{
			PORTB = 0b00000000;
			_delay_ms(100);
			PORTB = 0b11111111;
			_delay_ms(100);

			if (cti_klavesnici() < 20)	// blik� dokud nebude cokoliv stisknuto
			{
				blik = false;
			}
		}	
		

    }
}

