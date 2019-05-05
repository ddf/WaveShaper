#pragma  once

#include "IControl.h"


class EnumControl : public IControl
{
public:
	EnumControl(IRECT rect, int paramIdx, const IText& textStyle);

	void Draw(IGraphics& g) override;
	void OnMouseDown(float x, float y, const IMouseMod& pMod) override;
	void OnMouseWheel(float x, float y, const IMouseMod& pMod, float d) override;
private:

	void StepValue(int amount);

	int   mMin, mMax;
	// where we draw the text
	IRECT mTextRect;
	// where we respond to clicks to create the popup
	IRECT mPopupRect;
	IRECT mDecrementRect;
	IRECT mIncrementRect;
};

// based on IContactControl, but without the need for a bitmap
class BangControl : public IControl
{
public:
	enum Action
	{
		ActionBangParam,
		ActionLoad, // by default will load fxp files only, specify fileTypes to handle different files
		ActionSave, // by default will save fxp files only, specify fileTypes to save to different files
		ActionDumpPreset,

		// if the action is greater than or equal to this value,
		// the Bang will call HandleAction on the owning plug
		ActionCustom = 100,
	};

	BangControl(IRECT iRect, Action action, IColor onColor, IColor offColor, IText* textStyle = nullptr, const char * label = nullptr, int paramIdx = -1, const char * fileTypes = "fxp");

	void Draw(IGraphics& g) override;

	void OnMouseDown(float x, float y, const IMouseMod& pMod) override;
	void OnMouseUp(float x, float y, const IMouseMod& pMod) override;



private:
	const Action mAction;
	const char * mLabel;
	IColor mOnColor;
	IColor mOffColor;
	// used as argument to PromptForFile with actions that deal with files
	const char * mFileTypes;
};


namespace Minim
{
	class MultiChannelBuffer;
}

// control that draws the peaks of a buffer by chunking it into sections
class PeaksControl : public IPanelControl
{
public:
	PeaksControl(IRECT rect, IColor backColor, IColor peaksColor);
	~PeaksControl();

	void Draw(IGraphics& g) override;
	void UpdatePeaks(const Minim::MultiChannelBuffer& withSamples);

private:
	float* mPeaks;
	size_t mPeaksSize;
	IColor mPeaksColor;
};

// visualization of the section of the loaded file that is being scrubbed over
class ShaperVizControl : public IControl
{
public:
	ShaperVizControl(IRECT rect, IColor bracketColor, IColor lineColor);

	void Draw(IGraphics& g) override;
	void OnMouseUp(float x, float y, const IMouseMod& pMod) override;
	void OnMouseOver(float x, float y, const IMouseMod& pMod) override;

private:
	IColor mBracketColor;
	IColor mLineColor;
};

// control that displays a single control UI in a rectangle that controls two params - one on the x-axis, the other on the y-axis.
class XYControl : public IControl
{
public:
	XYControl(IPlugBase* pPlug, IRECT rect, const int paramX, const int paramY, const int pointRadius, IColor pointColor);

	bool Draw(IGraphics* pGraphics) override;
	
	void OnMouseDown(int x, int y, IMouseMod* pMod) override;
	void OnMouseUp(int x, int y, IMouseMod* pMod) override;
	void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod) override;


	void SetAuxParamValueFromPlug(int auxParamIdx, double value) override;

private:
	IRECT mPointRect;
	// where the point current is.
	int mPointX, mPointY;
	int mPointRadius;
	IColor mPointColor;
	bool   mGribbed;
};

class SnapshotControl : public IControl
{
public:
	SnapshotControl(IPlugBase* pPlug, IRECT rect, const int snapshotParam, const int snapshotIdx, const int pointRadius, IColor backgroundColor, IColor pointColorA, IColor pointColorB);

	bool Draw(IGraphics* pGraphics) override;

	void OnMouseDown(int x, int y, IMouseMod* pMod) override;

	void Update();

private:
	int mSnapshotIdx;
	IRECT mPointRect;
	int mPointRadius;
	float  mHighlight;
	IColor mBackgroundColor;
	IColor mPointColorA;
	IColor mPointColorB;
};

class SnapshotSlider : public IFaderControl
{
public:
	SnapshotSlider(IPlugBase * pPlug, int x, int y, int len, int handleRadius, int paramIdx, IColor lineColor, IColor handleColor);

	bool Draw(IGraphics* pGraphics) override;
	void SetDirty(bool pushParamToPlug = true) override;

private:
	IColor mLineColor;
	IColor mHandleColor;
};

class PlayStopControl : public IPanelControl
{
public:
	PlayStopControl(IPlugBase* pPlug, IRECT rect, IColor backgroundColor, IColor foregroundColor);

	virtual void OnMouseDown(int x, int y, IMouseMod* pMod) override;
	virtual bool Draw(IGraphics* pGraphics) override;

private:
	IRECT  mIconRect;
	IColor mForeground;
	bool   mPlaying;
};