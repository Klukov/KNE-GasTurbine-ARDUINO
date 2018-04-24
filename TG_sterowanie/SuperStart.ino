int jaki_rozkaz ()
{
  if (Serial.available() > 1)
  {
    check = Serial.read();
    rozkaz = Serial.read();
    if (check == 'x')
    {
      if (rozkaz == 't')
      {
        return rozkaz_start;
      }
      else if (rozkaz == '+')
      {
        return rozkaz_power_up;
      }
      else if (rozkaz == '-')
      {
        return rozkaz_power_down;
      }
      else if (rozkaz == 'p')
      {
        return rozkaz_stop;
      }
      else
      {
        return 0;
      }
      czyszczenie_bufora();
    }
    else
    {
      czyszczenie_bufora();
      return 0;
    }
  }
  else
  {
    return 0;
  }
}

void czyszczenie_bufora ()
{
  while (Serial.available() > 0)
  {char x = Serial.read();}
}

void wyslij_wartosci (char x, char y) // program na kompie czyta wdl okreslonej kolejnosci
{
  String dane = "";
  dane += x; dane += "\r\n";
  dane += y; dane += "\r\n";
  float wysylany_gaz = ((((float)gas_auto_level)*100.00)/ 1024.00);
  dane += wysylany_gaz; dane += "\r\n";
  dane += crank_rev; dane += "\r\n";
  dane += exhoust_temp; dane += "\r\n";
  dane += oil_press; dane += "\r\n";
  Serial.print(dane);
}
