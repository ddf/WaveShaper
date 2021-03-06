#pragma once

#include "config.h"
#include "IGraphicsStructs.h"
#include "Params.h"

class PLUG_CLASS_NAME;
class KnobLineCoronaControl;
class PeaksControl;
class SnapshotControl;

namespace Minim
{
	class MultiChannelBuffer;
}

namespace iplug
{
  class IEditorDelegate;
}

using namespace iplug;
using namespace igraphics;

class Interface
{
public:
	Interface(PLUG_CLASS_NAME* inPlug);
	~Interface();

	void CreateControls(IGraphics* pGraphics);

	// called by the plug when a preset is loaded or saved
	void OnPresetChanged();

	// called by the plug when HandleAction is called with a snapshot index
	void UpdateSnapshot(const int idx);

	// called when the plug loads a new audio file
	void RebuildPeaks(const Minim::MultiChannelBuffer& forSamples);

	// used by Controls to initiate MIDILearn functionality in the Standalone
	static void BeginMIDILearn(IEditorDelegate* plug, const int paramIdx1, const int paramIdx2, const int x, const int y);

private:	
	IControl* AttachEnum(IGraphics* pGraphics, IRECT rect, const int paramIdx, const char * label = nullptr);
	IControl* AttachTextBox(IGraphics* pGraphics, IRECT rect, const int paramIdx, const float scrollSpeed, const char * maxValue, const char * label = nullptr);
	KnobLineCoronaControl* AttachKnob(IGraphics* pGraphics, IRECT rect, const int paramIdx, const char * label = nullptr);

	PLUG_CLASS_NAME* mPlug;

	IControl* mPresetControl;
	PeaksControl* mPeaksControl;
	SnapshotControl* mSnapshotControls[kNoiseSnapshotCount];
};

