#include <Arduino.h>
#include <Wire.h>   // standardowa biblioteka Arduino
#include <LiquidCrystal_I2C.h> // dolaczenie pobranej biblioteki I2C dla LCD
enum{
	MANUAL,
	AUTO,
	TEST,
};
#define ON false
#define OFF true
//Debugowanie ogólne
#define in  0 //Sprawdzanie stanu wejść
#define out 1 //Sprawdzanie stanu wyjść
#define com 2 //Podgląd komunikacji
bool debug[] = {LOW, LOW, LOW};

// Piny analogowe
byte POT_GAS = 15;
byte POT_AIR = 14;

// Sterowanie wyj�ciami
byte PWM_AIR = 2;
byte PWM_GAS = 3;
byte RELAY_PUMP_OIL = 4;
byte RELAY_SPARK = 5;
byte RELAY_VALVE_AIR = 6;
byte RELAY_NC = 7;
byte RELAY_PUMP_WATER = 8;
byte RELAY_VALVE_GAS = 9;

// Panel kontrolny
byte SW_MAN_AUTO = 43;
byte DIODE_START = 44;
byte SW_PUMP = 45;
byte SW_SPARK = 46;
byte SW_START = 47;
byte SW_STOP = 48;
byte SW_VALVE_GAS = 49;
byte SW_VALVE_AIR = 50;
byte DIODE_STOP = 51;
byte SW_TACT2 = 52;
byte SW_TACT1 = 53;

//Zmienne opisujące wejścia cyfrowe tj. przelaczniki
#define num_of_DI 9
String DI_names[num_of_DI]	= { "SW_MAN_AUTO", "SW_PUMP", "SW_SPARK", "SW_START", "SW_STOP", "SW_VALVE_GAS", "SW_VALVE_AIR", "SW_TACT2", "SW_TACT1" };
byte DI_pins[num_of_DI]		= { SW_MAN_AUTO, SW_PUMP, SW_SPARK, SW_START, SW_STOP, SW_VALVE_GAS, SW_VALVE_AIR, SW_TACT2, SW_TACT1 };
boolean DI_vals[num_of_DI]	= { LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW };

//Zmienne opisujace wejsca analogowe tj. potencjometry
#define num_of_AI 2
String AI_names[num_of_AI]		= {  "POT_AIR", "POT_GAS", };
byte AI_pins[num_of_AI]		= { POT_AIR, POT_GAS };
unsigned int AI_vals[num_of_AI]= { 0, 0 };

//Zmienne opisujace wyjscia cyfrowe tj przekazniki i diody
#define num_of_DO 8
String DO_names[num_of_DO]		= { "RELAY_PUMP_OIL", "RELAY_SPARK", "RELAY_VALVE_AIR", "RELAY_NC", "RELAY_PUMP_WATER", "RELAY_VALVE_GAS", "DIODE_START", "DIODE_STOP" };
byte DO_pins[num_of_DO]		= { RELAY_PUMP_OIL, RELAY_SPARK, RELAY_VALVE_AIR, RELAY_NC, RELAY_PUMP_WATER, RELAY_VALVE_GAS, DIODE_START, DIODE_STOP };
bool DO_vals[num_of_DO]	= { OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF };
enum {PUMP_OIL, SPARK, VALVE_AIR, NC, PUMP_WATER, VALVE_GAS, START, STOP };

//Zmienne opisujace wyjsca PWM czyli sterownik ZR i SR
#define num_of_PWM 2
String PWM_names[num_of_PWM]	= { "PWM_AIR", "PWM_GAS" };
byte PWM_pins[num_of_PWM]		= { PWM_AIR, PWM_GAS, };
int PWM_vals[num_of_PWM]		= { 0, 0 };
byte PWM_adjust[2][2] = {{40, 140},{51, 206}};			// Tablica korygująca sterowanie. Sterowanie PWM zawiera się od wartości pierwszej do drugiej. Tablica 1 dla powietrza 2 dla gazu.
enum { AIR, GAS, };

//Tablica grupujaca wszystkie wejscia
byte control_panel_DI[] = {SW_PUMP, SW_SPARK, SW_VALVE_GAS, SW_VALVE_AIR, SW_TACT2};
byte control_panel_AI[] = {POT_GAS, POT_AIR};

