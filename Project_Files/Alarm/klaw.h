#include "MKL05Z4.h"

#define C1 9		// Kolumna pierwsza zestawu 4x4 tact switches
#define C2 10
#define C3 11
#define C4 12

#define R1 6		// Wiersz pierwszy zestawu 4x4 tact switches
#define R2 7
#define R3 11
#define R4 13


#define C1_MASK	(1<<9)		// Maska dla kolumny C4
#define C2_MASK	(1<<10)		// Maska dla kolumny C3
#define C3_MASK	(1<<11)		// Maska dla kolumny C2
#define C4_MASK	(1<<12)		// Maska dla kolumny C1

#define R1_MASK	(1<<6)		// Maska dla wiersza R1
#define R2_MASK	(1<<7)		// Maska dla wiersza R2
#define R3_MASK	(1<<11)		// Maska dla wiersza R3
#define R4_MASK	(1<<13)		// Maska dla wiersza R4


void Klaw_Init(void);		// Funkcja inicjujaca porty do pracy
unsigned int read_keypad(void);		//Funkcja zwraca wartosci wcisnietego klawisza zgodnie z kolejnoscia 1-16. Dla S1 return 1; Dla S8 return 8;
