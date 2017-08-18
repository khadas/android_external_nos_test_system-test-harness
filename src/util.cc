#include "src/util.h"

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <sstream>
#include <thread>

#include "src/lib/inc/crc_16.h"

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;

namespace test_harness {

string find_uart(){
  constexpr char dir_path[] = "/dev/";
  auto dir = opendir(dir_path);
  if (!dir) {
    return "";
  }

  string return_value = "";
  const char prefix[] = "ttyUltraTarget";
  const size_t prefix_length = sizeof(prefix) / sizeof(prefix[0]) - 1;
  while (auto listing = readdir(dir)) {
    // The following is always true so it is not checked:
    // sizeof(listing->d_name) >= sizeof(prefix)
    std::cout << "CHECKING: " <<listing->d_name << std::endl;
    if (std::equal(prefix, prefix + prefix_length, listing->d_name)) {
      return_value = string(dir_path) + listing->d_name;
      break;
    }
  }

  std::cout << "USING: " << return_value << std::endl;

  closedir(dir);
  return return_value;
}

TestHarness::TestHarness() {
  string path = find_uart();
  init(path.c_str());

  switchFromConsoleToProtoApi();
}

TestHarness::TestHarness(const char* path) {
  init(path);

  switchFromConsoleToProtoApi();
}

TestHarness::~TestHarness() {
  free(encoder.frame_buffer);
  free(decoder.pdu_buffer);
  std::cout << "CLOSING TEST HARNESS" << std::endl;
  close(tty_fd);
}

bool TestHarness::ttyState() const {
  return tty_fd != -1;
}

void TestHarness::flushConsole() {
  while (readLineUntilBlock().size() > 0) {}
}

int TestHarness::sendAhdlc(const raw_message &msg) {
  if (EncodeNewFrame(&encoder) != AHDLC_OK) {
    return 1;
  }

  if (EncodeAddByteToFrameBuffer(&encoder, (uint8_t) (msg.type >> 8))
      != AHDLC_OK || EncodeAddByteToFrameBuffer(&encoder, (uint8_t) msg.type)
      != AHDLC_OK) {
    return 1;
  }
  if (EncodeBuffer(&encoder, msg.data, msg.data_len) != AHDLC_OK) {
    return 1;
  }

  blockingWrite((const char*) encoder.frame_buffer,
                encoder.frame_info.buffer_index);
  return 0;
}

void TestHarness::init(const char* path) {
  std::cout << "init() start\n";
  std::cout.flush();

  encoder.buffer_len = PROTO_BUFFER_MAX_LEN * 2;
  encoder.frame_buffer = reinterpret_cast<uint8_t*>(malloc(encoder.buffer_len));
  if (ahdlcEncoderInit(&encoder, CRC16) != AHDLC_OK) {
    fatal_error("ahdlcEncoderInit()");
  }

  decoder.buffer_len = PROTO_BUFFER_MAX_LEN * 2;
  decoder.pdu_buffer = reinterpret_cast<uint8_t*>(malloc(decoder.buffer_len));
  if (AhdlcDecoderInit(&decoder, CRC16) != AHDLC_OK) {
    fatal_error("AhdlcDecoderInit()");
  }

  memset(&tty_state, 0, sizeof(tty_state));
  tty_state.c_cc[VMIN] = 0;
  tty_state.c_cc[VTIME] = 0;
  tty_state.c_iflag = CREAD | CS8 | CLOCAL;
  cfsetispeed(&tty_state, B115200);
  cfsetospeed(&tty_state, B115200);

  errno = 0;
  tty_fd = open(path, O_RDWR | O_NONBLOCK);
  if (errno != 0) {
    perror("ERROR open()");
  }
  errno = 0;
  tcsetattr(tty_fd, TCSAFLUSH, &tty_state);
  if (errno != 0) {
    perror("ERROR tcsetattr()");
  }

  std::cout << "init() finish\n";
  std::cout.flush();
}

void TestHarness::switchFromConsoleToProtoApi() {
  std::cout << "switchFromConsoleToProtoApi() start\n";
  std::cout.flush();

  if (tty_fd == -1) { fatal_error("Unable to open tty!"); }

  flushUntil(BYTE_TIME * 1024);

  blockingWrite("version\n", 1);

  flushUntil(BYTE_TIME * 1024);

  blockingWrite("\n", 1);

  while (readLineUntilBlock() != "> ") {}

  const char command[] = "protoapi uart on 1\n";
  blockingWrite(command, sizeof(command) - 1);

  flushUntil(BYTE_TIME * 1024);

  std::cout << "switchFromConsoleToProtoApi() finish\n";
  std::cout.flush();
}


void TestHarness::blockingWrite(const char* data, size_t len) {
  std::cout << "blockingWrite(..., " <<std::dec << len << ")\n";
  size_t loc = 0;
  while (loc < len) {
    errno = 0;
    int return_value = write(tty_fd, data + loc, len - loc);
    if (errno != 0){
      perror("ERROR write()");
    }
    if (return_value < 0) {
      if (errno != EWOULDBLOCK && errno != EAGAIN) {
        fatal_error("write(tty_fd,...)");
      } else {
        std::this_thread::sleep_for(BYTE_TIME);
      }
    } else {
      loc += return_value;
    }
  }
}

string TestHarness::readLineUntilBlock() {
  string line = "";
  line.reserve(128);
  char read_value = ' ';
  std::stringstream ss;

  auto last_success = high_resolution_clock::now();
  while (true) {
    errno = 0;
    while (read_value != '\n' && read(tty_fd, &read_value, 1) > 0) {
      last_success = high_resolution_clock::now();
      if (isprint(read_value)) {
        ss << read_value;
      } else {
        ss << "\\0x" << std::hex << (uint32_t) read_value;
      }
      line.append(1, read_value);
    }
    if (errno != 0) {
      perror("ERROR read()");
    }

    /* If there wasn't anything to read yet, or the end of line is reached
     * there is no need to continue. */
    if (read_value == '\n' || line.size() == 0 ||
        duration_cast<microseconds>(high_resolution_clock::now() -
                                    last_success) > 4 * BYTE_TIME) {
      break;
    }

    /* Wait for at least one bit time before checking read() again. */
    std::this_thread::sleep_for(BIT_TIME);
  }

  if (line.size() > 0) {
    std::cout << "RD: " << ss.str() <<"\n";
    std::cout.flush();
  }
  return line;
}

void TestHarness::flushUntil(microseconds end) {
  char read_value = ' ';
  std::stringstream ss;

  auto start = high_resolution_clock::now();
  while (duration_cast<microseconds>(high_resolution_clock::now() -
      start) < end) {
    errno = 0;
    while (read_value != '\n' && read(tty_fd, &read_value, 1) > 0) {
      if (isprint(read_value)) {
        ss << read_value;
      } else {
        ss << "\\0x" << std::hex << (uint32_t) read_value;
      }
    }
    if (errno != 0) {
      perror("ERROR read()");
    }

    /* If there wasn't anything to read yet, or the end of line is reached
     * there is no need to continue. */
    if (read_value == '\n') {
      read_value = ' ';
      std::cout << "RD: " << ss.str() <<"\n";
      std::cout.flush();
      ss.str("");
      continue;
    }

    /* Wait for at least one bit time before checking read() again. */
    std::this_thread::sleep_for(BIT_TIME);
  }

  string remaining = ss.str();
  if (remaining.size() > 0) {
    std::cout << "RD: " << ss.str() <<"\n";
    std::cout.flush();
  }
}


void fatal_error(const string& msg) {
  std::cerr << "FATAL ERROR: " << msg << std::endl;
  exit(1);
}

}  // namespace test_harness
