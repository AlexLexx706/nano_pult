#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define ENABLE_DEBIG
#define BUTTON_WAIT_STATE_PERIOD_MS 1
#define BUTTON_SEND_STATE_PERIOD 200

//button class, used for get stable button state
class Button {
	int pin;
	char last_state;
	char first_change_flag;
	char stable_state;
	unsigned long start_wait_stable_time;
	unsigned long start_send_state_time;

public:
	Button(int in_pin):
		pin(in_pin),
		last_state(0),
		first_change_flag(0),
		stable_state(0),
		start_wait_stable_time(0),
		start_send_state_time(0) {
	}

	// initialize the pushbutton pin as an input:
	void init() {
		pinMode(pin, INPUT_PULLUP);
		//enable pull up resister, 20-50 kOm, after that the default button state is 1
		digitalWrite(pin, HIGH);
	}

	//read button state, return 1- if button pressed 0 - not pressed
	int update() {
		unsigned long cur_time = millis();
		char state = digitalRead(pin);

		//1. state changed
		if (state != last_state) {
			if (first_change_flag) {
				first_change_flag = 0;
				start_wait_stable_time = cur_time;
			}
		}
		last_state = state;

		//2. stable button state
		if (first_change_flag == 0 && (cur_time - start_wait_stable_time) > BUTTON_WAIT_STATE_PERIOD_MS) {
			first_change_flag = 1;
			stable_state = state;
		}

		//3. periodecly send button state pressed
		if (stable_state == LOW) {
			if (cur_time - start_send_state_time > BUTTON_SEND_STATE_PERIOD) {
			start_send_state_time = cur_time;
			return 1;
			}
		}
		return 0;
	}
};

RF24 radio(10, 9);			// CE, CSN
const byte address[6] = "1Node";
Button button_1(8);
Button button_2(7);
Button button_3(6);
Button button_4(5);
Button button_5(4);

void setup() {
	//1. setup serial
	Serial.begin(115200);
	Serial.println("Start radio remote XXXXX");

	//2. setup radion channel
	radio.begin();
	radio.openWritingPipe(address);
	radio.setPALevel(RF24_PA_MIN);
	radio.stopListening();

	if (radio.isChipConnected()) {
	Serial.println("Rado chip Connected");
	} else {
	Serial.println("Rado chip not Connected");
	}

	//3.
	button_1.init();
	button_2.init();
	button_3.init();
	button_4.init();
	button_5.init();
}

//#define PROXY_MODE
static char proxy_byffer[10];
static char * pb_cur_p(proxy_byffer);
static float packet[2] = {0.f, 0.f};
static unsigned long start_time = 0;

void loop() {
	#ifdef PROXY_MODE
		unsigned long cur_time = millis();

		//read data from serial, proxy mode
		if (Serial.available()) {
			*(pb_cur_p++) = (char)Serial.read();

			//1. end off buffer
			if (pb_cur_p == &proxy_byffer[sizeof(proxy_byffer)]) {
				// Serial.println("1.");

				//2. check packet
				if (proxy_byffer[0] == 'A' && proxy_byffer[sizeof(proxy_byffer) -1] == 'B') {
					// Serial.println("2.");

					packet[0] = *((float *)(&proxy_byffer[1]));
					packet[1] = *((float *)(&proxy_byffer[5]));
					pb_cur_p = proxy_byffer;
				//3. move buffer
				} else {
					// Serial.println("3.");

					for (unsigned int i = 0; i < sizeof(proxy_byffer) - 1; i++) {
						proxy_byffer[i] = proxy_byffer[i + 1];
					}
					pb_cur_p = &proxy_byffer[sizeof(proxy_byffer) - 1];
				}
			}
		}

		//send packet
		if ((cur_time - start_time) >= 20) {
			#ifdef ENABLE_DEBIG
				Serial.print("send w:"); Serial.print(packet[0], 4); Serial.print(" v:"); Serial.println(packet[1], 4);
				start_time = cur_time;

				if (!radio.write(packet, sizeof(packet))) {
					Serial.println("failed send w");
				}
			#else
				start_time = cur_time;
				radio.write(packet, sizeof(packet));
			#endif
		}
	#else
		#ifdef ENABLE_DEBIG
			if (button_1.update()) {
				Serial.println("send w");
				unsigned long ch = 'w';
				if (!radio.write(&ch, sizeof(ch))) {
				Serial.println("failed send w");
				}
			}
			if (button_2.update()) {
				Serial.println("send s");
				unsigned long ch = 's';
				if (!radio.write(&ch, sizeof(ch))) {
				Serial.println("failed send s");
				}
			}
			if (button_3.update()) {
				Serial.println("send a");
				unsigned long ch = 'a';
				if (!radio.write(&ch, sizeof(ch))) {
				Serial.println("failed send a");
				}
			}
			if (button_4.update()) {
				Serial.println("send d");
				unsigned long ch = 'd';
				if (!radio.write(&ch, sizeof(ch))) {
				Serial.println("failed send d");
				}
			}
			if (button_5.update()) {
				Serial.println("send space");
				unsigned long ch = ' ';
				if (!radio.write(&ch, sizeof(ch))) {
				Serial.println("failed send space");
				}
			}
		#else
			if (button_1.update()) {
				unsigned long ch = 'w';
				radio.write(&ch, sizeof(ch));
			}
			if (button_2.update()) {
				unsigned long ch = 's';
				radio.write(&ch, sizeof(ch));
			}
			if (button_3.update()) {
				unsigned long ch = 'a';
				radio.write(&ch, sizeof(ch));
			}
			if (button_4.update()) {
				unsigned long ch = 'd';
				radio.write(&ch, sizeof(ch));
			}
			if (button_5.update()) {
				unsigned long ch = ' ';
				radio.write(&ch, sizeof(ch));
			}
		#endif
	#endif
}
