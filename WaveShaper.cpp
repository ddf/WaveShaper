#include "WaveShaper.h"
#include "IPlug_include_in_plug_src.h"
#include "resource.h"

#include "Params.h"
#include "Interp.h"

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
#include "Constant.h"

// this is hacky, but we can't compile the UGen source file as its own compilation unit becuase the file name is the same,
// so we simply directly include it here.
#include "ugens\Waveshaper.h"
#include "ugens\Waveshaper.cpp"

#if SA_API
extern char * gINIPath;

static const char * kAboutBoxText = "Version " VST3_VER_STR "\nCreated by Damien Quartz\nBuilt on " __DATE__;
#endif

// The number of presets/programs
const int kNumPrograms = 1;

// name of the section of the INI file we save midi cc mappings to
const char * kMidiControlIni = "midicc";
const IMidiMsg::EControlChangeMsg kUnmappedParam = (IMidiMsg::EControlChangeMsg)128;

// move these to sliders at some point.
const double kDefaultMod = 0.5;
const double kMinMod = 0.;
const double kMaxMod = 120.;

const double kDefaultRate = 0.0005;
const double kMinRate = 0.00001;
const double kMaxRate = 0.001;

// range is the noise offset, which basically determines where in the file the center point of scrubbing is.
const double kDefaultRange = 0;
const double kMinRange = -1.; // 0.01;
const double kMaxRange = 1.0; // 0.5f;

// shape is the mapAmplitude, which basically determines how many samples from the source are scrubbed over
const double kDefaultShape = 0.1;
const double kMinShape = 0.05;
const double kMaxShape = 0.35;

