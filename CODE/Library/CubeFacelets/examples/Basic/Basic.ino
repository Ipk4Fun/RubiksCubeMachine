#include <CubeFacelets.h>

CubeFacelets cube;

void setup() {
  cube.resetSolved();
  cube.applyScramble("R U R' U'");

  char facelets[55];
  cube.getFacelets(facelets);
}

void loop() {}
