uint8_t DEBUG=1;

#include  <Servo.h>
#include  <EEPROM.h>

Servo LEFT_ARM;
Servo LEFT_STICK;
Servo RIGHT_ARM;
Servo RIGHT_STICK;

#define NUMSONGS 4
String songs[NUMSONGS][3] = { // Name, part1, part2 (part2=255 means no part2)
  { "Lattal-e mar valaha",  "0", "1" },
  { "Boci-boci tarka",      "2", "3" },
  { "Pasztorok, pasztorok", "4", "255" },
  { "Notas Mikulas",        "5", "255" }
};
String parts[6] = {
  "G24A24H24C34H24A24H22G24A24H24C34D32D32D34H24D34C34H24A24H22G24A24H24A24G22G22",
  "G12D22G12D22G12D22G12D22G12D22G12D22G12D22G12D22",

  "C24E24C24E24G22G22C24E24C24E24G22G22C34H24A24G24F22A22G24F24E24D24C22C22C24E24C24E24G24G24G22C24E24C24E24G24G24G22C34H24A24G24F24F24A22G24F24E24D24C24C24C22",
  "C32C32G12G12C32C32G12G12C22A12F32A12G12E32C32C32C32G12G12C32C32G12G12C22A12F32A12G12E32C32",

  "C24E28G28C24E28G28F28E28D24C22G24H28D38G24H28D38C38H28A24G22C34H28A28G24F28E28G28F28E24D22C24E28G28C24E28G28F28E28D24C22",
  "G24E24F28E28D24G24E24F28E28D24E28F28G24A24A24G28E28D24C24C24G24E24F28E28D24G24E24F28E28D24E28F28G24A24A24G28E28D24C24C24",
};

#define NONE 0
#define LEFT 1
#define RIGHT 2
uint8_t stick, stick1, stick2;
uint8_t part1, part2;
int l_pos, r_pos, l_soft=120, r_soft=92, l_up=115, r_up=97;
int delay_time = 3000;
unsigned long nextHitMillis1, nextHitMillis2, nextUpMillis1, nextUpMillis2;
uint8_t ptrSongs;
int index1, index2;
char note1[2], note2[2];
uint8_t duration1, duration2;
#define NOSONG 0
#define HITNOTE 1
#define UPSTICK 2
uint8_t state1, state2;

void setup() {
  Serial.begin(9600);
  delay(2000);
  if (DEBUG) Serial.println("HURBA Xylophone Ver 2.0 2019-03-14 (International PI Day)");
  
  pinMode(3, OUTPUT); pinMode(5, OUTPUT); pinMode(6, OUTPUT); pinMode(9, OUTPUT);
  LEFT_ARM.attach(3);
  LEFT_STICK.attach(5);
  RIGHT_ARM.attach(6);
  RIGHT_STICK.attach(9);

  sendHome();
 
  state1 = state2 = NOSONG;
  delay(2000);
}

char cmd[32];
int lenCmd;
bool hit;
void loop() { // Calibration loop
  if (Serial.available()) {
    lenCmd = 0; char c = Serial.read();
    while (c!=' ' && c!='\n') {
      cmd[lenCmd++] = c;
      c = Serial.read(); 
    }
    Serial.print("INPUT: "); for (int i=0; i<lenCmd; i++) Serial.print(cmd[i]);
    Serial.print(" length: "); Serial.println(lenCmd);
    if (cmd[0]=='g') { // g-oto l-eft/r-ight a-rm/s-tick xxx-pos (6 chars)
      uint8_t pos = 100*(cmd[3]-'0') + 10*(cmd[4]-'0') + 1*(cmd[5]-'0');
      Serial.print("pos: "); Serial.println(pos);
      if (cmd[1]=='l') { // left
        if (cmd[2]=='a') { // arm
          LEFT_ARM.write(pos);
        }
        if (cmd[2]=='s') { // stick
          LEFT_STICK.write(pos);
        }
      }
      if (cmd[1]=='r') { // right
        if (cmd[2]=='a') { // arm
          RIGHT_ARM.write(pos);
        }
        if (cmd[2]=='s') { // stick
          RIGHT_STICK.write(pos);
        }
      }
    }
    else if (cmd[0]=='n') { // n-ote
      getNotePos(cmd[1],cmd[2]);
      posNote(stick,l_pos,r_pos);
    }
    else if (cmd[0]=='s') { // song
      ptrSongs = cmd[1]-'0';
      hit = cmd[2]=='1'?true:false;
      part1 = songs[ptrSongs][1].toInt();
      part2 = songs[ptrSongs][2].toInt();
      state1 = HITNOTE; if (part2!=255) state2 = HITNOTE;
      showSong();
      index1 = 0; nextHitMillis1 = millis(); nextUpMillis1 = millis() + 100;
      index2 = 0; nextHitMillis2 = millis(); nextUpMillis2 = millis() + 100;
      getNote1(); showNote1(); stick1=getNotePos(note1[0],note1[1]); posNote(stick,l_pos,r_pos);
      if (part2!=255) { getNote2(); showNote2(); stick2=getNotePos(note2[0],note2[1]); posNote(stick,l_pos,r_pos); }
    }
    else if (cmd[0]=='h') { // home
      sendHome();
    }
    else if (cmd[0]=='d') { // debug
      DEBUG = cmd[1]=='1'?true:false;
      Serial.print("DEBUG: "); Serial.println(DEBUG);
    }
    else if (cmd[0]=='t') { // tempo
      delay_time = 100 * ( 10*(cmd[1]-'0') + 1*(cmd[2]-'0') );
      Serial.print("Tempo delay_time: "); Serial.println(delay_time);
    }
    else if (cmd[0]=='r') { // reset
      state1 = NOSONG; state2 = NOSONG;
    }
  }
  if (state1 == HITNOTE) if (millis()>nextHitMillis1) {
    hitNote(stick1, hit); nextUpMillis1 = millis() + 100; state1 = UPSTICK;
    nextHitMillis1 = millis() + delay_time/duration1;
  }
  if (state1 == UPSTICK) if (millis()>nextUpMillis1) {
    upStick(stick1, hit); state1 = HITNOTE;
    index1+=3; getNote1(); showNote1(); stick1=getNotePos(note1[0],note1[1]); posNote(stick,l_pos,r_pos);
    if (index1>=parts[part1].length()) state1 = NOSONG;
  }
  if (state2 == HITNOTE) if (millis()>nextHitMillis2) {
      hitNote(stick2, hit); nextUpMillis2 = millis() + 100; state2 = UPSTICK;
      nextHitMillis2 = millis() + delay_time/duration2;
  }
  if (state2 == UPSTICK) if (millis()>nextUpMillis2) {
    upStick(stick2, hit); state2 = HITNOTE;
    index2+=3; getNote2(); showNote2(); stick2=getNotePos(note2[0],note2[1]); posNote(stick,l_pos,r_pos);
    if (index2>=parts[part2].length()) state2 = NOSONG;
  }
}

