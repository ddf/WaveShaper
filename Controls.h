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