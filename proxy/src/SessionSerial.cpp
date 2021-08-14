#include <SessionSerial.h>
#include <ppp_frame.h>

SessionSerial::SessionSerial(Thread &thread, Config config)
    : SessionAbstract(thread, config), _incomingMessage(10,"_incomingMessage"), _outgoingMessage(10,"_outgoingMessage"),
      _incomingSerial(10,"_incomingSerial") {

  _errorInvoker = new SerialSessionError(*this);
  _port = config["port"].as<std::string>();
  _baudrate = config["baudrate"].as<uint32_t>();

}

bool SessionSerial::init() {
  _serialPort.port(_port);
  _serialPort.baudrate(_baudrate);
  _serialPort.init();
  _incomingSerial >> bytesToFrame >> _incomingMessage;
  _outgoingMessage >> frameToBytes >> [&](const Bytes &data) {
    //        INFO("TXD %s => %s", _serialPort.port().c_str(),
    //        hexDump(data).c_str());
    _serialPort.txd(data);
  };
  return true;
}

bool SessionSerial::connect() {
  _serialPort.connect();
  thread().addReadInvoker(_serialPort.fd(), this);
  thread().addErrorInvoker(_serialPort.fd(), _errorInvoker);
  return true;
}

bool SessionSerial::disconnect() {
  thread().deleteInvoker(_serialPort.fd());
  _serialPort.disconnect();
  return true;
}
// on data incoming on filedescriptor
void SessionSerial::invoke() {
  int rc = _serialPort.rxd(_rxdBuffer);
  if (rc == 0) {                  // read ok
    if (_rxdBuffer.size() == 0) { // but no data
      WARN(" 0 data ");
    } else {
      _incomingSerial.on(_rxdBuffer);
    }
  }
}
// on error issue onf ile descriptor
void SessionSerial::onError() { disconnect(); }

int SessionSerial::fd() { return _serialPort.fd(); }

Source<Bytes>& SessionSerial::incoming() {
  return _incomingMessage;
}

Sink<Bytes>& SessionSerial::outgoing() {
  return _outgoingMessage;
}

Source<bool>& SessionSerial::connected() {
  return _connected;
}