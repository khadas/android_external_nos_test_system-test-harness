#include "src/util.h"

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <sstream>
#include <thread>

#include "proto/control.pb.h"
#include "proto/header.pb.h"
#include "src/lib/inc/crc_16.h"

using nugget::app::protoapi::ControlRequest;
using nugget::app::protoapi::ControlRequestType;
using nugget::app::protoapi::Notice;
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
  switchFromProtoApiToConsole();

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

void print_bin(std::ostream &out, uint8_t c) {
  if (c == '\\') {
    out << "\\\\";
  } else if (isprint(c)) {
    out << c;
  } else if (c < 16) {
    out << "\\x0" << std::hex << (uint32_t) c;
  } else {
    out << "\\x" << std::hex << (uint32_t) c;
  }
}

int TestHarness::sendAhdlc(const raw_message &msg) {
  if (EncodeNewFrame(&encoder) != AHDLC_OK) {
    return TRANSPORT_ERROR;
  }

  if (EncodeAddByteToFrameBuffer(&encoder, (uint8_t) (msg.type >> 8))
      != AHDLC_OK || EncodeAddByteToFrameBuffer(&encoder, (uint8_t) msg.type)
      != AHDLC_OK) {
    return TRANSPORT_ERROR;
  }
  if (EncodeBuffer(&encoder, msg.data, msg.data_len) != AHDLC_OK) {
    return TRANSPORT_ERROR;
  }

  blockingWrite((const char*) encoder.frame_buffer,
                encoder.frame_info.buffer_index);
  return NO_ERROR;
}

int TestHarness::sendOneofProto(uint16_t type, uint16_t subtype,
                                const google::protobuf::Message &message) {
  test_harness::raw_message msg;
  msg.type = type;
  int msg_size = message.ByteSize();
  if (msg_size + 2 > (int) PROTO_BUFFER_MAX_LEN) {
    return OVERFLOW_ERROR;
  }
  msg.data[0] = subtype >> 8;
  msg.data[1] = (uint8_t) subtype;

  msg.data_len = (uint16_t) (msg_size + 2);
  if (!message.SerializeToArray(msg.data + 2, msg_size)) {
    return SERIALIZE_ERROR;
  }

  auto return_value = sendAhdlc(msg);
  return return_value;
}

int TestHarness::sendProto(uint16_t type,
                           const google::protobuf::Message &message) {
  test_harness::raw_message msg;
  msg.type = type;
  int msg_size = message.ByteSize();
  if (msg_size > (int) (PROTO_BUFFER_MAX_LEN - 2)) {
    return OVERFLOW_ERROR;
  }
  msg.data_len = (uint16_t) msg_size;
  if (!message.SerializeToArray(msg.data, msg.data_len)) {
    return SERIALIZE_ERROR;
  }

  auto return_value = sendAhdlc(msg);
  return return_value;
}

int TestHarness::getAhdlc(raw_message* msg, microseconds timeout) {
  std::cout << "RX: ";
  size_t read_count = 0;
  while (true) {  //TODO? timeout
    uint8_t read_value;
    auto start = high_resolution_clock::now();
    while (read(tty_fd, &read_value, 1) <= 0) {
      if(timeout >= microseconds(0) &&
         duration_cast<microseconds>(high_resolution_clock::now() - start) >
         microseconds(timeout)) {
        std::cout <<"\n";
        std::cout.flush();
        return TIMEOUT;
      }
    }
    ++read_count;

    ahdlc_op_return return_value =
        DecodeFrameByte(&decoder, read_value);

    if (read_value == '\n') {
      std::cout << "\nRX: ";
    } else {
      print_bin(std::cout, read_value);
    }
    std::cout.flush();

    if (read_count > 7) {
      if (return_value == AHDLC_COMPLETE || decoder.decoder_state == DECODE_COMPLETE_BAD_CRC) {
        if (decoder.frame_info.buffer_index < 2) {
          std::cout <<"\n";
          std::cout.flush();
          std::cout << "UNDERFLOW ERROR\n";
          return TRANSPORT_ERROR;
        }

        msg->type = (decoder.pdu_buffer[0] << 8) | decoder.pdu_buffer[1];
        msg->data_len = decoder.frame_info.buffer_index - 2;
        std::copy(decoder.pdu_buffer + 2,
                  decoder.pdu_buffer + decoder.frame_info.buffer_index,
                  msg->data);
        std::cout <<"\n";
        if (return_value == AHDLC_COMPLETE) {
          std::cout << "GOOD CRC\n";
        } else {
          std::cout << "BAD CRC\n";
        }
        std::cout.flush();
        return NO_ERROR;
      } else if (decoder.decoder_state == DECODE_COMPLETE_BAD_CRC) {
        std::cout <<"\n";
        std::cout << "AHDLC BAD CRC\n";
        std::cout.flush();
        return TRANSPORT_ERROR;
      } else if (decoder.frame_info.buffer_index >= PROTO_BUFFER_MAX_LEN) {
        if (AhdlcDecoderInit(&decoder, CRC16) != AHDLC_OK) {
          fatal_error("AhdlcDecoderInit()");
        }
        std::cout <<"\n";
        std::cout.flush();
        std::cout << "OVERFLOW ERROR\n";
        return OVERFLOW_ERROR;
      }
    }
  }
}

