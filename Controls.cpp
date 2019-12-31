#include "Controls.h"
#include "Interface.h"
#include "WaveShaper.h"
#include "Params.h"
#include "MultiChannelBuffer.h"
#include "Interp.h"

void ControlPoint::Draw(IGraphics& g, const IColor& color, const ControlPoint::Shape shape, float x, float y, float r, const IBlend* blend)
{
  switch (shape)
  {
    case ControlPoint::Circle:
    {
      g.FillCircle(color, x, y, r, blend);
    }
    break;

    case ControlPoint::Square:
    {
      g.FillRect(color, IRECT(x-r, y-r, x+r, y+r), blend);
    }
    break;

    case ControlPoint::Diamond:
    {
      g.FillTriangle(color, x-r, y, x+r, y, x, y-r, blend);
      g.FillTriangle(color, x-r, y, x+r, y, x, y+r, blend);
    }
    break;
  }
}


#pragma  region EnumControl
EnumControl::EnumControl(IRECT rect, int paramIdx, const IText& textStyle)
	: IControl(rect, paramIdx)
{
	SetText(textStyle);
	mDblAsSingleClick = true;
	mDisablePrompt = false;
	// our contents need to fit into a 2px padded rect so we can draw the outline using mRECT
	// and have 1px background color between that and the contents
	IRECT contentsRect = mRECT.GetPadded(-5);
	mDecrementRect = IRECT(contentsRect.L, contentsRect.T, contentsRect.L + 5, contentsRect.B);
	mIncrementRect = IRECT(contentsRect.R - 5, contentsRect.T, contentsRect.R, contentsRect.B);

	mTextRect = contentsRect.GetHPadded(-7);

	// rect for displaying the popup should be full height, but only as wide as the text rect
	mPopupRect = mRECT.GetHPadded(-12);
}

void EnumControl::OnInit()
{
  if (GetParamIdx() < kNumParams)
  {
    mMin = GetParam()->GetMin();
    mMax = GetParam()->GetMax();
  }
  else
  {
    mMin = 0;
    // #TODO get the number of presets
    mMax = 1; // GetDelegate()->NPresets() - 1;
  }
}

void EnumControl::Draw(IGraphics& g)
{
  g.FillRect(mText.mTextEntryBGColor, mRECT);
  g.DrawRect(mText.mTextEntryFGColor, mRECT);

  // buttons
  IColor buttonColor = mText.mTextEntryFGColor;
  // #TODO get current preset index
  const int value = GetParamIdx() < kNumParams ? GetParam()->Int() : 0;  // mPlug->GetCurrentPresetIdx();
  if (value == mMin || IsDisabled())
  {
    buttonColor.R *= 0.5f;
    buttonColor.G *= 0.5f;
    buttonColor.B *= 0.5f;
  }
  g.FillTriangle(buttonColor, mDecrementRect.L, mDecrementRect.MH(), mDecrementRect.R, mDecrementRect.T, mDecrementRect.R, mDecrementRect.B, 0);

  buttonColor = mText.mTextEntryFGColor;
  if (value == mMax || IsDisabled())
  {
    buttonColor.R *= 0.5f;
    buttonColor.G *= 0.5f;
    buttonColor.B *= 0.5f;
  }
  g.FillTriangle(buttonColor, mIncrementRect.L, mIncrementRect.T, mIncrementRect.R, mIncrementRect.MH(), mIncrementRect.L, mIncrementRect.B, 0);

  WDL_String display;
  if (GetParamIdx() < kNumParams)
  {
    GetParam()->GetDisplayForHost(display);
  }
  else
  {
    // #TODO get preset name
    display.Set("");  // const_cast<char*>(mPlug->GetPresetName(mPlug->GetCurrentPresetIdx()));
  }

  mText.mFGColor.A = IsDisabled() ? 128 : 255;
  g.DrawText(mText, display.Get(), mTextRect);
}