WaveShaper::WaveShaper(IPlugInstanceInfo instanceInfo)
	: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
	, mInterface(this)
	, mVolume(1.)
	, mNoiseTint(Minim::Noise::eTintPink)
	, mMidiLearnParamIdx(-1)
{
	TRACE;

	// set all control change mappings to unmapped.
	for (int i = 0; i < kNumParams; ++i)
	{
		mControlChangeForParam[i] = kUnmappedParam;
	}

	for (int i = 0; i < kNoiseSnapshotCount; ++i)
	{
		mNoiseSnapshots[i].AmpMod = kDefaultMod;
		mNoiseSnapshots[i].Range = kDefaultRange;
		mNoiseSnapshots[i].Rate = kDefaultRate;
		mNoiseSnapshots[i].Shape = kDefaultShape;
	}

	// Define parameter ranges, display units, labels.
	//arguments are: name, defaultVal, minVal, maxVal, step, label
	GetParam(kVolume)->InitDouble("Volume", kVolumeDefault, kVolumeMin, kVolumeMax, 0.1, "dB");
	GetParam(kVolume)->SetDisplayText((double)kVolumeMin, "-inf");

	GetParam(kNoiseType)->InitEnum("Noise Type", NT_Pink, NT_Count);
	GetParam(kNoiseType)->SetDisplayText(0, "White");
	GetParam(kNoiseType)->SetDisplayText(1, "Pink");
	GetParam(kNoiseType)->SetDisplayText(2, "Red");

	GetParam(kNoiseAmpMod)->InitDouble("Noise Amp Mod", kDefaultMod, kMinMod, kMaxMod, 0.1, "Hz", "", 2.0);
	GetParam(kNoiseRate)->InitDouble("Noise Rate", kDefaultRate, kMinRate, kMaxRate, kMinRate);
	GetParam(kNoiseRange)->InitDouble("Noise Range", kDefaultRange, kMinRange, kMaxRange, 0.01);
	GetParam(kNoiseShape)->InitDouble("Noise Shape", kDefaultShape, kMinShape, kMaxShape, 0.01);

	GetParam(kNoiseSnapshot)->InitDouble("Noise Snapshot", kNoiseSnapshotDefault, kNoiseSnapshotMin, kNoiseSnapshotMax, 0.01);
	// when this parameter changes, Noise Amp Mod, Noise Rate, Noise Range, and Noise Shape are all changed
	// by lerping between the values in two Noise Snapshots.
	GetParam(kNoiseSnapshot)->SetIsMeta(true);

	IGraphics* pGraphics = MakeGraphics(this, GUI_WIDTH, GUI_HEIGHT);
	mInterface.CreateControls(pGraphics);
	AttachGraphics(pGraphics);

	mFileLoader.Load(SND_01_ID, SND_01_FN, mBuffer);
	mInterface.RebuildPeaks(mBuffer);

	mNoizeRate = new Minim::TickRate(kDefaultRate);
	mNoizeRate->setInterpolation(true);

	mRateCtrl.activate(0, 0, 0);
	mRateCtrl.patch(mNoizeRate->value);

	mNoize = new Minim::Noise(1.0f, mNoiseTint);

	//		const float startRange = 0.05f;
	//		mRangeCtrl.activate( 0.f, startRange, startRange );
	//		mRangeCtrl.patch( mNoize->offset );

	mNoizeAmp = new Minim::Multiplier(1.f);

	const int   channelCount = mBuffer.getChannelCount();
	mNoizeShaperLeft = new Minim::WaveShaper(1.0f, 1.0f, new Minim::Wavetable(mBuffer.getChannel(0), mBuffer.getBufferSize()), true);
	mNoizeShaperRight = new Minim::WaveShaper(1.0f, 1.0f, new Minim::Wavetable(channelCount == 2 ? mBuffer.getChannel(1) : mBuffer.getChannel(0), mBuffer.getBufferSize()), true);

	mShapeCtrl.activate(0.f, kDefaultShape, kDefaultShape);
	//		mShapeCtrl.patch( mNoizeShaperLeft->mapAmplitude );
	//		mShapeCtrl.patch( mNoizeShaperRight->mapAmplitude );

	mPanLeft = new Minim::Pan(-1.f);
	mPanRight = new Minim::Pan(1.f);

	mNoizeMod = new Minim::Oscil(kDefaultMod, 1.f, Minim::Waves::SINE());
	// mNoizeMod->offset.setLastValue( 1.f );
	// set phase at 0.25 so that when we "pause" the modulation, it will output 1.0
	mNoizeMod->phase.setLastValue(0.25f);

	mModCtrl.activate(0.f, kDefaultMod, kDefaultMod);
	mModCtrl.patch(mNoizeMod->frequency);

	mShapeCtrl.patch(mNoizeMod->amplitude);

	mNoizeMod->patch(mNoizeAmp->amplitude);

	mNoizeSum = new Minim::Summer();

	mNoizeOffset = new Minim::Constant(0.f);
	const float startRange = 0.f;
	mRangeCtrl.activate(0.f, startRange, startRange);
	mRangeCtrl.patch(mNoizeOffset->value);
	mNoizeOffset->patch(*mNoizeSum);

	// noise generator
	mNoize->patch(*mNoizeRate).patch(*mNoizeAmp).patch(*mNoizeSum);

	// left shaper
	mNoizeSum->patch(*mNoizeShaperLeft).patch(*mPanLeft);
	// right right
	mNoizeSum->patch(*mNoizeShaperRight).patch(*mPanRight);

	mMainSignal = new Minim::Summer();
	mMainSignalVol = new Minim::Multiplier(0.f);

	mPanLeft->patch(*mMainSignal);
	mPanRight->patch(*mMainSignal);

	mMainSignal->patch(mEnvelope).patch(*mMainSignalVol);
	mMainSignalVol->setAudioChannelCount(2);
}

WaveShaper::~WaveShaper() 
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
	delete mMainSignalVol;
}

void WaveShaper::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
	// Mutex is already locked for us.

	double* in1 = inputs[0];
	double* in2 = inputs[1];
	double* out1 = outputs[0];
	double* out2 = outputs[1];

	float result[2];
	for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
	{
		while (!mMidiQueue.Empty())
		{
			IMidiMsg* pMsg = mMidiQueue.Peek();
			if (pMsg->mOffset > s) break;

			switch (pMsg->StatusMsg())
			{
			case IMidiMsg::kControlChange:
				HandleMidiControlChange(pMsg);
				break;

			case IMidiMsg::kNoteOn:
				// make sure this is a real NoteOn
				if (pMsg->Velocity() > 0)
				{
					mMidiNotes.push_back(*pMsg);
					if (!mEnvelope.isOn())
					{
						mEnvelope.noteOn(pMsg->Velocity() / 127.0f, 0.1f, 0.1f, 1, 0.1f);
						OnParamChange(kNoiseRate);
					}
					break;
				}
				// fallthru in the case that a NoteOn is supposed to be treated like a NoteOff

			case IMidiMsg::kNoteOff:
				for (auto iter = mMidiNotes.crbegin(); iter != mMidiNotes.crend(); ++iter)
				{
					// remove the most recent note on with the same pitch
					if (pMsg->NoteNumber() == iter->NoteNumber())
					{
						mMidiNotes.erase((iter + 1).base());
						break;
					}
				}

				if (mMidiNotes.empty())
				{
					mEnvelope.noteOff();
					mRateCtrl.activate(mEnvelope.getRelease(), mRateCtrl.getAmp(), 0);
				}
				break;
			}

			mMidiQueue.Remove();
		}

		mNoize->setTint(mNoiseTint);
		mMainSignalVol->amplitude.setLastValue(mVolume);
		mMainSignalVol->tick(result, 2);

		*out1 = result[0];
		*out2 = result[1];
	}
}

