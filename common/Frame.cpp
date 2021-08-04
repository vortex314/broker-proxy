#include <Frame.h>
#include <CborDump.h>
//================================================================

BytesToFrame::BytesToFrame() : Flow<Bytes, Bytes>() {}

void BytesToFrame::on(const Bytes &bs) { handleRxd(bs); }

void BytesToFrame::toStdout(const Bytes &bs)
{
  if (bs.size())
  {
    LOGW << "ignoring Bytes : " << bs.size()  << LEND;
  }
}

bool BytesToFrame::handleFrame(const Bytes &bs)
{
  if (bs.size() == 0)
    return false;
  if (ppp_deframe(_cleanData, bs))
  {
    emit(_cleanData);
    return true;
  }
  else
  {
    toStdout(bs);
    return false;
  }
}

void BytesToFrame::handleRxd(const Bytes &bs)
{
  for (uint8_t b : bs)
  {
    if (b == PPP_FLAG_CHAR)
    {
      _lastFrameFlag = Sys::millis();
      handleFrame(_inputFrame);
      _inputFrame.clear();
    }
    else
    {
      _inputFrame.push_back(b);
    }
  }
  if ((Sys::millis() - _lastFrameFlag) > _frameTimeout)
  {
    //   cout << " skip  Bytes " << hexDump(bs) << endl;
    //   cout << " frame data drop " << hexDump(frameData) << flush;
    toStdout(bs);
    _inputFrame.clear();
  }
}
void BytesToFrame::request(){};

FrameToBytes::FrameToBytes()
    : LambdaFlow<Bytes, Bytes>([&](Bytes &out, const Bytes &in)
                               {
                                 out = ppp_frame(in);
                                 return true;
                               }){};
