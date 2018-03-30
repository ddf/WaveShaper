#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Params.h"
#include "Interface.h"
#include "IMidiQueue.h"

#include "FileLoader.h"
#include "MultiChannelBuffer.h"
#include "Line.h"

namespace Minim
{
	class TickRate;
	class Multiplier;
	class Oscil;
	class Pan;
	class Summer;
	class WaveShaper;
	class Noise;
	class Constant;
}

/*

 WaveShaper - an IPlug project template that includes controls and features typically used in Damien Quartz's IPlugs.

*/

class WaveShaper : public IPlug
{
public:

	WaveShaper(IPlugInstanceInfo instanceInfo);
	~WaveShaper();

	void Reset() override;
	void OnParamChange(int paramIdx) override;

	void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames) override;

	bool SerializeState(ByteChunk* pChunk) override;
	int UnserializeState(ByteChunk* pChunk, int startPos) override;
	bool CompareState(const unsigned char* incomingState, int startPos) override;

	void PresetsChangedByHost() override;
	bool HostRequestingAboutBox() override;

	void ProcessMidiMsg(IMidiMsg* pMsg) override;

	// our additions
	void BeginMIDILearn(int param1, int param2, int x, int y);

private:

	void HandleMidiControlChange(IMidiMsg* pMsg);
	void SetControlChangeForParam(const IMidiMsg::EControlChangeMsg cc, const int paramIdx);
	// helper to send Midi Messages when a midi-mapped parameter changes
	void BroadcastParamChange(const int paramIdx);

	Interface mInterface;

	// params
	double mVolume;

	// midi
	IMidiQueue mMidiQueue;
	IMidiMsg::EControlChangeMsg mControlChangeForParam[kNumParams];
	int mMidiLearnParamIdx;

	// audio
	FileLoader mFileLoader;
	Minim::MultiChannelBuffer mBuffer; // contains the currently loaded audio file
	
	Minim::Noise	   * mNoize;
	Minim::TickRate	   * mNoizeRate;
	Minim::Multiplier  * mNoizeAmp;
	Minim::Oscil	   * mNoizeMod;
	Minim::Summer	   * mNoizeSum;
	Minim::Constant	   * mNoizeOffset;
	Minim::WaveShaper  * mNoizeShaperLeft;
	Minim::WaveShaper  * mNoizeShaperRight;
	Minim::Pan		   * mPanLeft;
	Minim::Pan		   * mPanRight;
	Minim::Summer	   * mMainSignal;
	Minim::Multiplier  * mMainSignalVol;

	// controls
	Minim::Line			 mRateCtrl;
	Minim::Line		     mModCtrl;
	Minim::Line		     mRangeCtrl;
	Minim::Line          mShapeCtrl;
};
