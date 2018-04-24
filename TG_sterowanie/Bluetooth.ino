int jaki_rozkaz_BT ()
{
  if (Serial3.available() > 1)
  {
    check = Serial3.read();
    rozkaz = Serial3.read();
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
      czyszczenie_bufora_BT();
    }
    else
    {
      czyszczenie_bufora_BT();
      return 0;
    }
  }
  else
  {
    return 0;
  }
}

void czyszczenie_bufora_BT ()
{
  while (Serial3.available() > 0)
  {char x = Serial3.read();}
}

void wyslij_wartosci_BT (char x, char y) // program na kompie czyta wdl okreslonej kolejnosci
{
  String dane = "\r";
  dane += x; dane += "\r\n";
  dane += y; dane += "\r\n";
  float wysylany_gaz = ((((float)gas_auto_level)*100.0)/ 1024.0);
  dane += wysylany_gaz; dane += "\r\n";
  dane += crank_rev; dane += "\r\n";
  dane += exhoust_temp; dane += "\r\n";
  dane += oil_press; dane += "\r\n";
  Serial3.print(dane);
}
