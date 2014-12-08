Arduino_RGB_Temperature_Board
=============================

7-Segment board with to rows for temperature (inside/outside) and RGB-LEDs showing the temperature in color


Serial Input
--

  1:12,5\n

and 

  2:-3,5\n

(decimal point can be . or ,)


FHEM Quick n Dirty (for S300)
--
# Show Temperature on RGB-Board
define Outdoor_Temp_Board notify Indoor:T:.* {\
  open(FILE,">/dev/ttyUSB0");;\
  print FILE "1:";; print FILE %EVTPART1;; print FILE "\n";;\
  close(FILE);;\
}

# Show Temperature on RGB-Board
define Outdoor_Temp_Board notify Outdoor:T:.* {\
  open(FILE,">/dev/ttyUSB0");;\
  print FILE "2:";; print FILE %EVTPART1;; print FILE "\n";;\
  close(FILE);;\
}