void EnumControl::OnMouseDown(float x, float y, const IMouseMod& pMod)
{
	if (pMod.L)
	{
		if (mPopupRect.Contains(x, y))
		{
			if (GetParamIdx() < kNumParams)
			{
				PromptUserInput();
			}
			else // need to build the popup for presets by hand
			{
        // #TODO preset popup
				//IPopupMenu menu;
				//const int currentPresetIdx = mPlug->GetCurrentPresetIdx();
				//for (int i = 0; i < mPlug->NPresets(); ++i)
				//{
				//	const char * presetName = mPlug->GetPresetName(i);
				//	if (i == currentPresetIdx)
				//	{
				//		menu.AddItem(new IPopupMenuItem(presetName, IPopupMenuItem::kChecked), -1);
				//	}
				//	else
				//	{
				//		menu.AddItem(new IPopupMenuItem(presetName), -1);
				//	}
				//}

				//if (GetGUI()->CreateIPopupMenu(&menu, &mRECT))
				//{
				//	mPlug->RestorePreset(menu.GetChosenItemIdx());
				//	SetDirty(false);
				//}
			}
		}
		else if (mIncrementRect.Contains(x, y))
		{
			StepValue(1);
		}
		else if (mDecrementRect.Contains(x, y))
		{
			StepValue(-1);
		}
	}
	else if (pMod.R && GetParamIdx() < kNumParams)
	{
		Interface::BeginMIDILearn(GetDelegate(), GetParamIdx(), -1, x, y);
	}
}

void EnumControl::OnMouseWheel(float x, float y, const IMouseMod& pMod, float d)
{
	StepValue(d);
}

void EnumControl::StepValue(int amount)
{
	const bool isParam = GetParamIdx() < kNumParams;
	if (isParam)
	{
		int count = GetParam()->NDisplayTexts();
    double value = GetValue();
		if (count > 1)
		{
			value += 1.0 / (double)(count - 1) * amount;
		}
		else
		{
			value += 1.0;
		}
    SetValue(value);
	}
	else
	{
    // #TODO restore preset correctly
		//const int presetIdx = BOUNDED(mPlug->GetCurrentPresetIdx() + amount, mMin, mMax);
		//if (presetIdx != mPlug->GetCurrentPresetIdx())
		//{
		//	mPlug->RestorePreset(presetIdx);
		//}
	}

	SetDirty(isParam);
}
#pragma  endregion EnumControl


#pragma  region BangControl

BangControl::BangControl(IRECT iRect, Action action, IColor onColor, IColor offColor, const IText* textStyle /*= nullptr*/, const char * label /*= nullptr*/, int paramIdx /*= -1*/, const char * fileTypes /*= "fxp"*/)
	: IControl(iRect, paramIdx)
	, mAction(action)
	, mOnColor(onColor)
	, mOffColor(offColor)
	, mLabel(label)
	, mFileTypes(fileTypes)
{
	if (textStyle != nullptr)
	{
		SetText(*textStyle);
	}
	mDblAsSingleClick = true;
}

void BangControl::Draw(IGraphics& g)
{
	if (GetValue() > 0.5)
	{
		g.FillRect(mOnColor, mRECT);
	}
	else
	{
		g.FillRect(mOffColor, mRECT);
	}

	//pGraphics->DrawRect(&mOnColor, &mRECT);

	if (mLabel != nullptr)
	{
		g.DrawText(mText, mLabel, mRECT);
	}
}

