#pragma once

#include "IPlugStructs.h"
#include "UGen.h"
#include "Line.h"
#include "Multiplier.h"
#include "Constant.h"
#include "Noise.h"
#include "TickRate.h"

#include "vessl.h"

#include <vector>

#define BUFFER_SIZE 44100*4

using namespace iplug;

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
    mSignalDT = 1.0 / sampleRate;
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
  void SetNoiseMod(double value) { mMod = value; TriggerModChange(value, 0.01); }
  void SetNoiseRate(double value) { mRate = value; if (!mMidiNotes.empty()) TriggerRateChange(value, 0.01); }
  void SetNoiseRange(double value) { mRange = value; TriggerRangeChange(value, 0.1); }
  void SetNoiseShape(double value) { mShape = value; TriggerShapeChange(value, 0.1); }

  float GetNoiseOffset() const { return mNoizeOffset->value.getLastValue(); }
  float GetNoiseRate() const { return mNoizeRate->getLastValues()[0]; }
  float GetShape() const { return mShapeCtrl.getLastValues()[0]; }
  int   GetShaperSize() const { return mShaperSize; }
  float GetShaperMapValue() const { return mShaperMapValue; }

private:
  void TriggerModChange(sample target, sample duration)
  {
    mModCtrl.activate(duration, mModCtrl.getAmp(), target);
    vModCtrl.begin = vModCtrl.value;
    vModCtrl.end = target;
    vModCtrl.duration = duration;
    vModCtrl.trigger();
  }

  void TriggerRateChange(sample target, sample duration)
  {
    mRateCtrl.activate(duration, mRateCtrl.getAmp(), target);
    vRateCtrl.begin = vRateCtrl.value;
    vRateCtrl.end = target;
    vRateCtrl.duration = duration;
    vRateCtrl.trigger();
  }

  void TriggerRangeChange(sample target, sample duration)
  {
    mRangeCtrl.activate(duration, mRangeCtrl.getAmp(), target);
    vRangeCtrl.begin = vRangeCtrl.value;
    vRangeCtrl.end = target;
    vRangeCtrl.duration = duration;
    vRangeCtrl.trigger();
  }

  void TriggerShapeChange(sample target, sample duration)
  {
    mShapeCtrl.activate(duration, mShapeCtrl.getAmp(), target);
    vShapeCtrl.begin = vShapeCtrl.value;
    vShapeCtrl.end = target;
    vShapeCtrl.duration = duration;
    vShapeCtrl.trigger();
  }

  // params
  double mVolume, mAttack, mDecay, mSustain, mRelease;
  double mMod, mRate, mRange, mShape;
  int mShaperSize;
  float mShaperMapValue;
  double mSignalDT;

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

  // vessl version
  vessl::samplebuffer<sample, BUFFER_SIZE> mBufferLeft;
  vessl::samplebuffer<sample, BUFFER_SIZE> mBufferRight;

  vessl::noise<sample> vNoize;
  vessl::multiplier<sample> vNoizeAmp;
  vessl::oscil<sample> vNoizeMod;
  vessl::multiplier<sample> vNoizeModAmp;
  vessl::summer<sample, 2> vNoizeSum;
  vessl::waveshaper<sample> vNoizeShaperLeft;
  vessl::waveshaper<sample> vNoizeShaperRight;
  vessl::mixer<sample, 2, 2> vNoizeShaperMixer;

  vessl::ramp<sample> vRateCtrl;
  vessl::ramp<sample> vModCtrl;
  vessl::ramp<sample> vRangeCtrl;
  vessl::ramp<sample> vShapeCtrl;

  vessl::signal<sample> vMainSignal;
};
