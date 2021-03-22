	
// CONFIGURATION =============================================================

const bool DEBUG = true; // FALSE to disable debug messages on serial port

const int CLOCK_INPUT = 2; // Input signal pin, must be usable for interrupts
const int RESET_INPUT = 3; // Reset signal pin, must be usable for interrupts

const int GATE_MODE_SWITCH = A3; // 2 positions switch to chose between gate and trigger mode 

const int DIVISIONS_OUTPUT[] { 4, 5, 6, 7, 8, 9, 10, 11 }; // Output pins

const int NUMBER_OF_DIVISIONS = 8;
const int DIVISIONS[NUMBER_OF_DIVISIONS] { 2, 3, 4, 5, 6, 8, 16, 32 }; // Integer divisions of the input clock

const unsigned long MODE_SWITCH_LONG_PRESS_DURATION_MS = 3000; // Reset button long-press duration for trig/gate mode switch
const unsigned long BUTTON_DEBOUNCE_DELAY = 50; // Debounce delay for all buttons

// ===========================================================================

//#include "lib/Button.cpp"

long count = -1; // Input clock counter, -1 in order to go to 0 no the first pulse
bool gateMode = false; // TRUE if gate mode is active, FALSE if standard trig mode is active

//Button resetButton;

volatile bool clock = false; // Clock signal digital reading, set in the clock ISR
volatile bool clockFlag = false; // Clock signal change flag, set in the clock ISR
volatile bool resetFlag = false; // Reset flag, set in the reset ISR

void setup() {
	
	// Debugging
	if (DEBUG) Serial.begin(9600);
	
	// Input
	//resetButton.init(RESET_BUTTON, BUTTON_DEBOUNCE_DELAY);
  	pinMode(CLOCK_INPUT, INPUT);
	pinMode(RESET_INPUT, INPUT_PULLUP);
	pinMode(GATE_MODE_SWITCH, INPUT_PULLUP);
	
	// Setup outputs
	for (int i = 0; i < NUMBER_OF_DIVISIONS; i++) {
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
		for (int i = 0; i < NUMBER_OF_DIVISIONS; i++) {
			bool v = (count % DIVISIONS[i] == 0);
			digitalWrite(DIVISIONS_OUTPUT[i], v ? HIGH : LOW);
		}
		
	} else {
		
		// Falling edge, go LOW on every output
		for (int i = 0; i < NUMBER_OF_DIVISIONS; i++) {
			digitalWrite(DIVISIONS_OUTPUT[i], LOW);
		}
		
	}
	
}

void processGateMode() {
	
	// Keep outputs high for ~50% of divided time
	for (int i = 0; i < NUMBER_OF_DIVISIONS; i++) {
		
		// Go HIGH on the rising edges that corresponds to the division
		int modulo = (count % DIVISIONS[i]);
		if (clock && modulo == 0) {
			digitalWrite(DIVISIONS_OUTPUT[i], HIGH);
		}
		
		// Go LOW on rising edges for even divisions and falling edges for odd divisions,
		// considering the edges that corresponds to the half value of the division
		if (modulo == (int)(floor(DIVISIONS[i] / 2.0))) {
			bool divisionIsOdd = (DIVISIONS[i] % 2 != 0);
			if ((clock && !divisionIsOdd) || (!clock && divisionIsOdd)) {
				digitalWrite(DIVISIONS_OUTPUT[i], LOW);
			}
		}
		
	}
	
}

void isrClock() {
	clock = (digitalRead(CLOCK_INPUT) == HIGH);
	clockFlag = true;
}

void isrReset() {
	resetFlag = true;
}
