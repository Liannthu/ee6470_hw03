
# General description or introduction of the problem and your solution

This homework is very similar to hw01, but it changes the data transfering way from fifo to TLMp2p.  

# Implementation details (data structure, flows and algorithms)

The code is very similar to previous hw01.
### main.cpp
```
Testbench tb("tb");
SobelFilter sobel_filter("sobel_filter");
tb.initiator.i_skt(sobel_filter.t_skt);
```
In main.cpp file, most of the code remain unchanged. I change the fifo to initiator for data transfering. 
### testbench.cpp
```
for (y = 0; y != height; ++y) {
    for (x = 0; x != width; ++x) {
      adjustX = (MASK_X % 2) ? 1 : 0; // 1
      adjustY = (MASK_Y % 2) ? 1 : 0; // 1
      xBound = MASK_X / 2;            // 1
      yBound = MASK_Y / 2;            // 1

      for (v = -yBound; v != yBound + adjustY; ++v) {   //-1, 0, 1
        for (u = -xBound; u != xBound + adjustX; ++u) { //-1, 0, 1
          if (x + u >= 0 && x + u < width && y + v >= 0 && y + v < height) {
            R = *(source_bitmap +
                  bytes_per_pixel * (width * (y + v) + (x + u)) + 2);
            G = *(source_bitmap +
                  bytes_per_pixel * (width * (y + v) + (x + u)) + 1);
            B = *(source_bitmap +
                  bytes_per_pixel * (width * (y + v) + (x + u)) + 0);
          } else {
            R = 0;
            G = 0;
            B = 0;
          }
          data.uc[0] = R;
          data.uc[1] = G;
          data.uc[2] = B;
          mask[0] = 0xff;
          mask[1] = 0xff;
          mask[2] = 0xff;
          mask[3] = 0;
          initiator.write_to_socket(SOBEL_FILTER_R_ADDR, mask, data.uc, 4);
        }
      }

      initiator.read_from_socket(SOBEL_FILTER_RESULT_ADDR, mask, data.uc, 4);
      unsigned char red, green, blue;
      red = data.uc[0];
      green = data.uc[1];
      blue = data.uc[2];
      //std::cout << "[testbench] red = " << red << std::endl;


      *(target_bitmap + bytes_per_pixel * (width * y + x) + 2) = std::min(std::max(int((int) red), 0), 255);
      *(target_bitmap + bytes_per_pixel * (width * y + x) + 1) = std::min(std::max(int((int) green), 0), 255);
      *(target_bitmap + bytes_per_pixel * (width * y + x) + 0) = std::min(std::max(int((int) blue), 0), 255);
      }
    }
```
Most of the code remains unchanges as hw01. I change fifo to data which will be sented to sobel module to do calculation. Then, I will read data to write new .bmp.

### sobelfilter.cpp
```

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



```
In SobelFilter.cpp, I changed the mask and the calculation process so that it can apply the mask on red, green, and blue separately, instead of merging those colors to gray in the previous process. I register blocking_transport function for TLMp2p.


# Additional features of your design and models

none

# Experimental results (on different benchmarks and settings)

## origin image
![lena_std_short](https://user-images.githubusercontent.com/76727373/112020617-74aa2000-8b6b-11eb-9897-ef471ba26100.png)
## generated img
![req01](https://user-images.githubusercontent.com/76727373/112020712-8ab7e080-8b6b-11eb-8f3e-42f7223407b5.png)

# Discussions and conclusions.

This homework is quite hard, but it is without a doubet a good practice for us to learn systemC. Besides, we have to write our report in English and submit our homework to GitHub, which are not familiar to me. Anyway, thank you for providing us this challenge.
