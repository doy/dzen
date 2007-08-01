#!/bin/sh
#
# (c) 2007 by Robert Manea <rob dot manea @ gmail dot com>
#
# KITT Scanner for dzen - a man, a car, a dzen
#


#---[ KITT configuration ]----------------------------------------------

SCANNER_LEDS=17 
LED_SPACING=3
LED_WIDTH=25
LED_HEIGHT=10

INACTIVE_LED_COLOR=darkred
ACTIVE_LED_COLOR=red
BG=black

SLEEP=0.1

DZEN=/usr/local/bin/dzen2
DZENOPTS="-bg $BG -fg $INACTIVE_LED_COLOR"

#-----------------------------------------------------------------------

DFG="^fg(${INACTIVE_LED_COLOR})"
LFG="^fg(${ACTIVE_LED_COLOR})"

RECT="^r(${LED_WIDTH}x${LED_HEIGHT})"

i=; j=1; SIGN='+'

nr_list_leds() {
	l=1
	lnr=$1

	while [ $l -le $lnr ]; do
		NRLIST=${NRLIST}' '${l}
		l=`expr $l + 1`
	done

	echo $NRLIST
}

while :; do
    for i in `nr_list_leds $SCANNER_LEDS`; do
        if [ "$i" -eq "$j" ]; then
            KBAR=${KBAR}"^p(${LED_SPACING})"${LFG}${RECT}${DFG}
        else
            KBAR=${KBAR}"^p(${LED_SPACING})"${RECT}
        fi

    done

    echo $KBAR; KBAR=
    
    if [ $SIGN = '+' ] && [ $j -ge $SCANNER_LEDS ]; then
        j=$SCANNER_LEDS
        SIGN='-'
    fi
    if [ $SIGN = '-' ] && [ $j -eq 1 ]; then
        j=1
        SIGN='+'
    fi
        
    j=`expr $j $SIGN 1`

    sleep $SLEEP;
done | $DZEN $DZENOPTS