void TestHarness::init(const char* path) {
  std::cout << "init() start\n";
  std::cout.flush();

  encoder.buffer_len = PROTO_BUFFER_MAX_LEN;
  encoder.frame_buffer = reinterpret_cast<uint8_t*>(malloc(encoder.buffer_len));
  if (ahdlcEncoderInit(&encoder, CRC16) != AHDLC_OK) {
    fatal_error("ahdlcEncoderInit()");
  }

  decoder.buffer_len = PROTO_BUFFER_MAX_LEN;
  decoder.pdu_buffer = reinterpret_cast<uint8_t*>(malloc(decoder.buffer_len));
  if (AhdlcDecoderInit(&decoder, CRC16) != AHDLC_OK) {
    fatal_error("AhdlcDecoderInit()");
  }

  /*memset(&tty_state, 0, sizeof(tty_state));

  tty_state.c_cc[VMIN] = 0;
  tty_state.c_cc[VTIME] = 0;
  tty_state.c_iflag = CREAD | CS8 | CLOCAL;
  cfsetispeed(&tty_state, B115200);
  cfsetospeed(&tty_state, B115200);*/

  errno = 0;
  //tty_fd = open(path, O_RDWR | O_NONBLOCK);
  tty_fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
  if (errno != 0) {
    perror("ERROR open()");
    fatal_error("");
  }
  errno = 0;

  if (!isatty(tty_fd)) {
    fatal_error("Path is not a tty");
  }

  if (tcgetattr(tty_fd, &tty_state)) {
    perror("ERROR tcgetattr()");
    fatal_error("");
  }

  if (cfsetospeed(&tty_state, B115200) ||
      cfsetispeed(&tty_state, B115200)) {
    perror("ERROR cfsetospeed()");
    fatal_error("");
  }

  tty_state.c_cc[VMIN] = 0;
  tty_state.c_cc[VTIME] = 0;

  tty_state.c_iflag = tty_state.c_iflag & ~(IXON | ISTRIP | INPCK | PARMRK |
                                            INLCR | ICRNL | BRKINT | IGNBRK);
  tty_state.c_iflag = 0;
  tty_state.c_oflag = 0;
  tty_state.c_lflag = tty_state.c_lflag & ~(ECHO | ECHONL | ICANON | IEXTEN |
                                            ISIG);
  tty_state.c_cflag = (tty_state.c_cflag & ~(CSIZE | PARENB)) | CS8;

  if (tcsetattr(tty_fd, TCSAFLUSH, &tty_state)) {
    perror("ERROR tcsetattr()");
    fatal_error("");
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

raw_message TestHarness::switchFromProtoApiToConsole() {
  std::cout << "switchFromProtoApiToConsole() start\n";
  std::cout.flush();

  ControlRequest controlRequest;
  controlRequest.set_type(ControlRequestType::REVERT_TO_CONSOLE);
  string line;
  controlRequest.SerializeToString(&line);

  raw_message msg;
  msg.type = APImessageID::CONTROL_REQUEST;

  std::copy(line.begin(), line.end(), msg.data);
  msg.data_len = line.size();

  sendAhdlc(msg);


  if (getAhdlc(&msg, 4096 * BYTE_TIME) == NO_ERROR &&
      msg.type == APImessageID::NOTICE) {
    Notice message;
    message.ParseFromArray((char *) msg.data, msg.data_len);
    std::cout << message.DebugString() <<std::endl;
  } else {
    std::cout << "Receive Error" <<std::endl;
  }

  flushUntil(BYTE_TIME * 4096);

  std::cout << "switchFromProtoApiToConsole() finish\n";
  std::cout.flush();
  return msg;
}

void TestHarness::blockingWrite(const char* data, size_t len) {
  std::cout << "TX: ";
  for (size_t i = 0; i < len; ++i) {
    uint8_t value = data[i];
    if (value == '\n') {
      std::cout << "\nTX: ";
    } else {
      print_bin(std::cout, value);
    }
  }
  std::cout << "\n";
  std::cout.flush();

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
      print_bin(ss, read_value);
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
    std::cout << "RX: " << ss.str() <<"\n";
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
      print_bin(ss, read_value);
    }
    if (errno != 0) {
      perror("ERROR read()");
    }

    /* If there wasn't anything to read yet, or the end of line is reached
     * there is no need to continue. */
    if (read_value == '\n') {
      read_value = ' ';
      std::cout << "RX: " << ss.str() <<"\n";
      std::cout.flush();
      ss.str("");
      continue;
    }

    /* Wait for at least one bit time before checking read() again. */
    std::this_thread::sleep_for(BIT_TIME);
  }

  string remaining = ss.str();
  if (remaining.size() > 0) {
    std::cout << "RX: " << ss.str() <<"\n";
    std::cout.flush();
  }
}


void fatal_error(const string& msg) {
  std::cerr << "FATAL ERROR: " << msg << std::endl;
  exit(1);
}

const char* error_codes_name(int code) {
  switch (code) {
    case error_codes::NO_ERROR:
      return "NO_ERROR";
    case error_codes::GENERIC_ERROR:
      return "GENERIC_ERROR";
    case error_codes::TIMEOUT:
      return "TIMEOUT";
    case error_codes::TRANSPORT_ERROR:
      return "TRANSPORT_ERROR";
    case error_codes::OVERFLOW_ERROR:
      return "OVERFLOW_ERROR";
    case error_codes::SERIALIZE_ERROR:
      return "SERIALIZE_ERROR";
    default:
      return "unknown";
  }
}

}  // namespace test_harness
