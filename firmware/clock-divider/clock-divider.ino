	
// CONFIGURATION =============================================================

const bool DEBUG = false; // FALSE to disable debug messages on serial port

const byte CLOCK_INPUT = 2; // Input signal pin, must be usable for interrupts
const byte RESET_INPUT = 3; // Reset signal pin, must be usable for interrupts

const byte DIV_SELECT_MSB = A1;
const byte DIV_SELECT_LSB = A2;
const byte GATE_MODE_SWITCH = A3; // 2 positions switch to chose between gate and trigger mode 

const byte DIVISIONS_OUTPUT[] { 4, 5, 6, 7, 8, 9, 10, 11 }; // Output pins

const byte NUMBER_OF_DIVISIONS = 8;
const word DIVISIONS[][NUMBER_OF_DIVISIONS] {
	{ 	1,	2,	4,	8, 	16,  32,  64, 128 },
	{ 	1,	2,	3,	5, 	 7,  11,  13,  17 },
	{ 	1,	2,	3,	4, 	 5,   6,   7,   8 },
	{ 	0,	0,	0,	0, 	 0,   0,   0,	0 }
}; // Integer divisions of the input clock

const byte DEBOUNCE_DELAY = 50; // Debounce delay for all buttons

// ===========================================================================

byte divisionsSet = 0; //The set of divisions in use, from 0 to 3. Coded in binary by DIV_SELECT inputs. Set 3 not implemented in hardware (needs 1P4T switch).
long count = -1; // Input clock counter, -1 in order to go to 0 on the first pulse
bool gateMode = false; // TRUE if gate mode is active, FALSE if standard trig mode is active

volatile bool clock = false; // Clock signal digital reading, set in the clock ISR
volatile bool clockFlag = false; // Clock signal change flag, set in the clock ISR
volatile bool resetFlag = false; // Reset flag, set in the reset ISR

void setup() {
	
	// Debugging
	if (DEBUG) Serial.begin(9600);
	
	// Input
  	pinMode(CLOCK_INPUT, INPUT_PULLUP);
	pinMode(RESET_INPUT, INPUT_PULLUP);
	pinMode(DIV_SELECT_MSB, INPUT_PULLUP);
	pinMode(DIV_SELECT_LSB, INPUT_PULLUP);
	pinMode(GATE_MODE_SWITCH, INPUT_PULLUP);
	
	// Setup outputs
	for (byte i = 0; i < NUMBER_OF_DIVISIONS; i++) {
		pinMode(DIVISIONS_OUTPUT[i], OUTPUT);
		digitalWrite(DIVISIONS_OUTPUT[i], LOW);
	}

	// Interrupts
	attachInterrupt(digitalPinToInterrupt(CLOCK_INPUT), isrClock, CHANGE);
	attachInterrupt(digitalPinToInterrupt(RESET_INPUT), isrReset, FALLING);
	
}

void loop() {

	// Mode switch
	if (digitalRead(GATE_MODE_SWITCH) != gateMode) {
		gateMode = digitalRead(GATE_MODE_SWITCH);
		
		if (DEBUG) {
			Serial.print("Gate mode changed: ");
			Serial.println(gateMode);
		}
	}
	
	// Clock signal changed
	if (clockFlag) {
		clockFlag = false;
		
		if (DEBUG) {
			Serial.print("Clock signal changed: ");
			Serial.println(clock);
		}

		//Check the choosen divisions set
		divisionsSet = !digitalRead(DIV_SELECT_LSB);
		divisionsSet |= !digitalRead(DIV_SELECT_MSB) << 1;
		
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
			
			if (DEBUG) {
				Serial.print("Counter changed: ");
				Serial.println(count);
			}
			
		}
		
		// Update outputs according to current trig/gate mode
		if (gateMode) {
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

void isrClock() {
	clock = (digitalRead(CLOCK_INPUT) == LOW); //remember clock signal is reversed to use internal pullup resistor
	clockFlag = true;
}

void isrReset() {
	resetFlag = true;
}
