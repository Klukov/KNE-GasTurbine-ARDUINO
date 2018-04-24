// Funkcje do obslugi DS18B20 //
void select_DS18B20(byte sensor_number, byte *sensor_rom)
{
	int rom_start = sensor_number * 8;	
	for (byte stepper = 0; stepper < 8; stepper++) // odczytaj z tablicy pojedynczy [8bajt] sensor.
	{
		sensor_rom[stepper] = DS18B20_ROMS[rom_start + stepper];
	}
}

void start_conversion(byte *sensor_rom )
{
	ds.reset();
	ds.select(sensor_rom);
	ds.write(0x44, 1);        // start conversion, with parasite power on at the end
}

float convert_hex_to_temp1(byte *hex_data)
{
  int16_t raw = (hex_data[1] << 8) | hex_data[0];
	byte cfg = (hex_data[4] & 0x60);
	// at lower res, the low bits are undefined, so let's zero them
	if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
	else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
	else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
	//// default is 12 bit resolution, 750 ms conversion time
  return (float)raw / 16.0;
}

float convert_hex_to_temp(byte *hex_data)
{
	int temp;
	int HighByte, LowByte, TReading, SignBit, Tc_100;
	LowByte = hex_data[0];
	HighByte = hex_data[1];
	TReading = (HighByte << 8) + LowByte;
	SignBit = TReading & 0x8000;  // test most sig bit
	if (SignBit) // negative
	{
		TReading = (TReading ^ 0xffff) + 1; // 2's comp
	}
	Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25
	return Tc_100/100;
}

float read_temperature_sensor(byte *sensor_rom)
{
	byte hex_data[9]; // data z czujnika , patrz nizej
	ds.reset();
	ds.select(sensor_rom);
	ds.write(0xBE);         // Read Scratchpad
	for (byte byte = 0; byte < 9; byte++) {           // we need 9 bytes
		hex_data[byte] = ds.read();
	}
	return convert_hex_to_temp1(hex_data);
}


// Funkcje konwertuj�ce odczyt czujnik�w analagowych na ich warto��.

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
 return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float analog_to_voltage(unsigned int val)
{
	return mapfloat(val, 0, 1023, 0, 5); 
}

float convert_omron_flow(float voltage)
{
	return 0.2611*pow(voltage, 4)
		  -2.3259*pow(voltage, 3) 
		  +8.3539*pow(voltage, 2) 
		  -6.2458 * voltage 
		  -0.0432;
}

float convert_bosh_flow(float voltage)
{
	return 14.94*pow(voltage,3)
		- 72.767*pow(voltage, 2)
		+ 159.06*voltage
		- 108.77;
}

float convert_press_1(float voltage)
{
	return mapfloat(voltage, 0, 5, 0, 1);
}

float convert_press_4(float voltage)
{
	return mapfloat(voltage, 0, 5, 0, 4);
}

float convert_press_10(float voltage)
{
	return mapfloat(voltage, 0, 5, 0, 10);
}

float convert_bosh_temp(float voltage)
{
	return 27.799*voltage-29.969;
}

float convert_voltage_div(float voltage)
{
	return voltage;
}

float convert_current_sens(float voltage)
{
	return voltage;
}

float convert_thermocouple(float voltage)
{
	return voltage;
}

float convert_adafruit_flow(float freq)
{
	return freq;
}

float convert_freq_sens1(float freq)
{
	return freq;
}

float convert_freq_sens2(float freq)
{
	return freq*15;
}
