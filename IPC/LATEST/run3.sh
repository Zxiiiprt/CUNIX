echo `gcc ATM.c -w -lm -o atm`
echo `gcc DBeditor.c -lm -w  -o editor`
echo `gcc DBserver.c -lm -w -o my_server`
echo `gcc interestcalc.c -lm -w -o ic`

gnome-terminal -x ./my_server
gnome-terminal -x ./editor
gnome-terminal -x ./atm
gnome-terminal -x ./ic

