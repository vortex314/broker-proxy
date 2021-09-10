#include <errno.h>
#include <fcntl.h>
#include <log.h>
#include <stdio.h>
#include <string.h>     // for strerror
#include <sys/ioctl.h>  //ioctl() call defenitions
#include <unistd.h>     // for close

#include <string>
typedef vector<uint8_t> Bytes;

LogS logger;

/*
    /DTR    /RTS   /RST  GPIO0   TIOCM_DTR    TIOCM_RTS
    1       0       0       1       0           1           => reset
    0       1       1       0       1           0           => prog enable
    1       1       1       1       0           0           => run
    0       0       1       1       1           1           => run

to run mode
    run => reset => run
    run => prog enable => reset => run

*/

#define ASSERT(xx) \
  if (!(xx)) WARN(" failed " #xx " %d : %s ", errno, strerror(errno));

int modeInfo(int fd, const char* comment = "") {
  int mode;
  int rc = ioctl(fd, TIOCMGET, &mode);
  ASSERT(rc == 0);
  std::string sMode;
  if (mode & TIOCM_CAR) sMode += "TIOCM_CAR,";
  if (mode & TIOCM_CD) sMode += "TIOCM_CD,";
  if (mode & TIOCM_DTR) sMode += "TIOCM_DTR,";
  if (mode & TIOCM_RTS) sMode += "TIOCM_RTS,";
  if (mode & TIOCM_DSR) sMode += "TIOCM_DSR,";
  INFO("%s line mode : %s", comment, sMode.c_str());
  fflush(stdout);
  return 0;
}

static bool setBit(int fd, int flags) {
  int rc = ioctl(fd, TIOCMBIS, &flags);  // set DTR pin
  if (rc) WARN("ioctl()= %s (%d)", strerror(errno), errno);
  return rc == 0;
}
static bool clrBit(int fd, int flags) {
  int rc = ioctl(fd, TIOCMBIC, &flags);
  if (rc) WARN("ioctl()= %s (%d)", strerror(errno), errno);
  return rc == 0;
}

bool setBits(int fd, int flags) {
  int rc = ioctl(fd, TIOCMSET, &flags);  // set DTR pin
  if (rc) WARN("ioctl()= %s (%d)", strerror(errno), errno);
  return rc == 0;
}
/*
to toggle RESET pin RTS pin should be opposite DTR
*/
void reset(int fd) {
  int mode;
  int rc = ioctl(fd, TIOCMGET, &mode);
  ASSERT(rc == 0)
  if (mode & TIOCM_DTR) {
    sleep(1);
    clrBit(fd, TIOCM_RTS);  // raises RTS pin
    sleep(1);
    setBit(fd, TIOCM_RTS);  // lowers RTS pin
  } else {
    sleep(1);
    setBit(fd, TIOCM_RTS);  // lowers RTS pin
    sleep(1);
    clrBit(fd, TIOCM_RTS);  // raises RTS pin
  }
}

void setDtr(int fd) { setBit(fd, TIOCM_DTR); }
void clrDtr(int fd) { clrBit(fd, TIOCM_DTR); }

void setRts(int fd) { setBit(fd, TIOCM_RTS); }
void clrRts(int fd) { clrBit(fd, TIOCM_RTS); }

void resetLow(int fd) { setBits(fd, TIOCM_RTS); }
void progLow(int fd) { setBits(fd, TIOCM_DTR); }
void allHigh(int fd) { setBits(fd, 0); }

const uint8_t ESP_SYNC = 0x08;  // sync char
const uint8_t ESP_NONE = 0x00;
const uint32_t SYNC_TIMEOUT_MS = 100;

Bytes slipFrame(Bytes data) {
  Bytes result = {0xC0};
  for (auto b : data) {
    if (b == 0xDB) {
      result.push_back(0xDB);
      result.push_back(0xDD);
    } else if (b == 0xC0) {
      result.push_back(0xDB);
      result.push_back(0xDC);
    } else {
      result.push_back(b);
    }
  }
  result.push_back(0xC0);
  return result;
}

Bytes toPacket(uint8_t op, Bytes data, uint8_t chk = 0) {
  Bytes result = {'<', 'B', 'B', 'H', 'I', 0x00, op, (uint8_t)data.size(), chk};
  for (uint8_t b : data) result.push_back(b);
  return result;
}

Bytes command(int fd, uint8_t op, Bytes& bs, uint32_t timeout) {
  uint8_t buffer[1024];
  uint64_t startTime = Sys::millis();
  Bytes packet = slipFrame(toPacket(op, bs));
  INFO("FLUSH");
  int cnt;
  while (cnt = read(fd, buffer, sizeof(buffer)) > 0) {
    INFO("FLUSH %d ", cnt);
  };  // flush input
  INFO("WRITE %s", hexDump(packet).c_str());
  int rc = write(fd, packet.data(), packet.size());
  while (startTime + timeout > Sys::millis()) {
    INFO("READ");
    int count = read(fd, buffer, sizeof(buffer));
    if (count > 0) {
      Bytes result(buffer, buffer + count);
      INFO("RXD %s == %s ", hexDump(result).c_str(), charDump(result).c_str());
      return result;
    }
    usleep(1000);
  }
  return Bytes();
}

bool syncEsp32(int fd) {
  Bytes syncCommand = {0x07, 0x07, 0x12, 0x20};
  for (int i = 0; i < 32; i++) syncCommand.push_back(0x55);
  Bytes result = command(fd, ESP_SYNC, syncCommand, SYNC_TIMEOUT_MS);
  return result.size() > 0 && result[0] != 0x80;
}

void progUc(int fd, bool esp32r0Delay = false) {
  clrDtr(fd); // GPIO0 = 1
  setRts(fd); // NRST = 0
  usleep(100000);
  if (esp32r0Delay) usleep(1200000);
  clrRts(fd); // NRST =1
  setDtr(fd); // GPIO0 = 0
  usleep(40000);
  if (esp32r0Delay) usleep(50000);
  clrDtr(fd); // GPIO0 = 1
}

void runUc(int fd) {
  clrDtr(fd);
  setRts(fd);
  usleep(200000);
  clrRts(fd);
  usleep(200000);
}

int main(int argc, char** argv) {
  int fd;
  INFO("open %s ...", argv[1]);
  fd = open(argv[1], O_EXCL | O_RDWR | O_NOCTTY | O_NDELAY |
                         O_SYNC);  // Open Serial Port
  ASSERT(fd > 0);
  int flags = fcntl(fd, F_GETFL, 0);
  int rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  progUc(fd);
  modeInfo(fd);
  printf("Prog mode ... ");
  fflush(stdout);
  getchar();

  runUc(fd);
  modeInfo(fd);
  printf("Run mode ...");
  fflush(stdout);
  getchar();

  close(fd);
  return 0;
}