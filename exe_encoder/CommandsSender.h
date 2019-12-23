/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

#pragma once

#include "ICommandsSender.h"
extern "C"
{
#include "lib_encode/lib_encoder.h"
}

class CommandsSender : public ICommandsSender
{
public:
  explicit CommandsSender(AL_HEncoder hEnc) : hEnc(hEnc)
  {
    Reset();
  };
  ~CommandsSender() {};
  void notifySceneChange(int lookAhead) override;
  void notifyIsLongTerm() override;
  void notifyUseLongTerm() override;
  void restartGop() override;
  void setGopLength(int gopLength) override;
  void setNumB(int numB) override;
  void setFreqIDR(int freqIDR) override;
  void setFrameRate(int frameRate, int clockRatio) override;
  void setBitRate(int bitRate) override;
  void setQP(int qp) override;
  void setQPBounds(int iMinQP, int iMaxQP) override;
  void setQPIPDelta(int iQPDelta) override;
  void setQPPBDelta(int iQPDelta) override;
  void setDynamicInput(int iInputIdx) override;
  void setLFBetaOffset(int iBetaOffset) override;
  void setLFTcOffset(int iTcOffset) override;

  void Reset();
  bool HasInputChanged(int& iInputIdx);

private:
  AL_HEncoder hEnc;
  bool bInputChanged;
  int iInputIdx;
};

