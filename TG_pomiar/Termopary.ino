// Funkcje służące do odczytania termopar przez moduły MAX6675 i MAX31855
// funkcje wyrzucają już gotową temperaturę

void thermocouples()
{
   prev_KS_temp = KS_temp; // pozostalosc po poprzedniej wersji
   KS_temp = thermocouple_KS.readCelsius(); // MAX31855 - KS
   therm_1 = thermocouple_1.readCelsius(); // MAX6675 - miedzystopniowe
   therm_2 = thermocouple_2.readCelsius(); // MAX6675 - za TC

   //MAX31855 - KS
   if (isnan(KS_temp)) {
     vals_str += ("040:-999\n");
     KS_temp = 0;
   } else {
     vals_str += ("040:"); 
     vals_str += (KS_temp);
     vals_str += ("\n");
   }

   // MAX6675 - miedzystopniowe
   if (isnan(therm_1)) {
     vals_str += ("050:-999\n");
   } else {
     vals_str += ("050:"); 
     vals_str += (therm_1);
     vals_str += ("\n");
   }

  // MAX6675 - za TC
   if (isnan(therm_2)) {
     vals_str += ("060:-999\n");
   } else {
     vals_str += ("060:"); 
     vals_str += (therm_2);
     vals_str += ("\n");
   }
}

