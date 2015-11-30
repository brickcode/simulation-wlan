//#######################################################################
//#
//#	Metoda:		Interakcja Procesow
//# Temat:		Symulacja sieci WLAN
//#
//#	Data:		1 maja 2012
//#	
//#######################################################################

#include <iostream>
#include <cmath>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <queue>	// kolejka FIFO
#include <deque>	// kolejka dwukierunkowa
using namespace std;

//#######################################################################
//#				WSTEPNE DEKLARACJE
//#######################################################################

class Process;
class Event;
class Pakiet;
class Stacja;
class Monitor;
class Medium;

//#######################################################################
//#				ZMIENNE I STALE GLOBALNE
//#######################################################################

const int BN = 10;
const int RTmax = 5;
const int D = 16;
const double Pz = 0.001;
const double czas_transmisji = 0.1; // [ms]
const int rozmiar_pakietu = 1500;	// [bajtow]
const double MI = 15;
const double test_kanalu = 0.001;

double zegar = 0.0;		// [ms]
priority_queue <Event> agenda;
Monitor* Dane;
Medium* kanal;

//#######################################################################
//#				KLASA: Generator
//#######################################################################

class Generator
{
	int modulnik;
	int mnoznik;
	int mnoznik2;
	double mi;
	unsigned long long int ziarno;
public:
	Generator(long long int ziarno_poczatkowe);
	Generator();
	long long int hash();
	double losuj();
	int losuj_przedzial();
	double losuj_wykladniczy();
};

//#######################################################################
//#				KLASA: Event
//#######################################################################

class Event
{
public:
	double event_time;
	Process* proc;
	Event(Process* ptr): event_time(-1.0), proc(ptr) {}
	bool operator < (const Event &other) const
	{
		return event_time > other.event_time;
	}
};

//#######################################################################
//#				KLASA: Process
//#######################################################################

class Process
{
	Event* my_event;
public:
	int phase;
	bool terminated;
	Process(): phase(0), terminated(false)
	{
		my_event = new Event(this);
	}
	void activate(double time)
	{
		my_event->event_time = zegar + time;
		agenda.push(*my_event);
	}
	void virtual execute() {}
};

//#######################################################################
//#				KLASA: Monitor
//#######################################################################

class Monitor: public Process
{
	double time_test, czas_symulacji, delta;
	int liczba_stacji, dostarczone, tracone, oferowane;
	int retransmisje, kolizje;
	deque <double> opoznienia;
	FILE* delay;
	bool write;
public:
	Monitor(int ilosc_stacji, double czas, bool wr);
	void execute();
	void opoznienie(double czas);
	void reset();
	void zwieksz_kolizje();
	void zwieksz_oferowane();
	void zwieksz_retransmisje();
	void zwieksz_tracone();
	~Monitor();
};

//#######################################################################
//#				KLASA: Pakiet
//#######################################################################

class Pakiet: public Process
{
	Stacja* nadajnik;
	double czas_nadania;
public:
	Pakiet(Stacja* nadajnik);
	void execute();
};

//#######################################################################
//#				KLASA: Stacja
//#######################################################################

class Stacja: public Process
{
	queue <Pakiet *> bufor;	// bufor nadawczy przechowujacy pakiety
	unsigned int KL, RT;	// licznik retransmisji, kolizji
	int licznik;			// licznik D
public:
	Generator generator;	// generator liczb pseudolosowych
	Stacja(long long int seed);
	bool aktywacja();
	bool bufor_test();
	void dodaj_pakiet(Pakiet* x);
	void execute();
	void kolizja();
	void przeslano();
	void reset();
	void retransmisja();
};

//#######################################################################
//#				KLASA: Medium
//#######################################################################

class Medium: public Process
{
	queue <Stacja *> kolejka;
	queue <Pakiet *> transmitowane;
	bool kolizja, zajety;
public:
	Medium();
	void dodaj_oczekujaca(Stacja* x);
	void execute();
	bool test_kolizji();
	bool test_zajetosci();
	void zajmij_kanal(Pakiet* x);
	void zwolnij_kanal();
};

//#######################################################################
//#				DEFINICJE FUNKCJI KLASY: Generator
//#######################################################################

Generator :: Generator(long long int ziarno_poczatkowe)
{
	ziarno = ziarno_poczatkowe;
	mnoznik = 16807;
	modulnik = 2147483647;
	mi = MI;
	mnoznik2 = 630360016;
}

