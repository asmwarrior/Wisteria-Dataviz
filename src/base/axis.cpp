﻿///////////////////////////////////////////////////////////////////////////////
// Name:        axis.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "axis.h"

namespace Wisteria::GraphItems
    {
    //-----------------------------------------
    void Axis::AdjustLabelSizeIfUsingBackgroundColor(Label& axisLabel,
                                                     wxDC& dc,
                                                     const bool useMaxWidth) const
        {
        if (GetFontBackgroundColor().IsOk() &&
            GetFontBackgroundColor() != wxTransparentColour &&
            GetAxisLabelOrientation() == AxisLabelOrientation::Parallel)
            {
            if (IsHorizontal())
                {
                if (useMaxWidth)
                    {
                    axisLabel.SetMinimumUserSizeDIPs(
                        dc.ToDIP(m_maxLabelWidth), std::nullopt);
                    }
                axisLabel.SetPadding(std::max(4, GetTopPadding()),
                    std::max(4, GetRightPadding()),
                    std::max(4, GetBottomPadding()), std::max(4, GetLeftPadding()));
                }
            else
                {
                if (useMaxWidth)
                    {
                    axisLabel.SetMinimumUserSizeDIPs(
                        std::nullopt, dc.ToDIP(m_maxLabelWidth));
                    }
                axisLabel.SetPadding(std::max(4, GetTopPadding()),
                    std::max(4, GetRightPadding()),
                    std::max(4, GetBottomPadding()), std::max(4, GetLeftPadding()));
                }
            }
        }

    //-----------------------------------------
    void Axis::Reset(const AxisResetLevel level /*= AxisResetLevel::RangeAndLabelValues*/)
        {
        if (level == AxisResetLevel::CosmeticSettings ||
            level == AxisResetLevel::AllSettings)
            {
            GetFont() = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).MakeSmaller();
            SetFontColor(wxColour(*wxBLACK));
            SetFontBackgroundColor(wxNullColour);
            SetTextAlignment(TextAlignment::Centered);
            m_axisLabelAlignment = AxisLabelAlignment::AlignWithAxisLine;
            m_axisLinePen = *wxBLACK_PEN;
            m_gridlinePen = wxPen(wxPenInfo(wxColour(211, 211, 211)).Cap(wxCAP_BUTT));
            m_outlineSize = wxDefaultSize;
            }
        if (level == AxisResetLevel::Brackets ||
            level == AxisResetLevel::AllSettings)
            { ClearBrackets(); }
        if (level == AxisResetLevel::TitleHeaderFooter ||
            level == AxisResetLevel::AllSettings)
            {
            GetTitle() = GetHeader() = GetFooter() =
                Label(GraphItemInfo(L"").DPIScaling(GetDPIScaleFactor()));
            }
        if (level == AxisResetLevel::RangeAndLabelValues ||
            level == AxisResetLevel::AllSettings)
            {
            m_axisLabels.clear();
            m_customAxisLabels.clear();
            m_tickMarks.clear();
            m_customTickMarks.clear();
            m_labelSpacingPhysicalOffset = 0;
            m_rangeStart = 0;
            m_rangeEnd = 0;
            m_customXPosition = 0;
            m_customYPosition = 0;
            m_physicalCustomYPosition = -1;
            m_physicalCustomXPosition = -1;
            m_displayPrecision = 0;
            m_interval = 1;
            m_displayInterval = 1;
            m_tickMarkDisplayInterval = 1;
            m_minorTickMarkLength = 5;
            m_majorTickMarkLength = 10;
            m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
            }
        if (level == AxisResetLevel::AllSettings)
            {
            m_stackLabelsToFit = false;
            m_enableAutoStacking = IsHorizontal();
            m_scaledReserved = false;
            m_startAtZero = false;
            m_labelDisplay = AxisLabelDisplay::DisplayCustomLabelsOrValues;
            m_suggestedMaxLengthPerLine = 100;
            m_tickMarkDisplayType = TickMark::DisplayType::NoDisplay;
            m_doubleSidedAxisLabels = false;
            GetTitle().SetTextOrientation((IsVertical() ?
                Orientation::Vertical : Orientation::Horizontal));
            }
        }

    void Axis::SetBoundingBox(const wxRect& rect,
                              [[maybe_unused]] wxDC& dc,
                              [[maybe_unused]] const double parentScaling)
        {
        wxASSERT_LEVEL_2_MSG(!IsFreeFloating(),
                             L"SetBoundingBox() should only be called on fixed objects!");
        if (IsFreeFloating())
            { return; }
        SetScaling(parentScaling);

        SetAxisLabelScaling(GetScaling());
        GetHeader().SetScaling(GetScaling());
        GetFooter().SetScaling(GetScaling());

        const wxRect boundingBox = GetBoundingBox(dc);
        const wxRect protrudingBox = GetProtrudingBoundingBox(dc);

        const auto calculateVerticalLabelOverhang = [this, &dc]()
            {
            const wxPoint topLeftCornerOriginal{ GetTopPoint() }, bottomRightCornerOriginal{ GetBottomPoint() };
            wxPoint topLeftCorner{ GetTopPoint() }, bottomRightCorner{ GetBottomPoint() };
            CalcVerticalLabelOverhang(dc, topLeftCorner, bottomRightCorner);
            const wxCoord protrudingLabelTopPadding = topLeftCornerOriginal.y - topLeftCorner.y;
            const wxCoord protrudingLabelBottomPadding = bottomRightCorner.y - bottomRightCornerOriginal.y;
            return std::make_pair(protrudingLabelTopPadding, protrudingLabelBottomPadding);
            };

        const auto calculateHorizontalLabelOverhang = [this, &dc]()
            {
            const wxPoint topLeftCornerOriginal{ GetLeftPoint() }, bottomRightCornerOriginal{ GetRightPoint() };
            wxPoint topLeftCorner{ GetLeftPoint() }, bottomRightCorner{ GetRightPoint() };
            CalcHorizontalLabelOverhang(dc, topLeftCorner, bottomRightCorner);
            const wxCoord protrudingLabelLeftPadding = topLeftCornerOriginal.x - topLeftCorner.x;
            const wxCoord protrudingLabelRightPadding = bottomRightCorner.x - bottomRightCornerOriginal.x;
            return std::make_pair(protrudingLabelLeftPadding, protrudingLabelRightPadding);
            };

        if (GetAxisType() == AxisType::LeftYAxis)
            {
            const auto [protrudingLabelTopPadding, protrudingLabelBottomPadding] = calculateVerticalLabelOverhang();

            wxCoord headerPadding = GetHeader().GetText().length() ?
                GetHeader().GetBoundingBox(dc).GetSize().GetHeight() +
                    ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+protrudingLabelTopPadding :
                protrudingLabelTopPadding;
            const wxCoord footerPadding = GetFooter().GetText().length() ?
                GetFooter().GetBoundingBox(dc).GetSize().GetWidth() +
                    ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+protrudingLabelBottomPadding :
                protrudingLabelBottomPadding;
            if (GetAnchoring() == Anchoring::TopLeftCorner ||
                GetAnchoring() == Anchoring::BottomLeftCorner)
                {
                SetPoints(wxPoint(rect.GetTopLeft().x+protrudingBox.GetWidth(),
                                  rect.GetTopLeft().y+headerPadding),
                          wxPoint(rect.GetBottomLeft().x+protrudingBox.GetWidth(),
                                  rect.GetBottomLeft().y-footerPadding),
                          dc);
                }
            else if (GetAnchoring() == Anchoring::Center)
                {
                SetPoints(wxPoint(rect.GetTopLeft().x+((rect.GetWidth()/2)),
                                  rect.GetTopLeft().y+headerPadding),
                          wxPoint(rect.GetBottomLeft().x+((rect.GetWidth()/2)),
                                  rect.GetBottomLeft().y-footerPadding),
                          dc);
                }
            else if (GetAnchoring() == Anchoring::TopRightCorner ||
                     GetAnchoring() == Anchoring::BottomRightCorner)
                {
                SetPoints(wxPoint(rect.GetTopLeft().x+(rect.GetWidth()-(boundingBox.GetWidth()-protrudingBox.GetWidth())),
                                  rect.GetTopLeft().y+headerPadding),
                          wxPoint(rect.GetBottomLeft().x+(rect.GetWidth()-(boundingBox.GetWidth()-protrudingBox.GetWidth())),
                                  rect.GetBottomLeft().y-footerPadding),
                          dc);
                }
            }
        else if (GetAxisType() == AxisType::RightYAxis)
            {
            const auto [protrudingLabelTopPadding, protrudingLabelBottomPadding] = calculateVerticalLabelOverhang();

            wxCoord headerPadding = GetHeader().GetText().length() ?
                GetHeader().GetBoundingBox(dc).GetSize().GetHeight() +
                    ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+protrudingLabelTopPadding :
                protrudingLabelTopPadding;
            const wxCoord footerPadding = GetFooter().GetText().length() ?
                GetFooter().GetBoundingBox(dc).GetSize().GetWidth() +
                    ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+protrudingLabelBottomPadding :
                protrudingLabelBottomPadding;
            if (GetAnchoring() == Anchoring::TopLeftCorner || GetAnchoring() == Anchoring::BottomLeftCorner)
                {
                SetPoints(wxPoint(rect.GetTopLeft().x+(boundingBox.GetWidth()-protrudingBox.GetWidth()),
                                  rect.GetTopLeft().y+headerPadding),
                          wxPoint(rect.GetBottomLeft().x+(boundingBox.GetWidth()-protrudingBox.GetWidth()),
                                  rect.GetBottomLeft().y-footerPadding),
                          dc);
                }
            else if (GetAnchoring() == Anchoring::Center)
                {
                SetPoints(wxPoint(rect.GetTopLeft().x+((rect.GetWidth()/2)),
                                  rect.GetTopLeft().y+headerPadding),
                          wxPoint(rect.GetBottomLeft().x+((rect.GetWidth()/2)),
                                  rect.GetBottomLeft().y-footerPadding),
                          dc);
                }
            else if (GetAnchoring() == Anchoring::TopRightCorner ||
                     GetAnchoring() == Anchoring::BottomRightCorner)
                {
                SetPoints(wxPoint(rect.GetTopLeft().x+(rect.GetWidth()-protrudingBox.GetWidth()),
                                  rect.GetTopLeft().y+headerPadding),
                          wxPoint(rect.GetBottomLeft().x+(rect.GetWidth()-protrudingBox.GetWidth()),
                                  rect.GetBottomLeft().y-footerPadding),
                          dc);
                }
            }
        else if (GetAxisType() == AxisType::BottomXAxis)
            {
            const auto [protrudingLabelLeftPadding, protrudingLabelRightPadding] = calculateHorizontalLabelOverhang();

            const wxCoord headerPadding = GetHeader().GetText().length() ?
                GetHeader().GetBoundingBox(dc).GetSize().GetWidth() +
                    ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+protrudingLabelRightPadding :
                protrudingLabelRightPadding;
            const wxCoord footerPadding = GetFooter().GetText().length() ?
                GetFooter().GetBoundingBox(dc).GetSize().GetWidth() +
                    ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+protrudingLabelLeftPadding :
                protrudingLabelLeftPadding;
            if (GetAnchoring() == Anchoring::TopLeftCorner || GetAnchoring() == Anchoring::BottomLeftCorner)
                {
                SetPoints(wxPoint(rect.GetLeft()+footerPadding,
                                  rect.GetTop()+(boundingBox.GetHeight()-protrudingBox.GetHeight())),
                          wxPoint(rect.GetLeft()+(rect.GetWidth()-headerPadding),
                                  rect.GetTop()+(boundingBox.GetHeight()-protrudingBox.GetHeight())),
                          dc);
                }
            else if (GetAnchoring() == Anchoring::Center)
                {
                SetPoints(wxPoint(rect.GetLeft()+footerPadding,
                                  rect.GetTopLeft().y+((rect.GetHeight()/2))),
                          wxPoint(rect.GetRight()-headerPadding,
                                  rect.GetTopLeft().y+((rect.GetHeight()/2))),
                          dc);
                }
            else if (GetAnchoring() == Anchoring::TopRightCorner ||
                     GetAnchoring() == Anchoring:: BottomRightCorner)
                {
                SetPoints(wxPoint(rect.GetLeft()+footerPadding,
                                  rect.GetBottom()-protrudingBox.GetHeight()),
                          wxPoint(rect.GetLeft()+(rect.GetWidth()-headerPadding),
                                  rect.GetBottom()-protrudingBox.GetHeight()),
                          dc);
                }
            }
        else if (GetAxisType() == AxisType::TopXAxis)
            {
            const auto [protrudingLabelLeftPadding, protrudingLabelRightPadding] = calculateHorizontalLabelOverhang();

            const wxCoord headerPadding = GetHeader().GetText().length() ?
                GetHeader().GetBoundingBox(dc).GetSize().GetWidth() +
                    ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+protrudingLabelRightPadding :
                protrudingLabelRightPadding;
            const wxCoord footerPadding = GetFooter().GetText().length() ?
                GetFooter().GetBoundingBox(dc).GetSize().GetWidth() +
                    ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+protrudingLabelLeftPadding :
                protrudingLabelLeftPadding;
            if (GetAnchoring() == Anchoring::TopLeftCorner ||
                GetAnchoring() == Anchoring::BottomLeftCorner)
                {
                SetPoints(wxPoint(rect.GetLeft()+footerPadding,
                                  rect.GetTop()+protrudingBox.GetHeight()),
                          wxPoint(rect.GetLeft()+(rect.GetWidth()-headerPadding),
                                  rect.GetTop()+protrudingBox.GetHeight()),
                          dc);
                }
            else if (GetAnchoring() == Anchoring::Center)
                {
                SetPoints(wxPoint(rect.GetLeft()+footerPadding,
                                  rect.GetTopLeft().y+((rect.GetHeight()/2))),
                          wxPoint(rect.GetRight()-headerPadding,
                                   rect.GetTopLeft().y+((rect.GetHeight()/2))),
                          dc);
                }
            else if (GetAnchoring() == Anchoring::TopRightCorner ||
                     GetAnchoring() == Anchoring::BottomRightCorner)
                {
                SetPoints(wxPoint(rect.GetLeft()+footerPadding,
                                  rect.GetBottom()-(boundingBox.GetHeight()-protrudingBox.GetHeight())),
                          wxPoint(rect.GetLeft()+(rect.GetWidth()-headerPadding),
                                  rect.GetBottom()-(boundingBox.GetHeight()-protrudingBox.GetHeight())),
                          dc);
                }
            }

        // Force titles to fit in this area in the next call to GetBoundingBox().
        // Because axis label scaling dynamically changes to the best fit, we need to
        // clip titles within the size used here later.
        if (IsHorizontal())
            { m_maxHeight = rect.GetHeight(); }
        else if (IsVertical())
            { m_maxWidth = rect.GetWidth(); }
        }

    wxRect Axis::GetProtrudingBoundingBox(wxDC& dc) const
        {
        wxRect boudingBox = GetBoundingBox(dc);
        if (GetAxisType() == AxisType::LeftYAxis)
            { boudingBox.SetRight(GetTopPoint().x); }
        else if (GetAxisType() == AxisType::RightYAxis)
            {
            const auto diff = GetTopPoint().x - boudingBox.GetLeft();
            boudingBox.SetWidth(boudingBox.GetWidth() - diff);
            boudingBox.SetLeft(GetTopPoint().x);
            }
        else if (GetAxisType() == AxisType::BottomXAxis)
            {
            const auto diff = GetTopPoint().y - boudingBox.GetTop();
            boudingBox.SetTop(GetTopPoint().y);
            boudingBox.SetHeight(boudingBox.GetHeight()-diff);
            }
        else if (GetAxisType() == AxisType::TopXAxis)
            { boudingBox.SetBottom(GetTopPoint().y); }
        return boudingBox;
        }

    //-------------------------------------------
    void Axis::CalcHorizontalLabelOverhang(wxDC& dc, wxPoint& topLeftCorner, wxPoint& bottomRightCorner) const
        {
        // the first (far most left) axis label
        const auto [firstLabel, firstLabelPosition] = GetFirstDisplayedLabel(dc);
        wxCoord firstLabelPhysicalPos{ 0 };
        if (!std::isnan(firstLabelPosition) && GetPhysicalCoordinate(firstLabelPosition, firstLabelPhysicalPos))
            {
            const auto spaceToStart = firstLabelPhysicalPos - GetLeftPoint().x;
            if (GetAxisLabelOrientation() == AxisLabelOrientation::Parallel)
                {
                if (GetParallelLabelAlignment() == RelativeAlignment::FlushRight)
                    { topLeftCorner.x -= firstLabel.GetBoundingBox(dc).GetWidth()-spaceToStart; }
                else if (GetParallelLabelAlignment() == RelativeAlignment::Centered)
                    { topLeftCorner.x -= (firstLabel.GetBoundingBox(dc).GetWidth()/2)-spaceToStart; }
                // FlushLeft needs no space on the left outer side
                }
            else
                {
                topLeftCorner.x -= (firstLabel.GetBoundingBox(dc).GetHeight()/2)-spaceToStart;
                }
            }
        // the last (far most right) axis label
        const auto [lastLabel, lastLabelPosition] = GetLastDisplayedLabel(dc);
        wxCoord lastLabelPhysicalPos{ 0 };
        if (!std::isnan(lastLabelPosition) && GetPhysicalCoordinate(lastLabelPosition, lastLabelPhysicalPos))
            {
            const auto spaceToEnd = GetRightPoint().x - lastLabelPhysicalPos;
            if (GetAxisLabelOrientation() == AxisLabelOrientation::Parallel)
                {
                if (GetParallelLabelAlignment() == RelativeAlignment::FlushLeft)
                    { bottomRightCorner.x += lastLabel.GetBoundingBox(dc).GetWidth()-spaceToEnd; }
                else if (GetParallelLabelAlignment() == RelativeAlignment::Centered)
                    { bottomRightCorner.x += (lastLabel.GetBoundingBox(dc).GetWidth()/2)-spaceToEnd; }
                // FlushRight needs no space on the right outer side
                }
            else
                {
                bottomRightCorner.x += (lastLabel.GetBoundingBox(dc).GetHeight()/2)-spaceToEnd;
                }
            }
        }

    //-------------------------------------------
    void Axis::CalcVerticalLabelOverhang(wxDC& dc, wxPoint& topLeftCorner, wxPoint& bottomRightCorner) const
        {
        // the first (far most bottom) axis label
        const auto [firstLabel, firstLabelPosition] = GetFirstDisplayedLabel(dc);
        wxCoord firstLabelPhysicalPos{ 0 };
        if (!std::isnan(firstLabelPosition) && GetPhysicalCoordinate(firstLabelPosition, firstLabelPhysicalPos))
            {
            const auto spaceToStart = GetBottomPoint().y - firstLabelPhysicalPos;
            if (GetAxisLabelOrientation() == AxisLabelOrientation::Parallel)
                {
                if (GetParallelLabelAlignment() == RelativeAlignment::FlushBottom)
                    { bottomRightCorner.y += firstLabel.GetBoundingBox(dc).GetWidth()-spaceToStart; }
                else if (GetParallelLabelAlignment() == RelativeAlignment::Centered)
                    { bottomRightCorner.y += (firstLabel.GetBoundingBox(dc).GetWidth()/2)-spaceToStart; }
                // FlushTop needs no space on the upper outside
                }
            else
                {
                bottomRightCorner.y += (firstLabel.GetBoundingBox(dc).GetHeight()/2)-spaceToStart;
                }
            }
        // the last (far most top) axis label
        const auto [lastLabel, lastLabelPosition] = GetLastDisplayedLabel(dc);
        wxCoord lastLabelPhysicalPos{ 0 };
        if (!std::isnan(lastLabelPosition) && GetPhysicalCoordinate(lastLabelPosition, lastLabelPhysicalPos))
            {
            const auto spaceToEnd = lastLabelPhysicalPos - GetTopPoint().y;
            if (GetAxisLabelOrientation() == AxisLabelOrientation::Parallel)
                {
                if (GetParallelLabelAlignment() == RelativeAlignment::FlushTop)
                    { topLeftCorner.y -= lastLabel.GetBoundingBox(dc).GetWidth(); }
                else if (GetParallelLabelAlignment() == RelativeAlignment::Centered)
                    { topLeftCorner.y -= lastLabel.GetBoundingBox(dc).GetWidth()/2; }
                // FlushBottom needs no space on the lower outer side
                }
            else
                {
                topLeftCorner.y -= (lastLabel.GetBoundingBox(dc).GetHeight()/2)-spaceToEnd;
                }
            }
        }

    //-------------------------------------------
    wxRect Axis::GetBoundingBox(wxDC& dc) const
        {
        const auto textMeasurement = [this, &dc]()
            {
            if (!IsShowingLabels())
                { return 0; }
            else
                {
                if (GetAxisLabelOrientation() == AxisLabelOrientation::Perpendicular)
                    {
                    return GetWidestTextLabel(dc).GetBoundingBox(dc).GetWidth();
                    }
                else
                    {
                    // if using blocked backgrounds on the axis labels, then account for
                    // the padding above and below the label that that would add
                    auto tallestLabel{ GetTallestTextLabel(dc) };
                    auto tallestLabelHeight = tallestLabel.GetBoundingBox(dc).GetHeight();
                    if (GetFontBackgroundColor().IsOk())
                        { tallestLabelHeight += std::max(2, GetTopPadding()+GetBottomPadding()); }
                    return tallestLabelHeight;
                    }
                }
            }();
        wxPoint topLeftCorner, bottomRightCorner;
        const wxCoord spaceBetweenAxisAndLabel = !IsShowingLabels() ?
            0 : ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine());

        if (GetAxisType() == AxisType::LeftYAxis)
            {
            topLeftCorner = wxPoint(GetTopPoint().x-(CalcTickMarkOuterWidth()+spaceBetweenAxisAndLabel), GetTopPoint().y);
            bottomRightCorner = wxPoint(GetBottomPoint().x +
                (CalcTickMarkInnerWidth()+(HasDoubleSidedAxisLabels() ? spaceBetweenAxisAndLabel : 0)),
                GetBottomPoint().y);//the line, tickmarks, and space after that
            if (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::CenterOnAxisLine)
                { bottomRightCorner.x += textMeasurement/2;}
            topLeftCorner.x -= (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::CenterOnAxisLine) ?
                                0 :
                                IsStackingLabels() ? textMeasurement*2 : textMeasurement;
            if (HasDoubleSidedAxisLabels() && (GetPerpendicularLabelAxisAlignment() != AxisLabelAlignment::CenterOnAxisLine))
                { bottomRightCorner.x += IsStackingLabels() ? textMeasurement*2 : textMeasurement; }

            CalcVerticalLabelOverhang(dc, topLeftCorner, bottomRightCorner);

            if (GetBrackets().size())
                {
                topLeftCorner.x -= CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine());
                if (HasDoubleSidedAxisLabels())
                    { bottomRightCorner.x += CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()); }
                }
            if (GetTitle().IsShown() && GetTitle().GetText().length())
                {
                auto title{ GetTitle() };
                title.SetScaling(GetScaling());
                topLeftCorner.x -= title.GetBoundingBox(dc).GetWidth() +
                                   ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine());
                // title is NOT drawn on the inside if axis is double sided,
                // that wouldn't really make sense
                }
            }
        else if (GetAxisType() == AxisType::RightYAxis)
            {
            topLeftCorner = wxPoint(GetTopPoint().x, GetTopPoint().y);
            if (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::CenterOnAxisLine)
                { topLeftCorner.x -= textMeasurement; }
            else
                {
                topLeftCorner.x -= (CalcTickMarkInnerWidth()+(HasDoubleSidedAxisLabels() ? spaceBetweenAxisAndLabel : 0));
                }

            bottomRightCorner = wxPoint(GetBottomPoint().x+(CalcTickMarkOuterWidth()+spaceBetweenAxisAndLabel),
                                        GetBottomPoint().y);//the line, tickmarks, and space after that
            bottomRightCorner.x += (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::CenterOnAxisLine) ?
                                    0 :
                                    IsStackingLabels() ? textMeasurement*2 : textMeasurement;
            if (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::CenterOnAxisLine)
                { bottomRightCorner.x += textMeasurement/2;}
            if (HasDoubleSidedAxisLabels() && (GetPerpendicularLabelAxisAlignment() != AxisLabelAlignment::CenterOnAxisLine))
                { topLeftCorner.x -= IsStackingLabels() ? textMeasurement*2 : textMeasurement; }

            CalcVerticalLabelOverhang(dc, topLeftCorner, bottomRightCorner);

            if (GetBrackets().size())
                {
                bottomRightCorner.x += CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine());
                if (HasDoubleSidedAxisLabels())
                    { topLeftCorner.x -= CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()); }
                }
            if (GetTitle().IsShown() && GetTitle().GetText().length())
                {
                auto title{ GetTitle() };
                title.SetScaling(GetScaling());
                bottomRightCorner.x += title.GetBoundingBox(dc).GetWidth() +
                                       ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine());
                // title is NOT drawn on the inside if axis is double sided,
                // that wouldn't really make sense
                }
            }
        else if (GetAxisType() == AxisType::BottomXAxis)
            {
            topLeftCorner = wxPoint(GetTopPoint().x,
                                    GetTopPoint().y - (CalcTickMarkInnerWidth()+(HasDoubleSidedAxisLabels() ?
                                        spaceBetweenAxisAndLabel : 0)));
            bottomRightCorner = wxPoint(GetBottomPoint().x,
                                        // include the line, tickmarks, and space after that
                                        GetBottomPoint().y + (CalcTickMarkOuterWidth()+spaceBetweenAxisAndLabel));
            bottomRightCorner.y += IsStackingLabels() ? textMeasurement*2 : textMeasurement;
            if (HasDoubleSidedAxisLabels())
                { topLeftCorner.y -= IsStackingLabels() ? textMeasurement*2 : textMeasurement; }

            CalcHorizontalLabelOverhang(dc, topLeftCorner, bottomRightCorner);

            if (GetBrackets().size())
                {
                bottomRightCorner.y += CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine());
                if (HasDoubleSidedAxisLabels())
                    { topLeftCorner.y -= CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()); }
                }
            if (GetTitle().IsShown() && GetTitle().GetText().length())
                {
                auto title{ GetTitle() };
                title.SetScaling(GetScaling());
                bottomRightCorner.y += title.GetBoundingBox(dc).GetHeight() +
                                       ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine());
                // title is NOT drawn on the inside if axis is double sided,
                // that wouldn't really make sense
                }
            }
        else if (GetAxisType() == AxisType::TopXAxis)
            {
            topLeftCorner = wxPoint(GetTopPoint().x, GetTopPoint().y-(CalcTickMarkOuterWidth()+spaceBetweenAxisAndLabel));
            bottomRightCorner = wxPoint(GetBottomPoint().x, GetBottomPoint().y+(CalcTickMarkInnerWidth()+(HasDoubleSidedAxisLabels() ? spaceBetweenAxisAndLabel : 0)));//the line, tickmarks, and space after that
            topLeftCorner.y -= IsStackingLabels() ? textMeasurement*2 : textMeasurement;
            if (HasDoubleSidedAxisLabels())
                { bottomRightCorner.y += IsStackingLabels() ? textMeasurement*2 : textMeasurement; }

            CalcHorizontalLabelOverhang(dc, topLeftCorner, bottomRightCorner);

            if (GetBrackets().size())
                {
                topLeftCorner.y -= CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine());
                if (HasDoubleSidedAxisLabels())
                    { bottomRightCorner.y += CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()); }
                }
            if (GetTitle().IsShown() && GetTitle().GetText().length())
                {
                auto title{ GetTitle() };
                title.SetScaling(GetScaling());
                topLeftCorner.y -= title.GetBoundingBox(dc).GetHeight() +
                                   ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine());
                // title is NOT drawn on the inside if axis is double sided,
                // that wouldn't really make sense
                }
            }

        // headers and footers (these aren't on a particular side of an axis,
        // so simple vertical vs. horizontal logic is fine here)
        if (IsVertical())
            {
            const auto adjustMarginForHeaderOrFooter = [this, &dc, &bottomRightCorner, &topLeftCorner]
                (Label label, const bool isHeader)
                {
                label.SetAnchorPoint(isHeader ? GetTopPoint() : GetBottomPoint());
                label.SetScaling(GetScaling());
                label.SetAnchoring(Anchoring::Center);
                wxRect labelBox = label.GetBoundingBox(dc);
                if (isHeader)
                    {
                    topLeftCorner.y -= labelBox.GetSize().GetHeight() + ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine());
                    }
                else
                    {
                    bottomRightCorner.y += labelBox.GetSize().GetHeight() + ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine());
                    }

                // move the label left or right, depending on its placement
                if (label.GetRelativeAlignment() == RelativeAlignment::FlushRight)
                    { labelBox.Offset(-(labelBox.GetWidth()/2), 0); }
                else if (label.GetRelativeAlignment() == RelativeAlignment::FlushLeft)
                    { labelBox.Offset((labelBox.GetWidth()/2), 0); }
                // see if the left or right hinges of this box goes outside of the current bounding box
                if (labelBox.GetLeft() < topLeftCorner.x)
                    { topLeftCorner.x = labelBox.GetLeft(); }
                if (labelBox.GetRight() > bottomRightCorner.x)
                    { bottomRightCorner.x = labelBox.GetRight(); }
                };

            if (GetHeader().GetText().length())
                { adjustMarginForHeaderOrFooter(Label(GetHeader()), true); }
            if (GetFooter().GetText().length())
                { adjustMarginForHeaderOrFooter(Label(GetFooter()), false); }
            }
        else if (IsHorizontal())
            {
            const auto adjustMarginForHeaderOrFooter = [this, &dc, &bottomRightCorner, &topLeftCorner]
                (Label label, const bool isHeader)
                {
                label.SetAnchorPoint(isHeader ? GetRightPoint() : GetLeftPoint());
                label.SetScaling(GetScaling());
                label.SetAnchoring(Anchoring::Center);
                wxRect labelBox = label.GetBoundingBox(dc);
                if (isHeader)
                    {
                    // push over the x to fit the header
                    bottomRightCorner.x += labelBox.GetSize().GetWidth() + ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine());
                    }
                else
                    {
                    // push over the x to fit the footer
                    topLeftCorner.x -= labelBox.GetSize().GetWidth() + ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine());
                    }
                // move the label up or down, depending on its placement
                if (label.GetRelativeAlignment() == RelativeAlignment::FlushBottom)
                    { labelBox.Offset(0, labelBox.GetHeight()/2); }
                else if (label.GetRelativeAlignment() == RelativeAlignment::FlushTop)
                    { labelBox.Offset(0, -(labelBox.GetHeight()/2)); }
                // adjust y now
                if (labelBox.GetY() < topLeftCorner.y)
                    { topLeftCorner.y = labelBox.GetY(); }
                if (labelBox.GetY()+ labelBox.GetHeight() > bottomRightCorner.y)
                    { bottomRightCorner.y = labelBox.GetY()+ labelBox.GetHeight(); }
                };

            if (GetHeader().IsShown() && GetHeader().GetText().length())
                { adjustMarginForHeaderOrFooter(Label(GetHeader()), true); }
            if (GetFooter().IsShown() && GetFooter().GetText().length())
                { adjustMarginForHeaderOrFooter(Label(GetFooter()), false); }
            }

        wxRect adjustedRect(topLeftCorner, bottomRightCorner);
        // if an axis embedded on a canvas, we may need to shrink it back down to make
        // the titles fit because of how the axis labels and brackets dynamically size
        // (i.e., not using the parent scaling)
        if (m_maxHeight.has_value() &&
            m_maxHeight.value() < adjustedRect.GetHeight())
            { adjustedRect.SetHeight(m_maxHeight.value()); }
        if (m_maxWidth.has_value() &&
            m_maxWidth.value() < adjustedRect.GetWidth())
            { adjustedRect.SetWidth(m_maxWidth.value()); }

        return (GetOutlineSize().IsFullySpecified()) ?
                adjustedRect.Inflate(ScaleToScreenAndCanvas(GetOutlineSize().GetWidth()),
                                                            ScaleToScreenAndCanvas(GetOutlineSize().GetHeight())) :
                adjustedRect;
        }

    //-------------------------------------------
    void Axis::CopySettings(const Axis& that)
        {
        // note that we won't copy over the brackets, title, footer, or header; these are not technically part of the axis
        // (they're an add on), and client may not want that to be copied from axis to axis
        m_axisLabels = that.m_axisLabels;
        m_customAxisLabels = that.m_customAxisLabels;

        m_tickMarks = that.m_tickMarks;
        m_customTickMarks = that.m_customTickMarks;
        m_tickMarkDisplayInterval = that.m_tickMarkDisplayInterval;
        m_minorTickMarkLength = that.m_minorTickMarkLength;
        m_majorTickMarkLength = that.m_majorTickMarkLength;
        m_tickMarkDisplayType = that.m_tickMarkDisplayType;

        m_doubleSidedAxisLabels = that.m_doubleSidedAxisLabels;

        m_labelOrientation = that.m_labelOrientation;
        m_stackLabelsToFit = that.m_stackLabelsToFit;
        m_enableAutoStacking = that.m_enableAutoStacking;
        m_labelSpacingPhysicalOffset = that.m_labelSpacingPhysicalOffset;

        GetFont() = that.GetFont();
        SetFontColor(that.GetFontColor());
        SetFontBackgroundColor(that.GetFontBackgroundColor());
        SetTextAlignment(that.GetTextAlignment());

        m_scaledReserved = that.m_scaledReserved;
        m_startAtZero = that.m_startAtZero;
        m_labelDisplay = that.m_labelDisplay;
        m_axisLabelAlignment = that.m_axisLabelAlignment;
        m_labelAlignmet = that.m_labelAlignmet;
        m_gridlinePen = that.m_gridlinePen;
        m_axisLinePen = that.m_axisLinePen;

        m_rangeStart = that.m_rangeStart;
        m_rangeEnd = that.m_rangeEnd;

        // date info
        m_fyQ1 = that.m_fyQ1;
        m_fyQ2 = that.m_fyQ2;
        m_fyQ3 = that.m_fyQ3;
        m_fyQ4 = that.m_fyQ4;

        m_firstDay = that.m_firstDay;
        m_lastDay = that.m_lastDay;
        m_dateDisplayInterval = that.m_dateDisplayInterval;

        m_customXPosition = that.m_customXPosition;
        m_customYPosition = that.m_customYPosition;
        // these should be invalidated now
        m_physicalCustomYPosition = -1;
        m_physicalCustomXPosition = -1;

        m_displayPrecision = that.m_displayPrecision;
        m_interval = that.m_interval;
        m_displayInterval = that.m_displayInterval;
        }

    //-------------------------------------------
    void Axis::CalcTickMarkPositions()
        {
        if (IsVertical())
            {
            auto pos = GetAxisPoints().begin();
            for (size_t i = 0; i < GetAxisPointsCount(); ++i, ++pos)
                {
                pos->SetPhysicalCoordinate(
                    static_cast<double>(static_cast<double>(GetTopPoint().y)+
                                                            (static_cast<double>(GetBottomPoint().y)-GetTopPoint().y))
                                        - static_cast<double>(GetLabelPhysicalOffset()*i));
                }
            // tick marks
            const auto [start, end] = GetRange();
            GetTickMarks().clear();
            double i = start;
            for (/*already initialized*/;
                (i <= end);
                i += GetTickMarkInterval())
                {
                const bool MajorTickMark = PointHasLabel(i);
                TickMark tk(GetTickMarkDisplay(), i,
                    MajorTickMark ? GetMajorTickMarkLength() : GetMinorTickMarkLength());
                GetPhysicalCoordinate(tk.m_position, tk.m_physicalCoordinate);
                GetTickMarks().push_back(tk);
                }
            // custom tick marks
            for (auto& tick : GetCustomTickMarks())
                { GetPhysicalCoordinate(tick.m_position, tick.m_physicalCoordinate); }
            }
        else if (IsHorizontal())
            {
            auto pos = GetAxisPoints().begin();
            for (size_t i = 0; i < GetAxisPointsCount(); ++i, ++pos)
                {
                pos->SetPhysicalCoordinate(GetLeftPoint().x+(GetLabelPhysicalOffset()*i));
                }
            // tick marks
            const auto [start, end] = GetRange();
            GetTickMarks().clear();
            double i = start;
            for (/*already initialized*/;
                (i <= end);
                i += GetTickMarkInterval())
                {
                const bool MajorTickMark = PointHasLabel(i);
                TickMark tk(GetTickMarkDisplay(), i,
                    MajorTickMark ? GetMajorTickMarkLength() : GetMinorTickMarkLength());
                GetPhysicalCoordinate(tk.m_position, tk.m_physicalCoordinate);
                GetTickMarks().push_back(tk);
                }
            // custom tick marks
            for (auto& tick : GetCustomTickMarks())
                { GetPhysicalCoordinate(tick.m_position, tick.m_physicalCoordinate); }
            }
        }

    //-------------------------------------------
    void Axis::CalcLabelPositions() noexcept
        {
        if (IsVertical())
            { SetLabelPhysicalOffset(safe_divide<double>(std::abs(GetBottomPoint().y-GetTopPoint().y), (GetAxisPointsCount()-1))); }
        else if (IsHorizontal())
            { SetLabelPhysicalOffset(safe_divide<double>(std::abs(GetRightPoint().x-GetLeftPoint().x), (GetAxisPointsCount()-1))); }
        }

    //---------------------------------------------------------------
    wxRect Axis::Draw(wxDC& dc) const
        {
        if (!IsShown())
            { return wxRect(); }

        const wxRect axisRect = GetBoundingBox(dc);

        wxPen axisPen(GetAxisLinePen());
        if (GetAxisLinePen().IsOk())
            {
            // if adding a terminal arrow to the axis line and the line is thin, then make
            // it a little thicker to match the arrowhead
            if (GetCapStyle() == AxisCapStyle::Arrow && GetAxisLinePen().GetWidth() == 1)
                { axisPen.SetWidth(ScaleToScreenAndCanvas(2)); }
            else
                { axisPen.SetWidth(ScaleToScreenAndCanvas(GetAxisLinePen().GetWidth())); }
            }
        wxPen tickMarkPen(axisPen);

        if (GetAxisType() == AxisType::LeftYAxis && GetTickMarkDisplay() != TickMark::DisplayType::NoDisplay)
            {
            wxDCPenChanger dcPenCh(dc, tickMarkPen);
            // regular tick marks
            for (auto tickIter = GetTickMarks().cbegin();
                tickIter != GetTickMarks().cend();
                ++tickIter)
                {
                if (tickIter->GetTickMarkDisplay() == TickMark::DisplayType::Inner)
                    {
                    dc.DrawLine(GetTopPoint().x, tickIter->m_physicalCoordinate,
                        (GetTopPoint().x+ScaleToScreenAndCanvas(tickIter->m_lineLength)), tickIter->m_physicalCoordinate);
                    }
                else if (tickIter->GetTickMarkDisplay() == TickMark::DisplayType::Outer)
                    {
                    dc.DrawLine(GetTopPoint().x-ScaleToScreenAndCanvas(tickIter->m_lineLength), tickIter->m_physicalCoordinate,
                        GetTopPoint().x, tickIter->m_physicalCoordinate);
                    }
                else if (tickIter->GetTickMarkDisplay() == TickMark::DisplayType::Crossed)
                    {
                    dc.DrawLine(GetTopPoint().x-ScaleToScreenAndCanvas(tickIter->m_lineLength), tickIter->m_physicalCoordinate,
                        (GetTopPoint().x+ScaleToScreenAndCanvas(tickIter->m_lineLength)), tickIter->m_physicalCoordinate);
                    }
                }
            // custom tick marks
            for (auto tickIter = GetCustomTickMarks().cbegin();
                tickIter != GetCustomTickMarks().cend();
                ++tickIter)
                {
                if (tickIter->m_displayType == TickMark::DisplayType::Inner)
                    {
                    dc.DrawLine(GetTopPoint().x, tickIter->m_physicalCoordinate,
                        (GetTopPoint().x+ScaleToScreenAndCanvas(tickIter->m_lineLength)), tickIter->m_physicalCoordinate);
                    }
                else if (tickIter->m_displayType == TickMark::DisplayType::Outer)
                    {
                    dc.DrawLine(GetTopPoint().x-ScaleToScreenAndCanvas(tickIter->m_lineLength), tickIter->m_physicalCoordinate,
                        GetTopPoint().x, tickIter->m_physicalCoordinate);
                    }
                else if (tickIter->m_displayType == TickMark::DisplayType::Crossed)
                    {
                    dc.DrawLine(GetTopPoint().x-ScaleToScreenAndCanvas(tickIter->m_lineLength), tickIter->m_physicalCoordinate,
                        (GetTopPoint().x+ScaleToScreenAndCanvas(tickIter->m_lineLength)), tickIter->m_physicalCoordinate);
                    }
                }
            }
        else if (GetAxisType() == AxisType::RightYAxis && GetTickMarkDisplay() != TickMark::DisplayType::NoDisplay)
            {
            wxDCPenChanger dcPenCh(dc, tickMarkPen);
            // regular tick marks
            for (auto tickIter = GetTickMarks().cbegin();
                tickIter != GetTickMarks().cend();
                ++tickIter)
                {
                if (tickIter->GetTickMarkDisplay() == TickMark::DisplayType::Inner)
                    {
                    dc.DrawLine(GetTopPoint().x, tickIter->m_physicalCoordinate,
                        ((GetTopPoint().x)-ScaleToScreenAndCanvas(tickIter->m_lineLength)), tickIter->m_physicalCoordinate);
                    }
                else if (tickIter->GetTickMarkDisplay() == TickMark::DisplayType::Outer)
                    {
                    dc.DrawLine((GetTopPoint().x)-ScaleToScreenAndCanvas(tickIter->m_lineLength), tickIter->m_physicalCoordinate,
                        (GetTopPoint().x), tickIter->m_physicalCoordinate);
                    }
                else if (tickIter->GetTickMarkDisplay() == TickMark::DisplayType::Crossed)
                    {
                    dc.DrawLine((GetTopPoint().x)-ScaleToScreenAndCanvas(tickIter->m_lineLength), tickIter->m_physicalCoordinate,
                        ((GetTopPoint().x)+ScaleToScreenAndCanvas(tickIter->m_lineLength)), tickIter->m_physicalCoordinate);
                    }
                }
            // custom tick marks
            for (auto tickIter = GetCustomTickMarks().cbegin();
                tickIter != GetCustomTickMarks().cend();
                ++tickIter)
                {
                if (tickIter->m_displayType == TickMark::DisplayType::Inner)
                    {
                    dc.DrawLine(GetTopPoint().x, tickIter->m_physicalCoordinate,
                        ((GetTopPoint().x)-ScaleToScreenAndCanvas(tickIter->m_lineLength)), tickIter->m_physicalCoordinate);
                    }
                else if (tickIter->m_displayType == TickMark::DisplayType::Outer)
                    {
                    dc.DrawLine(GetTopPoint().x, tickIter->m_physicalCoordinate,
                        ((GetTopPoint().x)+ScaleToScreenAndCanvas(tickIter->m_lineLength)), tickIter->m_physicalCoordinate);
                    }
                else if (tickIter->m_displayType == TickMark::DisplayType::Crossed)
                    {
                    dc.DrawLine((GetTopPoint().x)-ScaleToScreenAndCanvas(tickIter->m_lineLength), tickIter->m_physicalCoordinate,
                        ((GetTopPoint().x)+ScaleToScreenAndCanvas(tickIter->m_lineLength)), tickIter->m_physicalCoordinate);
                    }
                }
            }
        else if (GetAxisType() == AxisType::BottomXAxis && GetTickMarkDisplay() != TickMark::DisplayType::NoDisplay)
            {
            wxDCPenChanger dcPenCh(dc, tickMarkPen);
            // regular tick marks
            for (auto tickIter = GetTickMarks().cbegin();
                tickIter != GetTickMarks().cend();
                ++tickIter)
                {
                if (tickIter->GetTickMarkDisplay() == TickMark::DisplayType::Inner)
                    {
                    dc.DrawLine(tickIter->m_physicalCoordinate,
                        (GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y)),
                        tickIter->m_physicalCoordinate,
                        ((GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y))-ScaleToScreenAndCanvas(tickIter->m_lineLength)));
                    }
                else if (tickIter->GetTickMarkDisplay() == TickMark::DisplayType::Outer)
                    {
                    dc.DrawLine(tickIter->m_physicalCoordinate,
                        (GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y)),
                        tickIter->m_physicalCoordinate,
                        ((GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y))+ScaleToScreenAndCanvas(tickIter->m_lineLength)));
                    }
                else if (tickIter->GetTickMarkDisplay() == TickMark::DisplayType::Crossed)
                    {
                    dc.DrawLine(tickIter->m_physicalCoordinate,
                        (GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y))-ScaleToScreenAndCanvas(tickIter->m_lineLength),
                        tickIter->m_physicalCoordinate,
                        ((GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y))+ScaleToScreenAndCanvas(tickIter->m_lineLength)));
                    }
                }
            // custom tick marks
            for (auto tickIter = GetCustomTickMarks().cbegin();
                tickIter != GetCustomTickMarks().cend();
                ++tickIter)
                {
                if (tickIter->m_physicalCoordinate != -1)
                    {
                    if (tickIter->m_displayType == TickMark::DisplayType::Inner)
                        {
                        dc.DrawLine(tickIter->m_physicalCoordinate,
                            (GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y)),
                            tickIter->m_physicalCoordinate,
                            ((GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y))-ScaleToScreenAndCanvas(tickIter->m_lineLength)));
                        }
                    else if (tickIter->m_displayType == TickMark::DisplayType::Outer)
                        {
                        dc.DrawLine(tickIter->m_physicalCoordinate,
                            (GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y)),
                            tickIter->m_physicalCoordinate,
                            ((GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y))+ScaleToScreenAndCanvas(tickIter->m_lineLength)));
                        }
                    else if (tickIter->m_displayType == TickMark::DisplayType::Crossed)
                        {
                        dc.DrawLine(tickIter->m_physicalCoordinate,
                            (GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y))-ScaleToScreenAndCanvas(tickIter->m_lineLength),
                            tickIter->m_physicalCoordinate,
                            ((GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y))+ScaleToScreenAndCanvas(tickIter->m_lineLength)));
                        }
                    }
                }
            }
        else if (GetAxisType() == AxisType::TopXAxis && GetTickMarkDisplay() != TickMark::DisplayType::NoDisplay)
            {
            wxDCPenChanger dcPenCh(dc, tickMarkPen);
            // regular tick marks
            for (auto tickIter = GetTickMarks().cbegin();
                tickIter != GetTickMarks().cend();
                ++tickIter)
                {
                if (tickIter->GetTickMarkDisplay() == TickMark::DisplayType::Inner)
                    {
                    dc.DrawLine(tickIter->m_physicalCoordinate,
                        GetTopPoint().y,
                        tickIter->m_physicalCoordinate,
                        (GetTopPoint().y+ScaleToScreenAndCanvas(tickIter->m_lineLength)));
                    }
                else if (tickIter->GetTickMarkDisplay() == TickMark::DisplayType::Outer)
                    {
                    dc.DrawLine(tickIter->m_physicalCoordinate,
                        GetTopPoint().y,
                        tickIter->m_physicalCoordinate,
                        (GetTopPoint().y-ScaleToScreenAndCanvas(tickIter->m_lineLength)));
                    }
                else if (tickIter->GetTickMarkDisplay() == TickMark::DisplayType::Crossed)
                    {
                    dc.DrawLine(tickIter->m_physicalCoordinate,
                        (GetTopPoint().y+ScaleToScreenAndCanvas(tickIter->m_lineLength)),
                        tickIter->m_physicalCoordinate,
                        (GetTopPoint().y-ScaleToScreenAndCanvas(tickIter->m_lineLength)));
                    }
                }
            // custom tick marks
            for (auto tickIter = GetCustomTickMarks().cbegin();
                tickIter != GetCustomTickMarks().cend();
                ++tickIter)
                {
                if (tickIter->m_physicalCoordinate != -1)
                    {
                    if (tickIter->m_displayType == TickMark::DisplayType::Inner)
                        {
                        dc.DrawLine(tickIter->m_physicalCoordinate,
                            GetTopPoint().y,
                            tickIter->m_physicalCoordinate,
                            (GetTopPoint().y+ScaleToScreenAndCanvas(tickIter->m_lineLength)));
                        }
                    else if (tickIter->m_displayType == TickMark::DisplayType::Outer)
                        {
                        dc.DrawLine(tickIter->m_physicalCoordinate,
                            GetTopPoint().y,
                            tickIter->m_physicalCoordinate,
                            (GetTopPoint().y-ScaleToScreenAndCanvas(tickIter->m_lineLength)));
                        }
                    else if (tickIter->m_displayType == TickMark::DisplayType::Crossed)
                        {
                        dc.DrawLine(tickIter->m_physicalCoordinate,
                            (GetTopPoint().y+ScaleToScreenAndCanvas(tickIter->m_lineLength)),
                            tickIter->m_physicalCoordinate,
                            (GetTopPoint().y-ScaleToScreenAndCanvas(tickIter->m_lineLength)));
                        }
                    }
                }
            }

        // draw the main line
        if (GetAxisLinePen().IsOk())
            {
            wxDCPenChanger dcPenCh(dc, axisPen);
            if (GetCapStyle() == AxisCapStyle::Arrow)
                {
                const auto arrowHeadSize = wxSize(ScaleToScreenAndCanvas(10*GetAxisLinePen().GetWidth()),
                                                  ScaleToScreenAndCanvas(10*GetAxisLinePen().GetWidth()));
                if (IsVertical())
                    {
                    GraphItems::Polygon::DrawArrow(dc, GetBottomPoint(), GetTopPoint(), arrowHeadSize);
                    }
                else
                    {
                    GraphItems::Polygon::DrawArrow(dc, GetLeftPoint(), GetRightPoint(), arrowHeadSize);
                    }
                }
            else
                { dc.DrawLine(GetTopPoint(), GetBottomPoint()); }
            }

        // draw the header
        if (GetHeader().GetText().length())
            {
            Label header(GetHeader());
            header.SetScaling(GetScaling());
            const wxSize headerSize = header.GetBoundingBox(dc).GetSize();
            if (IsVertical())
                {
                if (header.GetRelativeAlignment() == RelativeAlignment::FlushRight)
                    {
                    header.SetAnchoring(Anchoring::TopLeftCorner);
                    header.SetAnchorPoint(axisRect.GetTopLeft());
                    }
                else if (header.GetRelativeAlignment() == RelativeAlignment::FlushLeft)
                    {
                    header.SetAnchoring(Anchoring::TopRightCorner);
                    header.SetAnchorPoint(axisRect.GetTopRight());
                    }
                else if (header.GetRelativeAlignment() == RelativeAlignment::Centered)
                    {
                    header.SetAnchoring(Anchoring::Center);
                    header.SetAnchorPoint(wxPoint(GetTopPoint().x,
                                    axisRect.GetTop() + ((headerSize.GetHeight()/2))));
                    }
                }
            else if (IsHorizontal())
                {
                if (header.GetRelativeAlignment() == RelativeAlignment::FlushBottom)
                    {
                    header.SetAnchoring(Anchoring::BottomRightCorner);
                    header.SetAnchorPoint(axisRect.GetBottomRight());
                    }
                else if (header.GetRelativeAlignment() == RelativeAlignment::FlushTop)
                    {
                    header.SetAnchoring(Anchoring::TopRightCorner);
                    header.SetAnchorPoint(axisRect.GetTopRight());
                    }
                else if (header.GetRelativeAlignment() == RelativeAlignment::Centered)
                    {
                    header.SetAnchoring(Anchoring::Center);
                    header.SetAnchorPoint(wxPoint(GetTopPoint().x,
                                          axisRect.GetTopRight().y+(axisRect.GetHeight()/2)));
                    }
                }
            header.Draw(dc);
            }

        // draw the footer
        if (GetFooter().GetText().length())
            {
            Label footer(GetFooter());
            footer.SetScaling(GetScaling());
            const wxSize footerSize = footer.GetBoundingBox(dc).GetSize();
            if (IsVertical())
                {
                if (footer.GetRelativeAlignment() == RelativeAlignment::FlushRight)
                    {
                    footer.SetAnchoring(Anchoring::BottomLeftCorner);
                    footer.SetAnchorPoint(axisRect.GetBottomLeft());
                    }
                else if (footer.GetRelativeAlignment() == RelativeAlignment::FlushLeft)
                    {
                    footer.SetAnchoring(Anchoring::BottomRightCorner);
                    footer.SetAnchorPoint(axisRect.GetBottomRight());
                    }
                else if (footer.GetRelativeAlignment() == RelativeAlignment::Centered)
                    {
                    footer.SetAnchoring(Anchoring::Center);
                    footer.SetAnchorPoint(wxPoint(GetBottomPoint().x,
                                    axisRect.GetBottom() - (footerSize.GetHeight()/2)));
                    }
                }
            else if (IsHorizontal())
                {
                if (footer.GetRelativeAlignment() == RelativeAlignment::FlushBottom)
                    {
                    footer.SetAnchoring(Anchoring::BottomLeftCorner);
                    footer.SetAnchorPoint(axisRect.GetBottomLeft());
                    }
                else if (footer.GetRelativeAlignment() == RelativeAlignment::FlushTop)
                    {
                    footer.SetAnchoring(Anchoring::TopLeftCorner);
                    footer.SetAnchorPoint(axisRect.GetTopLeft());
                    }
                else if (footer.GetRelativeAlignment() == RelativeAlignment::Centered)
                    {
                    footer.SetAnchoring(Anchoring::Center);
                    footer.SetAnchorPoint(wxPoint(axisRect.GetX() + (footer.GetBoundingBox(dc).GetWidth()/2),
                                          axisRect.GetTopRight().y + (axisRect.GetHeight()/2)));
                    }
                }
            footer.Draw(dc);
            }

        // draw the title
        Label titleLabel = GetTitle();
        if (GetTitle().IsShown() && GetTitle().GetText().length())
            {
            titleLabel.SetScaling(GetScaling());

            if (GetAxisType() == AxisType::LeftYAxis)
                {
                if (titleLabel.GetRelativeAlignment() == RelativeAlignment::FlushBottom)
                    {
                    titleLabel.SetAnchoring(Anchoring::TopLeftCorner);
                    titleLabel.SetAnchorPoint(axisRect.GetBottomLeft());
                    }
                else if (titleLabel.GetRelativeAlignment() == RelativeAlignment::Centered)
                    {
                    titleLabel.SetAnchoring(Anchoring::Center);
                    auto middlePoint = axisRect.GetTopLeft();
                    middlePoint.y += axisRect.GetHeight()/2;
                    middlePoint.x += titleLabel.GetBoundingBox(dc).GetWidth()/2;
                    titleLabel.SetAnchorPoint(middlePoint);
                    }
                else if (titleLabel.GetRelativeAlignment() == RelativeAlignment::FlushTop)
                    {
                    titleLabel.SetAnchoring(Anchoring::TopRightCorner);
                    titleLabel.SetAnchorPoint(axisRect.GetTopLeft());
                    }
                }
            else if (GetAxisType() == AxisType::RightYAxis)
                {
                if (titleLabel.GetRelativeAlignment() == RelativeAlignment::FlushBottom)
                    {
                    titleLabel.SetAnchoring(Anchoring::TopLeftCorner);
                    auto position = axisRect.GetBottomRight();
                    position.x -= titleLabel.GetBoundingBox(dc).GetWidth();
                    titleLabel.SetAnchorPoint(position);
                    }
                else if (titleLabel.GetRelativeAlignment() == RelativeAlignment::Centered)
                    {
                    titleLabel.SetAnchoring(Anchoring::Center);
                    auto middlePoint = axisRect.GetTopRight();
                    middlePoint.y += axisRect.GetHeight()/2;
                    middlePoint.x -= titleLabel.GetBoundingBox(dc).GetWidth()/2;
                    titleLabel.SetAnchorPoint(middlePoint);
                    }
                else if (titleLabel.GetRelativeAlignment() == RelativeAlignment::FlushTop)
                    {
                    titleLabel.SetAnchoring(Anchoring::BottomRightCorner);
                    titleLabel.SetAnchorPoint(axisRect.GetTopRight());
                    }
                }
            else if (GetAxisType() == AxisType::BottomXAxis)
                {
                if (titleLabel.GetRelativeAlignment() == RelativeAlignment::FlushRight)
                    {
                    titleLabel.SetAnchoring(Anchoring::BottomLeftCorner);
                    titleLabel.SetAnchorPoint(axisRect.GetBottomLeft());
                    }
                else if (titleLabel.GetRelativeAlignment() == RelativeAlignment::Centered)
                    {
                    titleLabel.SetAnchoring(Anchoring::Center);
                    auto middlePoint = axisRect.GetBottomLeft();
                    middlePoint.x += axisRect.GetWidth()/2;
                    middlePoint.y -= titleLabel.GetBoundingBox(dc).GetHeight()/2;
                    titleLabel.SetAnchorPoint(middlePoint);
                    }
                else if (titleLabel.GetRelativeAlignment() == RelativeAlignment::FlushLeft)
                    {
                    titleLabel.SetAnchoring(Anchoring::BottomRightCorner);
                    titleLabel.SetAnchorPoint(axisRect.GetBottomRight());
                    }
                }
            else if (GetAxisType() == AxisType::TopXAxis)
                {
                if (titleLabel.GetRelativeAlignment() == RelativeAlignment::FlushRight)
                    {
                    titleLabel.SetAnchoring(Anchoring::TopLeftCorner);
                    titleLabel.SetAnchorPoint(axisRect.GetTopLeft());
                    }
                else if (titleLabel.GetRelativeAlignment() == RelativeAlignment::Centered)
                    {
                    titleLabel.SetAnchoring(Anchoring::Center);
                    auto middlePoint = axisRect.GetTopLeft();
                    middlePoint.x += axisRect.GetWidth()/2;
                    middlePoint.y += titleLabel.GetBoundingBox(dc).GetHeight()/2;
                    titleLabel.SetAnchorPoint(middlePoint);
                    }
                else if (titleLabel.GetRelativeAlignment() == RelativeAlignment::FlushLeft)
                    {
                    titleLabel.SetAnchoring(Anchoring::TopRightCorner);
                    titleLabel.SetAnchorPoint(axisRect.GetTopRight());
                    }
                }

            titleLabel.Draw(dc);
            }

        // draw the brackets
        for (const auto& bracket : GetBrackets())
            {
            wxPen scaledPen = bracket.GetLinePen();
            if (scaledPen.IsOk())
                { scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth())); }
            wxDCPenChanger penCh(dc, scaledPen);

            // if not showing labels, don't include the space between the lines and the
            // non-existant labels when positioning the brackets
            const auto spaceAreasNeeded = !IsShowingLabels() ?
                1 : 2;

            if (GetAxisType() == AxisType::LeftYAxis)
                {
                long connectionX = GetLeftPoint().x, connectionXDoubleSided = GetLeftPoint().x;

                auto labelWidth = !IsShowingLabels() ?
                    0 : GetWidestTextLabel(dc).GetBoundingBox(dc).GetWidth();
                labelWidth = IsStackingLabels() ? labelWidth*2 : labelWidth;
                
                connectionX -= (labelWidth + CalcTickMarkOuterWidth() +
                    (ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())*spaceAreasNeeded));
                connectionXDoubleSided += (labelWidth + CalcTickMarkOuterWidth() +
                    (ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())*spaceAreasNeeded));

                wxCoord position1(0), position2(0), position3(0);
                if (GetPhysicalCoordinate(bracket.GetStartPosition(), position1) &&
                    GetPhysicalCoordinate(bracket.GetLabelPosition(), position2) &&
                    GetPhysicalCoordinate(bracket.GetEndPosition(), position3))
                    {
                    if (bracket.GetBracketLineShape() != AxisBracket::BracketLineShape::NoLine)
                        {
                        if (bracket.IsSingleLine())
                            {
                            bracket.DrawConnectionLine(dc,
                                                       // bracket needs to know what the scaling is, so this is how we do it
                                                       ScaleToScreenAndCanvas(1),
                                                       wxPoint(connectionX, position1),
                                                       wxPoint(connectionX-ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position1));
                            }
                        else
                            {
                            bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                            wxPoint(connectionX, position1),
                                                            wxPoint(connectionX-ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position1));
                            dc.DrawLine(connectionX-ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position1,
                                        connectionX-ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position3);
                            bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                            wxPoint(connectionX, position3),
                                                            wxPoint(connectionX-ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position3));
                            }
                        }

                    Label bracketLabel(bracket.GetLabel());
                    bracketLabel.SetDPIScaleFactor(GetDPIScaleFactor());
                    const wxSize bracketLabelSize = bracketLabel.GetBoundingBox(dc).GetSize();
                    labelWidth = bracketLabelSize.GetWidth();
                    if (bracket.GetPerpendicularLabelConnectionLinesAlignment() == AxisLabelAlignment::AlignWithBoundary)
                        {
                        bracketLabel.SetAnchorPoint(wxPoint(axisRect.GetLeft(), position2-safe_divide(bracketLabelSize.GetHeight(),2)) );
                        bracketLabel.SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                        }
                    else // AnchorWithLine
                        { bracketLabel.SetAnchorPoint(wxPoint(connectionX-(ScaleToScreenAndCanvas(bracket.GetLineSpacing())+labelWidth/2), position2)); }
                    bracketLabel.Draw(dc);
                    if (HasDoubleSidedAxisLabels())
                        {
                        if (bracket.GetBracketLineShape() != AxisBracket::BracketLineShape::NoLine)
                            {
                            if (bracket.IsSingleLine())
                                {
                                bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                    wxPoint(connectionXDoubleSided, position1),
                                    wxPoint(connectionXDoubleSided+ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position1));
                                }
                            else
                                {
                                bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                                wxPoint(connectionXDoubleSided, position1),
                                                                wxPoint(connectionXDoubleSided+ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position1));
                                dc.DrawLine(connectionXDoubleSided+ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position1,
                                            connectionXDoubleSided+ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position3);
                                bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                                wxPoint(connectionXDoubleSided, position3),
                                                                wxPoint(connectionXDoubleSided+ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position3));
                                }
                            }
                        if (bracket.GetPerpendicularLabelConnectionLinesAlignment() == AxisLabelAlignment::AlignWithBoundary)
                            {
                            bracketLabel.SetAnchorPoint(wxPoint(axisRect.GetRight(), position2-safe_divide(bracketLabelSize.GetHeight(),2)) );
                            bracketLabel.SetAnchoring(Wisteria::Anchoring::TopRightCorner);
                            }
                        else // AnchorWithLine
                            { bracketLabel.SetAnchorPoint(wxPoint(connectionXDoubleSided+(labelWidth/2)+ScaleToScreenAndCanvas(bracket.GetLineSpacing()), position2)); }
                        bracketLabel.Draw(dc);
                        }
                    }
                }
            else if (GetAxisType() == AxisType::RightYAxis)
                {
                auto labelWidth = !IsShowingLabels() ?
                    0 : GetWidestTextLabel(dc).GetBoundingBox(dc).GetWidth();
                labelWidth = IsStackingLabels() ? labelWidth*2 : labelWidth;
                long connectionX = GetLeftPoint().x, connectionXDoubleSided = GetLeftPoint().x;
                connectionX += (labelWidth + CalcTickMarkOuterWidth() +
                    (ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())*spaceAreasNeeded));
                connectionXDoubleSided -= (labelWidth + CalcTickMarkOuterWidth() +
                    (ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())*spaceAreasNeeded));

                wxCoord position1(0), position2(0), position3(0);
                if (GetPhysicalCoordinate(bracket.GetStartPosition(), position1) &&
                    GetPhysicalCoordinate(bracket.GetLabelPosition(), position2) &&
                    GetPhysicalCoordinate(bracket.GetEndPosition(), position3))
                    {
                    if (bracket.GetBracketLineShape() != AxisBracket::BracketLineShape::NoLine)
                        {
                        if (bracket.IsSingleLine())
                            {
                            bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                wxPoint(connectionX, position1),
                                wxPoint(connectionX+ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position1));
                            }
                        else
                            {
                            bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                            wxPoint(connectionX, position1),
                                                            wxPoint(connectionX+ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position1));
                            dc.DrawLine(connectionX+ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position1,
                                        connectionX+ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position3);
                            bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                            wxPoint(connectionX, position3),
                                                            wxPoint(connectionX+ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position3));
                            }
                        }

                    Label bracketLabel(bracket.GetLabel());
                    bracketLabel.SetDPIScaleFactor(GetDPIScaleFactor());
                    const wxSize bracketLabelSize = bracketLabel.GetBoundingBox(dc).GetSize();
                    labelWidth = bracketLabelSize.GetWidth();
                    if (bracket.GetPerpendicularLabelConnectionLinesAlignment() == AxisLabelAlignment::AlignWithBoundary)
                        {
                        bracketLabel.SetAnchorPoint(wxPoint(axisRect.GetRight(), position2-safe_divide(bracketLabelSize.GetHeight(),2)) );
                        bracketLabel.SetAnchoring(Wisteria::Anchoring::TopRightCorner);
                        }
                    else // AnchorWithLine
                        { bracketLabel.SetAnchorPoint(wxPoint(connectionX+(labelWidth/2)+ScaleToScreenAndCanvas(bracket.GetLineSpacing()), position2)); }
                    bracketLabel.Draw(dc);
                    if (HasDoubleSidedAxisLabels())
                        {
                        if (bracket.GetBracketLineShape() != AxisBracket::BracketLineShape::NoLine)
                            {
                            if (bracket.IsSingleLine())
                                {
                                bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                                wxPoint(connectionXDoubleSided, position1),
                                                                wxPoint(connectionXDoubleSided-ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position1));
                                }
                            else
                                {
                                bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                            wxPoint(connectionXDoubleSided, position1),
                                                            wxPoint(connectionXDoubleSided-ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position1));
                                dc.DrawLine(connectionXDoubleSided-ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position1,
                                            connectionXDoubleSided-ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position3);
                                bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                            wxPoint(connectionXDoubleSided, position3),
                                                            wxPoint(connectionXDoubleSided-ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position3));
                                }
                            }
                        if (bracket.GetPerpendicularLabelConnectionLinesAlignment() == AxisLabelAlignment::AlignWithBoundary)
                            {
                            bracketLabel.SetAnchorPoint(wxPoint(axisRect.GetLeft(), position2-safe_divide(bracketLabelSize.GetHeight(),2)) );
                            bracketLabel.SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                            }
                        else // AnchorWithLine
                            { bracketLabel.SetAnchorPoint(wxPoint(connectionXDoubleSided-(ScaleToScreenAndCanvas(bracket.GetLineSpacing())+labelWidth/2), position2)); }
                        bracketLabel.Draw(dc);
                        }
                    }
                }
            else if (GetAxisType() == AxisType::BottomXAxis)
                {
                auto labelHeight = !IsShowingLabels() ?
                    0 : GetTallestTextLabel(dc).GetBoundingBox(dc).GetHeight();
                labelHeight = IsStackingLabels() ? labelHeight*2 : labelHeight;
                long connectionY = GetTopPoint().y, connectionYDoubleSided = GetTopPoint().y;
                connectionY += (labelHeight + CalcTickMarkOuterWidth() +
                    (ScaleToScreenAndCanvas(ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()))*spaceAreasNeeded));
                connectionYDoubleSided -= (labelHeight + CalcTickMarkOuterWidth() +
                    (ScaleToScreenAndCanvas(ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()))*spaceAreasNeeded));

                wxCoord position1(0), position2(0), position3(0);
                if (GetPhysicalCoordinate(bracket.GetStartPosition(), position1) &&
                    GetPhysicalCoordinate(bracket.GetLabelPosition(), position2) &&
                    GetPhysicalCoordinate(bracket.GetEndPosition(), position3))
                    {
                    if (bracket.GetBracketLineShape() != AxisBracket::BracketLineShape::NoLine)
                        {
                        if (bracket.IsSingleLine())
                            {
                            bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                            wxPoint(position1, connectionY),
                                                            wxPoint(position1, connectionY+ScaleToScreenAndCanvas(bracket.GetTickmarkLength())));
                            }
                        else
                            {
                            bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                            wxPoint(position1, connectionY+ScaleToScreenAndCanvas(bracket.GetTickmarkLength())),
                                                            wxPoint(position1, connectionY));
                            dc.DrawLine(position1, connectionY+ScaleToScreenAndCanvas(bracket.GetTickmarkLength()),
                                        position3, connectionY+ScaleToScreenAndCanvas(bracket.GetTickmarkLength()));
                            bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                            wxPoint(position3, connectionY+ScaleToScreenAndCanvas(bracket.GetTickmarkLength())),
                                                            wxPoint(position3, connectionY));
                            }
                        }

                    Label bracketLabel(bracket.GetLabel());
                    bracketLabel.SetDPIScaleFactor(GetDPIScaleFactor());
                    const wxSize bracketLabelSize = bracketLabel.GetBoundingBox(dc).GetSize();
                    labelHeight = bracketLabelSize.GetHeight();
                    if (bracket.GetPerpendicularLabelConnectionLinesAlignment() == AxisLabelAlignment::AlignWithBoundary)
                        {
                        bracketLabel.SetAnchorPoint(wxPoint(position2-safe_divide(bracketLabelSize.GetWidth(),2), axisRect.GetBottom()) );
                        bracketLabel.SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                        }
                    else // AnchorWithLine
                        { bracketLabel.SetAnchorPoint(wxPoint(position2, connectionY+ScaleToScreenAndCanvas(bracket.GetLineSpacing())+(labelHeight/2))); }
                    bracketLabel.Draw(dc);

                    if (HasDoubleSidedAxisLabels())
                        {
                        if (bracket.GetBracketLineShape() != AxisBracket::BracketLineShape::NoLine)
                            {
                            if (bracket.IsSingleLine())
                                {
                                bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                                wxPoint(position1, connectionYDoubleSided),
                                                                wxPoint(position1, connectionYDoubleSided-ScaleToScreenAndCanvas(bracket.GetTickmarkLength())));
                                }
                            else
                                {
                                bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                                wxPoint(position1, connectionYDoubleSided),
                                                                wxPoint(position1, connectionYDoubleSided-ScaleToScreenAndCanvas(bracket.GetTickmarkLength())));
                                dc.DrawLine(position1, connectionYDoubleSided-ScaleToScreenAndCanvas(bracket.GetTickmarkLength()), position3, connectionYDoubleSided-bracket.GetTickmarkLength()*GetScaling());
                                bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                                wxPoint(position3, connectionYDoubleSided-ScaleToScreenAndCanvas(bracket.GetTickmarkLength())),
                                                                wxPoint(position3, connectionYDoubleSided));
                                }
                            }
                        if (bracket.GetPerpendicularLabelConnectionLinesAlignment() == AxisLabelAlignment::AlignWithBoundary)
                            {
                            bracketLabel.SetAnchorPoint(wxPoint(position2-safe_divide(bracketLabelSize.GetWidth(),2), axisRect.GetTop()) );
                            bracketLabel.SetAnchoring(Wisteria::Anchoring::TopRightCorner);
                            }
                        else // AnchorWithLine
                            { bracketLabel.SetAnchorPoint(wxPoint(position2, connectionYDoubleSided-(ScaleToScreenAndCanvas(bracket.GetLineSpacing())+(labelHeight/2)))); }
                        bracketLabel.Draw(dc);
                        }
                    }
                }
            else if (GetAxisType() == AxisType::TopXAxis)
                {
                auto labelHeight = !IsShowingLabels() ?
                    0 : GetTallestTextLabel(dc).GetBoundingBox(dc).GetHeight();
                labelHeight = IsStackingLabels() ? labelHeight*2 : labelHeight;
                auto connectionY = GetTopPoint().y;
                auto connectionYDoubleSided = GetTopPoint().y;
                connectionY -= (labelHeight + CalcTickMarkOuterWidth() +
                    (ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())*spaceAreasNeeded));
                connectionYDoubleSided += (labelHeight + CalcTickMarkOuterWidth() +
                    (ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())*spaceAreasNeeded));

                wxCoord position1(0), position2(0), position3(0);
                if (GetPhysicalCoordinate(bracket.GetStartPosition(), position1) &&
                    GetPhysicalCoordinate(bracket.GetLabelPosition(), position2) &&
                    GetPhysicalCoordinate(bracket.GetEndPosition(), position3))
                    {
                    if (bracket.GetBracketLineShape() != AxisBracket::BracketLineShape::NoLine)
                        {
                        if (bracket.IsSingleLine())
                            {
                            bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                wxPoint(position1, connectionY),
                                wxPoint(position1, connectionY-ScaleToScreenAndCanvas(bracket.GetTickmarkLength())));
                            }
                        else
                            {
                            bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                            wxPoint(position1, connectionY),
                                                            wxPoint(position1, connectionY-ScaleToScreenAndCanvas(bracket.GetTickmarkLength())));
                            dc.DrawLine(position1, connectionY-ScaleToScreenAndCanvas(bracket.GetTickmarkLength()),
                                        position3, connectionY-ScaleToScreenAndCanvas(bracket.GetTickmarkLength()));
                            bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                            wxPoint(position3, connectionY),
                                                            wxPoint(position3, connectionY-ScaleToScreenAndCanvas(bracket.GetTickmarkLength())));
                            }
                        }

                    Label bracketLabel(bracket.GetLabel());
                    bracketLabel.SetDPIScaleFactor(GetDPIScaleFactor());
                    const wxSize bracketLabelSize = bracketLabel.GetBoundingBox(dc).GetSize();
                    labelHeight = bracketLabelSize.GetHeight();
                    if (bracket.GetPerpendicularLabelConnectionLinesAlignment() == AxisLabelAlignment::AlignWithBoundary)
                        {
                        bracketLabel.SetAnchorPoint(wxPoint(position2-safe_divide(bracketLabelSize.GetWidth(),2), axisRect.GetTop()) );
                        bracketLabel.SetAnchoring(Wisteria::Anchoring::TopRightCorner);
                        }
                    else // AnchorWithLine
                        { bracketLabel.SetAnchorPoint(wxPoint(position2, connectionY-(ScaleToScreenAndCanvas(bracket.GetLineSpacing())+(labelHeight/2)))); }
                    bracketLabel.Draw(dc);

                    if (HasDoubleSidedAxisLabels())
                        {
                        if (bracket.GetBracketLineShape() != AxisBracket::BracketLineShape::NoLine)
                            {
                            if (bracket.IsSingleLine())
                                {
                                bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                                wxPoint(position1, connectionYDoubleSided),
                                                                wxPoint(position1, connectionYDoubleSided+ScaleToScreenAndCanvas(bracket.GetTickmarkLength())));
                                }
                            else
                                {
                                bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                                wxPoint(position1, connectionYDoubleSided),
                                                                wxPoint(position1, connectionYDoubleSided+ScaleToScreenAndCanvas(bracket.GetTickmarkLength())));
                                dc.DrawLine(position1, connectionYDoubleSided+ScaleToScreenAndCanvas(bracket.GetTickmarkLength()),
                                            position3, connectionYDoubleSided+ScaleToScreenAndCanvas(bracket.GetTickmarkLength()));
                                bracket.DrawConnectionLine(dc, ScaleToScreenAndCanvas(1),
                                                                wxPoint(position3, connectionYDoubleSided+ScaleToScreenAndCanvas(bracket.GetTickmarkLength())),
                                                                wxPoint(position3, connectionYDoubleSided));
                                }
                            }
                        if (bracket.GetPerpendicularLabelConnectionLinesAlignment() == AxisLabelAlignment::AlignWithBoundary)
                            {
                            bracketLabel.SetAnchorPoint(wxPoint(position2-safe_divide(bracketLabelSize.GetWidth(),2), axisRect.GetBottom()) );
                            bracketLabel.SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                            }
                        else // AnchorWithLine
                            { bracketLabel.SetAnchorPoint(wxPoint(position2, connectionYDoubleSided+ScaleToScreenAndCanvas(bracket.GetLineSpacing())+(labelHeight/2))); }
                        bracketLabel.Draw(dc);
                        }
                    }
                }
            }

        // draw the Y axis labels
        if (IsVertical())
            {
            bool drawingInnerLabel{ true };
            for (auto axisPtIter = GetAxisPoints().cbegin();
                 axisPtIter != GetAxisPoints().cend();
                 ++axisPtIter)
                {
                if (IsPointDisplayingLabel(*axisPtIter))
                    {
                    Label axisLabel(GetDisplayableValue(*axisPtIter));
                    axisLabel.GetPen() = wxNullPen; // don't draw box around axis labels
                    if (Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection) && IsSelected())
                        { axisLabel.GetPen() = wxPen(*wxRED, 2, wxPENSTYLE_DOT); }
                    axisLabel.SetDPIScaleFactor(GetDPIScaleFactor());
                    axisLabel.SetScaling(GetAxisLabelScaling());
                    axisLabel.SetFontColor(GetFontColor());
                    axisLabel.SetFontBackgroundColor(GetFontBackgroundColor());
                    axisLabel.GetFont() = GetFont();
                    axisLabel.SetTextAlignment(GetTextAlignment());
                    axisLabel.SetPadding(GetTopPadding(), GetRightPadding(), GetBottomPadding(), GetLeftPadding());
                    // outer labels hanging entirely off the plot should just be drawn
                    // with the standard padding
                    if (!(axisPtIter == GetAxisPoints().cbegin() &&
                          GetParallelLabelAlignment() == RelativeAlignment::FlushRight) &&
                        !(axisPtIter == GetAxisPoints().cend() &&
                          GetParallelLabelAlignment() == RelativeAlignment::FlushLeft))
                        { AdjustLabelSizeIfUsingBackgroundColor(axisLabel, dc, true); }
                    else
                        { AdjustLabelSizeIfUsingBackgroundColor(axisLabel, dc, false); }

                    if (GetAxisLabelOrientation() == AxisLabelOrientation::Perpendicular)
                        {
                        const wxSize labelSize = axisLabel.GetBoundingBox(dc).GetSize();
                        const wxCoord axisTextWidth = labelSize.GetWidth();
                        const wxCoord axisTextHeight = labelSize.GetHeight();
                        wxCoord x(0), y(0);
                        // y will be the same for left and right Y axes
                        y = (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::AlignWithBoundary && !IsStackingLabels()) ?
                                static_cast<wxCoord>(axisPtIter->GetPhysicalCoordinate())-safe_divide(axisTextHeight,2) :
                                static_cast<wxCoord>(axisPtIter->GetPhysicalCoordinate());

                        if (GetAxisType() == AxisType::LeftYAxis)
                            {
                            if (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::AlignWithBoundary && !IsStackingLabels())
                                {
                                x = axisRect.GetLeft();
                                if (GetBrackets().size())
                                    { x += CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()); }
                                if (titleLabel.IsShown() && titleLabel.GetText().length())
                                    { x += titleLabel.GetBoundingBox(dc).GetWidth(); }
                                axisLabel.SetAnchoring(Anchoring::TopLeftCorner);
                                }
                            else if (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::CenterOnAxisLine)
                                {
                                x = axisRect.GetLeft()+(axisRect.GetWidth()/2);
                                axisLabel.SetAnchoring(Anchoring::Center);
                                }
                            else // AnchorWithLine
                                {
                                x = GetTopPoint().x-((axisTextWidth/2)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()))-CalcTickMarkOuterWidth();
                                if (IsStackingLabels())
                                    { x -= drawingInnerLabel ? 0 : axisTextWidth; }
                                }
                            axisLabel.SetAnchorPoint(wxPoint(x,y));
                            axisLabel.Draw(dc);
                            if (HasDoubleSidedAxisLabels() &&
                                // doesn't make sense to draw labels on top of each other
                                GetPerpendicularLabelAxisAlignment() != AxisLabelAlignment::CenterOnAxisLine)
                                {
                                if (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::AlignWithBoundary && !IsStackingLabels())
                                    {
                                    x = axisRect.GetRight();
                                    if (GetBrackets().size())
                                        { x -= CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()); }
                                    axisLabel.SetAnchoring(Wisteria::Anchoring::TopRightCorner);
                                    }
                                else // AnchorWithLine
                                    {
                                    x = GetTopPoint().x+(axisTextWidth/2)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+CalcTickMarkInnerWidth();
                                    if (IsStackingLabels())
                                        { x += drawingInnerLabel ? 0 : axisTextWidth; }
                                    }
                                axisLabel.SetAnchorPoint(wxPoint(x,y));
                                axisLabel.Draw(dc);
                                }
                            }
                        else if (GetAxisType() == AxisType::RightYAxis)
                            {
                            if (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::AlignWithBoundary && !IsStackingLabels())
                                {
                                x = axisRect.GetRight();
                                if (GetBrackets().size())
                                    { x -= CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()); }
                                if (titleLabel.IsShown() && titleLabel.GetText().length())
                                    { x -= titleLabel.GetBoundingBox(dc).GetWidth(); }
                                axisLabel.SetAnchoring(Wisteria::Anchoring::TopRightCorner);
                                }
                            else if (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::CenterOnAxisLine)
                                {
                                x = axisRect.GetLeft()+(axisRect.GetWidth()/2);
                                axisLabel.SetAnchoring(Wisteria::Anchoring::Center);
                                }
                            else // AnchorWithLine
                                {
                                x = GetTopPoint().x + (GetBottomPoint().x-GetTopPoint().x) +
                                    (axisTextWidth/2) + ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()) + CalcTickMarkOuterWidth();
                                if (IsStackingLabels())
                                    { x += drawingInnerLabel ? 0 : axisTextWidth; }
                                }
                            axisLabel.SetAnchorPoint(wxPoint(x,y));
                            axisLabel.Draw(dc);
                            if (HasDoubleSidedAxisLabels() &&
                                GetPerpendicularLabelAxisAlignment() != AxisLabelAlignment::CenterOnAxisLine)
                                {
                                if (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::AlignWithBoundary && !IsStackingLabels())
                                    {
                                    x = axisRect.GetLeft();
                                    if (GetBrackets().size())
                                        { x += CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()); }
                                    axisLabel.SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                                    }
                                else // AnchorWithLine
                                    {
                                    x = GetTopPoint().x+(GetBottomPoint().x-GetTopPoint().x) -
                                        ((axisTextWidth/2)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+CalcTickMarkInnerWidth());
                                    if (IsStackingLabels())
                                        { x -= drawingInnerLabel ? 0 : axisTextWidth; }
                                    }
                                axisLabel.SetAnchorPoint(wxPoint(x,y));
                                axisLabel.Draw(dc);
                                }
                            }
                        drawingInnerLabel = !drawingInnerLabel;
                        }
                    else if (GetAxisLabelOrientation() == AxisLabelOrientation::Parallel)
                        {
                        axisLabel.SetTextOrientation(Orientation::Vertical);

                        const wxSize labelSize = axisLabel.GetBoundingBox(dc).GetSize();
                        const wxCoord axisTextHeight = labelSize.GetWidth();

                        const wxCoord y = static_cast<wxCoord>(axisPtIter->GetPhysicalCoordinate());
                        if (GetAxisType() == AxisType::LeftYAxis)
                            {
                            wxCoord x = GetTopPoint().x-(ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()))-CalcTickMarkOuterWidth();
                            if (GetParallelLabelAlignment() == RelativeAlignment::FlushBottom)
                                {
                                axisLabel.SetAnchoring(Anchoring::BottomLeftCorner);
                                }
                            else if (GetParallelLabelAlignment() == RelativeAlignment::FlushTop)
                                {
                                axisLabel.SetAnchoring(Anchoring::BottomRightCorner);
                                }
                            else if (GetParallelLabelAlignment() == RelativeAlignment::Centered)
                                {
                                x -= (axisTextHeight/2);
                                axisLabel.SetAnchoring(Anchoring::Center);
                                }
                            if (IsStackingLabels())
                                { x -= drawingInnerLabel ? 0 : axisTextHeight; }
                            axisLabel.SetAnchorPoint(wxPoint(x,y));
                            axisLabel.Draw(dc);
                            if (HasDoubleSidedAxisLabels())
                                {
                                x = GetTopPoint().x+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+CalcTickMarkInnerWidth();
                                if (GetParallelLabelAlignment() == RelativeAlignment::FlushBottom || GetParallelLabelAlignment() == RelativeAlignment::FlushTop)
                                    { x += axisTextHeight; }
                                else if (GetParallelLabelAlignment() == RelativeAlignment::Centered)
                                    { x += (axisTextHeight/2); }
                                if (IsStackingLabels())
                                    { x += drawingInnerLabel ? 0 : axisTextHeight; }
                                axisLabel.SetAnchorPoint(wxPoint(x,y));
                                axisLabel.Draw(dc);
                                }
                            }
                        else if (GetAxisType() == AxisType::RightYAxis)
                            {
                            wxCoord x = GetTopPoint().x+(GetBottomPoint().x-GetTopPoint().x)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+CalcTickMarkOuterWidth();
                            if (GetParallelLabelAlignment() == RelativeAlignment::FlushBottom)
                                {
                                axisLabel.SetAnchoring(Anchoring::TopLeftCorner);
                                }
                            else if (GetParallelLabelAlignment() == RelativeAlignment::FlushTop)
                                {
                                axisLabel.SetAnchoring(Anchoring::TopRightCorner);
                                }
                            else if (GetParallelLabelAlignment() == RelativeAlignment::Centered)
                                {
                                x += (axisTextHeight/2);
                                axisLabel.SetAnchoring(Anchoring::Center);
                                }
                            if (IsStackingLabels())
                                { x += drawingInnerLabel ? 0 : axisTextHeight; }
                            axisLabel.SetAnchorPoint(wxPoint(x,y));
                            axisLabel.Draw(dc);
                            if (HasDoubleSidedAxisLabels())
                                {
                                x = GetTopPoint().x+(GetBottomPoint().x-GetTopPoint().x)-(ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+CalcTickMarkInnerWidth());
                                if (GetParallelLabelAlignment() == RelativeAlignment::FlushBottom || GetParallelLabelAlignment() == RelativeAlignment::FlushTop)
                                    { x -= axisTextHeight; }
                                else if (GetParallelLabelAlignment() == RelativeAlignment::Centered)
                                    { x -= (axisTextHeight/2); }
                                if (IsStackingLabels())
                                    { x -= drawingInnerLabel ? 0 : axisTextHeight; }
                                axisLabel.SetAnchorPoint(wxPoint(x,y));
                                axisLabel.Draw(dc);
                                }
                            }
                        drawingInnerLabel = !drawingInnerLabel;
                        }
                    }
                }
            }
        // draw the X axis labels
        else if (IsHorizontal())
            {
            bool drawingInnerLabel{ true };
            for (auto axisPtIter = GetAxisPoints().cbegin();
                axisPtIter != GetAxisPoints().cend();
                ++axisPtIter)
                {
                if (IsPointDisplayingLabel(*axisPtIter))
                    {
                    Label axisLabel(GetDisplayableValue(*axisPtIter));
                    axisLabel.GetPen() = wxNullPen;
                    if (Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection) && IsSelected())
                        { axisLabel.GetPen() = wxPen(*wxRED, 2, wxPENSTYLE_DOT); }
                    axisLabel.SetDPIScaleFactor(GetDPIScaleFactor());
                    axisLabel.SetScaling(GetAxisLabelScaling());
                    axisLabel.SetFontColor(GetFontColor());
                    axisLabel.SetFontBackgroundColor(GetFontBackgroundColor());
                    axisLabel.GetFont() = GetFont();
                    axisLabel.SetTextAlignment(GetTextAlignment());
                    axisLabel.SetPadding(GetTopPadding(), GetRightPadding(), GetBottomPadding(), GetLeftPadding());
                    // outer labels hanging entirely off the plot should just be drawn
                    // with the standard padding
                    if (!(axisPtIter == GetAxisPoints().cbegin() &&
                          GetParallelLabelAlignment() == RelativeAlignment::FlushRight) &&
                        !(axisPtIter == GetAxisPoints().cend()-1 &&
                          GetParallelLabelAlignment() == RelativeAlignment::FlushLeft))
                        { AdjustLabelSizeIfUsingBackgroundColor(axisLabel, dc, true); }
                    else
                        { AdjustLabelSizeIfUsingBackgroundColor(axisLabel, dc, false); }

                    if (GetAxisLabelOrientation() == AxisLabelOrientation::Parallel)
                        {
                        const wxSize labelSize = axisLabel.GetBoundingBox(dc).GetSize();
                        const wxCoord axisTextHeight = labelSize.GetHeight();
                        const long x = axisPtIter->GetPhysicalCoordinate();
                        long y = GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y) +
                                 ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+CalcTickMarkOuterWidth();
                        if (GetParallelLabelAlignment() == RelativeAlignment::FlushLeft)
                            {
                            axisLabel.SetAnchoring(Anchoring::TopLeftCorner);
                            }
                        else if (GetParallelLabelAlignment() == RelativeAlignment::FlushRight)
                            {
                            axisLabel.SetAnchoring(Anchoring::TopRightCorner);
                            }
                        else if (GetParallelLabelAlignment() == RelativeAlignment::Centered)
                            {
                            y += (axisTextHeight/2);
                            axisLabel.SetAnchoring(Anchoring::Center);
                            }
                        if (IsStackingLabels())
                            { y += drawingInnerLabel ? 0 : axisTextHeight; }
                        if (GetAxisType() == AxisType::BottomXAxis)
                            {
                            axisLabel.SetAnchorPoint(wxPoint(x,y));
                            axisLabel.Draw(dc);
                            if (HasDoubleSidedAxisLabels())
                                {
                                y = GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y)-(ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+CalcTickMarkInnerWidth());
                                if (GetParallelLabelAlignment() == RelativeAlignment::FlushLeft || GetParallelLabelAlignment() == RelativeAlignment::FlushRight)
                                    { y -= axisTextHeight; }
                                else if (GetParallelLabelAlignment() == RelativeAlignment::Centered)
                                    { y -= (axisTextHeight/2); }
                                if (IsStackingLabels())
                                    { y -= drawingInnerLabel ? 0 : axisTextHeight; }
                                axisLabel.SetAnchorPoint(wxPoint(x,y));
                                axisLabel.Draw(dc);
                                }
                            }
                        else if (GetAxisType() == AxisType::TopXAxis)
                            {
                            y = GetTopPoint().y - ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()) - CalcTickMarkOuterWidth();
                            if (GetParallelLabelAlignment() == RelativeAlignment::FlushLeft || GetParallelLabelAlignment() == RelativeAlignment::FlushRight)
                                { y -= axisTextHeight; }
                            else if (GetParallelLabelAlignment() == RelativeAlignment::Centered)
                                { y -= (axisTextHeight/2); }
                            if (IsStackingLabels())
                                { y -= drawingInnerLabel ? 0 : axisTextHeight; }
                            axisLabel.SetAnchorPoint(wxPoint(x,y));
                            axisLabel.Draw(dc);
                            if (HasDoubleSidedAxisLabels())
                                {
                                y = GetTopPoint().y+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+CalcTickMarkInnerWidth();
                                if (IsStackingLabels())
                                    { y += drawingInnerLabel ? 0 : axisTextHeight; }
                                axisLabel.SetAnchorPoint(wxPoint(x,y));
                                axisLabel.Draw(dc);
                                }
                            }
                        drawingInnerLabel = !drawingInnerLabel;
                        }
                    else if (GetAxisLabelOrientation() == AxisLabelOrientation::Perpendicular)
                        {
                        axisLabel.SetTextOrientation(Orientation::Vertical);

                        const wxSize labelSize = axisLabel.GetBoundingBox(dc).GetSize();
                        const wxCoord axisTextWidth = labelSize.GetHeight();
                        const wxCoord axisTextHeight = labelSize.GetWidth();

                        long x(0);
                        long y(0);
                        if (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::AlignWithBoundary && !IsStackingLabels())
                            {
                            x = axisPtIter->GetPhysicalCoordinate()-safe_divide(axisTextHeight,2);
                            y = axisRect.GetBottom();
                            if (GetBrackets().size())
                                {
                                y -= CalcBracketsWidth(dc) +
                                     ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine());
                                }
                            if (titleLabel.IsShown() && titleLabel.GetText().length())
                                {
                                y -= titleLabel.GetBoundingBox(dc).GetHeight();
                                }
                            axisLabel.SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                            }
                        else // AnchorWithLine
                            {
                            x = axisPtIter->GetPhysicalCoordinate();
                            y = GetTopPoint().y + (GetBottomPoint().y-GetTopPoint().y) +
                                ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()) +
                                (axisTextWidth/2) + CalcTickMarkOuterWidth();
                            }
                        if (GetAxisType() == AxisType::BottomXAxis)
                            {
                            axisLabel.SetAnchorPoint(wxPoint(x,y));
                            axisLabel.Draw(dc);
                            if (HasDoubleSidedAxisLabels())
                                {
                                if (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::AlignWithBoundary && !IsStackingLabels())
                                    {
                                    y = axisRect.GetTop();
                                    if (GetBrackets().size())
                                        { y += CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()); }
                                    axisLabel.SetAnchorPoint(wxPoint(x, y));
                                    axisLabel.SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                                    }
                                else // AnchorWithLine
                                    {
                                    axisLabel.SetAnchorPoint(wxPoint(x,
                                        GetTopPoint().y+(GetBottomPoint().y-GetTopPoint().y)-(ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+(axisTextWidth/2)+CalcTickMarkInnerWidth())) );
                                    }
                                axisLabel.Draw(dc);
                                }
                            }
                        else if (GetAxisType() == AxisType::TopXAxis)
                            {
                            if (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::AlignWithBoundary && !IsStackingLabels())
                                {
                                y = axisRect.GetTop();
                                if (GetBrackets().size())
                                    { y += CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()); }
                                if (titleLabel.IsShown() && titleLabel.GetText().length())
                                    { y += titleLabel.GetBoundingBox(dc).GetHeight(); }
                                axisLabel.SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                                }
                            else // AnchorWithLine
                                { y = GetTopPoint().y-(ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+CalcTickMarkOuterWidth()+(axisTextWidth/2)); }
                            axisLabel.SetAnchorPoint(wxPoint(x,y));
                            axisLabel.Draw(dc);
                            if (HasDoubleSidedAxisLabels())
                                {
                                if (GetPerpendicularLabelAxisAlignment() == AxisLabelAlignment::AlignWithBoundary && !IsStackingLabels())
                                    {
                                    y = axisRect.GetBottom();
                                    if (GetBrackets().size())
                                        { y -= CalcBracketsWidth(dc)+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine()); }
                                    axisLabel.SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                                    }
                                else // AnchorWithLine
                                    { y = GetTopPoint().y+ScaleToScreenAndCanvas(GetSpacingBetweenLabelsAndLine())+(axisTextWidth/2)+CalcTickMarkInnerWidth(); }
                                axisLabel.SetAnchorPoint(wxPoint(x,y));
                                axisLabel.Draw(dc);
                                }
                            }
                        }
                    }
                }
            }

        // draw the selection outline
        if (IsSelected())
            {
            wxDCPenChanger pc(dc, wxPen(*wxBLACK, ScaleToScreenAndCanvas(2), wxPENSTYLE_DOT));
            wxPoint pts[5];
            GraphItems::Polygon::GetRectPoints(axisRect, pts);
            pts[4] = pts[0];//close the square
            dc.DrawLines(5, pts);
            }

        if (GetOutlineSize().IsFullySpecified())
            {
            wxDCPenChanger pc(dc, wxPen(*wxBLACK, ScaleToScreenAndCanvas(1), wxPenStyle::wxPENSTYLE_SOLID));
            wxPoint pts[5];
            // area rect was already inflated from GetBoundingBox()
            GraphItems::Polygon::GetRectPoints(axisRect, pts);
            pts[4] = pts[0];// close the square
            dc.DrawLines(5, pts);
            }

        // highlight the selected protruding bounding box in debug mode
        if (Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection) && IsSelected())
            {
            wxPoint debugOutline[5];
            GraphItems::Polygon::GetRectPoints(GetProtrudingBoundingBox(dc), debugOutline);
            debugOutline[4] = debugOutline[0];
            wxDCPenChanger pcDebug(dc, wxPen(*wxRED, ScaleToScreenAndCanvas(2), wxPENSTYLE_SHORT_DASH));
            dc.DrawLines(std::size(debugOutline), debugOutline);
            }
        if (Settings::IsDebugFlagEnabled(DebugSettings::DrawInformationOnSelection) && IsSelected())
            {
            const auto bBox = GetBoundingBox(dc);
            Label infoLabel(GraphItemInfo(
                wxString::Format(L"Bounding Box (x,y,width,height): %d, %d, %d, %d\n"
                                  "Axis Line Points: (%d, %d), (%d, %d)\n"
                                  "Scaling: %s\n"
                                  "Axis Label Scaling: %s",
                    bBox.x, bBox.y, bBox.width, bBox.height,
                    GetBottomPoint().x, GetBottomPoint().y,
                    GetTopPoint().x, GetTopPoint().y,
                    wxNumberFormatter::ToString(GetScaling(), 1, wxNumberFormatter::Style::Style_NoTrailingZeroes),
                    wxNumberFormatter::ToString(GetAxisLabelScaling(), 1, wxNumberFormatter::Style::Style_NoTrailingZeroes))).
                AnchorPoint(wxPoint(bBox.GetBottomLeft().x + bBox.GetWidth()/2, bBox.GetBottomRight().y)).
                FontColor(*wxBLUE).
                Pen(*wxBLUE_PEN).DPIScaling(GetDPIScaleFactor()).
                FontBackgroundColor(*wxWHITE).Padding(2, 2, 2, 2));
            if (GetAxisType() == AxisType::LeftYAxis)
                { infoLabel.GetGraphItemInfo().Anchoring(Anchoring::BottomLeftCorner); }
            else
                { infoLabel.GetGraphItemInfo().Anchoring(Anchoring::BottomRightCorner); }
            infoLabel.Draw(dc);
            }

        return axisRect;
        }

    //--------------------------------------
    wxCoord Axis::CalcTickMarkOuterWidth() const
        {
        if (GetTickMarkDisplay() == TickMark::DisplayType::NoDisplay)
            { return 0; }
        wxCoord largestStandardTickMark{ 0 };
        for (auto tickIter = GetTickMarks().cbegin();
            tickIter != GetTickMarks().cend();
            ++tickIter)
            {
            if (tickIter->m_displayType != TickMark::DisplayType::Inner)
                { largestStandardTickMark = std::max<double>(largestStandardTickMark,
                                                             ScaleToScreenAndCanvas(tickIter->m_lineLength)); }
            }
        wxCoord largestCustomTickMark{ 0 };
        for (auto tickIter = GetCustomTickMarks().cbegin();
            tickIter != GetCustomTickMarks().cend();
            ++tickIter)
            {
            if (tickIter->m_displayType != TickMark::DisplayType::Inner)
                { largestCustomTickMark = std::max<double>(largestCustomTickMark,
                                                           ScaleToScreenAndCanvas(tickIter->m_lineLength)); }
            }
        return std::max(largestStandardTickMark, largestCustomTickMark);
        }

    //--------------------------------------
    wxCoord Axis::CalcTickMarkInnerWidth() const
        {
        if (GetTickMarkDisplay() == TickMark::DisplayType::NoDisplay)
            { return 0; }
        wxCoord largestStandardTickMark{ 0 };
        for (auto tickIter = GetTickMarks().cbegin();
            tickIter != GetTickMarks().cend();
            ++tickIter)
            {
            if (tickIter->m_displayType != TickMark::DisplayType::Outer)
                { largestStandardTickMark = std::max<double>(largestStandardTickMark,
                                                             ScaleToScreenAndCanvas(tickIter->m_lineLength)); }
            }
        wxCoord largestCustomTickMark{ 0 };
        for (auto tickIter = GetCustomTickMarks().cbegin();
            tickIter != GetCustomTickMarks().cend();
            ++tickIter)
            {
            if (tickIter->m_displayType != TickMark::DisplayType::Outer)
                { largestCustomTickMark = std::max<double>(largestCustomTickMark,
                                                           ScaleToScreenAndCanvas(tickIter->m_lineLength)); }
            }
        return std::max(largestStandardTickMark, largestCustomTickMark);
        }

    //--------------------------------------
    void Axis::CalcMaxLabelWidth()
        {
        wxASSERT_LEVEL_2(GetTopPoint().IsFullySpecified());
        wxASSERT_LEVEL_2(GetBottomPoint().IsFullySpecified());
        const size_t plotSize = IsVertical() ?
            std::abs(GetTopPoint().y - GetBottomPoint().y) : std::abs(GetBottomPoint().x - GetTopPoint().x);

        long displayedLabelsCount{ 0 };
        for (const auto& axisPt : GetAxisPoints())
            {
            if (IsPointDisplayingLabel(axisPt))
                { ++displayedLabelsCount; }
            }
        if (displayedLabelsCount > 2 && GetAxisPoints().size() > 2 && IsShowingOuterLabels() &&
            GetAxisPoints().begin()->IsShown() && GetDisplayableValue(*GetAxisPoints().begin()).GetText().length() &&
            (GetAxisPoints().end()-1)->IsShown() && GetDisplayableValue(*(GetAxisPoints().end()-1)).GetText().length())
            { --displayedLabelsCount; }

        m_maxLabelWidth = safe_divide<wxCoord>(plotSize, displayedLabelsCount)-ScaleToScreenAndCanvas(2);
        if (IsStackingLabels())
            { m_maxLabelWidth *= 2; }
        m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
        }

    //--------------------------------------
    double Axis::CalcBestScalingToFitLabels(wxDC& dc)
        {
        // no labels?
        if (GetAxisPointsCount() == 0)
            { return GetScaling(); }

        wxASSERT_LEVEL_2(GetTopPoint().IsFullySpecified());
        wxASSERT_LEVEL_2(GetBottomPoint().IsFullySpecified());

        // If the biggest label is bigger than the width of the graph divided by the number of displayed labels,
        // then suggest a lower font-size scaling  (also account for a little extra padding around the labels)
        CalcMaxLabelWidth();

        if (m_maxLabelWidth <= 0)
            {
            SetAxisLabelScaling(GetScaling());
            return GetScaling();
            }

        double currentScaling = GetAxisLabelScaling();

        if (GetAxisLabelOrientation() == AxisLabelOrientation::Parallel)
            {
            auto longestLabel{ GetWidestTextLabel(dc) };
            auto longestWidth = longestLabel.GetBoundingBox(dc).GetWidth();

            while (currentScaling > 1.0 && longestWidth > m_maxLabelWidth)
                {
                currentScaling -= 0.1;
                longestLabel.SetScaling(currentScaling);
                longestWidth = longestLabel.GetBoundingBox(dc).GetWidth();
                }
            }
        else if (GetAxisLabelOrientation() == AxisLabelOrientation::Perpendicular)
            {
            auto tallestLabel = GetTallestTextLabel(dc);
            auto tallestHeight = tallestLabel.GetBoundingBox(dc).GetHeight();

            // 1.0 will be the lowest scaling that we would recommend. Even if that continues to cause overlaps,
            // we don't want to suggest a scaling smaller than the default that the parent is probably using.
            while (currentScaling > 1.0 && tallestHeight > m_maxLabelWidth)
                {
                currentScaling -= .1;
                tallestLabel.SetScaling(currentScaling);
                tallestHeight = tallestLabel.GetBoundingBox(dc).GetHeight();
                }
            }
        else
            {
            SetAxisLabelScaling(GetScaling());
            return GetScaling();
            }
        // 1.0 will be the lowest scaling that we would recommend. Even if that continues to cause overlaps,
        // we don't want to suggest a scaling smaller than the default that the parent is probably using.
        SetAxisLabelScaling(std::max(1.0, currentScaling));
        return GetAxisLabelScaling();
        }

    //--------------------------------------
    bool Axis::ShouldLabelsBeStackedToFit(wxDC& dc) const
        {
        wxASSERT_LEVEL_2(GetTopPoint().IsFullySpecified());
        wxASSERT_LEVEL_2(GetBottomPoint().IsFullySpecified());

        // if we aren't auto stacking, then just return the stacking
        // method that the client specified earlier
        if (!IsAutoStackingEnabled())
            { return IsStackingLabels(); }

        const size_t axisWidth = IsVertical() ?
            std::abs(GetTopPoint().y-GetBottomPoint().y) :
            std::abs(GetBottomPoint().x-GetTopPoint().x);

        const bool isMeasuringByHeight =
            (GetAxisLabelOrientation() == AxisLabelOrientation::Perpendicular);

        long displayedLabelsCount{ 0 };
        for (const auto& axisPt : GetAxisPoints())
            {
            if (IsPointDisplayingLabel(axisPt))
                { ++displayedLabelsCount; }
            }
        // If outer labels are hanging outside of the plot area, then assume
        // half of both of these two labels don't need to be included in the
        // plot area's width; thus, remove necessary space for one label.
        if (displayedLabelsCount > 2 && GetAxisPoints().size() > 2 &&
            IsShowingOuterLabels() &&
            GetAxisPoints().begin()->IsShown() &&
            GetDisplayableValue(*GetAxisPoints().begin()).GetText().length() &&
            (GetAxisPoints().end()-1)->IsShown() &&
             GetDisplayableValue(*(GetAxisPoints().end()-1)).GetText().length())
            { --displayedLabelsCount; }

        // If any label is bigger than the width of the graph divided by the number of
        // displayed labels then stack them account for a little extra padding around
        // the labels if the labels are going across (e.g., parallel along the X axis);
        // Otherwise, don't expect padding if they are on top of each other
        // aggressively (e.g., perpendicular, going down the Y axis). This way, you
        // won't cause Y axis label to be stacked, which can look odd--
        // only resort to that is they really are overlapping.
        const long maxTextSize = safe_divide<long>(axisWidth, displayedLabelsCount) -
            (isMeasuringByHeight ? (2*GetScaling()) : 0);
        if (maxTextSize <= 0)
            { return false; }

        Label axisLabel(
            GraphItemInfo().Scaling(GetScaling()).Pen(wxNullPen).
            Font(GetFont()).DPIScaling(GetDPIScaleFactor()).
            Padding(GetTopPadding(), GetRightPadding(),
                    GetBottomPadding(), GetLeftPadding()) );
        AdjustLabelSizeIfUsingBackgroundColor(axisLabel, dc, true);

        for (auto pos = GetAxisPoints().cbegin();
            pos != GetAxisPoints().cend();
            ++pos)
            {
            if (IsPointDisplayingLabel(*pos))
                {
                axisLabel.SetText(GetDisplayableValue(*pos).GetText());
                const wxSize labelSize =
                    axisLabel.GetBoundingBox(dc).GetSize();
                wxCoord axisTextWidth = isMeasuringByHeight ?
                                        labelSize.GetHeight() : labelSize.GetWidth();
                // with the first and last labels, the outer halves
                // of them hang outside of the plot area
                if (IsShowingOuterLabels() &&
                    (pos == GetAxisPoints().cbegin() || pos == GetAxisPoints().cend()-1) )
                    { axisTextWidth /= 2; }
                if (axisTextWidth >= maxTextSize)
                    { return true; }
                }
            }
        return false;
        }

    //--------------------------------------
    void Axis::AddBrackets(const BracketType bracketType)
        {
        if (bracketType == BracketType::FiscalQuarterly)
            {
            wxASSERT_MSG((m_firstDay.IsValid() && m_lastDay.IsValid()),
                L"Date interval should be set via SetRange() if adding FY brackets!");
            if (m_firstDay.IsValid() && m_lastDay.IsValid())
                {
                ClearBrackets();
                // FY is the next calendar year
                int FY{ m_firstDay.GetYear()+1 };
                const auto [rangeStart, rangeEnd] = GetRange();
                const auto Q1Length{ (m_fyQ2-m_fyQ1).GetDays() };
                const auto Q2Length{ (m_fyQ3-m_fyQ2).GetDays() };
                const auto Q3Length{ (m_fyQ4-m_fyQ3).GetDays() };
                const auto Q4Length{ (wxDateTime(m_fyQ1).Add(
                                          wxDateSpan(1))-m_fyQ4).GetDays() };
                long currentStart{ 0 };
                while (currentStart <= rangeEnd)
                    {
                    const wxString FYLabel{ std::to_wstring(FY).substr(2,2) };
                    AddBracket(Axis::AxisBracket(currentStart,
                        (currentStart+Q1Length)-1, currentStart+(Q1Length/2),
                        wxString::Format(_(L"Q1FY%s"), FYLabel)));
                    currentStart += Q1Length;
                    AddBracket(Axis::AxisBracket(currentStart,
                        (currentStart+Q2Length)-1, currentStart+(Q2Length/2),
                        wxString::Format(_(L"Q2FY%s"), FYLabel)));
                    currentStart += Q2Length;
                    AddBracket(Axis::AxisBracket(currentStart,
                        (currentStart+Q3Length)-1, currentStart+(Q3Length/2),
                        wxString::Format(_(L"Q3FY%s"), FYLabel)));
                    currentStart += Q3Length;
                    AddBracket(Axis::AxisBracket(currentStart,
                        (currentStart+Q4Length)-1, currentStart+(Q4Length/2),
                        wxString::Format(_(L"Q4FY%s"), FYLabel)));
                    currentStart += Q4Length;
                    ++FY;
                    }
                }
            }
        }

    //--------------------------------------
    void Axis::SetRange(const wxDateTime& startDate, const wxDateTime& endDate,
                        const DateInterval displayInterval, const FiscalYear FYtype)
        {
        wxASSERT_MSG(startDate.IsValid() && endDate.IsValid(),
            L"Invalid date used for axis range!");
        // can't do much with this range if invalid, so have to ignore it
        if (!startDate.IsValid() || !endDate.IsValid())
            { return; }

        SetFiscalYearType(FYtype);
        m_firstDay = startDate;
        m_lastDay = endDate;
        m_dateDisplayInterval = displayInterval;

        // use the first day of the week based on the locale's calendar
        wxDateTime::WeekDay firstWeekDay;
        if (!wxDateTime::GetFirstWeekDay(&firstWeekDay))
            { firstWeekDay = wxDateTime::WeekDay::Sun; }

        // move back the range to include today, if necessary,
        // if using smaller intervals
        if (GetDateDisplayInterval() != DateInterval::FiscalQuarterly)
            { m_firstDay = std::min(wxDateTime::Now(), m_firstDay); }

        if (GetDateDisplayInterval() == DateInterval::FiscalQuarterly)
            {
            wxDateTime fyEnd = m_fyQ4;
            fyEnd.SetYear(m_firstDay.GetYear()+1);
            fyEnd.Add(wxDateSpan(0, 3, 0, 0));
            fyEnd.Subtract(wxDateSpan(0, 0, 0, 1));

            m_firstDay.SetDay(m_fyQ1.GetDay());
            // move back to the start of a fiscal year
            while (m_firstDay.GetMonth() != m_fyQ1.GetMonth())
                { m_firstDay.Subtract(wxDateSpan(0, 1, 0, 0)); }

            // move to first day of next FY, then step back to last day of the current FY
            if (m_lastDay < fyEnd)
                { m_lastDay = fyEnd; }
            }
        // adjust monthly intervals to land on the start of the months
        else if (GetDateDisplayInterval() == DateInterval::Monthly)
            {
            while (m_firstDay.GetDay() != 1)
                { m_firstDay.Subtract(wxDateSpan(0, 0, 0, 1)); }
            m_lastDay.SetToLastMonthDay();
            }
        // or adjust to weeks
        else if (GetDateDisplayInterval() == DateInterval::Weekly)
            {
            while (m_firstDay.GetWeekDay() != firstWeekDay)
                { m_firstDay.Subtract(wxDateSpan(0, 0, 0, 1)); }
            // move to start of following week and step back
            while (m_lastDay.GetWeekDay() != firstWeekDay)
                { m_lastDay.Add(wxDateSpan(0, 0, 0, 1)); }
            m_lastDay.Subtract(wxDateSpan(0, 0, 0, 1));
            }

        // quarterly intervals
        if (GetDateDisplayInterval() == DateInterval::FiscalQuarterly )
            {
            SetRange(0, (m_lastDay-m_firstDay).GetDays(), 0, 1, 1);
            long currentDate = 0;
            wxDateTime dateLabel = m_firstDay;
            while (currentDate <= (m_lastDay-m_firstDay).GetDays())
                {
                // only show first and last month of quarters if using FYs
                if (GetDateDisplayInterval() == DateInterval::FiscalQuarterly &&
                    ((dateLabel.GetMonth() == m_fyQ1.GetMonth() &&
                      dateLabel.GetDay() == m_fyQ1.GetDay()) ||
                     (dateLabel.GetMonth() == m_fyQ2.GetMonth() &&
                      dateLabel.GetDay() == m_fyQ2.GetDay()) ||
                     (dateLabel.GetMonth() == m_fyQ3.GetMonth() &&
                      dateLabel.GetDay() == m_fyQ3.GetDay()) ||
                     (dateLabel.GetMonth() == m_fyQ4.GetMonth() &&
                      dateLabel.GetDay() == m_fyQ4.GetDay())
                    ))
                    { SetCustomLabel(currentDate,
                                     GraphItems::Label(dateLabel.FormatDate())); }
                dateLabel.Add(wxDateSpan(0, 0, 0, 1));
                ++currentDate;
                }
            }
        // monthly
        if (GetDateDisplayInterval() == DateInterval::Monthly)
            {
            SetRange(0, (m_lastDay-m_firstDay).GetDays(), 0, 1, 1);
            long currentDate = 0;
            wxDateTime dateLabel = m_firstDay;
            while (currentDate <= (m_lastDay-m_firstDay).GetDays())
                {
                // only show first of the months
                if (dateLabel.GetDay() == 1)
                    { SetCustomLabel(currentDate,
                                     GraphItems::Label(dateLabel.FormatDate())); }
                dateLabel.Add(wxDateSpan(0, 0, 0, 1));
                ++currentDate;
                }
            }
        // weekly intervals
        else if (GetDateDisplayInterval() == DateInterval::Weekly)
            {
            SetRange(0, (m_lastDay-m_firstDay).GetDays(), 0, 1, 7);
            long currentDate = 0;
            wxDateTime dateLabel = m_firstDay;
            // move next label to the start of the next week,
            // so that all remaining labels show the first day of the week
            while (dateLabel.GetWeekDay() != firstWeekDay)
                {
                dateLabel.Add(wxDateSpan(0, 0, 0, 1));
                ++currentDate;
                }
            // add the rest
            while (currentDate <= (m_lastDay-m_firstDay).GetDays())
                {
                SetCustomLabel(currentDate,
                               GraphItems::Label(dateLabel.FormatDate()));
                currentDate += 7;
                if (currentDate > (m_lastDay-m_firstDay).GetDays())
                    { break; }
                dateLabel.Add(wxDateSpan(0, 0, 1, 0));
                }
            }
        // daily
        else if (GetDateDisplayInterval() == DateInterval::Daily)
            {
            SetRange(0, (m_lastDay - m_firstDay).GetDays(), 0, 1, 1);
            long currentDate = 0;
            wxDateTime dateLabel = m_firstDay;
            while (currentDate <= (m_lastDay-m_firstDay).GetDays())
                {
                SetCustomLabel(currentDate,
                               GraphItems::Label(dateLabel.FormatDate()));
                currentDate += 7;
                if (currentDate > (m_lastDay-m_firstDay).GetDays())
                    { break; }
                dateLabel.Add(wxDateSpan(0, 0, 1, 0));
                }
            }

        SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
        }

    //--------------------------------------
    void Axis::SetRange(double rangeStart, double rangeEnd,
                        uint8_t precision, bool includeExtraInterval /*= false*/)
        {
        if (IsStartingAtZero())
            { rangeStart = std::min<double>(0, rangeStart); }

        double rangeSize = (rangeEnd-rangeStart);
        double intervalSize{ 0 };
        /* these divisions purposefully avoid double precision so
           that the axis intervals are neat (unless the range is 1).*/
        size_t rangeDivisionFactor = 1;
        if (rangeSize > 100'000'000)
            { rangeDivisionFactor = 100'000'000; }
        else if (rangeSize > 10'000'000)
            { rangeDivisionFactor = 10'000'000; }
        else if (rangeSize > 1'000'000)
            { rangeDivisionFactor = 1'000'000; }
        else if (rangeSize > 100'000)
            { rangeDivisionFactor = 100'000; }
        else if (rangeSize > 10'000)
            { rangeDivisionFactor = 10'000; }
        else if (rangeSize > 1'000)
            { rangeDivisionFactor = 1'000; }
        else if (rangeSize > 100)
            { rangeDivisionFactor = 100; }
        else if (rangeSize > 10)
            { rangeDivisionFactor = 10; }
        else
            { rangeDivisionFactor = 1; }

        if (rangeSize <= 1)
            {
            intervalSize = 0.2;
            if (precision == 0)
                { precision = 1; }
            }
        else
            {
            // come up with an intelligent interval size based on the range.
            if (rangeSize >= 100)
                {
                intervalSize = std::ceil(safe_divide<double>(rangeSize, rangeDivisionFactor)) *
                            safe_divide<size_t>(rangeDivisionFactor, 10);
                }
            else if (rangeSize >= 20)
                { intervalSize = 5; }
            else
                { intervalSize = std::ceil(safe_divide<double>(rangeSize, 10)); }
            // bump up the range to fit the data and also to have an even spread for the intervals
            if (safe_modulus<size_t>(static_cast<size_t>(rangeSize),intervalSize))
                {
                rangeSize = (rangeSize -
                             (safe_modulus<size_t>(static_cast<size_t>(rangeSize),intervalSize))) +
                             intervalSize;
                }
            }
        rangeSize += (includeExtraInterval) ? intervalSize : 0;
        SetRange(rangeStart, (rangeStart+rangeSize), precision, intervalSize, 1);
        }

    //--------------------------------------
    void Axis::SetRange(double rangeStart, double rangeEnd,
                        const uint8_t precision, double interval,
                        const size_t displayInterval /*= 1*/)
        {
        m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));

        if (IsStartingAtZero())
            { rangeStart = std::min<double>(0, rangeStart); }
        wxASSERT(rangeEnd >= rangeStart);
        if (rangeEnd < rangeStart)
            { return; }
        m_displayPrecision = precision;
        SetInterval(interval);
        SetTickMarkInterval(interval);
        m_displayInterval = displayInterval;
        if (rangeEnd == rangeStart)
            {
            if (interval == 0)
                { interval = 1; }
            rangeStart -= interval;
            rangeEnd += interval;
            }
        // in case the interval is a nonsensical value then make it the size of the range
        if (interval <= 0)
            { interval = safe_divide<double>(rangeEnd-rangeStart,2); }
        // reset everything
        m_axisLabels.clear();

        size_t currentDisplayInterval = 1;
        if (IsReversed())
            {
            double lastValidPoint = rangeEnd;
            for (double i = rangeEnd; i > (rangeStart-interval); i -= interval)
                {
                // if not set to skip then set it to be displayed
                const bool display = (currentDisplayInterval == 1) ? true : false;
                if (currentDisplayInterval == 1)
                    { currentDisplayInterval = displayInterval; }
                else
                    { --currentDisplayInterval; }
                wxString textLabel = wxNumberFormatter::ToString(i, precision,
                    wxNumberFormatter::Style::Style_WithThousandsSep);
                // add it to the axis label collection
                m_axisLabels.push_back(AxisPoint(i, textLabel, display));
                lastValidPoint = i;
                }
            m_rangeStart = rangeEnd;
            m_rangeEnd = lastValidPoint;
            }
        else
            {
            /* Make sure that the user's range end is met.
               We might step over it a bit (depending on how
               the intervals are set up), so a range like 1-70 might wind up being 1-71*/
            double lastValidPoint = rangeStart;
            for (double i = rangeStart; i < (rangeEnd+interval); i += interval)
                {
                // if not set to skip then set it to be displayed
                const bool display = (currentDisplayInterval == 1) ? true : false;
                if (currentDisplayInterval == 1)
                    { currentDisplayInterval = displayInterval; }
                else
                    { --currentDisplayInterval; }
                wxString textLabel = wxNumberFormatter::ToString(i, precision,
                    wxNumberFormatter::Style::Style_WithThousandsSep);
                // add it to the axis label collection
                m_axisLabels.push_back(AxisPoint(i, textLabel, display));
                lastValidPoint = i;
                }
            m_rangeStart = rangeStart;
            m_rangeEnd = lastValidPoint;
            }
        }

    //--------------------------------------
    void Axis::SetCustomLabel(const double tickValue, const Label& label)
        {
        m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));

        Label theLabel = label;
        theLabel.SetDPIScaleFactor(GetDPIScaleFactor());
        theLabel.GetPen() = wxNullPen;
        theLabel.GetFont() = GetFont();
        theLabel.SetFontColor(GetFontColor());
        theLabel.SetFontBackgroundColor(GetFontBackgroundColor());
        theLabel.SplitTextToFitLength(m_suggestedMaxLengthPerLine);
        m_customAxisLabels[tickValue] = theLabel;
        }

    //--------------------------------------
    const Label& Axis::GetCustomLabel(const double value) const
        {
        const auto custLabelIter = m_customAxisLabels.find(value);
        return (custLabelIter != m_customAxisLabels.cend() ) ?
            custLabelIter->second : m_invalidLabel;
        }

    //--------------------------------------
    bool Axis::PointHasLabel(const double value) const
        {
        const auto labelIter = std::find(m_axisLabels.cbegin(),
                                         m_axisLabels.cend(), value);
        if (labelIter != m_axisLabels.cend() && labelIter->IsShown())
            { return true; }
        const auto custLabelIter = m_customAxisLabels.find(value);
        if (custLabelIter != m_customAxisLabels.cend())
            { return true; }
        return false;
        }

    //-------------------------------------------
    std::pair<Label, double> Axis::GetFirstDisplayedLabel(wxDC& dc) const
        {
        for (auto axisPos = GetAxisPoints().cbegin();
            axisPos != GetAxisPoints().cend();
            ++axisPos)
            {
            if (IsPointDisplayingLabel(*axisPos))
                {
                Label axisLabel{
                    GraphItemInfo(GetDisplayableValue(*axisPos).GetText()).
                    Scaling(GetAxisLabelScaling()).
                    Pen(wxNullPen).Font(GetFont()).DPIScaling(GetDPIScaleFactor()).
                    Padding(GetTopPadding(), GetRightPadding(),
                            GetBottomPadding(), GetLeftPadding()) };
                // outer label hanging entirely off the plot should just be drawn
                // with the standard padding
                if (!(axisPos == GetAxisPoints().cbegin() &&
                    GetParallelLabelAlignment() == RelativeAlignment::FlushRight))
                    { AdjustLabelSizeIfUsingBackgroundColor(axisLabel, dc, true); }
                else
                    { AdjustLabelSizeIfUsingBackgroundColor(axisLabel, dc, false); }
                return std::make_pair(axisLabel, axisPos->GetValue());
                }
            }
        return std::make_pair(Label(GraphItemInfo().DPIScaling(GetDPIScaleFactor())),
                              std::numeric_limits<double>::quiet_NaN());
        }

    //-------------------------------------------
    std::pair<Label, double> Axis::GetLastDisplayedLabel(wxDC& dc) const
        {
        for (auto axisPos = GetAxisPoints().crbegin();
            axisPos != GetAxisPoints().crend();
            ++axisPos)
            {
            if (IsPointDisplayingLabel(*axisPos))
                {
                Label axisLabel{
                    GraphItemInfo(GetDisplayableValue(*axisPos).GetText()).
                    Scaling(GetAxisLabelScaling()).
                    Pen(wxNullPen).Font(GetFont()).DPIScaling(GetDPIScaleFactor()).
                    Padding(GetTopPadding(), GetRightPadding(),
                            GetBottomPadding(), GetLeftPadding()) };
                // outer label hanging entirely off the plot should just be drawn
                // with the standard padding
                if (!(axisPos == GetAxisPoints().crbegin() &&
                    GetParallelLabelAlignment() == RelativeAlignment::FlushLeft))
                    { AdjustLabelSizeIfUsingBackgroundColor(axisLabel, dc, true); }
                else
                    { AdjustLabelSizeIfUsingBackgroundColor(axisLabel, dc, false); }
                return std::make_pair(axisLabel, axisPos->GetValue());
                }
            }
        return std::make_pair(Label(GraphItemInfo().DPIScaling(GetDPIScaleFactor())),
                              std::numeric_limits<double>::quiet_NaN());
        }

    //-------------------------------------------
    std::optional<double> Axis::GetPointFromDate(const wxDateTime& date) const noexcept
        {
        if (!m_firstDay.IsValid() || !date.IsValid())
            { return std::nullopt; }
        const auto daysDiff = date.GetDateOnly().Subtract(m_firstDay.GetDateOnly()).GetDays();
        return is_within<double>(GetRange(), daysDiff) ?
            std::optional<double>(daysDiff) : std::nullopt;
        }

    //-------------------------------------------
    void Axis::AddUnevenAxisPoint(const double value, const wxString& label)
        {
        // bail if something at the provided axis value is already in here
        if (std::find(m_axisLabels.cbegin(),
                      m_axisLabels.cend(), AxisPoint(value,label)) != m_axisLabels.cend())
            {
            wxLogVerbose(L"Value %f/%s in call to %s ignored; value already present on axis.",
                         value, label, __WXFUNCTION__);
            return;
            }
        std::vector<AxisPoint>::iterator pos;
        for (pos = m_axisLabels.begin();
             pos != m_axisLabels.end();
             ++pos)
            {
            if (m_scaledReserved)
                {
                if (value > pos->GetValue())
                    { break; }
                }
            else
                {
                if (value < pos->GetValue())
                    { break; }
                }
            }
        Label lab(label);
        lab.SplitTextToFitLength(m_suggestedMaxLengthPerLine);
        pos = m_axisLabels.insert(pos, AxisPoint(value, lab.GetText()));
        }

    //-------------------------------------------
    void Axis::AdjustRangeToLabels()
        {
        if (GetAxisPointsCount() == 0)
            { return; }
        m_rangeStart = GetAxisPoints().at(0).GetValue();
        m_rangeEnd = GetAxisPoints().at(0).GetValue();
        for (auto labelsIter = GetAxisPoints().cbegin();
            labelsIter != GetAxisPoints().cend();
            ++labelsIter)
            {
            if (IsReversed())
                {
                m_rangeStart = std::max(m_rangeStart, labelsIter->GetValue());
                m_rangeEnd = std::min(m_rangeEnd, labelsIter->GetValue());
                }
            else
                {
                m_rangeStart = std::min(m_rangeStart, labelsIter->GetValue());
                m_rangeEnd = std::max(m_rangeEnd, labelsIter->GetValue());
                }
            }
        }

    //-------------------------------------------
    Label Axis::GetTallestTextLabel(wxDC& dc) const
        {
        m_tallestLabel.SetDPIScaleFactor(GetDPIScaleFactor());
        m_tallestLabel.SetScaling(GetAxisLabelScaling());
        // use cached calculation from previous call if labels haven't changed
        if (m_tallestLabel.IsOk())
            { return m_tallestLabel; }

        // otherwise, reset and recalculate
        m_tallestLabel = Label(GraphItemInfo().Ok(false).DPIScaling(GetDPIScaleFactor()));
        if (GetAxisPoints().size() == 0)
            { return m_tallestLabel; }

        wxCoord tallestLabelHeight{ 0 };
        Label tallestAxisLabel;
        Label currentLabel(
            GraphItemInfo().Pen(wxNullPen).Scaling(GetAxisLabelScaling()).
            Font(GetFont()).DPIScaling(GetDPIScaleFactor()).
            Padding(GetTopPadding(), GetRightPadding(), GetBottomPadding(), GetLeftPadding()));

        if ((IsVertical() && GetAxisLabelOrientation() == AxisLabelOrientation::Parallel) ||
            (IsHorizontal() && GetAxisLabelOrientation() == AxisLabelOrientation::Perpendicular))
            { currentLabel.SetTextOrientation(Orientation::Vertical); }

        for (const auto& axisPt : GetAxisPoints())
            {
            if (IsPointDisplayingLabel(axisPt))
                {
                currentLabel.SetText(GetDisplayableValue(axisPt).GetText());
                const wxCoord textHeight =
                    currentLabel.GetBoundingBox(dc).GetSize().GetHeight();
                if (textHeight > tallestLabelHeight)
                    {
                    tallestAxisLabel = currentLabel;
                    tallestLabelHeight = textHeight;
                    }
                }
            }
        m_tallestLabel.SetDPIScaleFactor(GetDPIScaleFactor());
        m_tallestLabel = tallestAxisLabel;
        return m_tallestLabel;
        }

    //-------------------------------------------
    Label Axis::GetWidestTextLabel(wxDC& dc) const
        {
        m_widestLabel.SetDPIScaleFactor(GetDPIScaleFactor());
        m_widestLabel.SetScaling(GetAxisLabelScaling());
        // use cached calculation from previous call if labels haven't changed
        if (m_widestLabel.IsOk())
            { return m_widestLabel; }

        // otherwise, reset and recalculate
        m_widestLabel = Label(GraphItemInfo().Ok(false).DPIScaling(GetDPIScaleFactor()));
        if (GetAxisPoints().size() == 0)
            { return m_widestLabel; }

        wxCoord longestLabelWidth{ 0 };
        Label longestAxisLabel;
        Label currentLabel(
            GraphItemInfo().Pen(wxNullPen).Scaling(GetAxisLabelScaling()).
            Font(GetFont()).DPIScaling(GetDPIScaleFactor()).
            Padding(GetTopPadding(), GetRightPadding(), GetBottomPadding(), GetLeftPadding()));

        if ((IsVertical() && GetAxisLabelOrientation() == AxisLabelOrientation::Parallel) ||
            (IsHorizontal() && GetAxisLabelOrientation() == AxisLabelOrientation::Perpendicular))
            { currentLabel.SetTextOrientation(Orientation::Vertical); }

        for (const auto& axisPt : GetAxisPoints())
            {
            if (IsPointDisplayingLabel(axisPt))
                {
                currentLabel.SetText(GetDisplayableValue(axisPt).GetText());
                const auto textWidth = currentLabel.GetBoundingBox(dc).GetWidth();
                if (textWidth > longestLabelWidth)
                    {
                    longestAxisLabel = currentLabel;
                    longestLabelWidth = textWidth;
                    }
                }
            }
        m_widestLabel = longestAxisLabel;
        m_widestLabel.SetDPIScaleFactor(GetDPIScaleFactor());
        return m_widestLabel;
        }

    //-------------------------------------------
    void Axis::ReverseScale(bool reverse)
        {
        // reverse it if not already reversed and client is asking for it to be reversed
        if (reverse && !IsReversed())
            { std::reverse(m_axisLabels.begin(), m_axisLabels.end()); }
        // or reverse it if already reversed and client is asking for it to NOT be reversed
        else if (!reverse && IsReversed())
            { std::reverse(m_axisLabels.begin(), m_axisLabels.end()); }
        m_scaledReserved = reverse;
        m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
        }

    //-------------------------------------------
    void Axis::SetAxisLabelOrientation(const AxisLabelOrientation& orient) noexcept
        {
        m_labelOrientation = orient;
        m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
        }

    //-------------------------------------------
    bool Axis::GetValueFromPhysicalCoordinate(const wxCoord coordinate,
                                              double& value) const noexcept
        {
        value = -1; // init to invalid value in case we have to return false
        if (GetAxisPointsCount() == 0)
            { return false; }
        std::vector<AxisPoint>::const_iterator pos;
        for (pos = GetAxisPoints().cbegin();
            pos != GetAxisPoints().cend();
            ++pos)
            {
            if (IsHorizontal())
                {
                if (coordinate <= pos->GetPhysicalCoordinate())
                    { break; }
                }
            else if (IsVertical())
                {
                if (coordinate >= pos->GetPhysicalCoordinate())
                    { break; }
                }
            else
                { return false; }
            }
        if (pos == GetAxisPoints().end() )
            { return false; }
        if (coordinate == pos->GetPhysicalCoordinate())
            {
            value = pos->GetValue();
            return true;
            }
        // if this point is before the first axis line or
        // above the top axis line then it is out of range
        if (pos == GetAxisPoints().cbegin())
            { return false; }
        auto previousLine = pos; --previousLine;
        const double lowerLineValue = previousLine->GetPhysicalCoordinate();
        const double upperLineValue = pos->GetPhysicalCoordinate();
        const double percentage = safe_divide<double>((coordinate-lowerLineValue),
                                                      (upperLineValue-lowerLineValue));
        if (IsHorizontal())
            {
            const double coordinateDifference =
                    pos->GetValue() - previousLine->GetValue();
            value = previousLine->GetValue() +
                    (coordinateDifference*percentage);
            return true;
            }
        else if (IsVertical())
            {
            const double coordinateDifference =
                previousLine->GetValue() - pos->GetValue();
            value = previousLine->GetValue() -
                    (coordinateDifference*percentage);
            return true;
            }
        else
            {
            // shouldn't happen--invalid axis orientation
            return false;
            }
        }

    //-------------------------------------------
    bool Axis::GetPhysicalCoordinate(const double value, wxCoord& result) const
        {
        result = -1; // init to invalid value in case we have to return false
        if (GetAxisPointsCount() == 0)
            { return false; }
        std::vector<AxisPoint>::const_iterator pos;
        if (IsReversed())
            {
            for (pos = GetAxisPoints().cbegin();
                pos != GetAxisPoints().cend();
                ++pos)
                {
                if (value >= pos->GetValue() ||
                    compare_doubles(value, pos->GetValue()))
                    { break; }
                }
            }
        else
            {
            pos = std::lower_bound(GetAxisPoints().begin(),
                                   GetAxisPoints().end(), value);
            }
        if (pos == GetAxisPoints().end() )
            { return false; }
        if (*pos == value)
            {
            result = pos->GetPhysicalCoordinate();
            return true;
            }
        // if this point is before the first axis line or
        // above the top axis line, then it is out of range
        if (pos == GetAxisPoints().begin())
            { return false; }
        auto previousLine = pos; --previousLine;
        const double lowerLineValue = previousLine->GetValue();
        const double upperLineValue = pos->GetValue();
        const double percentage = safe_divide<double>(
            (value-lowerLineValue),
            (upperLineValue-lowerLineValue));
        if (IsHorizontal())
            {
            const long coordinateDifference = pos->GetPhysicalCoordinate() -
                                              previousLine->GetPhysicalCoordinate();
            result = previousLine->GetPhysicalCoordinate() +
                     (coordinateDifference*percentage);
            return true;
            }
        else if (IsVertical())
            {
            const long coordinateDifference = previousLine->GetPhysicalCoordinate() -
                                              pos->GetPhysicalCoordinate();
            result = previousLine->GetPhysicalCoordinate() -
                     (coordinateDifference*percentage);
            return true;
            }
        else
            {
            // shouldn't happen--invalid axis orientation
            return false;
            }
        }

    //-------------------------------------------
    void Axis::SetLabelLineLength(const size_t suggestedMaxLengthPerLine)
        {
        m_suggestedMaxLengthPerLine = suggestedMaxLengthPerLine;
        // reformat any existing labels
        for (auto custLabelIter = m_customAxisLabels.begin();
             custLabelIter != m_customAxisLabels.end();
             ++custLabelIter)
            {
            custLabelIter->second.SplitTextToFitLength(m_suggestedMaxLengthPerLine);
            }
        for (auto axisPointPos = m_axisLabels.begin();
             axisPointPos != m_axisLabels.begin();
             ++axisPointPos)
            {
            Label lab(axisPointPos->GetDisplayValue());
            lab.SplitTextToFitLength(m_suggestedMaxLengthPerLine);
            axisPointPos->SetDisplayValue(lab.GetText());
            }
        }

    //-------------------------------------------
    wxCoord Axis::CalcBracketsWidth(wxDC& dc) const
        {
        wxCoord spacing(0);
        for (const auto& bracket : GetBrackets())
            {
            spacing = std::max(bracket.CalcWidth(dc, GetDPIScaleFactor()), spacing);
            }
        return spacing;
        }

    //-------------------------------------------
    void Axis::AddBracket(AxisBracket bracket)
        {
        bracket.GetLabel().SetScaling(GetScaling());
        bracket.SetOrientation(IsHorizontal() ?
            Orientation::Horizontal : Orientation::Vertical);
        GetBrackets().emplace_back(bracket);
        }

    //-------------------------------------------
    void Axis::SetScaling(const double scaling)
        {
        GraphItemBase::SetScaling(scaling);
        std::for_each(GetBrackets().begin(), GetBrackets().end(),
            [this, scaling](auto& bracket)
            {
            bracket.GetLabel().SetScaling(scaling);
            // also update the DPI scale factor for the label
            bracket.GetLabel().SetDPIScaleFactor(GetDPIScaleFactor());
            });
        }

    //-------------------------------------------
    void Axis::SetDisplayInterval(const size_t interval, const size_t offset /*= 0*/)
        {
        m_displayInterval = interval;
        // first, reset the labels
        for (size_t i = 0; i < m_axisLabels.size(); ++i)
            { m_axisLabels.at(i).Show(false); }
        // then turn them on, based on the specified interval
        for (size_t i = 0+offset; i < m_axisLabels.size(); i += m_displayInterval)
            { m_axisLabels.at(i).Show(true); }
        m_widestLabel = m_tallestLabel = Label(GraphItemInfo().Ok(false));
        }

    //-------------------------------------------
    bool Axis::IsPointDisplayingLabel(const AxisPoint& point) const
        {
        if (!point.IsShown())
            { return false; }
        // is it set to show a custom label, but doesn't have one?
        const auto customLabel = m_customAxisLabels.find(point.GetValue());
        if (GetLabelDisplay() == AxisLabelDisplay::DisplayOnlyCustomLabels &&
            customLabel == m_customAxisLabels.cend())
            { return false; }
        // custom and/or regular label, but has neither?
        else if (customLabel == m_customAxisLabels.cend() &&
            point.GetDisplayValue().empty())
            { return false; }
        else
            { return true; }
        }

    //-------------------------------------------
    Label Axis::GetDisplayableValue(const AxisPoint& pt) const
        {
        // just return a blank label
        if (!IsShowingLabels())
            { return Label(GraphItemInfo().DPIScaling(GetDPIScaleFactor())); }

        const auto& customLabel = GetCustomLabel(pt.GetValue());
        if (GetLabelDisplay() == AxisLabelDisplay::DisplayCustomLabelsAndValues)
            {
            return Label(
                GraphItemInfo(customLabel.GetText() + L"    " + pt.GetDisplayValue()).
                DPIScaling(GetDPIScaleFactor()));
            }
        else if ((GetLabelDisplay() == AxisLabelDisplay::DisplayOnlyCustomLabels) ||
                 (customLabel.IsOk() && customLabel.GetText().length()) )
            { return customLabel; }
        else
            { return Label(GraphItemInfo(pt.GetDisplayValue()).DPIScaling(GetDPIScaleFactor())); }
        }

    //-------------------------------------------
    void Axis::SetPoints(const wxPoint pt1, const wxPoint pt2, wxDC& dc)
        {
        // make the higher point be the first one if a vertical axis
        // (this assumption needs to be made for later calculations)
        if (IsVertical())
            {
            if (pt1.y < pt2.y)
                {
                m_points.first = pt1;
                m_points.second = pt2;
                }
            else
                {
                m_points.first = pt2;
                m_points.second = pt1;
                }
            }
        // for horizontal axis, the left-most point is the first one
        else // bottom and top axes
            {
            if (pt1.x < pt2.x)
                {
                m_points.first = pt1;
                m_points.second = pt2;
                }
            else
                {
                m_points.first = pt2;
                m_points.second = pt1;
                }
            }

        CalcLabelPositions();
        CalcTickMarkPositions();
        CalcBestScalingToFitLabels(dc);
        }

    //-------------------------------------------
    wxCoord Axis::AxisBracket::CalcWidth(wxDC& dc, const double dpiScaling) const
        {
        auto theLabel{ GetLabel() };
        theLabel.SetDPIScaleFactor(dpiScaling);
        const wxSize labelSize = theLabel.GetBoundingBox(dc).GetSize();
        wxCoord size = GetLineSpacing() * theLabel.GetScaling() * dpiScaling;
        size += (GetOrientation() == Orientation::Vertical) ? labelSize.GetWidth() : labelSize.GetHeight();
        return size;
        }
    }
