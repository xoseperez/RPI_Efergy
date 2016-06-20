
/*---------------------------------------------------------------------

EFERGY E2 CLASSIC RTL-SDR DECODER via rtl_fm

Copyright 2013 Nathaniel Elijah

Permission is hereby granted to use this Software for any purpose
including combining with commercial products, creating derivative
works, and redistribution of source or binary code, without
limitation or consideration. Any redistributed copies of this
Software must include the above Copyright Notice.

THIS SOFTWARE IS PROVIDED "AS IS". THE AUTHOR OF THIS CODE MAKES NO
WARRANTIES REGARDING THIS SOFTWARE, EXPRESS OR IMPLIED, AS TO ITS
SUITABILITY OR FITNESS FOR A PARTICULAR PURPOSE.

Compile:

gcc -o EfergyRPI_001 EfergyRPI_001.c -lm

Execute using the following parameters:

rtl_fm -f 433500000 -s 250000 -r 96000 -g 19.7 2>/dev/null | ./EfergyRPI_001

--------------------------------------------------------------------- */

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

#define VOLTAGE         230	/* Refernce Voltage */
#define PREAMBLE_COUNT	40 	/* Number of high(1) samples for a valid preamble */
#define MINLOWBIT 		3 	/* Number of high(1) samples for a logic 0 */
#define MINHIGHBIT 		8	/* Number of high(1) samples for a logic 1 */
#define E2BYTECOUNT		8	/* Efergy E2 Message Byte Count */
#define FRAMEBITCOUNT	64	/* Number of bits for the entire frame (not including preamble) */


int calculate_watts(char bytes[]) {

	char tbyte;
	double current_adc;
	int result;
	int i;

	/* add all captured bytes and mask lower 8 bits */
	tbyte = 0;

	for(i=0;i<7;i++)
		tbyte += bytes[i];

	tbyte &= 0xff;

	/* if checksum matches get watt data */
	if (tbyte == bytes[7]) {

        current_adc = (bytes[4] * 256) + bytes[5];
    	result  = (VOLTAGE * current_adc) / ((double) 32768 / (double) pow(2,bytes[6]));

   		printf("%d\n",result);
		fflush(stdout);
		return 1;

	}

	//printf("Checksum Error \n");

	return 0;
}

void  main (int argc, char**argv) {

    int sample;
    int curval;
    int prvval;
    char bytearray[9];
    char bytedata;
    int hctr;
    int bitpos;
    int bytecount;
    int dbit;
    int state;
    long center;
    int i;

	/* initialize variables */
	sample = 0;
    curval = 0;
    prvval = 0;
	bytedata = 0;
	bytecount = 0;
	hctr = 0;
	bitpos = 0;
	dbit = 0;
    state = 0; // 0: idle, 1: preamble, 2: frame
	center = 0;

	while( !feof(stdin) ) {

		sample = (int16_t) (fgetc(stdin) | fgetc(stdin)<<8);

        // Dinamically compute wave center
        center = center * 0.99 + sample * 0.01;

        curval = (sample < center) ? 0 : 1;

        /* Detect for positive edge of frame data */
		if ((curval == 1) && (prvval == 0)) {

            hctr = 0;

        /* count samples at high logic */
        } else if ((curval == 1) && (prvval == 1)) {

			hctr++;
			if ((hctr > PREAMBLE_COUNT) && (state == 0)) {
                //printf("Start preamble\n");
                state = 1;
            }

        /* at negative edge */
        } else if ((curval == 0) && (prvval == 1)) {

			if ((hctr > MINLOWBIT) && (state == 2)) {

                dbit++;
				bitpos++;
				bytedata = bytedata << 1;
				if (hctr > MINHIGHBIT) {
					bytedata = bytedata | 0x1;
                }

                // full byte
				if (bitpos > 7) {

					bytearray[bytecount] = bytedata;
					bytecount++;
					bytedata = 0;
					bitpos = 0;

					/* calculate watt data when received E2BYTECOUNT bytes */
					if (bytecount == E2BYTECOUNT) {
						if (calculate_watts(bytearray) == 0) {
                            hctr  = 0;
            				bytedata = 0;
            				bytecount = 0;
            				bitpos = 0;
            				dbit = 0;
            				state = 0;
                        }
					}
				}

                /* reset frame variables */
				if (dbit > FRAMEBITCOUNT) {
					bitpos = 0;
					bytecount = 0;
					dbit = 0;
					state = 0;
					bytedata = 0;
				}

			}

			hctr = 0;

        /* at low logic */
		} else {
			hctr = 0;
        }

		/* end of preamble, start of frame data */
		if ((hctr == 0) && (state == 1)) {
			state = 2;
		}

        prvval = curval;

	} /* while */

}