uint8_t getNotePos(char c1, char c2) { // returns stick (LEFT or RIGHT)
  stick = NONE;
  if     (c1 == 'G' && c2=='1') { l_pos = 117; stick = LEFT; }
  else if(c1 == 'A' && c2=='1') { l_pos = 110; stick = LEFT; }
  else if(c1 == 'H' && c2=='1') { l_pos = 103; stick = LEFT; } 
  else if(c1 == 'C' && c2=='2') { l_pos = 96; stick = LEFT; } 
  else if(c1 == 'D' && c2=='2') { l_pos = 88; stick = LEFT; } 
  else if(c1 == 'E' && c2=='2') { l_pos = 81; stick = LEFT; } 
  else if(c1 == 'F' && c2=='2') { l_pos = 75; stick = LEFT;; }
  else if(c1 == 'G' && c2=='2') { r_pos = 119; stick = RIGHT; } 
  else if(c1 == 'A' && c2=='2') { r_pos = 112; stick = RIGHT; } 
  else if(c1 == 'H' && c2=='2') { r_pos = 105; stick = RIGHT; } 
  else if(c1 == 'C' && c2=='3') { r_pos = 97; stick = RIGHT; } 
  else if(c1 == 'D' && c2=='3') { r_pos = 90; stick = RIGHT; } 
  else if(c1 == 'E' && c2=='3') { r_pos = 83; stick = RIGHT; } 
  else if(c1 == 'F' && c2=='3') { r_pos = 76; stick = RIGHT; } 
  else if(c1 == 'G' && c2=='3') { r_pos = 68; stick = RIGHT; } //!!! itt már a csavart üti sajnos
  if (DEBUG) {
    if (stick==LEFT) { Serial.print("LEFT l_pos: "); Serial.println(l_pos); }
    if (stick==RIGHT) { Serial.print("RIGHT r_pos: "); Serial.println(r_pos); }
    if (stick == NONE) { Serial.println("NONE WARNING: Unknown position!"); }
  }
  return stick;
}

void sendHome() {
  LEFT_ARM.write(96);
  RIGHT_ARM.write(97);
  LEFT_STICK.write(l_up);
  RIGHT_STICK.write(r_up);
}

void hitNote(uint8_t stick, bool hit) {
  if (hit) {
    if(stick==LEFT) { LEFT_STICK.write(l_soft); }
    if(stick==RIGHT) { RIGHT_STICK.write(r_soft); }
  }
}

void upStick(uint8_t stick, bool hit) {
  if (hit) {
    if(stick==LEFT) { LEFT_STICK.write(l_up); }
    if(stick==RIGHT) { RIGHT_STICK.write(r_up); }
  }
}

void posNote(uint8_t stick, uint8_t l_pos, uint8_t r_pos) {
  if(stick==LEFT) LEFT_ARM.write(l_pos);
  if(stick==RIGHT) RIGHT_ARM.write(r_pos);
}

void showNote1() {
  if (DEBUG) { Serial.print("part1: "); Serial.print(part1); Serial.print(" index1 "); 
    Serial.print(index1); Serial.print(" note1: "); Serial.print(note1[0]); Serial.print(note1[1]); 
    Serial.print(" duration1: "); Serial.println(duration1); 
  }
}

void showNote2() {
  if (DEBUG) { Serial.print("part2: "); Serial.print(part2); Serial.print(" index2 "); 
    Serial.print(index2); Serial.print(" note2: "); Serial.print(note2[0]); Serial.print(note2[1]); 
    Serial.print(" duration2: "); Serial.println(duration2); 
  }
}

void showSong() {
  if (DEBUG) { Serial.print(ptrSongs); Serial.print(": "); Serial.println(songs[ptrSongs][0]); }
  if (DEBUG) { Serial.print(part1); Serial.print(": "); Serial.println(parts[part1]); }
  if (DEBUG) if (part2!=255) { Serial.print(part2); Serial.print(": "); Serial.println(parts[part2]); }
}

void getNote1() {
  note1[0] = parts[part1][index1];
  note1[1] = parts[part1][index1+1];
  duration1 = parts[part1][index1+2]-'0';
}

void getNote2() {
  note2[0] = parts[part2][index2];
  note2[1] = parts[part2][index2+1];
  duration2 = parts[part2][index2+2]-'0';
}
