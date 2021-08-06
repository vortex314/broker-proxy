#ifndef _SESSION_SERAIL_H_
#define _SESSION_SERIAL_H_
#include <limero.h>
#include <serial.h>

typedef enum { CMD_OPEN, CMD_CLOSE } TcpCommand;

class SerialSessionError;

class SerialSession : public Actor, public Invoker {
  SerialSessionError *_errorInvoker;
  int _serialfd;
  Serial _serialPort;
  string _port;
  uint32_t _baudrate;
  Bytes _rxdBuffer;
  Bytes _inputFrame;
  Bytes _cleanData;
  uint64_t _lastFrameFlag;
  uint64_t _frameTimeout = 2000;

 public:
  ValueSource<Bytes> incoming;
  Sink<Bytes> outgoing;
  ValueSource<bool> connected;
  ValueSource<TcpCommand> command;
  SerialSession(Thread &thread, Config config);
  bool init();
  bool connect();
  bool disconnect();
  void onError();
  int fd();
  void invoke();
};

class SerialSessionError : public Invoker {
  SerialSession &_serialSession;

 public:
  SerialSessionError(SerialSession &serialSession)
      : _serialSession(serialSession){};
  void invoke() { _serialSession.onError(); }
};
#endif