void WaveShaper::Reset()
{
	TRACE;
	IMutexLock lock(this);

	mMidiQueue.Clear();
	mMidiQueue.Resize(GetBlockSize());

	mMainSignalVol->setSampleRate(GetSampleRate());

	// read control mappings from the INI if we are running standalone
#if SA_API
	for (int i = 0; i < kNumParams; ++i)
	{
		int defaultMapping = (int)mControlChangeForParam[i];
		mControlChangeForParam[i] = (IMidiMsg::EControlChangeMsg)GetPrivateProfileInt(kMidiControlIni, GetParam(i)->GetNameForHost(), defaultMapping, gINIPath);

		BroadcastParamChange(i);
	}
#endif
}

void WaveShaper::OnParamChange(int paramIdx)
{
	TRACE;

	IMutexLock lock(this);

	IParam* param = GetParam(paramIdx);
	switch (paramIdx)
	{
	case kVolume:
		mVolume = param->Value() == kVolumeMin ? 0 : param->DBToAmp();
		break;

	case kNoiseType:
		switch (param->Int())
		{
		case NT_White: mNoiseTint = Minim::Noise::eTintWhite; break;
		case NT_Pink:  mNoiseTint = Minim::Noise::eTintPink;  break;
		case NT_Brown: mNoiseTint = Minim::Noise::eTintBrown; break;
		}
		break;

	case kNoiseAmpMod:
		mModCtrl.activate(0.01f, mModCtrl.getAmp(), param->Value());
		break;

	case kNoiseRate:
		if (!mMidiNotes.empty())
		{
			mRateCtrl.activate(0.01f, mRateCtrl.getAmp(), param->Value());
		}
		break;

	case kNoiseRange:
		mRangeCtrl.activate(0.1f, mRangeCtrl.getAmp(), param->Value());
		break;

	case kNoiseShape:
		mShapeCtrl.activate(0.1f, mShapeCtrl.getAmp(), param->Value());
		break;

	case kNoiseSnapshot:
	{
		int first = (int)param->Value();		
		float blend = param->Value() - first;
		const NoiseSnapshot& firstSnap = GetNoiseSnapshot(first);
		const NoiseSnapshot& secondSnap = first < kNoiseSnapshotMax ? GetNoiseSnapshot(first + 1) : GetNoiseSnapshot(first);
		SetParamBlend(kNoiseAmpMod, firstSnap.AmpMod, secondSnap.AmpMod, blend);
		SetParamBlend(kNoiseRange, firstSnap.Range, secondSnap.Range, blend);
		SetParamBlend(kNoiseRate, firstSnap.Rate, secondSnap.Rate, blend);
		SetParamBlend(kNoiseShape, firstSnap.Shape, secondSnap.Shape, blend);
	}
	break;

	default:
		break;
	}

	BroadcastParamChange(paramIdx);
}

void WaveShaper::SetParamBlend(int paramIdx, double begin, double end, double blend)
{
	BeginInformHostOfParamChange(paramIdx);
	double value = Lerp(begin, end, blend);
	GetParam(paramIdx)->Set(value);
	value = GetParam(paramIdx)->GetNormalized(value);
	InformHostOfParamChange(paramIdx, value);
	EndInformHostOfParamChange(paramIdx);
	
	GetGUI()->SetParameterFromPlug(paramIdx, value, true);

	// kick of the actual changes and get the change broadcasted
	OnParamChange(paramIdx);
}

static const int kStateVersion = 1;

