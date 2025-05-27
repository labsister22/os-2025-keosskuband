// torus.c
#include "header/usermode/commands/torus.h"


/*
REFERENCE : https://www.a1k0n.net/2006/09/15/obfuscated-c-donut.html
*/

#define PI 3.14159265f

static const float theta_spacing = 0.07f;
static const float phi_spacing   = 0.02f;
static const float pi = 3.14159265f;

// Very basic float-to-int cast with clamping
int fast_floor(float x) {
    return (int)(x);
}

// Simple sin/cos approximations (Taylor or precomputed tables recommended instead)
float sin(float x) {
    while (x > PI) x -= 2 * PI;
    while (x < -PI) x += 2 * PI;
    float x2 = x * x;
    return x - x2 * x / 6 + x2 * x2 * x / 120 - x2 * x2 * x2 * x / 5040 + x2 * x2 * x2 * x2 * x / 362880;
}

float cos(float x) {
    return sin(x + PI / 2);
}

const float R1 = 1;
const float R2 = 2;
const float K2 = 5;
const float K1 = 320*K2*3/(8*(R1+R2));

void render_torus_frame(float A, float B) {
  // precompute sines and cosines of A and B
  float cosA = cos(A), sinA = sin(A);
  float cosB = cos(B), sinB = sin(B);

  static uint8_t output[200][320] = {0};
  static float zbuffer[200][320] = {0};

  memset(output, 0, sizeof(output));
  memset(zbuffer, 0, sizeof(zbuffer));

  // theta goes around the cross-sectional circle of a torus
  for (float theta=0; theta < 2*pi; theta += theta_spacing) {
    // precompute sines and cosines of theta
    float costheta = cos(theta), sintheta = sin(theta);

    // phi goes around the center of revolution of a torus
    for(float phi=0; phi < 2*pi; phi += phi_spacing) {
      // precompute sines and cosines of phi
      float cosphi = cos(phi), sinphi = sin(phi);
    
      // the x,y coordinate of the circle, before revolving (factored
      // out of the above equations)
      float circlex = R2 + R1*costheta;
      float circley = R1*sintheta;

      // final 3D (x,y,z) coordinate after rotations, directly from
      // our math above
      float x = circlex*(cosB*cosphi + sinA*sinB*sinphi)
        - circley*cosA*sinB; 
      float y = circlex*(sinB*cosphi - sinA*cosB*sinphi)
        + circley*cosA*cosB;
      float z = K2 + cosA*circlex*sinphi + circley*sinA;
      if (z == 0) continue; // avoid division by zero 
      float ooz = 1/z;  // "one over z"
      
      // x and y projection.  note that y is negated here, because y
      // goes up in 3D space but down on 2D displays.
      int xp = (int) (320/2 + K1*ooz*x);
      int yp = (int) (200/2 - K1*ooz*y);
      
      // calculate luminance.  ugly, but correct.
      float L = cosphi*costheta*sinB - cosA*costheta*sinphi -
        sinA*sintheta + cosB*(cosA*sintheta - costheta*sinA*sinphi);
      // L ranges from -sqrt(2) to +sqrt(2).  If it's < 0, the surface
      // is pointing away from us, so we won't bother trying to plot it.
      if (L > 0) {
        // test against the z-buffer.  larger 1/z means the pixel is
        // closer to the viewer than what's already plotted.
        if (xp < 0 || xp >= 320 || yp < 0 || yp >= 200) continue; // bounds check
        if(ooz > zbuffer[yp][xp]) {
          zbuffer[yp][xp] = ooz;
          int luminance_index = L*8;
          // luminance_index is now in the range 0..11 (8*sqrt(2) = 11.3)
          // now we lookup the character corresponding to the
          // luminance and plot it in our output:
          output[yp][xp] = 230 + 2*luminance_index;
        }
      }
    }
  }

  // now, dump output[] to the screen.
  syscall(20, (uint32_t)output, 0, 320);
}

void torus() {
    float A = 0, B = 0;

    for (int i = 0; i <= 100 ; i++) {
      render_torus_frame(A, B);
      A += theta_spacing;
      B += phi_spacing; 
    }

    return;
}