Generator :: Generator()
{
	Generator(1);
}

long long int Generator :: hash()
{
	ziarno = (mnoznik2 * ziarno) % modulnik;
	return ziarno;
}

double Generator :: losuj()
{
	ziarno = (mnoznik * ziarno) % modulnik;
	return double(ziarno) / modulnik;
}

int Generator :: losuj_przedzial()
{
	ziarno = (mnoznik * ziarno) % modulnik;
	return ziarno % D + 1;
}

double Generator :: losuj_wykladniczy()
{
	return -mi * log(losuj());
}

//#######################################################################
//#				DEFINICJE FUNKCJI KLASY: Monitor
//#######################################################################

Monitor :: Monitor(int ilosc_stacji, double czas, bool wr) 
{ 
	write = wr;
	liczba_stacji = ilosc_stacji;
	delta = 10000;
	activate(delta);
	czas_symulacji = czas;
	reset();
	Generator gen = Generator(int(time(NULL)));
	kanal = new Medium();
	for(int i = 0; i < liczba_stacji; i++)
	{
		(new Stacja(gen.hash()))->activate(0);
	}
	if(write)
	{
		delay = fopen("delay.txt","w");
	}
}

void Monitor :: execute()
{
	reset();
	opoznienia.clear();
}

void Monitor :: opoznienie(double czas)
{
	if(write)
	{
		fprintf(delay, "%.6f %.6f\n", zegar, czas);
	}
	opoznienia.push_back(czas);
	dostarczone++;
}

void Monitor :: reset()
{
	opoznienia.clear();
	dostarczone = 0;
	oferowane = 0;
	tracone = 0;
	retransmisje = 0;
	kolizje = 0;
}

void Monitor :: zwieksz_kolizje()
{
	kolizje++;
}

void Monitor :: zwieksz_oferowane()
{
	oferowane++;
}

void Monitor :: zwieksz_retransmisje()
{
	retransmisje++;
}

void Monitor :: zwieksz_tracone()
{
	tracone++;
}

Monitor :: ~Monitor()
{
	double suma = 0;
	for(int i = 0; i < opoznienia.size(); i++)
	{
		suma += opoznienia[i];
	}
	double srednia = suma/dostarczone;

	suma = 0;
	for(int i = 0; i < opoznienia.size(); i++)
	{
		suma += pow((srednia - opoznienia[i]),2);
	}
	double wariancja = suma/dostarczone;

	if(dostarczone == 0)
	{
		srednia = 0;
		wariancja = 0;
	}

	printf(" ::: WYNIKI SYMULACJI CYFROWEJ ::: \n");
	printf(" --------------------------------- \n");
	printf("   stacje   : %7i\n", liczba_stacji);
	printf("    czas    : %7i [ms]\n", (int)czas_symulacji); 
	printf(" --------------------------------- \n");
	printf("max predkosc:  %.4f MBps\n", 15*(dostarczone*0.1)/(czas_symulacji-delta));
	printf("   srednia  :  %.4f [ms]\n", srednia);
	printf("  wariancja :  %.4f \n", wariancja);
	printf("  oferowane : %7i \n", oferowane);
	printf(" dostarczone: %7i \n", dostarczone);
	printf("   tracone  : %7i (%.4f)\n", tracone, (double)tracone/oferowane);
	printf("   kolizje  :  %.4f \n", (double)kolizje/(czas_symulacji-delta));
	printf("retransmisje:  %.4f \n", (double)retransmisje/(czas_symulacji-delta));

	if(write)
	{	
		FILE* fpredkosc = fopen("predkosc.txt","a");
		FILE* fsrednia = fopen("srednia.txt","a");
		FILE* fwariancja = fopen("wariancja.txt","a");
		FILE* ftracone = fopen("tracone.txt","a");
		FILE* fkolizje = fopen("kolizje.txt","a");
		FILE* fretransmisje = fopen("retransmisje.txt","a");
		FILE* wyniki = fopen("wyniki.txt","a");

		fprintf(wyniki, " ::: WYNIKI SYMULACJI CYFROWEJ ::: \n");
		fprintf(wyniki, " --------------------------------- \n");
		fprintf(wyniki, "   stacje   : %7i\n", liczba_stacji);
		fprintf(wyniki, "    czas    : %7i [ms]\n", (int)czas_symulacji); 
		fprintf(wyniki, " --------------------------------- \n");
		fprintf(wyniki, "max predkosc:  %2.4f MBps\n", 15*(dostarczone*0.1)/(czas_symulacji-delta));
		fprintf(wyniki, "   srednia  :  %2.4f [ms]\n", srednia);
		fprintf(wyniki, "  wariancja :  %2.4f \n", wariancja);
		fprintf(wyniki, "  oferowane : %7i \n", oferowane);
		fprintf(wyniki, " dostarczone: %7i \n", dostarczone);
		fprintf(wyniki, "   tracone  : %7i (%.4f)\n", tracone, (double)tracone/oferowane);
		fprintf(wyniki, "   kolizje  :  %2.4f \n", (double)kolizje/(czas_symulacji-delta));
		fprintf(wyniki, "retransmisje:  %2.4f \n\n", (double)retransmisje/(czas_symulacji-delta));

		fprintf(fpredkosc,"%.4f\n", 15*(dostarczone*0.1)/(czas_symulacji-delta));
		fprintf(fsrednia,"%.4f \n", srednia);
		fprintf(fwariancja,"%.4f \n", wariancja);
		fprintf(ftracone,"%.4f\n", (double)tracone/oferowane);
		fprintf(fkolizje,"%.4f \n", (double)kolizje/(czas_symulacji-delta));
		fprintf(fretransmisje,"%.4f \n", (double)retransmisje/(czas_symulacji-delta));

		fclose(wyniki);
		fclose(fpredkosc);
		fclose(fsrednia);
		fclose(fwariancja);
		fclose(ftracone);
		fclose(fkolizje);
		fclose(fretransmisje);
		fclose(delay);
	}
}