//Zmienne komunikacji z Measure
bool transmit = false;
bool com_started = false;
unsigned int tx_packets = 0;
unsigned int rx_packets = 0;

//Zmienne otrzymywane od Measure
float oil_press =2.22; 
float air_flow;
float air_press; 
float crank_rev = 1000; 
float exhoust_temp = 30;
#define history_number_KS 3
float historia_termopar[history_number_KS] = {30,30,30};
#define history_number_Oil 5
float historia_cisnienia_oleju [history_number_Oil] = {2,2,2,2,2};

//Progi na zmienne
float oil_press_treshold		= 1.2;	// Prog ponizej, ktorego turbina sie wylacza [bar]
float air_flow_start_treshold	= -1;	// Prog, ktory wskazuje, ze turbina ruszyla	[kg/h]
float air_press_start_treshold  = -1;	// Prog, ktory wskazuje, ze turbina ruszyla [bar]
float crank_rev_start_treshold	= 800;	// Prog, ktory wskazuje, ze turbina ruszyla [Hz]
float exhaust_temp_ignition_treshold = 400;// Prog, ktory zatrzymuje silnik rozruchowy podczas wychladzania turbiny [C]

//Jest problem z trybem manual, wiec dla manuala nie ma zadnego sprawdzania wartosci - tak jak bylo
float manual_problem [3] = {1.2, 900, 400}; // Jezeli jest manual to zmiana wartosci treshold, a tu przechowujemy orginaly


//Definicja parametrów wyswietlacza
#define max_lcd_col 16
#define max_lcd_row 2
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Ustawienie adresu ukladu na 0x27
bool lcd_update[] = {LOW, LOW, LOW, LOW};
enum { OIL=2, REV=3 };

//Zmienne wyznaczajace stan turbiny
bool start, started;
bool stop = false;
bool tryb_auto = false;   // 0 i nie dotykac
bool tryb_PC = false;     // 0 i nie dotykac
bool tryb_BT = false;     // 0 i nie dotykac

//Zmienne parametryzujace auto start
int autostart_delay = 2000;			  //standardowy delay do operacji w trybie auto
int autostart_abort_time = 15000;	//jesli po tym czasie nie dojdzie do zaplonu to procedura zatrzymania
int air_start_level = 300;        //poczatkowe ustawienie poziomu SR - wartosc 0-1023
int gas_start_level = 300;        //poczatkowe ustawienie poziomu ZR - wartosc 0-1023
int air_auto_level = 300;
int gas_auto_level = 300;
byte air_auto_incr = 20;				  //krok zwiekszania mocy SR
byte gas_auto_incr = 20;				  //krok zwiekszania otwarcia ZR
float air_press_drop = 0.2;

//Zmienne do startu z komputera
byte control = 0;
#define rozkaz_start       9
#define rozkaz_stop        8
#define rozkaz_power_up    2
#define rozkaz_power_down  1
char check;
char rozkaz;


void setup()
{
	Serial.begin(115200);   // PC
	Serial1.begin(115200);  // Arduino pomiarowe
  Serial3.begin(9600);    // Bluetooth
	lcd.backlight();
	lcd.begin(max_lcd_col, max_lcd_row);
	set_pins();
	zero_outputs();
}

void loop()
{
	boolean mode = mode_select();
	if (mode == AUTO)   { start_auto(); }	// Zrealizuj tryb rozruchu auto
	if (mode == MANUAL) { start_manual(); } // Zrealizuj tryb rozruchu manual
	if (started) { turbine_work(); }		// Jeśli turbina wystartowała poprawnie to wejdz w tryb pracy
	turbine_stop();							// Wyłącz turbinę.
}

boolean mode_select()
{
	lcd_print(0, "Wybierz tryb");
	lcd_print(1, "Zatwierdz PP");
	boolean mode;
	while(!digitalRead(SW_TACT2))
	{
		mode = digitalRead(SW_MAN_AUTO);
		communicate_with_measure();
	}
	start = true;
	return mode;
}

void time_delay(unsigned int time, bool check_oil = false)
{
  unsigned long timer = millis();
  while (millis() - timer < time)
  {
    communicate_with_measure();
    update_lcd();
    if (check_oil){ check_oil_press(); }
  }
}

