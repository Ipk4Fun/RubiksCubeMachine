#include <CubeFacelets.h>
#include <kociemba.h>

enum Face { U,
            D,
            R,
            L,
            F,
            B };  //Face vaules

const int stepPins[6] = { 23, 22, 21, 20, 19, 18 };        //STEP pins
const int dirPins[6] = { 2, 3, 4, 5, 6, 7 };   //Direction pins
const int buttonPins[6] = { 17, 16, 15, 14, 13, 41 };  //Button Pins
const int primeSwitchPin = 8;                       //Pin to switch direction
const int solveButtonPin = 9;
const int stepDelay = 70;   //Speed
const int turnSteps = 400;  //How much it roates by

const unsigned long debounceMs = 250;  //debonuce
unsigned long lastButtonEventMs = 0;


const unsigned long settleDelayMs = 50;  //delay between moves
unsigned long settleStartTime = 0;
bool settling = false;

CubeFacelets cube;

DMAMEM alignas(16) static uint8_t kociemba_mem479[479 * 1024];
alignas(16) static uint8_t kociemba_mem248[248 * 1024];



char moveLog[256];
int moveLogLen = 0;

char faceToChar(Face f) {
  switch (f) {
    case U: return 'U';
    case D: return 'D';
    case R: return 'R';
    case L: return 'L';
    case F: return 'F';
    case B: return 'B';
    default: return '?';
  }
}

void appendMoveChar(char face, bool prime) {
  if (moveLogLen + 3 >= (int)sizeof(moveLog)) return;

  moveLog[moveLogLen++] = face;
  if (prime) moveLog[moveLogLen++] = '\'';
  moveLog[moveLogLen++] = ' ';
  moveLog[moveLogLen] = '\0';

  Serial.print("LOG ADD: ");
  Serial.println(moveLog);
}

void clearMoveLog() {
  moveLogLen = 0;
  moveLog[0] = '\0';
}


struct Motor {  //Creates a motor class

  int stepPin;
  int dirPin;
  int stepsRemaining;
  bool active;
};

Motor motors[6];  //creates a list of 6 'Motors' made from the motor class

void updateMotors() {  //Makes it so that the multiple motors can be used at once

  static unsigned long lastStepTime = 0;                       //Remembers last it stepped
  unsigned long now = micros();                                //how many microseconds passed since started
  if (now - lastStepTime >= (unsigned long)stepDelay * 2UL) {  //using this to replace delay, just using a running clock instead
    lastStepTime = now;

    for (int i = 0; i < 6; i++) {
      if (motors[i].active && motors[i].stepsRemaining > 0) {
        digitalWrite(motors[i].stepPin, HIGH);
        delayMicroseconds(2);
        digitalWrite(motors[i].stepPin, LOW);

        motors[i].stepsRemaining--;

        if (motors[i].stepsRemaining == 0) {
          motors[i].active = false;
        }
      }
    }
  }
}

bool allMotorsIdle() {
  for (int i = 0; i < 6; i++) {
    if (motors[i].active) return false;
  }
  return true;
}

void startMove(const Face* faces, int count, int steps, const bool* dirs) {  //starts one or more faces at the same time
  for (int i = 0; i < count; i++) {
    Face f = faces[i];
    digitalWrite(motors[f].dirPin, dirs[i] ? HIGH : LOW);
    motors[f].stepsRemaining = steps;
    motors[f].active = true;
  }
}

struct PendingMove {
  bool hasMove;
  Face faces[2];
  uint8_t count;
  int steps;
  bool dirs[2];
};

bool primeMode() {
  // Switch to GND when ON → reads LOW
  return digitalRead(primeSwitchPin) == HIGH;
}


PendingMove pending = { false, { U, D }, 0, 0, { true, true } };