// this over-ridden method is called when the host is trying to store the plug-in state and needs to get the current data from your algorithm
bool WaveShaper::SerializeState(ByteChunk* pChunk)
{
	TRACE;
	IMutexLock lock(this);

	pChunk->Put(&kStateVersion);

	int snapshotCount = kNoiseSnapshotCount;
	pChunk->Put(&snapshotCount);
	for (int i = 0; i < snapshotCount; ++i)
	{
		pChunk->Put(&mNoiseSnapshots[i].AmpMod);
		pChunk->Put(&mNoiseSnapshots[i].Range);
		pChunk->Put(&mNoiseSnapshots[i].Rate);
		pChunk->Put(&mNoiseSnapshots[i].Shape);
	}

	return IPlugBase::SerializeParams(pChunk); // must remember to call SerializeParams at the end
}

// this over-ridden method is called when the host is trying to load the plug-in state and you need to unpack the data into your algorithm
int WaveShaper::UnserializeState(ByteChunk* pChunk, int startPos)
{
	TRACE;
	IMutexLock lock(this);

	int version = 0;
	startPos = pChunk->Get(&version, startPos);

	int snapshotCount;
	startPos = pChunk->Get(&snapshotCount, startPos);
	for (int i = 0; i < snapshotCount; ++i)
	{
		startPos = pChunk->Get(&mNoiseSnapshots[i].AmpMod, startPos);
		startPos = pChunk->Get(&mNoiseSnapshots[i].Range, startPos);
		startPos = pChunk->Get(&mNoiseSnapshots[i].Rate, startPos);
		startPos = pChunk->Get(&mNoiseSnapshots[i].Shape, startPos);
	}

	return IPlugBase::UnserializeParams(pChunk, startPos); // must remember to call UnserializeParams at the end
}

bool WaveShaper::CompareState(const unsigned char* incomingState, int startPos)
{
	bool isEqual = true;
	isEqual &= IPlugBase::CompareState(incomingState, startPos); // fuzzy compare regular params

	return isEqual;
}

void WaveShaper::PresetsChangedByHost()
{
	TRACE;
	IMutexLock lock(this);

	mInterface.OnPresetChanged();
}

void WaveShaper::BeginMIDILearn(int paramIdx1, int paramIdx2, int x, int y)
{
	if (GetAPI() == kAPIVST3)
	{
		// in Reaper on OSX the popup's Y position is inverted.
		// not sure if this is true in other hosts on OSX, so we only modify for Reaper.
#ifdef OS_OSX
		if (GetHost() == kHostReaper)
		{
			y = GUI_HEIGHT - y;
	}
#endif
		PopupHostContextMenuForParam(paramIdx1, x, y);
}
	else if (GetAPI() == kAPISA)
	{
		IPopupMenu menu;
		menu.SetMultiCheck(true);
		WDL_String str;
		if (paramIdx1 != -1)
		{
			bool isMapped = mControlChangeForParam[paramIdx1] != kUnmappedParam;
			int flags = isMapped ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;
			if (isMapped)
			{
				str.SetFormatted(64, "MIDI Learn: %s (CC %d)", GetParam(paramIdx1)->GetNameForHost(), (int)mControlChangeForParam[paramIdx1]);
			}
			else
			{
				str.SetFormatted(64, "MIDI Learn: %s", GetParam(paramIdx1)->GetNameForHost());
			}
			menu.AddItem(str.Get(), -1, flags);
		}
		if (paramIdx2 != -1)
		{
			bool isMapped = mControlChangeForParam[paramIdx2] != kUnmappedParam;
			int flags = isMapped ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;
			if (isMapped)
			{
				str.SetFormatted(64, "MIDI Learn: %s (CC %d)", GetParam(paramIdx2)->GetNameForHost(), (int)mControlChangeForParam[paramIdx2]);
			}
			else
			{
				str.SetFormatted(64, "MIDI Learn: %s", GetParam(paramIdx2)->GetNameForHost());
			}
			menu.AddItem(str.Get(), -1, flags);
		}
		if (GetGUI()->CreateIPopupMenu(&menu, x, y))
		{
			const int chosen = menu.GetChosenItemIdx();
			if (chosen == 0)
			{
				if (menu.GetItem(chosen)->GetChecked())
				{
					SetControlChangeForParam(kUnmappedParam, paramIdx1);
				}
				else
				{
					mMidiLearnParamIdx = paramIdx1;
				}
			}
			else if (chosen == 1)
			{
				if (menu.GetItem(chosen)->GetChecked())
				{
					SetControlChangeForParam(kUnmappedParam, paramIdx2);
				}
				else
				{
					mMidiLearnParamIdx = paramIdx2;
				}
			}
			else
			{
				mMidiLearnParamIdx = -1;
			}
		}
	}
}

