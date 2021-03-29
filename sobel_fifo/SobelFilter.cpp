#include <cmath>

#include "SobelFilter.h"

SobelFilter::SobelFilter(sc_module_name n) : sc_module(n) {
  SC_THREAD(do_filter);
  sensitive << i_clk.pos();
  dont_initialize();
  reset_signal_is(i_rst, false);
}

// sobel mask
int mask[MASK_X][MASK_Y] = 
{
  1,2,1,
  2,4,2,
  1,2,1
};

void SobelFilter::do_filter() {
  { wait(); }
  while (true) {
    for (unsigned int i = 0; i < MASK_N; ++i) {
      val[i] = 0;
      wait();
    }
    double  red = 0.0, green = 0.0, blue = 0.0;
    int  _red = 0.0, _green = 0.0, _blue = 0.0;
    for (unsigned int v = 0; v < MASK_Y; ++v) {
      for (unsigned int u = 0; u < MASK_X; ++u) {
        _red = int(i_r.read());
        _green = int(i_g.read());
        _blue = int(i_b.read());
        wait();
        red += _red * mask[u][v];
        green += _green * mask[u][v];
        blue += _blue * mask[u][v];
        wait();
      }
    }
    o_r.write(red);
    o_g.write(green);
    o_b.write(blue);
  }
}
