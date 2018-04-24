void erase_row(byte row)
{
	lcd.setCursor(0, row);
	lcd.print("                ");	// string 16 pustych znak�w	
	lcd.setCursor(0, row);
}

void lcd_print(byte row, char* text)
{

	erase_row(row);
	lcd.setCursor(0, row);
	lcd.print(text);
	
}

void update_lcd()
{
	if (lcd_update[AIR])
	{
		lcd.setCursor(4, 0);
		lcd.print("   ");
		lcd.setCursor(4, 0);
		lcd.print(PWM_vals[AIR]);
	}
	if (lcd_update[GAS])
	{
		lcd.setCursor(12, 0);
		lcd.print("   ");
		lcd.setCursor(12, 0);
		lcd.print(PWM_vals[GAS]);
	}
	if (lcd_update[OIL])
	{
		lcd.setCursor(2, 1);
		lcd.print("    ");
		lcd.setCursor(2, 1);
		lcd.print(oil_press,2);
	}
	if (lcd_update[REV])
	{
		lcd.setCursor(10, 1);
		lcd.print("      ");
		lcd.setCursor(10, 1);
		lcd.print((long)crank_rev);
	}	
}

void update_lcd_header()
{
	if (lcd_update[AIR] || lcd_update[GAS]){erase_row(0);}
	if (lcd_update[REV] || lcd_update[OIL]){erase_row(1);}
	if (lcd_update[AIR])
	{
		lcd.setCursor(0, 0);
		lcd.print("air:");
	}
	if (lcd_update[GAS])
	{
		lcd.setCursor(8, 0);
		lcd.print("gas:");
	}
	if (lcd_update[OIL])
	{
		lcd.setCursor(0, 1);
		lcd.print("p:");
	}
	if (lcd_update[REV])
	{
		lcd.setCursor(8, 1);
		lcd.print("n:     ");
	}
	lcd.setCursor(0,0);
}