//#######################################################################
//#				DEFINICJE FUNKCJI KLASY: Pakiet
//#######################################################################

Pakiet :: Pakiet(Stacja* ptr): nadajnik(ptr), czas_nadania(0) {}

void Pakiet :: execute()
{
	bool active = true;
	while(active)
	{
		switch(phase)
		{
		case 0: // zainicjowanie powstania nowego pakietu
			{
				czas_nadania = zegar;
				(new Pakiet(nadajnik))->activate(nadajnik->generator.losuj_wykladniczy());
				Dane->zwieksz_oferowane(); // ustalenie ciaglosci transmisji wiadomosci
				if(nadajnik->bufor_test()) // test przepelnienia
				{
					phase = 1; // pakiet beddzie ozcekiwal na wyslanie w buforze stacji
					nadajnik->dodaj_pakiet(this);  
					if(nadajnik->aktywacja())
					{
						nadajnik->activate(0);
					}
				}
				else // pakiet zostaje porzucony
				{
					Dane->zwieksz_tracone();
					terminated = true; 
				}
				active = false;
				break;
			}
		case 1: // analiza transmisji
			{
				if(kanal->test_kolizji()) // zdarzyla sie kolizja
				{
					nadajnik->kolizja();
					nadajnik->activate(0);
				}
				else // przeslano poprawnie
				{
					if(nadajnik->generator.losuj() < Pz) // blad w transmisji
					{
						nadajnik->retransmisja();
						nadajnik->activate(0);
					}
					else // poprawna transmisja
					{
						terminated = true;
						nadajnik->activate(0);
						Dane->opoznienie(zegar - czas_nadania);
					}
				}
				active = false;
				break;
			}
		}
	}
}

//#######################################################################
//#				DEFINICJE FUNKCJI KLASY: Stacja
//#######################################################################

Stacja :: Stacja(long long int seed)
{
	generator = Generator(seed);
	reset();
}

bool Stacja ::aktywacja()
{
	if(bufor.size() == 1)
	{
		return true;
	}
	return false;
}

bool Stacja :: bufor_test()
{
	if(bufor.size() >= BN)
	{
		return false; // brak miejsca na nowe zgloszenie
	}
	return true; // jest miejse na nowy pakiet
}

void Stacja :: dodaj_pakiet(Pakiet* x)
{
	bufor.push(x); // dodanie pakietu do buforu oczekujacego
}