void WaveShaper::ProcessMidiMsg(IMidiMsg* pMsg)
{
#ifdef TRACER_BUILD
	pMsg->LogMsg();
#endif

	const IMidiMsg::EStatusMsg status = pMsg->StatusMsg();
	if (status == IMidiMsg::kControlChange || status == IMidiMsg::kNoteOn || status == IMidiMsg::kNoteOff)
	{
		if (status == IMidiMsg::kControlChange)
		{
			const IMidiMsg::EControlChangeMsg cc = pMsg->ControlChangeIdx();
			if (mMidiLearnParamIdx != -1)
			{
				SetControlChangeForParam(cc, mMidiLearnParamIdx);
				mMidiLearnParamIdx = -1;
			}
		}

		mMidiQueue.Add(pMsg);
	}
}

void WaveShaper::HandleMidiControlChange(IMidiMsg* pMsg)
{
	const IMidiMsg::EControlChangeMsg cc = pMsg->ControlChangeIdx();
	for (int i = 0; i < kNumParams; ++i)
	{
		if (mControlChangeForParam[i] == cc)
		{
			const double value = pMsg->ControlChange(cc);
			GetParam(i)->SetNormalized(value);
			OnParamChange(i);
			GetGUI()->SetParameterFromPlug(i, GetParam(i)->GetNormalized(), true);
		}
	}
}

void WaveShaper::SetControlChangeForParam(const IMidiMsg::EControlChangeMsg cc, const int paramIdx)
{
	mControlChangeForParam[paramIdx] = cc;
#if SA_API
	if (cc == kUnmappedParam)
	{
		WritePrivateProfileString(kMidiControlIni, GetParam(paramIdx)->GetNameForHost(), 0, gINIPath);
	}
	else
	{
		char ccString[100];
		sprintf(ccString, "%u", (unsigned)cc);
		WritePrivateProfileString(kMidiControlIni, GetParam(paramIdx)->GetNameForHost(), ccString, gINIPath);
	}
#endif
}

void WaveShaper::BroadcastParamChange(const int paramIdx)
{
	// send MIDI CC messages with current param values for any mapped params,
	// which should enable some control surfaces to keep indicators in sync with the UI.
	if (mControlChangeForParam[paramIdx] != kUnmappedParam)
	{
		IMidiMsg msg;
		msg.MakeControlChangeMsg(mControlChangeForParam[paramIdx], GetParam(paramIdx)->GetNormalized(), 0);
		SendMidiMsg(&msg);
	}
}

bool WaveShaper::HostRequestingAboutBox()
{
#if SA_API
#ifdef OS_WIN
	GetGUI()->ShowMessageBox(kAboutBoxText, BUNDLE_NAME, MB_OK);
#else
	// sadly, on osx, ShowMessageBox uses an alert style box that does not show the app icon,
	// which is different from the default About window that is shown.
	// *that* code uses swell's MessageBox, so we use that directly on mac.
	MessageBox(0, kAboutBoxText, BUNDLE_NAME, MB_OK);
#endif
	return true;
#endif
	return false;
}

float WaveShaper::GetNoiseOffset() const
{
	return mNoizeOffset->value.getLastValue();
}

float WaveShaper::GetNoiseRate() const
{
	return mNoizeRate->getLastValues()[0];
}

float WaveShaper::GetShape() const 
{
	return mShapeCtrl.getLastValues()[0];
}


float WaveShaper::GetShaperSize() const
{
	return mNoizeShaperLeft->getWavetable().size();
}

float WaveShaper::GetShaperMapValue() const 
{
	return mNoizeShaperLeft->getLastMapValue();
}

void WaveShaper::UpdateNoiseSnapshot(int idx)
{
	mNoiseSnapshots[idx].AmpMod = GetParam(kNoiseAmpMod)->Value();
	mNoiseSnapshots[idx].Range = GetParam(kNoiseRange)->Value();
	mNoiseSnapshots[idx].Rate = GetParam(kNoiseRate)->Value();
	mNoiseSnapshots[idx].Shape = GetParam(kNoiseShape)->Value();
}

