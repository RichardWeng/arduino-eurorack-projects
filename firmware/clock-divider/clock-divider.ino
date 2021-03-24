	
// CONFIGURATION =============================================================

const bool DEBUG = true; // FALSE to disable debug messages on serial port

const byte CLOCK_INPUT = 2; // Input signal pin, must be usable for interrupts
const byte RESET_INPUT = 3; // Reset signal pin, must be usable for interrupts

const byte DIVSELECT1_SWITCH = A1;
const byte DIVSELECT2_SWITCH = A2;
const byte GATEMODE_SWITCH = A3; // 2 positions switch to chose between gate and trigger mode 

const byte DIVISIONS_OUTPUT[] { 4, 5, 6, 7, 8, 9, 10, 11 }; // Output pins

const byte NUMBER_OF_DIVISIONS = 8;
const word DIVISIONS[][NUMBER_OF_DIVISIONS] {
	{ 	1,	2,	4,	8, 	16,  32,  64, 128 },
	{ 	1,	2,	3,	5, 	 7,  11,  13,  17 },
	{ 	1,	2,	3,	4, 	 5,   6,   7,   8 },
	{ 	0,	0,	0,	0, 	 0,   0,   0,	0 } // This set is not implemented in hardware
}; // Integer divisions of the input clock

const byte DEBOUNCE_DELAY = 50; // Debounce delay for all buttons

// ===========================================================================

bool reset_input = 0,		old_reset_input = 0;
bool gateMode_switch = 0,	old_gateMode_switch = 0;	// If 1, gate mode is active, if 0, standard trig mode is active
bool divSelect1_switch = 0,	old_divSelect1_switch = 0;
bool divSelect2_switch = 0,	old_divSelect2_switch = 0;

byte divisionsSet = 0; // The set of divisions in use, from 0 to 3. Coded in binary by DIV_SELECT inputs. Set 3 not implemented in hardware (needs 1P4T switch).

long count = -1; // Input clock counter, -1 in order to go to 0 on the first pulse

unsigned long checktime = 0;

volatile bool clock = false; // Clock signal digital reading, set in the clock ISR
volatile bool clockFlag = false; // Clock signal change flag, set in the clock ISR
volatile bool resetFlag = false; // Reset flag, set in the reset ISR

void setup() {
	
	// Debugging
	if (DEBUG) Serial.begin(9600);
	
	// Input
  	pinMode(CLOCK_INPUT, INPUT_PULLUP);
	pinMode(RESET_INPUT, INPUT_PULLUP);
	pinMode(DIVSELECT1_SWITCH, INPUT_PULLUP);
	pinMode(DIVSELECT2_SWITCH, INPUT_PULLUP);
	pinMode(GATEMODE_SWITCH, INPUT_PULLUP);
	
	// Setup outputs
	for (byte i = 0; i < NUMBER_OF_DIVISIONS; i++) {
		pinMode(DIVISIONS_OUTPUT[i], OUTPUT);
		digitalWrite(DIVISIONS_OUTPUT[i], LOW);
	}

	// Interrupts
	attachInterrupt(digitalPinToInterrupt(CLOCK_INPUT), isrClock, CHANGE);
	attachInterrupt(digitalPinToInterrupt(RESET_INPUT), isrReset, FALLING);

	// Initial state for reset input
	old_reset_input = digitalRead(RESET_INPUT);

	// Read divisions set switch
	divSelect1_switch = digitalRead(DIVSELECT1_SWITCH);
	divSelect2_switch = digitalRead(DIVSELECT2_SWITCH);
	readDivisionsSet();

}

void loop() {

	// Read switches
	gateMode_switch = digitalRead(GATEMODE_SWITCH);
	divSelect1_switch = digitalRead(DIVSELECT1_SWITCH);
	divSelect2_switch = digitalRead(DIVSELECT2_SWITCH);


	// Mode switch
	if (gateMode_switch != old_gateMode_switch) {
		old_gateMode_switch = gateMode_switch;
		if (DEBUG) {
			Serial.print("Gate mode changed: ");
			Serial.println(gateMode_switch);
		}
	}

	// Divisions sets switch
	if ((divSelect1_switch != old_divSelect1_switch)|(divSelect2_switch != old_divSelect2_switch)) {
		old_divSelect1_switch = divSelect1_switch;
		old_divSelect2_switch = divSelect2_switch;
		// Update division set
		readDivisionsSet();
	}

	
	// Clock signal changed
	if (clockFlag) {
		clockFlag = false;
		
		/*if (DEBUG) {
			Serial.print("Clock signal changed: ");
			Serial.println(clock);
		}*/

		if (clock) {
			
			// Clock rising, update counter
			if (resetFlag) {
				resetFlag = false;
				count = 0;
				
				if (DEBUG) {
					Serial.println("Reset");
				}

			} else {
				count++;
			}
			
			/*if (DEBUG) {
				Serial.print("Counter changed: ");
				Serial.println(count);
			}*/
			
		}
		
		// Update outputs according to current trig/gate mode
		if (gateMode_switch) {
			processGateMode();
		} else {
			processTriggerMode();
		}
		
	}

}

void processTriggerMode() {
	
	// Copy input signal on current divisions
	if (clock) {
		
		// Rising edge, go HIGH on current divisions
		for (byte i = 0; i < NUMBER_OF_DIVISIONS; i++) {
			bool v = (count % DIVISIONS[divisionsSet][i] == 0);
			digitalWrite(DIVISIONS_OUTPUT[i], v ? HIGH : LOW);
		}
		
	} else {
		
		// Falling edge, go LOW on every output
		for (byte i = 0; i < NUMBER_OF_DIVISIONS; i++) {
			digitalWrite(DIVISIONS_OUTPUT[i], LOW);
		}
		
	}
	
}

void processGateMode() {
	
	// Keep outputs high for ~50% of divided time
	for (byte i = 0; i < NUMBER_OF_DIVISIONS; i++) {
		
		// Go HIGH on the rising edges that corresponds to the division
		int modulo = (count % DIVISIONS[divisionsSet][i]);
		if (clock && modulo == 0) {
			digitalWrite(DIVISIONS_OUTPUT[i], HIGH);
		}
		
		// Go LOW on rising edges for even divisions and falling edges for odd divisions,
		// considering the edges that corresponds to the half value of the division
		if (modulo == (int)(floor(DIVISIONS[divisionsSet][i] / 2.0))) {
			bool divisionIsOdd = (DIVISIONS[divisionsSet][i] % 2 != 0);
			if ((clock && !divisionIsOdd) || (!clock && divisionIsOdd)) {
				digitalWrite(DIVISIONS_OUTPUT[i], LOW);
			}
		}
		
	}
	
}

void readDivisionsSet() {
	divisionsSet = !divSelect2_switch;
	divisionsSet |= !divSelect1_switch << 1;
	if (DEBUG) {
		Serial.print("Divisions set changed: ");
		Serial.println(divisionsSet);
	}
}

void isrClock() {
	clock = (digitalRead(CLOCK_INPUT) == LOW); //remember clock signal is reversed to use internal pullup resistor
	clockFlag = true;
}

void isrReset() {


	resetFlag = true;
}