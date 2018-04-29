#pragma  once

#include "IControl.h"


class KnobLineCoronaControl : public IKnobLineControl
{
public:
	KnobLineCoronaControl(IPlugBase* pPlug, IRECT pR, int paramIdx,
		const IColor* pLineColor, const IColor* pCoronaColor,
		float coronaThickness = 0.0f, double innerRadius = 0.0, double outerRadius = 0.0,
		double minAngle = -0.75 * PI, double maxAngle = 0.75 * PI,
		EDirection direction = kVertical, double gearing = DEFAULT_GEARING);

	bool Draw(IGraphics* pGraphics) override;

	void OnMouseDown(int x, int y, IMouseMod* pMod) override;
	void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod) override;
	void OnMouseUp(int x, int y, IMouseMod* pMod) override;

	void OnMouseOver(int x, int y, IMouseMod* pMod) override;
	void OnMouseOut() override;

	void SetLabelControl(ITextControl* control, bool bShared = false);

private:
	void ShowLabel();
	void HideLabel();

	float		  mCX, mCY;
	bool		  mHasMouse;
	IColor        mCoronaColor;
	IChannelBlend mCoronaBlend;
	ITextControl* mLabelControl;
	WDL_String	  mLabelString;
	bool		  mSharedLabel;
};

class EnumControl : public IControl
{
public:
	EnumControl(IPlugBase* pPlug, IRECT rect, int paramIdx, IText* textStyle);

	bool Draw(IGraphics* pGraphics) override;
	void OnMouseDown(int x, int y, IMouseMod* pMod) override;
	void OnMouseWheel(int x, int y, IMouseMod* pMod, int d) override;
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

class TextBox : public ICaptionControl
{
public:
	TextBox(IPlugBase* pPlug, IRECT pR, int paramIdx, IText* pText, IGraphics* pGraphics, const char * maxText, bool showParamUnits, float scrollSpeed);

	bool Draw(IGraphics* pGraphics) override;
	void OnMouseDown(int x, int y, IMouseMod* pMod) override;
	void OnMouseWheel(int x, int y, IMouseMod* pMod, int d) override;


	virtual void GrayOut(bool gray) override;

private:
	bool  mShowParamUnits;
	float mScrollSpeed;
	IRECT mTextRect;
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

	BangControl(IPlugBase* pPlug, IRECT iRect, Action action, IColor onColor, IColor offColor, IText* textStyle = nullptr, const char * label = nullptr, int paramIdx = -1, const char * fileTypes = "fxp");

	bool Draw(IGraphics* pGraphics) override;

	void OnMouseDown(int x, int y, IMouseMod* pMod) override;
	void OnMouseUp(int x, int y, IMouseMod* pMod) override;



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
	PeaksControl(IPlugBase* pPlug, IRECT rect, IColor backColor, IColor peaksColor);
	~PeaksControl();

	bool Draw(IGraphics* pGraphics) override;
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
	ShaperVizControl(IPlugBase* pPlug, IRECT rect, IColor bracketColor, IColor lineColor);

	bool Draw(IGraphics* pGraphics) override;


	virtual void OnMouseUp(int x, int y, IMouseMod* pMod) override;


	virtual void OnMouseOver(int x, int y, IMouseMod* pMod) override;

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