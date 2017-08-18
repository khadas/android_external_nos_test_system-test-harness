#ifndef SRC_UTIL_H
#define SRC_UTIL_H

#include <termios.h>

#include <chrono>
#include <iostream>
#include <vector>

#include "src/lib/inc/frame_layer.h"
#include "src/lib/inc/frame_layer_types.h"

using std::string;

namespace test_harness {

const auto BIT_TIME = std::chrono::microseconds(10000 / 1152);
const auto BYTE_TIME = std::chrono::microseconds(80000 / 1152);

const size_t PROTO_BUFFER_MAX_LEN = 1024;

struct raw_message {
  uint16_t type;  // The "magic number" used to identify the contents of data[].
  uint16_t data_len;  // How much data is in the buffer data[].
  uint8_t data[PROTO_BUFFER_MAX_LEN];  // The payload of the message.
};

class TestHarness {
 public:
  TestHarness();
  /**
   * @param path The device path to the tty (e.g. "/dev/tty1"). */
  TestHarness(const char* path);

  ~TestHarness();

  /**
   * @return true if data can be sent and received over the tty. */
  bool ttyState() const;

  /** Reads from tty until it would block. */
  void flushConsole();
  /** Reads from tty until the specified duration has passed. */
  void flushUntil(std::chrono::microseconds end);

  int sendAhdlc(const raw_message &msg);
  int getAhdlc(raw_message* msg);

 protected:
  int tty_fd;
  struct termios tty_state;
  ahdlc_frame_encoder_t encoder;
  ahdlc_frame_decoder_t decoder;

  void init(const char* path);

  void switchFromConsoleToProtoApi();

  raw_message switchFromProtoApiToConsole();

  /** Writes @len bytes from @data until complete. */
  void blockingWrite(const char* data, size_t len);

  /** Reads raw bytes from the tty until either an end of line is reached or
   * the input would block.
   *
   * @return a single line with the '\n' character unless the last read() would
   * have blocked.*/
  string readLineUntilBlock();
};

void fatal_error(const string& msg);

}  // namespace test_harness

#endif  // SRC_UTIL_H
