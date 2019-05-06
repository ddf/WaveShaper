#pragma once

#include "IPlugStructs.h"
#include "UGen.h"
#include "Line.h"
#include "Multiplier.h"
#include "Constant.h"
#include "Noise.h"
#include "TickRate.h"

#include <vector>

class ADSR : public Minim::UGen
{
public:
	ADSR();

	bool isOn() const { return mState == kAttack || mState == kDecay || mState == kSustain; }

	void noteOn(float amp, float attack, float decay, float sustain, float release);
	void noteOff();

	// jump right to the Off state and set mAmp to 0. unpatch if patched.
	void stop();
	float getRelease() const { return mRelease; }

	UGenInput audio;

protected:
	virtual void uGenerate(float * channels, const int numChannels) override;

	virtual void sampleRateChanged() override;

private:

	enum
	{
		kOff,
		kAttack,
		kDecay,
		kSustain,
		kRelease
	} mState;

	bool mAutoRelease;
	float mAmp, mAttack, mDecay, mSustain, mRelease;
	float mTime, mStep;
};

namespace Minim
{
  class Oscil;
  class Pan;
  class Summer;
  class WaveShaper;
  class MultiChannelBuffer;
}

typedef std::vector<IMidiMsg> MidiMsgList;

class WaveShaperDSP
{
public:
#pragma mark -
  WaveShaperDSP(int channelCount);
  ~WaveShaperDSP();

  void ProcessBlock(sample** inputs, sample** outputs, int nOutputs, int nFrames);

  void Reset(double sampleRate, int blockSize)
  {
    mMidiQueue.Clear();
    mMidiQueue.Resize(blockSize);
    mMainSignalVol.setSampleRate((float)sampleRate);
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    mMidiQueue.Add(msg);
  }

  void SetWavetables(Minim::MultiChannelBuffer& buffer);

  void SetVolume(double value) { mVolume = value; }
  void SetAttack(double value) { mAttack = value; }
  void SetDecay(double value) { mDecay = value; }
  void SetSustain(double value) { mSustain = value; }
  void SetRelease(double value) { mRelease = value; }
  void SetNoiseTint(Minim::Noise::Tint value) { mNoiseTint = value; }
  void SetNoiseMod(double value) { mMod = value; mModCtrl.activate(0.01f, mModCtrl.getAmp(), value); }
  void SetNoiseRate(double value) { mRate = value; if (!mMidiNotes.empty()) mRateCtrl.activate(0.01f, mRateCtrl.getAmp(), value); }
  void SetNoiseRange(double value) { mRange = value; mRangeCtrl.activate(0.1f, mRangeCtrl.getAmp(), value); }
  void SetNoiseShape(double value) { mShape = value; mShapeCtrl.activate(0.1f, mShapeCtrl.getAmp(), value); }

  float GetNoiseOffset() const { return mNoizeOffset->value.getLastValue(); }
  float GetNoiseRate() const { return mNoizeRate->getLastValues()[0]; }
  float GetShape() const { return mShapeCtrl.getLastValues()[0]; }
  int   GetShaperSize() const { return mShaperSize; }
  float GetShaperMapValue() const { return mShaperMapValue; }

private:

  // params
  double mVolume, mAttack, mDecay, mSustain, mRelease;
  double mMod, mRate, mRange, mShape;
  int mShaperSize;
  float mShaperMapValue;

  IMidiQueue  mMidiQueue;
  MidiMsgList mMidiNotes;
  Minim::Noise::Tint mNoiseTint;

  Minim::Noise	     * mNoize;
  Minim::TickRate	   * mNoizeRate;
  Minim::Multiplier  * mNoizeAmp;
  Minim::Oscil	     * mNoizeMod;
  Minim::Summer	     * mNoizeSum;
  Minim::Constant	   * mNoizeOffset;
  Minim::WaveShaper  * mNoizeShaperLeft;
  Minim::WaveShaper  * mNoizeShaperRight;
  Minim::Pan		     * mPanLeft;
  Minim::Pan		     * mPanRight;
  Minim::Summer	     * mMainSignal;

  Minim::Multiplier  mMainSignalVol;

  // controls
  Minim::Line	mRateCtrl;
  Minim::Line	mModCtrl;
  Minim::Line	mRangeCtrl;
  Minim::Line mShapeCtrl;
  ADSR				mEnvelope;
};
