#include "table.h"

using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    Table::Table(Wisteria::Canvas* canvas) : Graph2D(canvas)
        {
        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    void Table::RecalcSizes(wxDC& dc)
        {
        if (m_table.size() == 0 || m_table[0].size() == 0)
            { return; }

        Graph2D::RecalcSizes(dc);

        const wxRect drawArea = GetPlotAreaBoundingBox();

        // calculate the necessary heights of the rows and widths of the column
        wxPoint pts[4];
        std::vector<wxCoord> rowHeights(m_table.size(), 0);
        std::vector<wxCoord> columnWidths(m_table[0].size(), 0);
        size_t currentRow{ 0 }, currentColumn{ 0 };
        Label measuringLabel(GraphItemInfo().Pen(*wxBLACK_PEN).
            Padding(5, 5, 5, 5).
            Scaling(GetScaling()).DPIScaling(GetDPIScaleFactor()));
        for (const auto& row : m_table)
            {
            currentColumn = 0;
            for (const auto& cell : row)
                {
                // make empty cells at least a space so that an empty
                // row or column will at least have some width or height
                const auto cellText = cell.GetDisplayValue();
                measuringLabel.SetText(cellText.length() ? cellText : L" ");
                const auto bBox = measuringLabel.GetBoundingBox(dc);
                rowHeights[currentRow] = std::max(bBox.GetHeight(), rowHeights[currentRow]);
                columnWidths[currentColumn] = std::max(bBox.GetWidth(), columnWidths[currentColumn]);
                ++currentColumn;
                }
            ++currentRow;
            }

        auto tableHeight = std::accumulate(rowHeights.cbegin(), rowHeights.cend(), 0);
        // adjust row heights if they collectively go outside of the drawing area
        if (tableHeight > drawArea.GetHeight())
            {
            const auto heightDiff = tableHeight - drawArea.GetHeight();
            // take away a proportional amount of the difference from each row
            for (auto& row : rowHeights)
                {
                const auto rowProportion = safe_divide<double>(row, tableHeight);
                row -= (rowProportion * heightDiff);
                }
            tableHeight = std::accumulate(rowHeights.cbegin(), rowHeights.cend(), 0);
            }

        auto tableWidth = std::accumulate(columnWidths.cbegin(), columnWidths.cend(), 0);
        // adjust column widths if they collectively go outside of the drawing area
        if (tableWidth > drawArea.GetWidth())
            {
            const auto widthDiff = tableWidth - drawArea.GetWidth();
            // take away a proportional amount of the difference from each column
            for (auto& col : columnWidths)
                {
                const auto colProportion = safe_divide<double>(col, tableWidth);
                col -= (colProportion * widthDiff);
                }
            tableWidth = std::accumulate(columnWidths.cbegin(), columnWidths.cend(), 0);
            }

        // draw the text
        currentRow = currentColumn = 0;
        wxCoord currentXPos{ drawArea.GetX() };
        wxCoord currentYPos{ drawArea.GetY() };
        for (const auto& row : m_table)
            {
            currentColumn = 0;
            currentXPos = drawArea.GetX();
            for (const auto& cell : row)
                {
                pts[0] = wxPoint(currentXPos,
                                 currentYPos);
                pts[1] = wxPoint((currentXPos + columnWidths[currentColumn]),
                                 currentYPos);
                pts[2] = wxPoint((currentXPos + columnWidths[currentColumn]),
                                 (currentYPos + rowHeights[currentRow]));
                pts[3] = wxPoint(currentXPos,
                                 (currentYPos + rowHeights[currentRow]));

                const wxRect boxRect(pts[0], pts[2]);

                const auto cellText = cell.GetDisplayValue();
                auto cellLabel = std::make_shared<Label>(
                    GraphItemInfo(cellText.length() ? cellText : L" ").
                    Pen(wxNullPen).Padding(5, 5, 5, 5).
                    Scaling(GetScaling()).DPIScaling(GetDPIScaleFactor()).
                    FontColor(
                        (cell.m_color.IsOk() ?
                            ColorContrast::BlackOrWhiteContrast(cell.m_color) : *wxBLACK)).
                    FontBackgroundColor(cell.m_color.IsOk() ? cell.m_color : *wxWHITE).
                    Anchoring(Anchoring::Center).
                    AnchorPoint(wxPoint(boxRect.GetLeft() + (boxRect.GetWidth() / 2),
                        boxRect.GetTop() + (boxRect.GetHeight() / 2))));
                cellLabel->SetBoundingBox(boxRect, dc, GetScaling());
                cellLabel->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
                cellLabel->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);

                AddObject(cellLabel);
                currentXPos += columnWidths[currentColumn];
                ++currentColumn;
                }
            currentYPos += rowHeights[currentRow];
            ++currentRow;
            }

        auto borderLines = std::make_shared<Lines>(GetPen(), GetScaling());
        // draw the row borders
        currentRow = currentColumn = 0;
        currentXPos = drawArea.GetX();
        currentYPos = drawArea.GetY();
        for (const auto& row : rowHeights)
            {
            borderLines->AddLine(wxPoint(drawArea.GetX(), currentYPos),
                                 wxPoint(currentXPos+tableWidth, currentYPos));
            currentYPos += row;
            }
        borderLines->AddLine(wxPoint(drawArea.GetX(), currentYPos),
                             wxPoint(currentXPos+tableWidth, currentYPos));

        // draw the column borders
        currentRow = currentColumn = 0;
        currentXPos = drawArea.GetX();
        currentYPos = drawArea.GetY();
        for (const auto& col : columnWidths)
            {
            borderLines->AddLine(wxPoint(currentXPos, drawArea.GetY()),
                                 wxPoint(currentXPos, currentYPos+tableHeight));
            currentXPos += col;
            }
        borderLines->AddLine(wxPoint(currentXPos, drawArea.GetY()),
                             wxPoint(currentXPos, currentYPos+tableHeight));
        AddObject(borderLines);
        }
    }
