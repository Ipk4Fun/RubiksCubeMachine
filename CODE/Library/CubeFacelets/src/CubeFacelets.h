#pragma once
#include <Arduino.h>

class CubeFacelets {
public:
  CubeFacelets();

  void resetSolved();
  void applyMove(const char* moveToken);
  void applyScramble(const char* scramble);
  void getFacelets(char out[55]) const;

private:
  struct Sticker {
    int8_t x, y, z;
    int8_t nx, ny, nz;
    char c;
  };

  Sticker stickers[54];

  static inline void rot2(int8_t &a, int8_t &b, int dir);
  static inline int faceStartFromNormal(int8_t nx, int8_t ny, int8_t nz);
  static int indexFromSticker(const Sticker &s);

  static void parseMoveToken(const char* tok, char &face, int &cwTurns);

  void rotateLayer(char axis, int8_t layerVal, int dir);
  void applyMoveInternal(char face, int cwTurns);
};