void start_auto()
{
  tryb_auto = true;
  tryb_PC = false;
  tryb_BT = false;
  czyszczenie_bufora(); // dla PC
  czyszczenie_bufora_BT(); // dla Bluetooth
  gas_auto_level = 0; // 0 gazu - 2x nadanie 0
  
  // Sprawdz wyzerowanie wejsc panelu kontrolnego
  lcd_print(0, "Tryb Auto");
  lcd_print(1, "Wyzeruj wejscia");
  while (!check_control_panel_I() && !tryb_PC && !tryb_BT) {
    control = jaki_rozkaz();
    if (control == rozkaz_start) { tryb_PC = true; }
      else {
        control = jaki_rozkaz_BT(); 
        if (control == rozkaz_start) { tryb_BT = true; } }
    communicate_with_measure(); }
  ////////////////////////////////////////////////////////////////////
  
	// Tryb auto, czekaj na inicjalizacje
	lcd_print(0, "Tryb Auto");
	lcd_print(1, "Wcisnij start");
	while(!digitalRead(SW_START) && !tryb_PC && !tryb_BT) {
	  control = jaki_rozkaz(); 
	  if (control == rozkaz_start) { tryb_PC = true; }
      else {
        control = jaki_rozkaz_BT(); 
        if (control == rozkaz_start) { tryb_BT = true; } }
	  communicate_with_measure(); }
	////////////////////////////////////////////////////////////////////
  
  if (tryb_PC) {wyslij_wartosci('x', 't');}
  if (tryb_BT) {wyslij_wartosci_BT('x', 't');}
    
	// Wlacz pompe
	lcd_update[OIL] = HIGH;
	lcd_update[REV] = HIGH;
	lcd_print(0, "Wlaczam pompe   ");
	lcd_print(1, "p:      n:");
	time_delay(autostart_delay, false);
  if (tryb_PC) {wyslij_wartosci('x', 't');}
  if (tryb_BT) {wyslij_wartosci_BT('x', 't');}
	set_DO(PUMP_OIL, ON);
	////////////////////////////////////////////////////////////////////

  // Poczekaj aż będzie 5 odczytów z pompy
  for (int i=0; i < history_number_Oil; i++) {
    time_delay(autostart_delay/2, false);
    if (tryb_PC) {wyslij_wartosci('x', 't');}
    if (tryb_BT) {wyslij_wartosci_BT('x', 't');} }
  ////////////////////////////////////////////////////////////////////

  // Przedmuch
  if (!stop) {
    lcd_print(0, "Przedmuch       ");
    if (tryb_PC) { control = jaki_rozkaz(); }
    if (tryb_BT) { control = jaki_rozkaz_BT(); }  
    if (!digitalRead(SW_STOP) && !(control == rozkaz_stop) && !(oil_press < oil_press_treshold)) {
      set_PWM(AIR, 300);
      for (int j=0; j < 8; j++) {
        time_delay(autostart_delay/2, true);
        if (tryb_PC) {wyslij_wartosci('x', 't');}
        if (tryb_BT) {wyslij_wartosci_BT('x', 't');} }
      set_PWM(AIR, air_start_level);
      } else {stop = true;} }
  //////////////////////////////////////////////////////////////////////
  
  // Wlacz zaplon
  if (tryb_PC) { control = jaki_rozkaz(); }
  if (tryb_BT) { control = jaki_rozkaz_BT(); }
  if (!digitalRead(SW_STOP) && !(control == rozkaz_stop) && !(oil_press < oil_press_treshold)) {
	lcd_print(0, "Wlaczam zaplon  ");
	time_delay(autostart_delay, true);
	set_DO(SPARK, ON);
  if (tryb_PC) {wyslij_wartosci('x', 't');}
  if (tryb_BT) {wyslij_wartosci_BT('x', 't');} 
  } else {stop = true;}
	////////////////////////////////////////////////////////////////////

	// Wlacz zawor odc
  if (tryb_PC) { control = jaki_rozkaz(); }
  if (tryb_BT) { control = jaki_rozkaz_BT(); }
  if (!stop) {
    if (!digitalRead(SW_STOP) && !(control == rozkaz_stop) && !(oil_press < oil_press_treshold)) {
      lcd_print(0, "Wlaczam zawor od");
      time_delay(autostart_delay, true);
      set_DO(VALVE_GAS, ON);
      if (tryb_PC) {wyslij_wartosci('x', 't');}
      if (tryb_BT) {wyslij_wartosci_BT('x', 't');} }
      else {stop = true;} }
	////////////////////////////////////////////////////////////////////

	// Ustawienia wartosci poczatkowej powietrza, oraz stopniowe zwiekszanie gazu do 30%
  if (!stop) {
	  lcd_print(0, "air:    gas:");
	  lcd_update[GAS] = true;
	  lcd_update[AIR] = true;
    if (tryb_PC) { control = jaki_rozkaz(); }
    if (tryb_BT) { control = jaki_rozkaz_BT(); }  
    if (!digitalRead(SW_STOP) && !(control == rozkaz_stop) && !(oil_press < oil_press_treshold)) {
      set_PWM(AIR, air_start_level);
      air_auto_level = air_start_level;
      for (int j=0; j < 5; j++) {
        time_delay(autostart_delay/2, true);
        if (tryb_PC) {wyslij_wartosci('x', 't');}
        if (tryb_BT) {wyslij_wartosci_BT('x', 't');} }
      } else {stop = true;} }
  gas_auto_level = 0;
  while ((gas_auto_level < gas_start_level) && !stop) {
    gas_auto_level = gas_auto_level + 50;
    if (tryb_PC) { control = jaki_rozkaz(); }
    if (tryb_BT) { control = jaki_rozkaz_BT(); }
    if (digitalRead(SW_STOP) || (control == rozkaz_stop) || (oil_press < oil_press_treshold))
      {stop = true;} else {
      time_delay(autostart_delay/2, true);
      if (tryb_PC) {control = jaki_rozkaz(); wyslij_wartosci('x', 't'); }
      if (tryb_BT) {control = jaki_rozkaz_BT(); wyslij_wartosci_BT('x', 't'); } 
      if (!digitalRead(SW_STOP) && !(control == rozkaz_stop) && !(oil_press < oil_press_treshold))
        {set_PWM(GAS, gas_auto_level);}      
        else {stop = true;} } }
  /////////////////////////////////////////////////////////////////////
 
   // Sprawdzenie zaplonu przez max 15 sekund
  if (tryb_PC) { control = jaki_rozkaz(); }
  if (tryb_BT) { control = jaki_rozkaz_BT(); }
	unsigned long timer = millis();
  while (((historia_termopar[0] < (exhaust_temp_ignition_treshold + 100)) || (exhoust_temp < exhaust_temp_ignition_treshold)) && !stop) {
    if (((millis() - timer)>autostart_abort_time) || digitalRead(SW_STOP) || (oil_press < oil_press_treshold) || (control == rozkaz_stop))
      {stop = true;} else {
        time_delay(autostart_delay/2, true);
        if (tryb_PC) { control = jaki_rozkaz(); wyslij_wartosci('x', 't'); }
        if (tryb_BT) { control = jaki_rozkaz_BT(); wyslij_wartosci_BT('x', 't'); } } }
  ///////////////////////////////////////////////////////////////////////

  //Poczekaj 10 sekund, jeżeli odrazu wykryje zapłon
  while (((millis()-timer) < 10000) && !stop)
  {
    if (digitalRead(SW_STOP) || (oil_press < oil_press_treshold) || (control == rozkaz_stop) || (exhoust_temp < exhaust_temp_ignition_treshold))
    {stop = true;} else {
      time_delay(autostart_delay/2, true);
      if (tryb_PC) { control = jaki_rozkaz(); wyslij_wartosci('x', 't'); }
      if (tryb_BT) { control = jaki_rozkaz_BT(); wyslij_wartosci_BT('x', 't'); } } }
  ///////////////////////////////////////////////////////////////////////
  
	// Petla zwiekszajaca moc powietrza do 60%
  if (tryb_PC) { control = jaki_rozkaz(); }
  if (tryb_BT) { control = jaki_rozkaz_BT(); }
	float before_air_press = air_press;
	while ((air_auto_level < 615) && !stop) {
    if (digitalRead(SW_STOP) || (control == 8))
      {stop = true;} else {
        time_delay(autostart_delay/2, true);
        if (tryb_PC) { wyslij_wartosci('x', 't'); control = jaki_rozkaz(); }
        if (tryb_BT) { wyslij_wartosci_BT('x', 't'); control = jaki_rozkaz_BT(); } }
		air_auto_level += air_auto_incr;
		if ((exhoust_temp < exhaust_temp_ignition_treshold) || digitalRead(SW_STOP) || (oil_press < oil_press_treshold) || (control == rozkaz_stop))
		  {stop = true;}
		if (!stop) {
      if ((before_air_press - air_press)>air_press_drop)    // turbina zaczyna kaszlec. brakuje gazu
        { gas_auto_level += gas_auto_incr*3; }              // to dodaj gazu 
		  set_PWM(AIR, air_auto_level);
		  set_PWM(GAS, gas_auto_level);
		  before_air_press = air_press;
		  if (tryb_PC) { wyslij_wartosci('x', 't'); }
		  if (tryb_BT) { wyslij_wartosci_BT('x', 't'); } } }
	////////////////////////////////////////////////////////////////////////

  // Sprawdzenie zaplonu 2
  if (tryb_PC) { control = jaki_rozkaz(); }
  if (tryb_BT) { control = jaki_rozkaz_BT(); }
  timer = millis();
  while (((historia_termopar[0] < (exhaust_temp_ignition_treshold + 200)) || (exhoust_temp < exhaust_temp_ignition_treshold)) && !stop) {
    if (((millis() - timer)>autostart_abort_time) || digitalRead(SW_STOP) || (oil_press < oil_press_treshold) || (control == rozkaz_stop))
      {stop = true;} else {
        time_delay(autostart_delay/2, true);
        if (tryb_PC) { control = jaki_rozkaz(); wyslij_wartosci('x', 't'); }
        if (tryb_BT) { control = jaki_rozkaz_BT(); wyslij_wartosci_BT('x', 't'); } } }
  ////////////////////////////////////////////////////////////////////////

    //Poczekaj jeszcze 5 sekund, o ile nie czekala na stan zaplonu
  while (((millis()-timer) < 5000) && !stop)
  {
    if (digitalRead(SW_STOP) || (oil_press < oil_press_treshold) || (control == rozkaz_stop) || (exhoust_temp < exhaust_temp_ignition_treshold))
    {stop = true;} else {
      time_delay(autostart_delay/2, true);
      if (tryb_PC) { control = jaki_rozkaz(); wyslij_wartosci('x', 't'); }
      if (tryb_BT) { control = jaki_rozkaz_BT(); wyslij_wartosci_BT('x', 't'); } } }
  ///////////////////////////////////////////////////////////////////////

	// Petla zwiekszajaca moc turbiny poprzez dodawanie gazu az do osiafnieca wymaganych obrotow
  if (tryb_PC) { control = jaki_rozkaz(); }
  while ((crank_rev < crank_rev_start_treshold) && !stop) {
    if (digitalRead(SW_STOP) || (control == rozkaz_stop))
      {stop = true;} else { 
        time_delay(autostart_delay/2, true);
        if (tryb_PC) { control = jaki_rozkaz(); wyslij_wartosci('x', 't'); if (control == rozkaz_stop) {stop=true;} }
        if (tryb_BT) { control = jaki_rozkaz_BT(); wyslij_wartosci_BT('x', 't'); if (control == rozkaz_stop) {stop=true;} }
        if (crank_rev < crank_rev_start_treshold) {
		      gas_auto_level += gas_auto_incr;
          if ((gas_auto_level > 650) || (exhoust_temp < exhaust_temp_ignition_treshold) || digitalRead(SW_STOP) || (oil_press < oil_press_treshold) || (control == rozkaz_stop))
            {stop = true;}
            else {set_PWM(GAS, gas_auto_level);} } } }
  ///////////////////////////////////////////////////////////////////////
  
  // Po osiagnieciu obrotow
  if (tryb_PC) { control = jaki_rozkaz(); wyslij_wartosci('x', 't'); if (control == rozkaz_stop) {stop=true;} }
  if (tryb_BT) { control = jaki_rozkaz_BT(); wyslij_wartosci_BT('x', 't'); if (control == rozkaz_stop) {stop=true;} }
  if (!digitalRead(SW_STOP) && !stop) {
    time_delay(autostart_delay/2, true);
    set_DO(VALVE_AIR, ON);
    set_PWM(AIR, 0);}
  else {stop = true;}
  set_DO(SPARK, OFF);
	/////////////////////////////////

	// Sprawdź czy turbina wystartowała - pozostalosc, ktora zostala
  if (tryb_PC) { control = jaki_rozkaz(); wyslij_wartosci('x', 't'); if (control == rozkaz_stop) {stop=true;} }
  if (tryb_BT) { control = jaki_rozkaz_BT(); wyslij_wartosci_BT('x', 't'); if (control == rozkaz_stop) {stop=true;} }
	if (((crank_rev) > crank_rev_start_treshold) && (exhoust_temp > exhaust_temp_ignition_treshold) && !stop && !digitalRead(SW_STOP) && !(oil_press < oil_press_treshold))
	  {started = true; start = false;}
    else {stop = true;}
  /////////////////////////////////
}

