#include "DSP.h"

#include "Noise.h"
#include "Multiplier.h"
#include "Waves.h"
#include "Wavetable.h"
#include "Oscil.h"
#include "TickRate.h"
#include "Pan.h"
#include "Multiplier.h"
#include "Summer.h"
#include "MultiChannelBuffer.h"

// this is hacky, but we can't compile the UGen source file as its own compilation unit becuase the file name is the same,
// so we simply directly include it here.
#include "ugens\Waveshaper.h"
#include "ugens\Waveshaper.cpp"

#pragma region Settings
extern const double kDefaultRate;
extern const double kDefaultShape;
extern const double kDefaultMod;
extern const double kDefaultRange;
extern const double kEnvAttackMin;
extern const double kEnvDecayMin;
extern const double kEnvSustainDefault;
extern const double kEnvReleaseDefault;
#pragma endregion

#pragma region ASDR
ADSR::ADSR()
	: UGen()
	, audio(*this, AUDIO)
	, mState(kOff)
	, mAutoRelease(false)
	, mAmp(0)
	, mAttack(0)
	, mDecay(0)
	, mSustain(0)
	, mRelease(0)
	, mStep(0)
	, mTime(0)
{

}

void ADSR::sampleRateChanged()
{
	mStep = 1.0f / sampleRate();
}

void ADSR::noteOn(float amp, float attack, float decay, float sustain, float release)
{
	mTime = 0;
	mAmp = amp;
	mAttack = attack;
	mDecay = decay;
	mSustain = sustain;
	mRelease = release;
	mAutoRelease = false;

	if (mAttack > 0) mState = kAttack;
	else if (mDecay > 0) mState = kDecay;
	else mState = kSustain;
}

void ADSR::noteOff()
{
	if (mState == kSustain)
	{
		mState = kRelease;
		mTime = 0;
	}
	else
	{
		mAutoRelease = true;
	}
}

void ADSR::stop()
{
	mState = kOff;
	mAmp = 0;
}

void ADSR::uGenerate(float * channels, const int numChannels)
{
	// when we are off, our amplitude should be zero, so we start with that and don't bother handling that case
	float amp = 0;
	switch (mState)
	{
	case kAttack:
		if (mTime >= mAttack)
		{
			amp = mAmp;
			mState = kDecay;
			mTime = 0;
		}
		else
		{
			amp = mAmp * (mTime / mAttack);
		}
		break;

	case kDecay:
		if (mTime >= mDecay)
		{
			amp = mAmp * mSustain;
			mState = kSustain;
		}
		else
		{
			const float t = mTime / mDecay;
			amp = mAmp + t * (mAmp*mSustain - mAmp);
		}
		break;

	case kSustain:
		amp = mAmp * mSustain;
		if (mAutoRelease)
		{
			mState = kRelease;
			mTime = 0;
		}
		break;

	case kRelease:
		if (mTime >= mRelease)
		{
			amp = 0;
			mState = kOff;
		}
		else
		{
			const float t = mTime / mRelease;
			const float rAmp = mAmp*mSustain;
			amp = rAmp - t*rAmp;
		}
		break;
	}

	mTime += mStep;

	for (int i = 0; i < numChannels; ++i)
	{
		channels[i] = audio.getLastValues()[i] * amp;
	}
}
#pragma endregion

