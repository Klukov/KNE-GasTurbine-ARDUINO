#include <OneWire.h>
#include <Wire.h>
#include <SFE_BMP180.h>
#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include "max6675.h"

//Wej�cia analogowe
byte PRESS_1  = 0;  //Gaz przed KS
byte PRESS_2  = 2;  //Olej za WC
byte PRESS_3  = 3;  //Powietrze za sprezarka
byte PRESS_4  = 8;  //Spaliny za TS
byte PRESS_5  = 9;  //Woda w KN
byte FLOW_1   = 6;  //Powietrze przed sprezarka
byte TEMP_1   = 7;  //Powietrze przed sprezarka
byte FLOW_2   = 1;  //Gaz przed KS
byte VOLTAGE_1  = 11; //Elektrycznosc alternator
byte CURRENT_1  = 10; //Elektrycznosc alternator

// Opis czujnik�w analogowych - AS - Analog Sensor
#define number_of_AS 10 // zmiana z 14 na 10
byte AS_pins[number_of_AS]    = { PRESS_1, PRESS_2, PRESS_3, PRESS_4, PRESS_5, FLOW_1, TEMP_1, FLOW_2, VOLTAGE_1, CURRENT_1}; // Usuniete temp2-temp5
unsigned int AS_raw_values[number_of_AS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // usuniete 4
float AS_values[number_of_AS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // usuniete 4
float (*AS_conv_funs[number_of_AS])(float) = {  convert_press_10, 
                        convert_press_4, 
                        convert_press_4, 
                        convert_press_4, 
                        convert_press_1, 
                        convert_bosh_flow, 
                        convert_bosh_temp, 
                        convert_omron_flow, 
                        convert_voltage_div, 
                        convert_current_sens};                       
String AS_quantities[number_of_AS] = {"1","1","1","1","1","2","0","2","4","5"};
String AS_places[number_of_AS]  = {"03","12","02","05","20","01","01","03","07","07"};   // w innych wersjach 20 może być 22

// Wej�cia cyfrowe czunik�ww cz�stotliwo�ci
byte FLOW_3    = 4;  //Olej przed WC
byte FLOW_4   = 5;  //Woda przed KN
byte REV_1    = 7;  //Wal TS
byte REV_2    = 6;  //Wal alternator

// Opis czujnik�w mierz�cych cz�stoliwo�� FS - frequency sensor
#define number_of_FS 4
byte FS_pins[number_of_FS]        = { FLOW_3, FLOW_4, REV_1, REV_2 };
boolean FS_states[number_of_FS]     = { LOW, LOW, LOW, LOW };
float FS_values[number_of_FS]     = { 0, 0, 0, 0};
float (*FS_conv_funs[number_of_FS])(float) = {  convert_adafruit_flow,
                        convert_adafruit_flow,
                        convert_freq_sens1,
                        convert_freq_sens2};
String FS_quantities[number_of_FS]    = {"2","2","3","3"};
String FS_places[number_of_FS]      = {"12","20","30","31"};

// Czujniki cyfrowe, protoko�owe
SFE_BMP180 BMP180;
double TEMP0, PRESS0;

// Czujniki temperatury DS18B20
#define number_of_DS18B20  7
byte DS18B20_pin = 14;
OneWire  ds(DS18B20_pin); // inicjalizacja magistrali OneWire obslugujacej czujniki temp ds18b20
byte DS18B20_ROMS[number_of_DS18B20 * 8] = {0x28,   0xFF,   0x47,   0x7,    0x11,   0x14,   0x0,    0x40, // Gaz przed KS
                      0x28, 0xFF, 0xB1, 0x39, 0x10, 0x14, 0x0,  0x8C, // Olej przed WC
                      0x28, 0xFF, 0xC4, 0xC,  0x11, 0x14, 0x0,  0x51, // Olej za WC
                      0x28,   0xFF,   0xB4,   0x11,   0x11,   0x14,   0x0,  0x99, // Powietrze za sprezarka
                      0x28, 0xFF,   0xC0,   0x33,   0x10,   0x14,   0x0,  0x37, // Woda KN
                      0x28, 0xFF,   0x4C,   0x7,    0x11,   0x14,   0x0,    0x30, // Woda zbiornik
                      0x28,   0xFF,   0xC0,   0x36,   0x11,   0x14, 0x0,  0x1D,}; // Spaliny za KN
String DS18B20_places[] = {"03",  "10", "11", "02", "20", "22", "08"}; // 11 nie istnieje, 3 pkt
float DS18B20_temps[number_of_DS18B20];

//Czujniki cyfrowe do termopar MAX6675 (x2) i MAX31855 (do KS)
float KS_temp;
float therm_1;
float therm_2;
float prev_KS_temp = 0;
Adafruit_MAX31855 thermocouple_KS(11,13,12); // W kolejności: CLK, CS, DO
MAX6675 thermocouple_1(8,9,3);              // W kolejności: CLK, CS, DO
MAX6675 thermocouple_2(17,15,16);          // W kolejności: CLK, CS, DO

// Transmisja parametr�w do Arduino control - większosc usunieta
#define num_of_tx_parameters 5
boolean com_started = false;
unsigned int tx_packets = 0;
unsigned int rx_packets = 0;    
boolean started = false;

// Lancuchy znakow do transmisji
String vals_str;
String serial_communication;



void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);
  BMP180.begin();
}

