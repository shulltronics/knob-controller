#include <SPI.h>
#include <Encoder.h>

#define NUM_CHANNELS 16
#define NUM_KNOBS 4
#define NUM_SWITCHES 2


// pin mappings
uint8_t knob_pins[NUM_KNOBS] = {A5, A8, A7, A6};
// cc numbers for the knobs
uint8_t knob_ccs[NUM_KNOBS] = {15, 16, 17, 18};
uint8_t switch_pins[NUM_SWITCHES] = {8, 9};
// my encoder on pins 2 and 3 (interrupt-based);
Encoder encoder(3, 2);
// encoder button pin
uint8_t enc_button_pin = 4;
// pins to the common anodes of each 7seg digit
#define NUM_DIGITS 2
uint8_t display_pins[NUM_DIGITS] = {5, 6};
// an array containing the digits to display, as indexed in seg_chars
uint8_t display_chars[NUM_DIGITS] = {16, 16};

// current state of knobs for each channel
uint8_t knob_vals[NUM_KNOBS][NUM_CHANNELS];
// a hash of the "active" state for each knob and switch
uint8_t activityHash = 0b00000000;

#define NUM_CHARS 10
// of the form 0b(g)(f)(e)(d)(c)(b)(a)(dot)
uint8_t seg_chars[] = {
  0b01111111, // 0
  0b00001101, // 1
  0b10110111, // 2
  0b10011111, // 3
  0b11001101, // 4
  0b11011011, // 5
  0b11111011, // 6
  0b00001111, // 7
  0b11111111, // 8
  0b11001111, // 9
  0b11101111, // A
  0b11111001, // b
  0b10110001, // c
  0b10111101, // d
  0b11110011, // e
  0b11100011, // f
  0b10000001  // -
};

void setup() {
  
  Serial.begin(9600);
  
  SPI.begin();
  
  // init knob_vals states and pin modes. should this later be recalled from EEPROM?
  for(int i = 0; i < NUM_KNOBS; i++) {
    pinMode(knob_pins[i], INPUT);
    for(int j = 0; j < NUM_CHANNELS; j++) {
      knob_vals[i][j] = analogRead(knob_pins[i]);
    }
  }
  // init display anode pins
  for(int i = 0; i < NUM_DIGITS; i++) {
    pinMode(display_pins[i], OUTPUT);
  }
  // init other pins
  pinMode(enc_button_pin, INPUT_PULLUP);
  pinMode(switch_pins[0], INPUT_PULLUP);
  pinMode(switch_pins[1], INPUT_PULLUP);

}


uint8_t cur_channel = 1;
long oldEncVal = encoder.read();
uint8_t enc_thresh = 8;
long displayTimer = micros();
void loop() {
 
  long curTime = micros();
  if(curTime - displayTimer > 3300) {
    displayTimer = curTime;
    updateDisplay();
  }
  
  if(!digitalRead(enc_button_pin)) {
    updateChannel();
  }

  display_chars[1] = cur_channel;

  updateKnobs();
  
  /*
  if(curTime - lastTime > 10000) {
    lastTime = curTime;
    Serial.println(cur_channel);
    //Serial.println(knob_vals[0][0]);
  }
*/
}

void updateChannel() {
  if((encoder.read() - oldEncVal > enc_thresh) && cur_channel < 16) {
    cur_channel += 1;
    oldEncVal = encoder.read();
  } else if ((encoder.read() - oldEncVal < -enc_thresh) && cur_channel > 1) {
    cur_channel -= 1;
    oldEncVal = encoder.read();
  }
}

  uint8_t curDig = 0;
void updateDisplay() {
  segWrite(~seg_chars[display_chars[curDig]]);
  lightDigit(curDig);
  if(curDig == 0) {
    curDig++;
  } else {
    curDig = 0;
  }
}

void lightDigit(uint8_t digit) {
  if(digit == 0) {
    digitalWrite(display_pins[0], HIGH);
    digitalWrite(display_pins[1], LOW);
  } else {
    digitalWrite(display_pins[0], LOW);
    digitalWrite(display_pins[1], HIGH);
  }
}
void segWrite(uint8_t num) {
  SPI.transfer(activityHash);
  SPI.transfer(num);
}

int updateEncoder() {
  int cur_enc_val = encoder.read();
  encoder.write(0);
  return cur_enc_val;
}

void updateKnobs() {
  for(uint8_t i = 0; i < NUM_KNOBS; i++) {
    uint8_t cur_val_i = analogRead(knob_pins[i]) >> 3;
    // if the knob is not at it's last position, don't update the value.
    if(abs(cur_val_i - knob_vals[i][cur_channel]) > 2) {
      bitClear(activityHash, 7-i);
    } else {
      // otherwise set it as active
      bitSet(activityHash, 7-i);
      // do a little filtering and send the cc if it's changed
      if(abs(knob_vals[i][cur_channel] - cur_val_i) > 1) {
        knob_vals[i][cur_channel] = cur_val_i;
        usbMIDI.sendControlChange(knob_ccs[i], cur_val_i, cur_channel);
      }
      //display_chars[0] = i;
      //display_chars[1] = cur_val_i;
    }
    /* filter +-1 knob fluctucations, then update channel states.
    if (abs(cur_val_i - knob_vals[i][cur_channel-1]) > 1) {
      knob_vals[i][cur_channel-1] = cur_val_i;
    }*/
  }
}
