#include "CubeFacelets.h"

CubeFacelets::CubeFacelets() {
  resetSolved();
}

// dir=+1 => +90° by RH rule about +axis, dir=-1 => -90°
inline void CubeFacelets::rot2(int8_t &a, int8_t &b, int dir) {
  int8_t oa = a, ob = b;
  if (dir > 0) {
    a = -ob; b = oa;
  } else {
    a = ob;  b = -oa;
  }
}

inline int CubeFacelets::faceStartFromNormal(int8_t nx, int8_t ny, int8_t nz) {
  if (ny == 1)  return 0;   // U
  if (nx == 1)  return 9;   // R
  if (nz == 1)  return 18;  // F
  if (ny == -1) return 27;  // D
  if (nx == -1) return 36;  // L
  return 45;                // B
}

int CubeFacelets::indexFromSticker(const Sticker &s) {
  int base = faceStartFromNormal(s.nx, s.ny, s.nz);
  int row = 0, col = 0;

  // Coordinates: x:+R, y:+U, z:+F
  // Face viewing: left->right, top->bottom as seen looking at that face

  if (s.ny == 1) {          // U: right=+x, down=+z
    row = s.z + 1;
    col = s.x + 1;
  } else if (s.ny == -1) {  // D: right=+x, down=-z
    row = 1 - s.z;
    col = s.x + 1;
  } else if (s.nz == 1) {   // F: right=+x, down=-y
    row = 1 - s.y;
    col = s.x + 1;
  } else if (s.nz == -1) {  // B: right=-x, down=-y
    row = 1 - s.y;
    col = 1 - s.x;
  } else if (s.nx == 1) {   // R: right=-z, down=-y
    row = 1 - s.y;
    col = 1 - s.z;
  } else {                  // L: right=+z, down=-y
    row = 1 - s.y;
    col = s.z + 1;
  }

  return base + row * 3 + col;
}

void CubeFacelets::rotateLayer(char axis, int8_t layerVal, int dir) {
  for (int i = 0; i < 54; i++) {
    Sticker &s = stickers[i];

    bool inLayer = false;
    if (axis == 'x') inLayer = (s.x == layerVal);
    if (axis == 'y') inLayer = (s.y == layerVal);
    if (axis == 'z') inLayer = (s.z == layerVal);
    if (!inLayer) continue;

    if (axis == 'x') {
      rot2(s.y, s.z, dir);
      rot2(s.ny, s.nz, dir);
    } else if (axis == 'y') {
      rot2(s.x, s.z, dir);
      rot2(s.nx, s.nz, dir);
    } else { // 'z'
      rot2(s.x, s.y, dir);
      rot2(s.nx, s.ny, dir);
    }
  }
}

// Apply face move with cwTurns quarter-turns clockwise (1,2,3) per cube notation
void CubeFacelets::applyMoveInternal(char m, int cwTurns) {
  if (cwTurns <= 0) return;
  cwTurns %= 4;
  if (cwTurns == 0) return;

  char axis = 0;
  int8_t layer = 0;
  int dir = 0;

  // Convention with U/D flipped per earlier discussion
  switch (m) {
    case 'U': axis = 'y'; layer = +1; dir = +1; break;
    case 'D': axis = 'y'; layer = -1; dir = -1; break;
    case 'R': axis = 'x'; layer = +1; dir = -1; break;
    case 'L': axis = 'x'; layer = -1; dir = +1; break;
    case 'F': axis = 'z'; layer = +1; dir = -1; break;
    case 'B': axis = 'z'; layer = -1; dir = +1; break;
    default: return;
  }

  for (int k = 0; k < cwTurns; k++) {
    rotateLayer(axis, layer, dir);
  }
}

void CubeFacelets::parseMoveToken(const char* tok, char &face, int &cwTurns) {
  face = 0;
  cwTurns = 0;
  if (!tok || !tok[0]) return;

  char f = tok[0];
  if (!(f=='U'||f=='D'||f=='R'||f=='L'||f=='F'||f=='B')) return;

  int amount = 1;
  bool prime = false;

  for (int i = 1; tok[i]; i++) {
    if (tok[i] == '2') amount = 2;
    else if (tok[i] == '\'') prime = true;
  }

  int cw = amount % 4;
  if (prime) cw = (4 - cw) % 4;
  if (cw == 0) return;

  face = f;
  cwTurns = cw;
}

void CubeFacelets::applyMove(const char* moveToken) {
  char face;
  int cw;
  parseMoveToken(moveToken, face, cw);
  if (face) applyMoveInternal(face, cw);
}

void CubeFacelets::applyScramble(const char* scramble) {
  if (!scramble) return;

  char tok[8];
  int ti = 0;

  for (int i = 0; ; i++) {
    char ch = scramble[i];
    bool end = (ch == '\0');
    bool sep = (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');

    if (!end && !sep) {
      if (ti < (int)sizeof(tok) - 1) tok[ti++] = ch;
    } else {
      if (ti > 0) {
        tok[ti] = '\0';
        applyMove(tok);
        ti = 0;
      }
      if (end) break;
    }
  }
}

void CubeFacelets::resetSolved() {
  int idx = 0;

  auto addFace = [&](char face) {
    for (int r = 0; r < 3; r++) {
      for (int c = 0; c < 3; c++) {
        Sticker &s = stickers[idx++];

        if (face == 'U') {
          s.nx = 0; s.ny = 1; s.nz = 0;
          s.y = 1;  s.z = r - 1;  s.x = c - 1;
        } else if (face == 'D') {
          s.nx = 0; s.ny = -1; s.nz = 0;
          s.y = -1; s.z = 1 - r; s.x = c - 1;
        } else if (face == 'F') {
          s.nx = 0; s.ny = 0; s.nz = 1;
          s.z = 1;  s.y = 1 - r; s.x = c - 1;
        } else if (face == 'B') {
          s.nx = 0; s.ny = 0; s.nz = -1;
          s.z = -1; s.y = 1 - r; s.x = 1 - c;
        } else if (face == 'R') {
          s.nx = 1; s.ny = 0; s.nz = 0;
          s.x = 1;  s.y = 1 - r; s.z = 1 - c;
        } else { // 'L'
          s.nx = -1; s.ny = 0; s.nz = 0;
          s.x = -1;  s.y = 1 - r; s.z = c - 1;
        }

        s.c = face;
      }
    }
  };

  addFace('U');
  addFace('R');
  addFace('F');
  addFace('D');
  addFace('L');
  addFace('B');
}

void CubeFacelets::getFacelets(char out[55]) const {
  for (int i = 0; i < 54; i++) out[i] = '?';
  out[54] = '\0';

  for (int i = 0; i < 54; i++) {
    int j = indexFromSticker(stickers[i]);
    out[j] = stickers[i].c;
  }
}