void start_manual()
{
  manual_problem [0] = oil_press_treshold;
  manual_problem [1] = crank_rev_start_treshold;
  manual_problem [2] = exhaust_temp_ignition_treshold;
  oil_press_treshold = -1;
  crank_rev_start_treshold  = -1;
  exhaust_temp_ignition_treshold = -1;
  
	// Sprawdz wyzerowanie wejsc panelu kontrolnego
	lcd_print(0, "Tryb Manual");
	lcd_print(1, "Wyzeruj wejscia");
	while (!check_control_panel_I())
	{
		communicate_with_measure();
	}
	/////////////////////////////////////////////

	// Czekaj na sygnal wlaczenia pompy
	lcd_print(0, "Wlacz pompe     ");
	lcd_print(1, "p:      n:");
	lcd_update[OIL] = HIGH;
	lcd_update[REV] = HIGH;
	while (!digitalRead(SW_PUMP) && !stop)
	{
    if (digitalRead(SW_STOP))
    {stop = true;} else {
		communicate_with_measure();
		update_lcd(); }
	}
	set_DO(PUMP_OIL, ON);
	set_DO(PUMP_WATER, ON);

  // dodajemy troche powietrza na start
  lcd_print(0, "Podaj powietrza");
  while ((analogRead(POT_AIR) < 200) && !stop)
  {
    if (digitalRead(SW_STOP))
    {stop = true;} else {
    set_PWM(AIR, analogRead(POT_AIR));
    communicate_with_measure();
    update_lcd();
    check_oil_press();}
  }
 
  // Czekaj na sygnal wlaczenia zaplonu
  lcd_print(0, "Wlacz zaplon");
  while (!digitalRead(SW_SPARK) && !stop)
  {
    if (digitalRead(SW_STOP))
    {stop = true;} else {
    set_PWM(AIR, analogRead(POT_AIR));
    communicate_with_measure();
    update_lcd();
    check_oil_press();}
  }
  if (!stop)
  {set_DO(SPARK, ON);}

  // Czekaj na sygnal wlaczenia zaworu odcinajacego
  lcd_print(0, "Wlacz zawor odc.");
  while (!digitalRead(SW_VALVE_GAS) && !stop)
  {
    if (digitalRead(SW_STOP))
    {stop = true;} else {
    set_PWM(AIR, analogRead(POT_AIR));
    communicate_with_measure();
    update_lcd();
    check_oil_press();}
  }
  if (!stop)
  {set_DO(VALVE_GAS, ON);}

	// Steruj silnikiem rozruchowym i zaworem gazu
	lcd_print(0, "air:    gas:");
	lcd_update[GAS] = true;
	lcd_update[AIR] = true;
	while (!digitalRead(SW_VALVE_AIR) && !stop)
	{
    if (digitalRead(SW_STOP))
    {stop = true;} else {
		set_PWM(AIR, analogRead(POT_AIR));
		set_PWM(GAS, analogRead(POT_GAS));
		communicate_with_measure();
		update_lcd();
		check_oil_press();}
	}
  if (digitalRead(SW_STOP))
  {stop = true;} else {
	set_PWM(AIR, 0);
	set_DO(VALVE_AIR, ON);}
  // zapłon zawsze wyłączamy
	set_DO(SPARK, OFF);

 if(stop) {
   oil_press_treshold = manual_problem [0];
   crank_rev_start_treshold = manual_problem [1];
   exhaust_temp_ignition_treshold = manual_problem [2]; }

	// Sprawdź czy turbina wystartowała
	if (check_start_tresholds() && !stop)
	{started = true; start = false;}
}

