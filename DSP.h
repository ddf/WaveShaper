#pragma once

#include "UGen.h"

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