// Timing
unsigned int FS_timeout;        // czas na odpytywanie czujnikow czestotliwosci w ms - minimalna mierzona cz�stotliwosc to 1/FS_timeout. wyliczany dynamicznie
unsigned int conversion_time = 750;
unsigned int cycle_time = 1000; // 1s na cykl
unsigned long cycle_start;
unsigned long loop_counter = 0;

void loop()
{
  cycle_start = millis();
  start_DS18B20_conversion();
  read_BMP180();  
  read_AS();
  convert_AS();
  FS_timeout = conversion_time - (millis() -  cycle_start);
  read_FS_freqs();
  read_DS18B20();
  thermocouples();
  communicate_with_control(); 
  vals_str += ("\n");
  do
  {
  Serial.print(vals_str); vals_str = "";
  }
  while ((millis()-cycle_start)<(cycle_time-1));
  loop_counter++;
}

void start_DS18B20_conversion()
{
  for (byte DS18B20_num = 0; DS18B20_num < number_of_DS18B20; DS18B20_num++)
  { byte DS18B20_rom[8];
    select_DS18B20(DS18B20_num, DS18B20_rom);
    start_conversion(DS18B20_rom); }
}

void read_AS()
{
  for (byte AS_index = 0; AS_index<number_of_AS; AS_index++)
  {AS_raw_values[AS_index] = analogRead(AS_pins[AS_index]);}
}

void convert_AS()
{
  for (byte AS_index=0; AS_index<number_of_AS; AS_index++)
  {
    float voltage = analog_to_voltage(AS_raw_values[AS_index]);
    AS_values[AS_index] = AS_conv_funs[AS_index](voltage);
      vals_str+=(AS_places[AS_index]); 
      vals_str+=(AS_quantities[AS_index]); 
      vals_str+=(":");
      vals_str+=(AS_values[AS_index]);
      vals_str+='\n';
  }
}

void read_BMP180()
{
  int conversion_time;
  conversion_time = BMP180.startTemperature();
  delay(conversion_time);
  BMP180.getTemperature(TEMP0);
  conversion_time = BMP180.startPressure(3);  // argument to poziom dok��dno�ci odczytu ci�nienia. 0-3
  delay(conversion_time);
  BMP180.getPressure(PRESS0, TEMP0);
    vals_str+=("000:"); //wyrzucone /t między temp i ooeczeniem, otoczeniem, a \t i \t na koncu
    vals_str+=(TEMP0); vals_str+='\n';
    vals_str+=("001:"); 
    vals_str+=PRESS0; vals_str+='\n';
}

void read_FS_freqs()
{
  unsigned int pulses[number_of_FS] = {0, 0, 0, 0};
  boolean state = LOW;
  boolean prev_state[number_of_FS] = {HIGH, HIGH, HIGH, HIGH};  
  unsigned long start = millis();         // odczytujemy czas pocz�tkowy
  unsigned long loop_counter = 0;
  boolean previous_state = HIGH;
  unsigned int my_pulses = 0;
  while (millis() - start <=FS_timeout) // p�tla kt�ra b�dzie dzia�a�a przez czas freq_sensor_poll_time
  {
    for (byte FS_num = 0; FS_num<number_of_FS; FS_num++)
    {
      state = digitalRead(FS_pins[FS_num]);
      if (state == HIGH && prev_state[FS_num] == LOW)
      {pulses[FS_num]++;}
      prev_state[FS_num] = state;
    }
    delayMicroseconds(200);  // bylo 200
    loop_counter++;
  }
  for (byte FS_num = 0; FS_num<number_of_FS; FS_num++)
  {
    FS_values[FS_num] = FS_conv_funs[FS_num](pulses[FS_num] / ((float)FS_timeout/1000));
    vals_str+=(FS_places[FS_num]); vals_str+=(FS_quantities[FS_num]); vals_str+=(":"); vals_str+=(FS_values[FS_num]); vals_str+='\n';
  }
}

void read_DS18B20()
{  
  for (byte DS18B20_num = 0; DS18B20_num < number_of_DS18B20; DS18B20_num++)
  {
    byte DS18B20_rom[8];
    select_DS18B20(DS18B20_num, DS18B20_rom);
    float temp = read_temperature_sensor(DS18B20_rom);
    if (temp >0 && temp < 200){ DS18B20_temps[DS18B20_num] = temp; }
    vals_str+=(DS18B20_places[DS18B20_num]); vals_str+=("0"); vals_str+=(":"); vals_str+=(DS18B20_temps[DS18B20_num]); vals_str+='\n';
  }
}

                       
// KOMUNIKACJA Z ARDUINO STERUJACYM         <>          KOMUNIKACJA Z ARDUINO STERUJĄCYM         <>           KOMUNIKACJA Z ARDUINO STERUJĄCYM

void communicate_with_control()
{
  establish_communication();  
  unsigned long start = millis();
  communicate();
}

void establish_communication()
{
  do
  {Serial1.print('s'); } 
  while (Serial1.read() !='s');
}

void communicate()
{
  serial_communication = ("");
  serial_communication += (AS_values[1]); serial_communication += (";");
  serial_communication += (AS_values[2]); serial_communication += (";");
  serial_communication += (FS_values[2]); serial_communication += (";");
  serial_communication += (AS_values[5]); serial_communication += (";");
  serial_communication += (KS_temp);
  Serial1.println(serial_communication);
  Serial1.flush();
  tx_packets++;
}