void handleButtons() {
  unsigned long nowMs = millis();
  if (nowMs - lastButtonEventMs < debounceMs) {
    return;  // ignore bouncing/rapid repeats
  }

  bool u = digitalRead(buttonPins[U]) == LOW;
  bool d = digitalRead(buttonPins[D]) == LOW;
  bool r = digitalRead(buttonPins[R]) == LOW;
  bool l = digitalRead(buttonPins[L]) == LOW;
  bool f = digitalRead(buttonPins[F]) == LOW;
  bool b = digitalRead(buttonPins[B]) == LOW;

  bool ud = u && d;
  bool rl = r && l;
  bool fb = f && b;


  static bool uPrev = false;
  static bool rPrev = false;
  static bool fPrev = false;
  static bool dPrev = false;
  static bool lPrev = false;
  static bool bPrev = false;

  static bool udPrev = false;
  static bool rlPrev = false;
  static bool fbPrev = false;

  if (!pending.hasMove) {
    if (fb && !fbPrev) {  //handles F+B
      pending.faces[0] = F;
      pending.faces[1] = B;
      pending.count = 2;
      pending.steps = turnSteps;
      pending.dirs[0] = !primeMode();
      pending.dirs[1] = !primeMode();
      pending.hasMove = true;
      appendMoveChar('F', primeMode());
      appendMoveChar('B', primeMode());
      lastButtonEventMs = nowMs;

    } else if (rl && !rlPrev) {  //handles R+L
      pending.faces[0] = R;
      pending.faces[1] = L;
      pending.count = 2;
      pending.steps = turnSteps;
      pending.dirs[0] = !primeMode();
      pending.dirs[1] = !primeMode();
      pending.hasMove = true;
      appendMoveChar('R', primeMode());
      appendMoveChar('L', primeMode());
      lastButtonEventMs = nowMs;
    } else if (ud && !udPrev) {  //handles U+D
      pending.faces[0] = U;
      pending.faces[1] = D;
      pending.count = 2;
      pending.steps = turnSteps;
      pending.dirs[0] = !primeMode();
      pending.dirs[1] = !primeMode();
      pending.hasMove = true;
      appendMoveChar('U', primeMode());
      appendMoveChar('D', primeMode());
      lastButtonEventMs = nowMs;
    } else {
      if (f && !fPrev && !fb) {  //Handles just F
        pending.faces[0] = F;
        pending.count = 1;
        pending.steps = turnSteps;
        pending.dirs[0] = !primeMode();
        pending.hasMove = true;
        appendMoveChar('F', primeMode());
        lastButtonEventMs = nowMs;
      } else if (b && !bPrev && !fb) {  //Handles just B
        pending.faces[0] = B;
        pending.count = 1;
        pending.steps = turnSteps;
        pending.dirs[0] = !primeMode();
        pending.hasMove = true;
        appendMoveChar('B', primeMode());
        lastButtonEventMs = nowMs;
      }

      else if (r && !rPrev && !rl) {  //Handles just R
        pending.faces[0] = R;
        pending.count = 1;
        pending.steps = turnSteps;
        pending.dirs[0] = !primeMode();
        pending.hasMove = true;
        appendMoveChar('R', primeMode());
        lastButtonEventMs = nowMs;
      } else if (l && !lPrev && !rl) {  //Handles just L
        pending.faces[0] = L;
        pending.count = 1;
        pending.steps = turnSteps;
        pending.dirs[0] = !primeMode();
        pending.hasMove = true;
        appendMoveChar('L', primeMode());
        lastButtonEventMs = nowMs;
      }

      else if (u && !uPrev && !ud) {  //Handles just U
        pending.faces[0] = U;
        pending.count = 1;
        pending.steps = turnSteps;
        pending.dirs[0] = !primeMode();
        pending.hasMove = true;
        appendMoveChar('U', primeMode());
        lastButtonEventMs = nowMs;
      } else if (d && !dPrev && !ud) {  //Handles just D
        pending.faces[0] = D;
        pending.count = 1;
        pending.steps = turnSteps;
        pending.dirs[0] = !primeMode();
        pending.hasMove = true;
        appendMoveChar('D', primeMode());
        lastButtonEventMs = nowMs;
      }
    }
  }
  uPrev = u;
  dPrev = d;
  rPrev = r;
  lPrev = l;
  fPrev = f;
  bPrev = b;
  udPrev = ud;
  rlPrev = rl;
  fbPrev = fb;
}