void BangControl::OnMouseDown(float x, float y, const IMouseMod& pMod)
{
	if (mAction == ActionBangParam)
	{
		SetValue(1);
		SetDirty();
	}
	else // not an actual parameter, direct-cast this bang on click
	{
		switch (mAction)
		{
		case ActionSave:
		{
			WDL_String fileName("");
			WDL_String directory("");
			GetUI()->PromptForFile(fileName, directory, EFileAction::Save, mFileTypes);
			if (fileName.GetLength() > 0)
			{
				PLUG_CLASS_NAME * plug = static_cast<PLUG_CLASS_NAME*>(GetDelegate());
				if (plug != nullptr)
				{
					plug->HandleSave(&fileName, &directory);
				}
			}
		}
		break;

		case ActionLoad:
		{
			WDL_String fileName("");
			WDL_String directory("");
			GetUI()->PromptForFile(fileName, directory, EFileAction::Open, mFileTypes);
			if (fileName.GetLength() > 0)
			{
				PLUG_CLASS_NAME* plug = static_cast<PLUG_CLASS_NAME*>(GetDelegate());
				if (plug != nullptr)
				{
					plug->HandleLoad(&fileName, &directory);
				}
			}
		}
		break;

		case ActionDumpPreset:
		{
			PLUG_CLASS_NAME* plug = static_cast<PLUG_CLASS_NAME*>(GetDelegate());
			if (plug != nullptr)
			{
				plug->DumpPresetSrc();
			}
		}
		break;

		case ActionCustom:
		default:
		{
			PLUG_CLASS_NAME* plug = static_cast<PLUG_CLASS_NAME*>(GetDelegate());
			if (plug != nullptr)
			{
				plug->HandleAction(mAction);
			}
			SetValue(1);
			SetDirty(false);
		}
    break;

		}
	}
}

void BangControl::OnMouseUp(float x, float y, const IMouseMod& pMod)
{
	if (GetParamIdx() < kNumParams)
	{
    SetValue(0);
		SetDirty();
	}
}

#pragma  endregion 

#pragma  region PeaksControl
PeaksControl::PeaksControl(IRECT rect, IColor backColor, IColor peaksColor)
	: IPanelControl(rect, backColor)
	, mPeaksColor(peaksColor)
	, mPeaksSize(rect.W())
{
	mPeaks = new float[mPeaksSize];
	memset(mPeaks, 0, sizeof(float)*mPeaksSize);
}

PeaksControl::~PeaksControl()
{
	delete[] mPeaks;
}

void PeaksControl::Draw(IGraphics& g)
{
	IPanelControl::Draw(g);

	for (int i = 0; i < mPeaksSize; ++i)
	{
		int x = mRECT.L + i;
		int ph = mRECT.H()*mPeaks[i];
		int y1 = mRECT.MH() + ph / 2;
		int y2 = mRECT.MH() - ph / 2;
		g.DrawLine(mPeaksColor, x, y1, x, y2);
	}
}

void PeaksControl::UpdatePeaks(const Minim::MultiChannelBuffer& withSamples)
{
	// calculate peaks
	const int chunkSize = withSamples.getBufferSize() / mPeaksSize;
	const bool bStereo = withSamples.getChannelCount() == 2;
	for (int i = 0; i < mPeaksSize; ++i)
	{
		float peak = 0;
		int s = 0;
		for (; s < chunkSize; ++s)
		{
			int frame = i*chunkSize + s;
			if (frame >= withSamples.getBufferSize())
			{
				break;
			}
			float val = 0;
			if (bStereo)
			{
				val = (withSamples.getChannel(0)[frame] + withSamples.getChannel(1)[frame]) / 2.f;
			}
			else
			{
				val = withSamples.getChannel(0)[frame];
			}
			peak += val*val;
		}
		peak /= s + 1;
		mPeaks[i] = sqrtf(peak);
	}

	SetDirty(false);
}
#pragma  endregion PeaksControl

#pragma  region ShaperVizControl
const float kVizTriangleSize = 5;
ShaperVizControl::ShaperVizControl(IRECT rect, IColor bracketColor, IColor lineColor)
	: IControl(rect)
	, mBracketColor(bracketColor)
	, mLineColor(lineColor)
{

}

