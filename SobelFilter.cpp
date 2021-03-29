#include <cmath>
#include <iomanip>

#include "SobelFilter.h"

SobelFilter::SobelFilter(sc_module_name n)
    : sc_module(n), t_skt("t_skt"), base_offset(0) {
  SC_THREAD(do_filter);

  t_skt.register_b_transport(this, &SobelFilter::blocking_transport);
}

// sobel mask
int mask[MASK_X][MASK_Y] = 
{
  1,2,1,
  2,4,2,
  1,2,1
};

double _factor = 1.0/16.0;
double _bias = 0.0;

void SobelFilter::do_filter() {
  { wait(CLOCK_PERIOD, SC_NS); }
  while (true) {
    double  red = 0.0, green = 0.0, blue = 0.0;
    int  _red = 0.0, _green = 0.0, _blue = 0.0;
    unsigned char middle_red, middle_green, middle_blue;
    for (unsigned int v = 0; v < MASK_Y; ++v) {
      for (unsigned int u = 0; u < MASK_X; ++u) {
        _red = int(i_r.read());
        _green = int(i_g.read());
        _blue = int(i_b.read());
        wait(CLOCK_PERIOD, SC_NS);
        red += _red * mask[u][v];
        green += _green * mask[u][v];
        blue += _blue * mask[u][v];
        wait(CLOCK_PERIOD, SC_NS);
        if (v == 1 && u == 1){
          middle_red = _red;
          middle_green = _green;
          middle_blue = _blue;
        }
      }
    }
    red = (int) (red * _factor + _bias);
    green = (int) (green * _factor + _bias);
    blue = (int) (blue * _factor + _bias);
    //std::cout << "[sobel] red = " << red << " uc red = " << (unsigned char) red << std::endl;
    o_r.write((unsigned char) red);
    o_g.write((unsigned char) green);
    o_b.write((unsigned char) blue);
  }
}

void SobelFilter::blocking_transport(tlm::tlm_generic_payload &payload,
                                     sc_core::sc_time &delay) {
  sc_dt::uint64 addr = payload.get_address();
  addr = addr - base_offset;
  unsigned char *mask_ptr = payload.get_byte_enable_ptr();
  unsigned char *data_ptr = payload.get_data_ptr();
  word buffer;
  switch (payload.get_command()) {
  case tlm::TLM_READ_COMMAND:
    switch (addr) {
    case SOBEL_FILTER_RESULT_ADDR:
      buffer.uc[0] = o_r.read();
      buffer.uc[1] = o_g.read();
      buffer.uc[2] = o_b.read();
      buffer.uc[3] = 0xff;
      break;
    default:
      std::cerr << "Error! SobelFilter::blocking_transport: address 0x"
                << std::setfill('0') << std::setw(8) << std::hex << addr
                << std::dec << " is not valid" << std::endl;
      break;
    }
    data_ptr[0] = buffer.uc[0];
    data_ptr[1] = buffer.uc[1];
    data_ptr[2] = buffer.uc[2];
    data_ptr[3] = buffer.uc[3];
    break;

  case tlm::TLM_WRITE_COMMAND:
    switch (addr) {
    case SOBEL_FILTER_R_ADDR:
      if (mask_ptr[0] == 0xff) {
        i_r.write(data_ptr[0]);
      }
      if (mask_ptr[1] == 0xff) {
        i_g.write(data_ptr[1]);
      }
      if (mask_ptr[2] == 0xff) {
        i_b.write(data_ptr[2]);
      }
      break;
    default:
      std::cerr << "Error! SobelFilter::blocking_transport: address 0x"
                << std::setfill('0') << std::setw(8) << std::hex << addr
                << std::dec << " is not valid" << std::endl;
      break;
    }
    break;

  case tlm::TLM_IGNORE_COMMAND:
    payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
    return;
  default:
    payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
    return;
  }
  payload.set_response_status(tlm::TLM_OK_RESPONSE); // Always OK
}