char algorithm[256];  // buffer for incoming algorithm
int algoIndex = 0;
bool algoRunning = false;
bool algoLoaded = false;

void startAlgorithmPlayback(const char* alg) {
  // Copy solver output into your existing playback buffer
  strncpy(algorithm, alg, sizeof(algorithm) - 1);
  algorithm[sizeof(algorithm) - 1] = '\0';

  algoIndex = 0;
  algoLoaded = true;
  algoRunning = false;
  settling = false;
}


void readAlgorithmFromSerial() {
  static int pos = 0;

  while (Serial.available()) {
    char c = Serial.read();

    // End of line = new algorithm
    if (c == '\n' || c == '\r') {
      if (pos > 0) {
        algorithm[pos] = '\0';  // terminate string
        pos = 0;
        algoIndex = 0;
        algoLoaded = true;
        settling = false;
        algoRunning = false;
      }
      return;
    }

    // Store character if space available
    if (pos < (int)sizeof(algorithm) - 1) {
      algorithm[pos++] = c;
    }
  }
}

bool parseTokenAt(int idx, char* faceChar, bool* prime, bool* dbl, int* nextIdx) {

  while (algorithm[idx] == ' ' || algorithm[idx] == '\t' || algorithm[idx] == ',') idx++;

  char c = algorithm[idx];
  if (c == '\0') return false;

  idx++;  // consume face char

  bool p = false;
  bool d2 = false;

  // optional prime
  if (algorithm[idx] == '\'') {
    p = true;
    idx++;
  }

  // optional 2 (allow either order: D2 or D'2)
  if (algorithm[idx] == '2') {
    d2 = true;
    idx++;
  }

  // also allow D2' (not standard, but harmless) → treat as D2
  if (algorithm[idx] == '\'') {
    p = true;
    idx++;
  }
  if (algorithm[idx] == '2') {
    d2 = true;
    idx++;
  }

  *faceChar = c;
  *prime = p;
  *dbl = d2;
  *nextIdx = idx;
  return true;
}

bool faceCharToFace(char c, Face* out) {
  switch (c) {
    case 'U': *out = U; return true;
    case 'D': *out = D; return true;
    case 'R': *out = R; return true;
    case 'L': *out = L; return true;
    case 'F': *out = F; return true;
    case 'B': *out = B; return true;
    default: return false;
  }
}

bool parseNextAlgorithmGroup(Face* faces, bool* dirs, uint8_t* count, int* steps) {
  // Parse first token
  char c1;
  bool prime1, dbl1;
  int i1;
  if (!parseTokenAt(algoIndex, &c1, &prime1, &dbl1, &i1)) return false;

  Face f1;
  if (!faceCharToFace(c1, &f1)) return false;

  // Look ahead for second token (to allow pairs)
  char c2;
  bool prime2, dbl2;
  int i2;
  bool hasSecond = parseTokenAt(i1, &c2, &prime2, &dbl2, &i2);

  // Default: single move
  faces[0] = f1;
  dirs[0] = !prime1;  // same convention you already used
  *count = 1;
  *steps = dbl1 ? (2 * turnSteps) : turnSteps;
  algoIndex = i1;

  if (!hasSecond) return true;

  Face f2;
  if (!faceCharToFace(c2, &f2)) {
    // second token isn't valid; just treat as single
    return true;
  }

  // Only pair if it’s one of your allowed opposite-face pairs
  bool isRL = ((f1 == R && f2 == L) || (f1 == L && f2 == R));
  bool isFB = ((f1 == F && f2 == B) || (f1 == B && f2 == F));
  bool isUD = ((f1 == U && f2 == D) || (f1 == D && f2 == U));

  if (isRL || isFB || isUD) {
    // For a pair, both moves must be same "double" state to run together.
    // If they differ (e.g., R2 L), we’ll just do the first as single and leave the second for next cycle.
    if (dbl1 == dbl2) {
      faces[0] = (isRL ? R : (isFB ? F : U));
      faces[1] = (isRL ? L : (isFB ? B : D));
      dirs[0] = !prime1;
      dirs[1] = !prime2;
      *count = 2;
      *steps = dbl1 ? (2 * turnSteps) : turnSteps;
      algoIndex = i2;  // consume both tokens
    }
  }

  return true;
}