#pragma region WaveShaperDSP
WaveShaperDSP::WaveShaperDSP(int channelCount)
  : mVolume(1.)
  , mAttack(kEnvAttackMin)
  , mDecay(kEnvDecayMin)
  , mSustain(kEnvSustainDefault / 100.0)
  , mRelease(kEnvReleaseDefault)
  , mNoiseTint(Minim::Noise::eTintPink)
  , mMod(kDefaultMod)
  , mRate(kDefaultRate)
  , mRange(kDefaultRange)
  , mShape(kDefaultShape)
  , mShaperSize(0)
  , mShaperMapValue(0)
  , mMainSignalVol(0)
  , vNoize(vessl::noiseTint::pink)
  , vNoizeAmp(1)
  , vNoizeMod(vessl::waves<sample>::sine, kDefaultMod)
  , vRateCtrl(0, 0, 0.01)
  , vModCtrl(kDefaultMod, kDefaultMod)
  , vRangeCtrl(kDefaultRange, kDefaultRange)
  , vShapeCtrl(kDefaultShape, kDefaultShape)
  , vNoizeShaperLeft(mBufferLeft)
  , vNoizeShaperRight(mBufferRight)
  , vNoizeShaperMixer(1)
{
  mNoizeRate = new Minim::TickRate(mRate);
  mNoizeRate->setInterpolation(true);

  mRateCtrl.activate(0, 0, 0);
  mRateCtrl.patch(mNoizeRate->value);

  mNoize = new Minim::Noise(1.0f, mNoiseTint);

  //		const float startRange = 0.05f;
  //		mRangeCtrl.activate( 0.f, startRange, startRange );
  //		mRangeCtrl.patch( mNoize->offset );

  mNoizeAmp = new Minim::Multiplier(1.f);

  mNoizeShaperLeft = new Minim::WaveShaper(1.0f, 1.0f, new Minim::Wavetable(512), true);
  mNoizeShaperRight = new Minim::WaveShaper(1.0f, 1.0f, new Minim::Wavetable(512), true);

  mShapeCtrl.activate(0.f, mShape, mShape);
  //		mShapeCtrl.patch( mNoizeShaperLeft->mapAmplitude );
  //		mShapeCtrl.patch( mNoizeShaperRight->mapAmplitude );

  mPanLeft = new Minim::Pan(-1.f);
  mPanRight = new Minim::Pan(1.f);

  mNoizeMod = new Minim::Oscil(mMod, 1.f, Minim::Waves::SINE());
  // mNoizeMod->offset.setLastValue( 1.f );
  // set phase at 0.25 so that when we "pause" the modulation, it will output 1.0
  mNoizeMod->phase.setLastValue(0.25f);

  mModCtrl.activate(0.f, mMod, mMod);
  mModCtrl.patch(mNoizeMod->frequency);

  mShapeCtrl.patch(mNoizeMod->amplitude);

  mNoizeMod->patch(mNoizeAmp->amplitude);

  mNoizeSum = new Minim::Summer();

  mNoizeOffset = new Minim::Constant(0.f);
  mRangeCtrl.activate(0.f, mRange, mRange);
  mRangeCtrl.patch(mNoizeOffset->value);
  mNoizeOffset->patch(*mNoizeSum);

  // noise generator
  mNoize->patch(*mNoizeRate).patch(*mNoizeAmp).patch(*mNoizeSum);

  // left shaper
  mNoizeSum->patch(*mNoizeShaperLeft).patch(*mPanLeft);
  // right right
  mNoizeSum->patch(*mNoizeShaperRight).patch(*mPanRight);

  mMainSignal = new Minim::Summer();

  mPanLeft->patch(*mMainSignal);
  mPanRight->patch(*mMainSignal);

  mMainSignal->patch(mEnvelope).patch(mMainSignalVol);
  mMainSignalVol.setAudioChannelCount(channelCount);
 
  vMainSignal.patch(vRateCtrl.value, vNoize.rate);
  vMainSignal.patch(vModCtrl.value, vNoizeMod.fhz);
  vMainSignal.patch(vNoizeMod.out, vNoizeModAmp.in);
  vMainSignal.patch(vShapeCtrl.value, vNoizeModAmp.factor);
  vMainSignal.patch(vNoizeModAmp.out, vNoizeAmp.factor);

  // offset value is summed with the noise to control where in the wavetable we are scrubbing
  vMainSignal.patch(vNoize.out, vNoizeAmp.in).patch(vNoizeAmp.out, vNoizeSum.in[0]);
  vMainSignal.patch(vRangeCtrl.value, vNoizeSum.in[1]);

  vMainSignal.patch(vNoizeSum.out, vNoizeShaperLeft.in).patch(vNoizeShaperLeft.out, vNoizeShaperMixer.in[0]);
  vMainSignal.patch(vNoizeSum.out, vNoizeShaperRight.in).patch(vNoizeShaperRight.out, vNoizeShaperMixer.in[1]);
}

WaveShaperDSP::~WaveShaperDSP()
{
  delete mNoize;
  delete mNoizeRate;
  delete mNoizeAmp;
  delete mNoizeSum;
  delete mNoizeOffset;
  delete mNoizeShaperLeft;
  delete mNoizeShaperRight;
  delete mPanLeft;
  delete mPanRight;
  delete mNoizeMod;
  delete mMainSignal;
}

void WaveShaperDSP::ProcessBlock(sample** inputs, sample** outputs, int nOutputs, int nFrames)
{
  sample* out1 = outputs[0];
  sample* out2 = outputs[1];

  float result[2];
  for (int s = 0; s < nFrames; ++s, ++out1, ++out2)
  {
    while (!mMidiQueue.Empty())
    {
      IMidiMsg& pMsg = mMidiQueue.Peek();
      if (pMsg.mOffset > s) break;

      switch (pMsg.StatusMsg())
      {
        case IMidiMsg::kNoteOn:
          // make sure this is a real NoteOn
          if (pMsg.Velocity() > 0)
          {
            mMidiNotes.push_back(pMsg);
            if (!mEnvelope.isOn())
            {
              mEnvelope.noteOn(pMsg.Velocity() / 127.0f, mAttack, mDecay, mSustain, mRelease);
              TriggerRateChange(mRate, 0.01);
            }
            break;
          }
          // fallthru in the case that a NoteOn is supposed to be treated like a NoteOff

        case IMidiMsg::kNoteOff:
          for (auto iter = mMidiNotes.crbegin(); iter != mMidiNotes.crend(); ++iter)
          {
            // remove the most recent note on with the same pitch
            if (pMsg.NoteNumber() == iter->NoteNumber())
            {
              mMidiNotes.erase((iter + 1).base());
              break;
            }
          }

          if (mMidiNotes.empty())
          {
            mEnvelope.noteOff();
            TriggerRateChange(0, mEnvelope.getRelease());
          }
          break;
      }

      mMidiQueue.Remove();
    }

    mNoize->setTint(mNoiseTint);
    mMainSignalVol.amplitude.setLastValue(mVolume);
    mMainSignalVol.tick(result, 2);

    *out1 = result[0];
    *out2 = result[0];

    vNoize.tint = (vessl::noiseTint::type)mNoiseTint;
    vNoizeShaperMixer.master = mVolume;
    auto& out = vMainSignal.tick(mSignalDT);
    result[0] = out[0];
    result[1] = out[1];
  }

  mShaperMapValue = mNoizeShaperLeft->getLastMapValue();
}

void WaveShaperDSP::SetWavetables(Minim::MultiChannelBuffer& buffer)
{
  const float* left = buffer.getChannel(0);
  const float* right = buffer.getChannelCount() > 1 ? buffer.getChannel(1) : left;
  const int size = buffer.getBufferSize();

  mNoizeShaperLeft->getWavetable().setWaveform(left, size);
  mNoizeShaperRight->getWavetable().setWaveform(right, size);

  for (int i = 0; i < size; ++i)
  {
    mBufferLeft.set(i, left[i]);
    mBufferRight.set(i, right[i]);
  }

  mShaperSize = size;
}

#pragma endregion