void ShaperVizControl::Draw(IGraphics& g)
{
	WaveShaper* shaper = dynamic_cast<WaveShaper*>(GetDelegate());
	if (shaper != nullptr)
	{
		const float center = Map(shaper->GetNoiseOffset(), -1, 1, mRECT.L, mRECT.R);
		const float widthPct = shaper->GetShape();
		const float waveWidth = mRECT.W();
		const float shaperSize = shaper->GetShaperSize();
		const float sampleWidth = (shaperSize * widthPct);
		const float chunkSize = shaperSize / mRECT.W();
		const float halfWidth = sampleWidth / chunkSize * 0.5f;

		float x1 = center - halfWidth, x2 = center + halfWidth;
		int y1 = mRECT.T, y2 = mRECT.B - 1;

		// this will be [0, 1]
		const float mapLookup = shaper->GetShaperMapValue();
		const float lx = Lerp(mRECT.L, mRECT.R, mapLookup);
		g.DrawLine(mLineColor, lx, y1, lx, y2-1);

		IBlend blend(EBlend::None, 0.4f);
		if (x1 < mRECT.L)
		{
			x1 += waveWidth;
			//pGraphics->DrawLine(&mBracketColor, x1, y1, x1, y2);
			//pGraphics->DrawLine(&mBracketColor, x1, y1, x1 + 4, y1);
			//pGraphics->DrawLine(&mBracketColor, x1, y2, mRECT.R, y2);
			//pGraphics->DrawLine(&mBracketColor, mRECT.L, y2, center, y2);

			IRECT rect(x1, y1, mRECT.R, y2);
			g.FillRect(mBracketColor, rect, &blend);
			rect = IRECT(mRECT.L, y1, center, y2);
			g.FillRect(mBracketColor, rect, &blend);
		}
		else
		{
			//pGraphics->DrawLine(&mBracketColor, x1, y1, x1, y2);
			//pGraphics->DrawLine(&mBracketColor, x1, y1, x1 + 4, y1);
			//pGraphics->DrawLine(&mBracketColor, x1, y2, center, y2);
			IRECT rect(x1, y1, center, y2);
			g.FillRect(mBracketColor, rect, &blend);
		}		

		if (x2 > mRECT.R)
		{
			x2 -= waveWidth;
			//pGraphics->DrawLine(&mBracketColor, x2, y1, x2, y2);
			//pGraphics->DrawLine(&mBracketColor, x2, y1, x2 - 4, y1);
			//pGraphics->DrawLine(&mBracketColor, x2, y2, mRECT.L, y2);
			//pGraphics->DrawLine(&mBracketColor, center, y2, mRECT.R, y2);

			IRECT rect(mRECT.L, y1, x2, y2);
			g.FillRect(mBracketColor, rect, &blend);
			rect = IRECT(center, y1, mRECT.R, y2);
			g.FillRect(mBracketColor, rect, &blend);
		}
		else
		{
			//pGraphics->DrawLine(&mBracketColor, x2, y1, x2, y2);
			//pGraphics->DrawLine(&mBracketColor, x2, y1, x2 - 4, y1);
			//pGraphics->DrawLine(&mBracketColor, x2, y2, center, y2);
			IRECT rect(center, y1, x2, y2);
			g.FillRect(mBracketColor, rect, &blend);
		}

		if (center > mRECT.L + kVizTriangleSize && center < mRECT.R - kVizTriangleSize)
		{
			g.FillTriangle(mBracketColor, center, y2, center - kVizTriangleSize, y2, center, y2 - kVizTriangleSize, 0);
			g.FillTriangle(mBracketColor, center, y2, center + kVizTriangleSize, y2, center, y2 - kVizTriangleSize, 0);
		}
		else
		{
			x1 = mRECT.L;
			x2 = mRECT.R;
			g.FillTriangle(mBracketColor, x1, y2, x1 + kVizTriangleSize, y2, x1, y2 - kVizTriangleSize, 0);
			g.FillTriangle(mBracketColor, x2, y2, x2 - kVizTriangleSize, y2, x2, y2 - kVizTriangleSize, 0);
		}
		
    SetDirty(false);
	}
}

void ShaperVizControl::OnMouseUp(float x, float y, const IMouseMod& pMod)
{
	TRACE;
}

void ShaperVizControl::OnMouseOver(float x, float y, const IMouseMod& pMod)
{
	TRACE;
}

#pragma  endregion ShaperVizControl

#pragma  region XYControl
XYControl::XYControl(IRECT rect, const int paramX, const int paramY, const int pointRadius, const IColor& pointColor, const ControlPoint::Shape pointShape)
  : IControl(rect, {paramX, paramY})
  , mPointRadius(pointRadius)
  , mPointShape(pointShape)
  , mPointColor(pointColor)
  , mGribbed(false)
  , mPointX(0)
  , mPointY(0)
{
  mPointRect = mRECT.GetPadded(-pointRadius - 1);
}