void turbine_work()
{
  lcd_update[AIR] = false;
  lcd_update[GAS] = true;
  if (tryb_PC) { control = jaki_rozkaz(); for (int i = 0; i<2; i++) {wyslij_wartosci('z', 't'); time_delay(autostart_delay/2, true);} if (control == rozkaz_stop) {stop=true;} }
  if (tryb_BT) { control = jaki_rozkaz_BT(); for (int i = 0; i<5; i++) {wyslij_wartosci('z', 't'); time_delay(autostart_delay/2, true);} if (control == rozkaz_stop) {stop=true;} }
  if (tryb_auto == true)
  {
    if (!tryb_PC)
    {
      if (!tryb_BT)
      {
        lcd_update[GAS] = false;
        lcd_print(0, "Ustaw regulator");
        lcd_print(1, "gazu");
    
        // Czekaj na ustawienie regulatora gazu
        while ((analogRead(POT_GAS) < gas_auto_level) && !digitalRead(SW_STOP) && !stop && (exhoust_temp > exhaust_temp_ignition_treshold) 
        && (crank_rev > (crank_rev_start_treshold/2.)) && !(oil_press < oil_press_treshold)) {
          communicate_with_measure();
          check_oil_press();}
          lcd_print(0, "Praca   gaz:");
          lcd_print(1, "p:      n:");
          lcd_update[GAS] = true;
      ///////////////////////////////////////
    
        // Praca turbiny, sterowanie moca poziomem otwarcia zaworu gazu
        while (!digitalRead(SW_STOP) && !stop && (exhoust_temp > exhaust_temp_ignition_treshold)
        && (crank_rev > (crank_rev_start_treshold/2.)) && !(oil_press < oil_press_treshold)) {
          set_PWM(GAS, analogRead(POT_GAS));
          communicate_with_measure();
          update_lcd();
          check_oil_press();}
        ////////////////////////////////////////
      }
      else
      {
        while (!digitalRead(SW_STOP) && !stop && (exhoust_temp > exhaust_temp_ignition_treshold)
        && (crank_rev > (crank_rev_start_treshold/2.)) && !(oil_press < oil_press_treshold)) {
          control = jaki_rozkaz_BT();
          switch (control)
          {
            case rozkaz_power_up:
              wyslij_wartosci_BT('x', '+'); // juz dodaje
              gas_auto_level += 20;
              if (gas_auto_level < 1023)
              { set_PWM(GAS, gas_auto_level); }
              else {gas_auto_level -= 20;}
              communicate_with_measure();
              update_lcd();
              wyslij_wartosci_BT('z', '+'); // dodalam
              break;
            case rozkaz_power_down:
              wyslij_wartosci_BT('x', '-'); // juz odejmuje
              gas_auto_level -= 20;
              if (gas_auto_level > 0)
              { set_PWM(GAS, gas_auto_level); }
              else {gas_auto_level += 20;}
              communicate_with_measure();
              update_lcd();
              wyslij_wartosci_BT('z', '-'); // odjelam
              break;
            case rozkaz_stop:
              stop = true;
              delay(10);
              break;
            case 0:  // nic nie rob
              communicate_with_measure();
              update_lcd();
              wyslij_wartosci_BT('y', 'y');
              break;
          } } }
    started = false;
    }
    else
    {
      while (!digitalRead(SW_STOP) && !stop && (exhoust_temp > exhaust_temp_ignition_treshold)
      && (crank_rev > (crank_rev_start_treshold/2.)) && !(oil_press < oil_press_treshold)) {
        control = jaki_rozkaz();
        switch (control)
        {
          case rozkaz_power_up:
            wyslij_wartosci('x', '+'); // juz dodaje
            gas_auto_level += 20;
            if (gas_auto_level < 1023)
            { set_PWM(GAS, gas_auto_level); }
            else {gas_auto_level -= 20;}
            communicate_with_measure();
            update_lcd();
            wyslij_wartosci('z', '+'); // dodalam
            break;
          case rozkaz_power_down:
            wyslij_wartosci('x', '-'); // juz odejmuje
            gas_auto_level -= 20;
            if (gas_auto_level > 0)
            { set_PWM(GAS, gas_auto_level); }
            else {gas_auto_level += 20;}
            communicate_with_measure();
            update_lcd();
            wyslij_wartosci('z', '-'); // odjelam
            break;
          case rozkaz_stop:
            stop = true;
            delay(10);
            break;
          case 0:  // nic nie rob
            communicate_with_measure();
            update_lcd();
            wyslij_wartosci('y', 'y');
            break;
        } } }
    started = false;
  }
  else
  {
	lcd_update[AIR] = false;
	lcd_update[GAS] = true;
	lcd_print(0, "Praca   gas:");
	while (!stop)
	  {
		if (!check_start_tresholds() || digitalRead(SW_STOP)) { stop = true; }
    else {
      set_PWM(GAS, analogRead(POT_GAS));
      communicate_with_measure();
      update_lcd();
      check_oil_press(); }
	  }
  oil_press_treshold = manual_problem [0];
  crank_rev_start_treshold = manual_problem [1];
  exhaust_temp_ignition_treshold = manual_problem [2];
  
	started = false;
  }
}

