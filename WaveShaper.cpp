#include "WaveShaper.h"
#include "IPlug_include_in_plug_src.h"
#include "resource.h"

#include "Params.h"

#if SA_API
extern char * gINIPath;

static const char * kAboutBoxText = "Version " VST3_VER_STR "\nCreated by Damien Quartz\nBuilt on " __DATE__;
#endif

// The number of presets/programs
const int kNumPrograms = 8;

// name of the section of the INI file we save midi cc mappings to
const char * kMidiControlIni = "midicc";
const IMidiMsg::EControlChangeMsg kUnmappedParam = (IMidiMsg::EControlChangeMsg)128;

WaveShaper::WaveShaper(IPlugInstanceInfo instanceInfo)
	: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
	, mInterface(this)
	, mVolume(1.)
	, mMidiLearnParamIdx(-1)
	, mReadFrame(0)
{
	TRACE;

	// set all control change mappings to unmapped.
	for (int i = 0; i < kNumParams; ++i)
	{
		mControlChangeForParam[i] = kUnmappedParam;
	}

	// Define parameter ranges, display units, labels.
	//arguments are: name, defaultVal, minVal, maxVal, step, label
	GetParam(kVolume)->InitDouble("Volume", kVolumeDefault, kVolumeMin, kVolumeMax, 0.1, "dB");
	GetParam(kVolume)->SetDisplayText((double)kVolumeMin, "-inf");

	IGraphics* pGraphics = MakeGraphics(this, GUI_WIDTH, GUI_HEIGHT);
	mInterface.CreateControls(pGraphics);
	AttachGraphics(pGraphics);

	mFileLoader.Load(SND_01_ID, SND_01_FN, mBuffer);
}

WaveShaper::~WaveShaper() {}

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
					break;
				}
				// fallthru in the case that a NoteOn is supposed to be treated like a NoteOff

			case IMidiMsg::kNoteOff:
				break;
			}

			mMidiQueue.Remove();
		}

		result[0] = result[1] = 0;

		if (mBuffer.getBufferSize() > mReadFrame)
		{
			result[0] = mBuffer.getSample(0, mReadFrame);
			if (mBuffer.getChannelCount() > 1)
			{
				result[1] = mBuffer.getSample(1, mReadFrame);
			}
		}

		*out1 = result[0] * mVolume;
		*out2 = result[1] * mVolume;

		mReadFrame++;
	}
}

void WaveShaper::Reset()
{
	TRACE;
	IMutexLock lock(this);

	mMidiQueue.Clear();
	mMidiQueue.Resize(GetBlockSize());

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
	default:
		break;
	}

	BroadcastParamChange(paramIdx);
}

// this over-ridden method is called when the host is trying to store the plug-in state and needs to get the current data from your algorithm
bool WaveShaper::SerializeState(ByteChunk* pChunk)
{
	TRACE;
	IMutexLock lock(this);

	return IPlugBase::SerializeParams(pChunk); // must remember to call SerializeParams at the end
}

// this over-ridden method is called when the host is trying to load the plug-in state and you need to unpack the data into your algorithm
int WaveShaper::UnserializeState(ByteChunk* pChunk, int startPos)
{
	TRACE;
	IMutexLock lock(this);

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