void XYControl::Draw(IGraphics& g)
{
  const IBlend* blend = mGribbed ? nullptr : &BLEND_75;
  ControlPoint::Draw(g, mPointColor, mPointShape, mPointX, mPointY, mPointRadius, blend);
}

void XYControl::OnMouseDown(float x, float y, const IMouseMod& pMod)
{
  mGribbed = mRECT.Contains(x, y);
  SetDirty(false);
}

void XYControl::OnMouseUp(float x, float y, const IMouseMod& pMod)
{
  mGribbed = false;
  SetDirty(false);
}

void XYControl::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& pMod)
{
  if (mGribbed)
  {
    mPointX = Clip(mPointX + dX, mPointRect.L, mPointRect.R);
    mPointY = Clip(mPointY + dY, mPointRect.T, mPointRect.B);

    mTargetRECT = IRECT(mPointX - mPointRadius, mPointY - mPointRadius, mPointX + mPointRadius, mPointY + mPointRadius);

    SetValue(Map(mPointX, mPointRect.L, mPointRect.R, 0, 1), 0);
    SetValue(Map(mPointY, mPointRect.B, mPointRect.T, 0, 1), 1);
    SetDirty();
  }
}

void XYControl::SetValueFromDelegate(double value, int valIdx /*= 0*/)
{
  if (valIdx == 0)
  {
    mPointX = Map(value, 0, 1, mPointRect.L, mPointRect.R);
  }
  else if (valIdx == 1)
  {
    mPointY = Map(value, 0, 1, mPointRect.B, mPointRect.T);
  }

  mTargetRECT = IRECT(mPointX - mPointRadius, mPointY - mPointRadius, mPointX + mPointRadius, mPointY + mPointRadius);

  SetDirty(false);
}
#pragma  endregion XYControl

#pragma  region SnapshotControl

SnapshotControl::SnapshotControl(IRECT rect, const int snapshotParam, const int snapshotIdx, const int pointRadius, IColor backgroundColor, IColor pointColorA, ControlPoint::Shape pointShapeA, IColor pointColorB, ControlPoint::Shape pointShapeB)
	: IControl(rect, snapshotParam)
	, mSnapshotIdx(snapshotIdx)
	, mPointRadius(pointRadius)
	, mBackgroundColor(backgroundColor)
	, mPointColorA(pointColorA)
  , mPointShapeA(pointShapeA)
	, mPointColorB(pointColorB)
  , mPointShapeB(pointShapeB)
	, mHighlight(0)
{
	mPointRect = mRECT.GetPadded(-pointRadius - 1);
}

void SnapshotControl::Draw(IGraphics& g)
{
  g.FillRect(mBackgroundColor, mRECT);

  WaveShaper* shaper = dynamic_cast<WaveShaper*>(GetDelegate());
  if (shaper != nullptr)
  {
    WaveShaper::NoiseSnapshot snapshot = shaper->GetNoiseSnapshotNormalized(mSnapshotIdx);

    float x = ::Lerp(mPointRect.L, mPointRect.R, snapshot.AmpMod);
    float y = ::Lerp(mPointRect.B, mPointRect.T, snapshot.Rate);
    ControlPoint::Draw(g, mPointColorA, mPointShapeA, x, y, mPointRadius);

    x = ::Lerp(mPointRect.L, mPointRect.R, snapshot.Range);
    y = ::Lerp(mPointRect.B, mPointRect.T, snapshot.Shape);
    ControlPoint::Draw(g, mPointColorB, mPointShapeB, x, y, mPointRadius);
  }

  double weight = abs(GetParam()->Value() - mSnapshotIdx);
  if (weight < 1)
  {
    IBlend blend(EBlend::None, 1 - weight);
    IColor border(255, 200, 200, 200);
    g.DrawRect(border, mRECT, &blend);
  }

  if (mHighlight > 0)
  {
    IBlend blend(EBlend::None, mHighlight);
    g.FillRect(COLOR_WHITE, mRECT, &blend);
    mHighlight -= 0.1f;
    SetDirty(false);
  }
}

