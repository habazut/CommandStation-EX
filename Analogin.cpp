/*
 *  © 2020, Harald Barth
 *  
 *  This file is part of DCC-EX
 *
 *  This is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  It is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CommandStation.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"
#ifdef ANALOGINPORT
#include "Analogin.h"
#include "AnalogReadFast.h"
#include "DCC.h"
#include "DIAG.h"

void Analogin::setup() {
    pinMode(A10, INPUT);
    pinMode(A11, INPUT);
}

void Analogin::loop() {
    static int a10old=-1,a11old=-1;
    static uint8_t i = 0;
    static int a10=-1,a11=-1;
    int d10, d11;
    int t, d, tspeed;
    static unsigned long int time = 0;

    if ((t = millis()) - time < 10) return;
    time=t;

    a10=analogReadFast(A10);
    a11=analogReadFast(A11);
    d10=a10-a10old;
    d11=a11-a11old;
    d = a10-a11;
    if (d10>10 || d10<-10 || d11>10 || d11<-10) {
	if (d < 10 && d > -10) d = 0;
	tspeed = d * 10 / 32;                 // convert range from DAC steps to DCC steps
	if (tspeed < 0  ) tspeed = -tspeed;   // fix sign
	if (tspeed > 126) tspeed = 126;       // cap at 126
	if (tspeed != 0 ) tspeed++;           // 1-126 => 2-127           
	DIAG(F("A10=%d A11=%d Tspeed=%d\n"),a10,a11,tspeed);
	DCC::setThrottle(3, tspeed, d<0 ? 0 : 1);
    }
    a10old=a10;
    a11old=a11;
    i++;
}
#endif // ANALOGINPORT
