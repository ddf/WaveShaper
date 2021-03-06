#pragma  once

#include "IControls.h"

using namespace iplug;
using namespace igraphics;

namespace ControlPoint
{
  enum Shape
  {
    Circle,
    Square,
    Diamond
  };

  static void Draw(IGraphics& g, const IColor& color, Shape shape, float x, float y, float radius, const IBlend* blend = nullptr);
}

class EnumControl : public IControl
{
public:
	EnumControl(IRECT rect, int paramIdx, const IText& textStyle);

  void OnInit() override;
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

	BangControl(IRECT iRect, Action action, IColor onColor, IColor offColor, const IText* textStyle = nullptr, const char * label = nullptr, int paramIdx = -1, const char * fileTypes = "fxp");

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
  XYControl(IRECT rect, const int paramX, const int paramY, const int pointRadius, const IColor& pointColor, const ControlPoint::Shape pointShape);

  void Draw(IGraphics& g) override;

  void OnMouseDown(float x, float y, const IMouseMod& pMod) override;
  void OnMouseUp(float x, float y, const IMouseMod& pMod) override;
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& pMod) override;

  void SetValueFromDelegate(double value, int valIdx = 0) override;

private:
  const int mPointRadius;
  const ControlPoint::Shape mPointShape;
  const IColor mPointColor;

  IRECT mPointRect;
  // where the point current is.
  int mPointX, mPointY;
  bool mGribbed;
};

class SnapshotControl : public IControl
{
public:
  SnapshotControl(IRECT rect, const int snapshotParam, const int snapshotIdx, const int pointRadius, IColor backgroundColor, IColor pointColorA, ControlPoint::Shape pointShapeA, IColor pointColorB, ControlPoint::Shape pointShapeB);

  void Draw(IGraphics& g) override;

  void OnMouseDown(float x, float y, const IMouseMod& pMod) override;

  void Update();

private:
  int mSnapshotIdx;
  IRECT mPointRect;
  int mPointRadius;
  float mHighlight;
  IColor mBackgroundColor;
  IColor mPointColorA;
  ControlPoint::Shape mPointShapeA;
  IColor mPointColorB;
  ControlPoint::Shape mPointShapeB;
};

class SnapshotSlider : public IVSliderControl
{
public:
	SnapshotSlider(float x, float y, float len, int handleRadius, int paramIdx, const char * label, const IVStyle& style);

	void DrawTrack(IGraphics& g, const IRECT& filledArea) override;
};

class PlayStopControl : public IPanelControl
{
public:
	PlayStopControl(IRECT rect, IColor backgroundColor, IColor foregroundColor);

	void OnMouseDown(float x, float y, const IMouseMod& pMod) override;
	void Draw(IGraphics& g) override;

private:
	IRECT  mIconRect;
	IColor mForeground;
	bool   mPlaying;
};

class ShapeControl : public IVTabSwitchControl
{
public:
  ShapeControl(const IRECT& bounds, const std::initializer_list<const char*>& options, const std::initializer_list<IColor>& optionColors, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EVShape shape = EVShape::Rectangle, EDirection direction = EDirection::Horizontal);

  void DrawWidget(IGraphics& g) override;

private:
  IVColorSpec mOptionColors;
};