void SnapshotControl::OnMouseDown(float x, float y, const IMouseMod& pMod)
{
  if (pMod.L)
  {
    SetValue(GetParam()->ToNormalized(mSnapshotIdx));
    mHighlight = 1;
    SetDirty();
    // GetGUI()->SetParameterFromGUI(mParamIdx, mValue);
  }
}

void SnapshotControl::Update()
{
  WaveShaper* shaper = dynamic_cast<WaveShaper*>(GetDelegate());
  if (shaper != nullptr)
  {
    shaper->UpdateNoiseSnapshot(mSnapshotIdx);
  }

  SetValue(GetParam()->ToNormalized(mSnapshotIdx));
  mHighlight = 1;
  SetDirty();
  // GetGUI()->SetParameterFromGUI(mParamIdx, mValue);
}

#pragma  endregion

#pragma  region SnapshotSlider
SnapshotSlider::SnapshotSlider(float x, float y, float len, int handleRadius, int paramIdx, const char* label, const IVStyle& style)
  : IVSliderControl(IRECT(x, y, x + handleRadius * 2, y + len), paramIdx, label, style)
{
}

void SnapshotSlider::DrawTrack(IGraphics& g, const IRECT& filledArea)
{
  const float cx = mTrack.MW();
  const IColor& color = GetColor(kSH);
  g.DrawLine(color, cx - 2, mTrack.T, cx + 2, mTrack.T);
  g.DrawLine(color, cx, mTrack.T, cx, mTrack.B);
  g.DrawLine(color, cx - 2, mTrack.B, cx + 2, mTrack.B);
}
#pragma  endregion

#pragma  region PlayStopControl
PlayStopControl::PlayStopControl(IRECT rect, IColor backgroundColor, IColor foregroundColor)
	: IPanelControl(rect, backgroundColor)
	, mForeground(foregroundColor)
{
	mIconRect = mRECT.GetPadded(-8);
	mDblAsSingleClick = true;
  mIgnoreMouse = false;
}

void PlayStopControl::OnMouseDown(float x, float y, const IMouseMod& pMod)
{
	IMidiMsg msg;
	if (GetValue())
	{
		msg.MakeNoteOffMsg(0, 0);
    SetValue(0);
	}
	else
	{
		msg.MakeNoteOnMsg(0, 127, 0);
    SetValue(1);
	}
  GetDelegate()->SendMidiMsgFromUI(msg);
	SetDirty(false);
}

void PlayStopControl::Draw(IGraphics& g)
{
	IPanelControl::Draw(g);

	if (GetValue())
	{
		g.FillRect(mForeground, mIconRect);
	}
	else
	{
		g.FillTriangle(mForeground, mIconRect.L, mIconRect.T, mIconRect.R, (int)mIconRect.MH(), mIconRect.L, mIconRect.B, 0);
	}
}

#pragma endregion

#pragma region ShapeControl
ShapeControl::ShapeControl(const IRECT& bounds, const std::initializer_list<const char*>& options, const std::initializer_list<IColor>& optionColors, const char* label, const IVStyle& style, EVShape shape, EDirection direction)
  : IVTabSwitchControl(bounds, kNoiseType, label, style, shape, direction)
  , mOptionColors(optionColors)
{
  assert(options.size() == optionColors.size());

  for (auto& option : options)
  {
    mTabLabels.Add(new WDL_String(option));
  }
}

void ShapeControl::DrawWidget(IGraphics& g)
{
  int hit = GetSelectedIdx();
  ETabSegment segment = ETabSegment::Start;

  for (int i = 0; i < mNumStates; i++)
  {
    IRECT r = mButtons.Get()[i];

    if (i > 0)
      segment = ETabSegment::Mid;

    if (i == mNumStates - 1)
      segment = ETabSegment::End;

    DrawButton(g, r, i == hit, mMouseOverButton == i, segment);

    if (mTabLabels.Get(i))
    {
      IText text = mText.WithFGColor(mOptionColors.GetColor((EVColor)i));
      const char* label = mTabLabels.Get(i)->Get();      
      g.DrawText(text, label, r);
    }
  }
}

#pragma endregion