void turbine_stop()
{
  set_PWM(AIR, 0);
  set_PWM(GAS, 0);
	lcd_print(0, "Stop turbiny");
  if (tryb_auto) { gas_auto_level = 0; }
  if (tryb_PC) { wyslij_wartosci('x', 'p'); }
  if (tryb_BT) { wyslij_wartosci_BT('x', 'p'); }
  if (DO_vals[0] == OFF) // jezeli wylaczona pompa
  {set_DO(PUMP_OIL, ON);} // to odpal
	stop = true;
	set_DO(STOP, HIGH);
	set_DO(START, LOW);
	set_DO(VALVE_GAS, OFF);
	time_delay(1000, true);
	set_DO(VALVE_AIR, OFF);
  if (tryb_PC) { wyslij_wartosci('x', 'p'); }
  if (tryb_BT) { wyslij_wartosci_BT('x', 'p'); }
	lcd_print(0, "air:       stop");
	lcd_update[GAS] = false;
	lcd_update[AIR] = true;

	// Ramp UP
	int air_ramp_time = 10000;
	for (int air_reading = 0; air_reading< 1023; air_reading++)
	{
		set_PWM(AIR, air_reading);

		unsigned long start = millis();
		while (millis() - start < air_ramp_time / 1024)
		{
			if (air_reading % 100 == 0){
				communicate_with_measure();
        if (tryb_PC) { wyslij_wartosci('x', 'p'); }
        if (tryb_BT) { wyslij_wartosci_BT('x', 'p'); }
				update_lcd();
			}
		}
	}
 
	unsigned long start_cool = millis();
	while (millis() - start_cool<30000){
		communicate_with_measure();
    if (tryb_PC) { wyslij_wartosci('x', 'p'); }
    if (tryb_BT) { wyslij_wartosci_BT('x', 'p'); }
		update_lcd();}
    
	// Ramp DOWN
	for (int air_reading = 1023; air_reading > 0; air_reading--)
	{
		set_PWM(AIR, air_reading);
		unsigned long start = millis();
		while (millis() - start < air_ramp_time / 1024)
		{
			if (air_reading % 100 == 0){
				communicate_with_measure();
        if (tryb_PC) { wyslij_wartosci('x', 'p'); }
        if (tryb_BT) { wyslij_wartosci_BT('x', 'p'); }
				update_lcd();
			}
		}
	}
	set_PWM(AIR, 0);
	set_DO(PUMP_OIL, OFF);
	set_DO(PUMP_WATER, OFF);
	lcd_update[AIR] = false;
	started = false;
	stop = false;
	set_DO(STOP, LOW);
  for (int k=0; k < 2; k++)  // bo ciezko idzie bluetooth, daltego 2x wysyla ze zatrzymal
  {
    if (tryb_PC) { wyslij_wartosci('z', 'p'); }
    if (tryb_BT) { wyslij_wartosci_BT('z', 'p'); }
    time_delay(autostart_delay/2, true);
  }
  tryb_auto = false;
  tryb_PC = false;
  tryb_BT = false;
  control = 0;
}