void Stacja :: execute()
{
	bool active = true;
	while(active)
	{
		switch(phase)
		{
		case 0: // inicjacja dzialania stacji (wykonywane tylko raz)
			{
				(new Pakiet(this))->activate(generator.losuj_wykladniczy());
				phase = 1;
				active = false;
				break;
			}
		case 1: 
			{
				if(bufor.size()) // sprawdzenie czy jest pakiet do wyslania
				{
					if(RT > RTmax) // test licznika RT
					{
						Dane->zwieksz_tracone();
						bufor.pop();
						reset();
					}
					else // mozna aktywowac do nadawania
					{
						licznik = generator.losuj_przedzial();
						phase = 2;
					}
				}
				else
				{
					active = false;
				}
				break;
			}
		case 2: // testowanie zajetosci kanalu
			{
				if(kanal->test_zajetosci()) // oczekiwanie na wolny kanal
				{
					active = false;
					kanal->dodaj_oczekujaca(this);
				}
				else // kanal jest wolny
				{
					if(licznik == 0) 
					{
						phase = 3; // rozpoczecie transmisji
					}
					else // dalsze zmiejszanie licznika
					{
						active = false;
						activate(test_kanalu);
						licznik--;
					}					
				}
				break;
			}
		case 3: // transmisja pakietu droga radiowa
			{
				kanal->zajmij_kanal(bufor.front());
				phase = 4;
				if(RT > 0)
				{
					Dane->zwieksz_retransmisje();
				}
				active = false;
				break;
			}
		case 4: //czyszczenie buforu i przygotowanie do przeslania kolejnej wiadomosci
			{
				bufor.pop();
				reset();
				phase = 1;
			}
		}
	}
}

void Stacja :: kolizja()
{
	retransmisja();
	KL++;
}

void Stacja :: przeslano()
{
	reset();
	phase = 1;
}

void Stacja :: reset()
{
	RT = 0;
	KL = 0;
}

void Stacja :: retransmisja()
{
	phase = 1;
	RT++;
}

//#######################################################################
//#				DEFINICJE FUNKCJI KLASY: Medium
//#######################################################################

Medium :: Medium()
{
	kolizja = false;
	zajety = false;
}

void Medium ::dodaj_oczekujaca(Stacja* x)
{
	kolejka.push(x); // dodaj stacje oczekujaca na wolny kanal
}

void Medium :: execute() 
{
	bool active = true;
	while(active)
	{
		switch(phase)
		{
		case 0: // procedura blokowania kanalu
			{
				zajety = true;
				activate(czas_transmisji);
				phase = 1;
				active = false;
				break;
			}
		case 1: // odblokowywanie kanalu
			{
				if(transmitowane.size() > 1) // kolizja
				{
					kolizja = true;
					while(transmitowane.size() > 0)
					{
						Dane->zwieksz_kolizje();
						transmitowane.front()->execute();
						transmitowane.pop();
					}
				}
				else // odbior bez kolizji
				{
					transmitowane.front()->execute();
					transmitowane.pop();
				}
				while(kolejka.size() > 0) // aktywacja oczekujacych stacji
				{
					kolejka.front()->activate(0);
					kolejka.pop();
				}
				phase = 0;
				kolizja = false;
				zajety = false;
				active = false;
				break;
			}
		}
	}
}

bool Medium :: test_kolizji()
{
	return kolizja;
}

bool Medium :: test_zajetosci()
{
	return zajety;
}

void Medium :: zajmij_kanal(Pakiet* x)
{
	transmitowane.push(x);
	if(transmitowane.size() == 1)
	{
		activate(1e-6);
	}
	else
	{
		kolizja = true;
	}
}

void Medium :: zwolnij_kanal()
{
	zajety = false;
}

//#######################################################################
//#				GLOWNA PETLA PROGRAMU
//#######################################################################

int main(int argc, char *argv[])
{	
	double simulation_time = atof(*++argv);
	int stacje = atoi(*++argv);
	bool write = (bool)atoi(*++argv);
	Dane = new Monitor(stacje, simulation_time, write);
	Process* current = Dane;
	time_t start = time(NULL);

	while(simulation_time > zegar)
	{
		zegar = agenda.top().event_time;
		current = agenda.top().proc;
		agenda.pop();
		current->execute();
		if(current->terminated)
		{
			delete current;
		}
	}
	
	delete Dane;
	printf("\nczas symulacji: %i [s]\n", (time(NULL) - start));
	if(write == 0) 
	{
		getchar();
	}
	return 0;
}

//#######################################################################
//#				ZAKONCZENIE SYMULACJI
//#######################################################################