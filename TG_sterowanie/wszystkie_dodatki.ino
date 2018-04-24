void set_pins()
{
  // Piny analogowe
  pinMode(POT_GAS, INPUT);
  pinMode(POT_AIR, INPUT);
  // Sterowanie wyj�ciami
  pinMode(PWM_AIR, OUTPUT);
  pinMode(PWM_GAS, OUTPUT);
  pinMode(RELAY_PUMP_OIL, OUTPUT);
  pinMode(RELAY_SPARK, OUTPUT);
  pinMode(RELAY_VALVE_AIR, OUTPUT);
  pinMode(RELAY_NC, OUTPUT);
  pinMode(RELAY_PUMP_WATER, OUTPUT);
  pinMode(RELAY_VALVE_GAS, OUTPUT);
  // Panel kontrolny
  pinMode(SW_MAN_AUTO, INPUT);
  pinMode(DIODE_START, OUTPUT);
  pinMode(SW_PUMP, INPUT);
  pinMode(SW_SPARK, INPUT);
  pinMode(SW_START, INPUT);
  pinMode(SW_STOP, INPUT);
  pinMode(SW_VALVE_GAS, INPUT);
  pinMode(SW_VALVE_AIR, INPUT);
  pinMode(DIODE_STOP, OUTPUT);
  pinMode(SW_TACT2, INPUT);
  pinMode(SW_TACT1, INPUT);
}

void zero_outputs()
{
  for (byte DO_device_index = 0; DO_device_index < num_of_DO-2; DO_device_index++) {
    byte pin = DO_pins[DO_device_index];
    digitalWrite(pin, OFF);}

  for (byte PWM_device_index = 0; PWM_device_index < num_of_PWM ; PWM_device_index++) {
    byte pin = PWM_pins[PWM_device_index];
    analogWrite(pin, 0); }
}


boolean check_control_panel_I()
{
  byte control_panel_DI_quantity  = 5;
  byte control_panel_AI_quantity  = 2;
  byte on_num = 0;
  int AI_check_treshold = 20;
  boolean checked = false;
    
  for (int DI_num = 0; DI_num < control_panel_DI_quantity; DI_num++)
  {if(digitalRead(control_panel_DI[DI_num])){on_num++;}}
  
  for (int AI_num = 0; AI_num < control_panel_AI_quantity; AI_num++)
  {if(analogRead(control_panel_AI[AI_num]) > AI_check_treshold){on_num++;}}
 
  if (on_num == 0)
  {checked = true;}
  return checked;
}

boolean check_start_tresholds()
{
  boolean started = true;
  if(air_flow < air_flow_start_treshold){started = false;}
  if(air_press < air_press_start_treshold){started = false;}
  if(crank_rev < crank_rev_start_treshold){started = false;}
  return true;   
}

void check_oil_press()
{
  if (oil_press < oil_press_treshold) {
    stop = true;
    lcd_print(0, "p olej nis stop!;");}
}

void set_PWM(byte device, int value)
{
  byte pin = PWM_pins[device];
  if (value < 6) {
    analogWrite(pin, 0); 
    PWM_vals[device] = 0;   
  }
  else
  {
    byte PWM_control_level = map(value, 0, 1023, PWM_adjust[device][0], PWM_adjust[device][1]); // z 1 0 sie zrobilo
    analogWrite(pin, PWM_control_level); 
    PWM_vals[device] = map(value, 0, 1023, 0, 100);
  }
}

void set_DO(byte device, boolean state)
{
  byte pin = DO_pins[device];
  digitalWrite(pin, state); 
  DO_vals[device] = state;
}


///////////////////////////////////////////////////////////////////////////////////////
//   >KOMUNIKACJA ZE STEROWNIKIEM<        <>        >KOMUNIKACJA ZE STEROWNIKIEM<

void communicate_with_measure()
{
  if (establish_communication()) { communicate(); }
}

boolean establish_communication()
{
  if (Serial1.read() == 's'){ 
    Serial1.print('s'); 
    return true; 
  } else{ return false; }
}

void communicate()
{
  float termopara;
  float cisnienie_oleju;
  while (Serial1.available()<=15){} // wait for buffer to load          
  cisnienie_oleju = Serial1.parseFloat(); 
  air_press = Serial1.parseFloat(); 
  crank_rev = Serial1.parseFloat();
  air_flow  = Serial1.parseFloat();   
  termopara = Serial1.parseFloat();
 
  if (termopara != 0) {
    for (int i=0; i<(history_number_KS-1); i++) {
      historia_termopar[(history_number_KS-1-i)] = historia_termopar[(history_number_KS-2-i)]; }
    historia_termopar[0] = termopara;
    exhoust_temp = 0;
    for(int i=0; i<history_number_KS; i++) {
      exhoust_temp += (historia_termopar[i]/history_number_KS); } }
      
  for (int i=0; i<(history_number_Oil-1); i++) {
    historia_cisnienia_oleju [(history_number_Oil-1-i)] = historia_cisnienia_oleju [(history_number_Oil-1-i-1)]; }
  historia_cisnienia_oleju [0] = cisnienie_oleju;
  oil_press = 0;
  for (int i=0; i<history_number_Oil; i++) {
    oil_press += (historia_cisnienia_oleju [i]/history_number_Oil); }
  transmit = true;
  rx_packets++;

  // CZYSZCZENIE RESZTY BUFORA
  while (Serial1.available() > 0) {
    char x = Serial1.read();}
}