WaveShaper::NoiseSnapshot WaveShaper::GetNoiseSnapshotNormalized(int idx)
{
	const NoiseSnapshot& snapshot = GetNoiseSnapshot(idx);
	
	NoiseSnapshot normalized;
	normalized.AmpMod = GetParam(kNoiseAmpMod)->GetNormalized(snapshot.AmpMod);
	normalized.Range = GetParam(kNoiseRange)->GetNormalized(snapshot.Range);
	normalized.Rate = GetParam(kNoiseRate)->GetNormalized(snapshot.Rate);
	normalized.Shape = GetParam(kNoiseShape)->GetNormalized(snapshot.Shape);

	return normalized;
}

void WaveShaper::HandleSave(WDL_String* fileName, WDL_String* directory)
{
	if (strcmp(fileName->get_fileext(), "fxp") == 0)
	{
		const char * programName = fileName->get_filepart();
		// change the current preset without actually loading it.
		mCurrentPresetIdx = 0;
		// modify this preset so that the preset name saved to the file is correct.
		ModifyCurrentPreset(programName);
		SaveProgramAsFXP(fileName);
		// notify the host because we changed the settings of the first preset
		InformHostOfProgramChange();

		mInterface.OnPresetChanged();
	}
}

void WaveShaper::HandleLoad(WDL_String* fileName, WDL_String* directory)
{
	if (strcmp(fileName->get_fileext(), "fxp") == 0)
	{
		// change preset index to first preset so that the program will load there
		// instead of overwriting a real preset.
		mCurrentPresetIdx = 0;
		LoadProgramFromFXP(fileName);

		mInterface.OnPresetChanged();
	}
	else
	{
		// we can load without locking cause mBuffer is not used by the DSP chain,
		// so it's better to hang the UI thread than the audio thread.
		mFileLoader.Load(fileName->Get(), mBuffer);
		mInterface.RebuildPeaks(mBuffer);

		// now we lock because we need to update the DSP
		IMutexLock lock(this);
		mNoizeShaperLeft->getWavetable().setWaveform(mBuffer.getChannel(0), mBuffer.getBufferSize());
		if (mBuffer.getChannelCount() == 1)
		{
			mNoizeShaperRight->getWavetable().setWaveform(mBuffer.getChannel(0), mBuffer.getBufferSize());
		}
		else
		{
			mNoizeShaperRight->getWavetable().setWaveform(mBuffer.getChannel(1), mBuffer.getBufferSize());
		}
	}
}


void WaveShaper::HandleAction(BangControl::Action action)
{
	const int snapshotIdx = (int)action - (int)BangControl::ActionCustom;
	mInterface.UpdateSnapshot(snapshotIdx);
}

// modified version of DumpPresetSrcCode 
void WaveShaper::DumpPresetSrc()
{
	IMutexLock lock(this);

	WDL_String path;
	GetGUI()->DesktopPath(&path);
	path.Append("\\dump.txt");
	FILE* fp = fopen(path.Get(), "w");
	// all the param indices we *should* include.
	// we collect these first because we need the count for the second argument
	// of MakePresetFromNamedParams
	std::vector<int> paramsForDump;
	for (int i = 0; i < kNumParams; ++i)
	{
		IParam* param = GetParam(i);
		if (param->Value() != param->GetDefault())
		{
			paramsForDump.push_back(i);
		}
	}
	const int paramCount = paramsForDump.size();
	WDL_String name;
	name.Set(GetPresetName(0));
	name.remove_fileext();
	fprintf(fp, "\tMakePresetFromNamedParams(\"%s\", %d", name.Get(), paramCount);
	for (int i = 0; i < paramCount; ++i)
	{
		const int paramIdx = paramsForDump[i];
		IParam* pParam = GetParam(paramIdx);
		char paramVal[32];
		switch (pParam->Type())
		{
		case IParam::kTypeBool:
			sprintf(paramVal, "%s", (pParam->Bool() ? "true" : "false"));
			break;
		case IParam::kTypeInt:
			sprintf(paramVal, "%d", pParam->Int());
			break;
		case IParam::kTypeEnum:
			sprintf(paramVal, "%d", pParam->Int());
			break;
		case IParam::kTypeDouble:
		default:
			sprintf(paramVal, "%.6f", pParam->Value());
			break;
		}
		fprintf(fp, "\n\t\t, %d, %s // %s", paramIdx, paramVal, pParam->GetNameForHost());
	}
	fprintf(fp, "\n\t);\n");
	fclose(fp);
}