void processAlgorithm() {

  if (algoRunning) {
    if (allMotorsIdle()) {
      algoRunning = false;
      settling = true;
      settleStartTime = millis();
    }
    return;
  }

  // Wait during settle time
  if (settling) {
    if (millis() - settleStartTime >= settleDelayMs) {
      settling = false;
    }
    return;
  }
  Face faces[2];
  uint8_t count;
  int steps;
  bool dirs[2];

  if (parseNextAlgorithmGroup(faces, dirs, &count, &steps)) {
    startMove(faces, count, steps, dirs);
    algoRunning = true;
  } else {
    // ✅ DONE: no more tokens
    algoLoaded = false;
    algoRunning = false;
    settling = false;
    algoIndex = 0;
    Serial.println("Algorithm finished.");
  }
}



void setup() {
  Serial.begin(115200);
  pinMode(primeSwitchPin, INPUT_PULLUP);
  pinMode(solveButtonPin, INPUT_PULLUP);

  clearMoveLog();
  cube.resetSolved();

  for (int i = 0; i < 6; i++) {
    pinMode(stepPins[i], OUTPUT);
    pinMode(dirPins[i], OUTPUT);

    pinMode(buttonPins[i], INPUT_PULLUP);
    motors[i] = { stepPins[i], dirPins[i], 0, false };
  }
  // Speed up Kociemba dramatically on Teensy 4.1
  kociemba::set_memory(kociemba_mem479, kociemba_mem248);
  
}

void loop() {
  updateMotors();

  if (allMotorsIdle() && !pending.hasMove && !algoLoaded) {
    handleButtons();
  }

  static bool solvePrev = false;
  bool solveNow = (digitalRead(solveButtonPin) == LOW);

  if (solveNow && !solvePrev) {
    Serial.print("Moves: ");
    Serial.println(moveLog);

    // Build facelets from the logged moves
    cube.resetSolved();
    cube.applyScramble(moveLog);

    char facelets[55];
    cube.getFacelets(facelets);

    Serial.print("Facelets: ");
    Serial.println(facelets);

    // Run Kociemba ON THE TEENSY
    // maxDepth=24 is standard, timeout in ms
    const char* sol = kociemba::solve(facelets, 24, 10000, 0);

    if (sol == nullptr) {
      Serial.println("Kociemba failed (timeout or invalid/unsolvable cube).");
    } else {
      Serial.print("Solution: ");
      Serial.println(sol);

      // Execute the solution using your existing motor playback engine
      startAlgorithmPlayback(sol);
    }

    pending.hasMove = false;
    clearMoveLog();
  }
  solvePrev = solveNow;

  if (pending.hasMove && allMotorsIdle()) {
    startMove(pending.faces, pending.count, pending.steps, pending.dirs);
    pending.hasMove = false;
    algoRunning = false;
    algoLoaded = false;
    return;
  }
  if (!algoLoaded && !algoRunning && !settling) {
    readAlgorithmFromSerial();
  }
  if (algoLoaded) processAlgorithm();
}
