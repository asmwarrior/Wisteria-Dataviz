/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_TABLE_H__
#define __WISTERIA_TABLE_H__

#include <vector>
#include <variant>
#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief A display of tabular data, which can either be imported from a dataset
            or be built from scratch.

        | %Table with Aggregates      | Styled %Table         |
        | :-------------- | :-------------------------------- |
        | @image html TableMajors.svg width=90% | @image html TablePrograms.svg width=90% |
        
        @par %Data:
            A table can accept a Data::Dataset, where any type of column can be include.
            Which of these columns to include (as well as their order) can be controlled
            by the caller.\n
            \n
            The table can use the same structure as the dataset, or be transposed (so that the
            columns are then the rows in the table).\n
            \n
            Consecutively repeated group labels across the rows and columns can be collapsed
            into larger cells, giving the appearance of grouped data
            (see GroupRow() and GroupColumn()).
            Numerous other functions are available for customizing the content and appearance of cells,
            rows, and columns (e.g., GetCell() or BoldRow()).\n
            \n
            Finally, aggregate columns (e.g., subtotals) can be added to the table via InsertAggregateColumn().

        @par Missing Data:
            Any missing data from the dataset will be displayed as an empty cell.

        @par Table Positioning:
            By default, the table will be draw with a text scaling of @c 1.0 and be scaled down to
            fit within the provided graph area is necessary. The table will then be placed in the top-left
            corner of the graph area, and any extra space remaining will be below and to the right of
            the table. To change this behavior, you can call SetPageHorizontalAlignment() and
            SetPageVerticalAlignment() to position the table in a different place within its graph area.

            A table can also be stretched to fit its entire graph area by calling SetMinWidthProportion()
            or SetMinHeightProportion().
            
        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the table
         auto canvas = new Wisteria::Canvas(this);
         canvas->SetFixedObjectsGridSize(1, 1);

         auto programAwards = std::make_shared<Data::Dataset>();
         try
            {
            programAwards->ImportCSV(L"/home/luna/data/Program Awards.csv",
                ImportInfo().
                ContinuousColumns({ L"Doctoral Degrees Awarded",
                    L"Time to Degree Since Entering Graduate School" }).
                CategoricalColumns({
                    { L"School" },
                    { L"Program" }
                    }));
            }
         catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                         _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

         auto tableGraph = std::make_shared<Table>(canvas);
         tableGraph->SetData(programAwards,
            { L"School", L"Program",
              L"Doctoral Degrees Awarded", L"Time to Degree Since Entering Graduate School" });
         // split some of the cells' text into multiple lines
         tableGraph->GetCell(14, 0).SetSuggestedLineLength(15);
         tableGraph->GetCell(0, 2).SetSuggestedLineLength(5);
         tableGraph->GetCell(0, 3).SetSuggestedLineLength(10);
         // group the schools together in the first row
         tableGraph->GroupColumn(0);
         // make the headers and row groups bold (and center the headers)
         tableGraph->BoldRow(0);
         tableGraph->BoldColumn(0);
         tableGraph->CenterRowHorizontally(0);
         // apply a zebra-stripe look
         tableGraph->ApplyAlternateRowColors(ColorBrewer::GetColor(Color::AzureMist), 1, 1);

         // add the table to the canvas
         canvas->SetFixedObject(0, 0, tableGraph);

         // make canvas taller and less wide
         canvas->SetCanvasMinHeightDIPs(
            canvas->GetDefaultCanvasWidthDIPs());
         canvas->SetCanvasMinWidthDIPs(
            canvas->GetDefaultCanvasHeightDIPs());
         // also, fit it to the entire page when printing (preferrably in portait)
         canvas->FitToPageWhenPrinting(true);
        @endcode
        
        @par Aggregate Row and Column Example:
        @code
         auto juniorSeniorMajors = std::make_shared<Data::Dataset>();
         try
            {
            juniorSeniorMajors->ImportCSV(L"/home/isabelle/data/Junior & Senior Majors (Top 20).csv",
                ImportInfo().
                ContinuousColumns({ L"Female", L"Male" }).
                CategoricalColumns({
                    { L"Division" },
                    { L"Department" }
                    }));
            }
         catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                         _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

         auto tableGraph = std::make_shared<Table>(canvas);
         tableGraph->SetData(juniorSeniorMajors,
            { L"Division", L"Department", L"Female", L"Male" });
         // group the schools together in the first row
         tableGraph->GroupColumn(0);

         // add ratio aggregate column and group row totals
         const wxColour aggColumnBkColor =
            ColorBrewer::GetColor(Colors::Color::LightGray,
                                  Settings::GetTranslucencyValue());
         tableGraph->InsertAggregateColumn(Table::AggregateInfo(Table::AggregateType::Ratio),
                                          _(L"Ratio"), std::nullopt, aggColumnBkColor);
         tableGraph->InsertRowTotals(aggColumnBkColor);

         // make the headers and row groups bold (and center the headers)
         tableGraph->BoldRow(0);
         tableGraph->BoldColumn(0);
         tableGraph->CenterRowHorizontally(0);

         const auto& ratioOutliers =
            // Find outlier in the female-to-male ratios for the majors.
            // (Note that we use a more liberal search, considering
            // z-scores > 2 as being outliers
            tableGraph->GetOutliers(tableGraph->GetColumnCount()-1, 2);
         // if any outliers, make a note of it off to the side
         if (ratioOutliers.size())
            {
            tableGraph->AddCellAnnotation(
                { L"Majors with the most lopsided female-to-male ratios",
                   ratioOutliers, Side::Right }
                );
            }

         // if you also want to place annotations on the left of the table,
         // then center it within its drawing area like so:
         // tableGraph->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);

         // add a title
         canvas->GetTopTitles().push_back(Label(
            GraphItemInfo(_(L"Top 20 Majors for Juniors & Seniors (AY2021-22)")).
            Padding(5, 5, 5, 5).Pen(wxNullPen).
            ChildAlignment(RelativeAlignment::FlushLeft).
            FontBackgroundColor(ColorBrewer::GetColor(Color::MossGreen))) );

         tableGraph->GetCaption().SetText(_(L"Source: Office of Institutional Research"));
         tableGraph->GetCaption().SetPadding(5, 5, 5, 5);

         // add the table to the canvas
         canvas->SetFixedObject(0, 0, tableGraph);

         // make the canvas tall since we it's a long table, but not very wide
         canvas->SetCanvasMinHeightDIPs(
            canvas->GetDefaultCanvasWidthDIPs());
         canvas->SetCanvasMinWidthDIPs(
            canvas->GetDefaultCanvasHeightDIPs());

         canvas->FitToPageWhenPrinting(true);
        @endcode*/
    class Table final : public Graph2D
        {
    public:
        /// @brief How to aggregate a row or column.
        enum class AggregateType
            {
            /// @brief Sums a series of values.
            Total,
            /// @brief Calculates the change from one value to another (as a percentage).
            ChangePercent,
            /// @brief Calculates the ratio between two values
            ///     (ratios will be rounded to integers if the cell's precision is zero).
            Ratio
            };

        /// @brief Information about how to build an aggregation column.
        struct AggregateInfo
            {
            friend class Table;
        public:
            /// @brief Constructor.
            /// @param type Which type of aggregation to perform.
            explicit AggregateInfo(const AggregateType type) : m_type(type)
                {}
            /// @brief The first column in the series of data.
            /// @param first The first column.
            /// @returns A self reference.
            AggregateInfo& FirstCell(const size_t first) noexcept
                {
                m_cell1 = first;
                return *this;
                }
            /// @brief The last column in the series of data.
            /// @param last The last column.
            /// @returns A self reference.
            AggregateInfo& LastCell(const size_t last) noexcept
                {
                m_cell2 = last;
                return *this;
                }
        private:
            AggregateType m_type{ AggregateType::Total };
            std::optional<size_t> m_cell1;
            std::optional<size_t> m_cell2;
            };

        /// @brief How to display a cell's content.
        enum class CellFormat
            {
            /// @brief Displays a number generically.
            General,
            /// @brief Displays a value such as @c 0.25 as @c 25%.
            Percent
            };

        /// @brief Types of values that can be used for a cell.
        using CellValueType = std::variant<double, wxString, wxDateTime,
                                           std::pair<double, double>>;
        /// @brief The row and column position of a cell.
        using CellPosition = std::pair<size_t, size_t>;

        /// @brief An annocation to add to the table, connected to a set of cells.
        struct CellAnnotation
            {
            /// @brief The note to display in the gutter next to the table.
            wxString m_note;
            /// @brief The cells to highlight and connect the note to.
            std::vector<CellPosition> m_cells;
            /// @brief Which side of the table that the note should be on.
            /// @note This will be overridden if there the page placement
            ///     of the table conflicts with this option. For example,
            ///     if the table is left aligned in the drawing area,
            ///     then this value will be ignored and the note will always
            ///     appear to the right of the table.
            Side m_side{ Side::Right };
            };

        /// @brief A cell in the table.
        class TableCell
            {
            friend class Table;
        public:
            /// @brief Constructor.
            /// @param value The value for the cell.
            /// @param bgColor The cell's background color.
            TableCell(const CellValueType& value, const wxColour bgColor) :
                m_value(value), m_bgColor(bgColor)
                {}
            /// @private
            TableCell() = default;
            /// @brief Gets the value as it is displayed in the cell.
            /// @returns The displayable string for the cell.
            [[nodiscard]] wxString GetDisplayValue() const;

            /// @returns @c true if the cell is text.
            [[nodiscard]] bool IsText() const noexcept
                { return (std::get_if<wxString>(&m_value) != nullptr); }
            /// @returns @c true if the cell is a number.
            [[nodiscard]] bool IsNumeric() const noexcept
                { return (std::get_if<double>(&m_value) != nullptr); }
            /// @returns @c true if the cell is a date.
            [[nodiscard]] bool IsDate() const noexcept
                { return (std::get_if<wxDateTime>(&m_value) != nullptr); }
            /// @returns @c true if the cell is a ratio.
            [[nodiscard]] bool IsRatio() const noexcept
                { return (std::get_if<std::pair<double, double>>(&m_value) != nullptr); }

            /// @brief Sets the value.
            /// @param value The value to set for the cell.
            void SetValue(const CellValueType& value)
                { m_value = value; }
            /// @brief Sets the color.
            /// @param color The value to set for the cell.
            void SetBackgroundColor(const wxColour color)
                { m_bgColor = color; }
            /// @returns Access to the cell's font. This can be useful for changing
            ///     an attribute of the font, rather than entirely setting a new font.
            [[nodiscard]] wxFont& GetFont() noexcept
                { return m_font; }
            /// @brief Sets the cell's font.
            /// @param font The font to use.
            void SetFont(const wxFont& font)
                { m_font = font; }
            /// @returns @c true if cell is being highlighted.
            [[nodiscard]] bool IsHighlighted() const noexcept
                { return m_isHighlighted; }
            /// @brief Draw a highlighted border around the cell.
            /// @param highlight @c true to highlight the cell.
            void Highlight(const bool highlight) noexcept
                { m_isHighlighted = highlight; }
            /// @brief Sets the number of columns that this cell should consume.
            /// @param colCount The number of cells that this should consume horizontally.
            void SetColumnCount(const int colCount) noexcept
                {
                if (colCount <= 0)
                    { m_columnCount = 1; }
                else
                    { m_columnCount = colCount; }
                }
            /// @brief Sets the number of rows that this cell should consume.
            /// @param rowCount The number of cells that this should consume vertically.
            void SetRowCount(const int rowCount) noexcept
                {
                if (rowCount <= 0)
                    { m_rowCount = 1; }
                else
                    { m_rowCount = rowCount; }
                }
            /// @brief Sets the suggested line length for the cell (if text).
            /// @param lineLength The suggested line length.
            void SetSuggestedLineLength(const size_t lineLength) noexcept
                { m_suggestedLineLength = lineLength; }
            /// @brief Shows or hides the left border of a cell if it's in the first column.
            /// @param show @c false to hide the left outer border of a cell.
            void ShowOuterLeftBorder(const bool show)
                { m_showOuterLeftBorder = show; }
            /// @brief Shows or hides the top border of a cell if it's in the first row.
            /// @param show @c false to hide the top outer border of a cell.
            void ShowOuterTopBorder(const bool show)
                { m_showOuterTopBorder = show; }
            /// @brief Shows or hides the bottom border of a cell if it's in the last row.
            /// @param show @c false to hide the bottom outer border of a cell.
            void ShowOuterBottomBorder(const bool show)
                { m_showOuterBottomBorder = show; }
            /// @brief Shows or hides the right border of a cell if it's in the last column.
            /// @param show @c false to hide the right outer border of a cell.
            void ShowOuterRightBorder(const bool show)
                { m_showOuterRightBorder = show; }
        private:
            /// @brief Returns a double value representing the cell.
            /// @details This is useful for comparing cells (or aggregating them).
            /// @returns If numeric, returns the underlying double value.
            ///     If a ratio, returns the magnitude of larger value compared to
            ///     the smaller one.\n
            ///     Otherwise, returns NaN.
            [[nodiscard]] double GetDoubleValue() const noexcept
                {
                if (IsNumeric())
                    {
                    const auto dVal{ std::get_if<double>(&m_value) };
                    if (dVal)
                        { return *dVal; }
                    else
                        { return std::numeric_limits<double>::quiet_NaN(); }
                    }
                else if (IsRatio())
                    {
                    const auto rVals{ std::get_if<std::pair<double, double>>(&m_value) };
                    if (rVals)
                        {
                        return (rVals->first >= rVals->second) ?
                            safe_divide<double>(rVals->first, rVals->second) :
                            safe_divide<double>(rVals->second, rVals->first);
                        }
                    else
                        { return std::numeric_limits<double>::quiet_NaN(); }
                    }
                else
                    { return std::numeric_limits<double>::quiet_NaN(); }
                }

            CellValueType m_value{ std::numeric_limits<double>::quiet_NaN() };
            CellFormat m_valueFormat{ CellFormat::General };
            uint8_t m_precision{ 0 };
            wxColour m_bgColor{ *wxWHITE };
            wxFont m_font{ wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT) };
            std::optional<PageHorizontalAlignment> m_horizontalCellAlignment;
            std::optional<size_t> m_suggestedLineLength;

            int m_columnCount{ 1 };
            int m_rowCount{ 1 };

            bool m_showOuterLeftBorder{ true };
            bool m_showOuterTopBorder{ true };
            bool m_showOuterRightBorder{ true };
            bool m_showOuterBottomBorder{ true };

            bool m_isHighlighted{ false };
            };

        /// @brief Constructor.
        /// @param canvas The canvas to draw the table on.
        explicit Table(Wisteria::Canvas* canvas);

        /** @brief Set the data for the table.
            @param data The data.
            @param columns The columns to display in the table.\n
                The columns will appear in the order that you specify here.
            @param transpose @c true to transpose the data (i.e., display the columns
                from the data as rows).
            @throws std::runtime_error If any columns can't be found by name,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const std::initializer_list<wxString>& columns,
                     const bool transpose = false);

        /// @name Table Functions
        /// @brief Functions for editing the table as a whole.
        /// @{

        /// @brief Sets the size of the table.
        /// @details This should only be used if building a table from scratch.
        ///     Prefer using SetData() instead, unless you plan to manually fill
        ///     the data cell-by-cell.
        /// @param rows The number of rows.
        /// @param cols The number of columns.
        /// @note If the table is being made smaller, then existing content
        ///     outside of the new size will be removed; other existing content
        ///     will be preserved.\n
        ///     Call ClearTable() to clear any existing content if you wish to
        ///     reset the table.
        void SetTableSize(const size_t rows, const size_t cols)
            {
            m_table.resize(rows);
            for (auto& row : m_table)
                { row.resize(cols); }
            }
        /// @brief Empties the contents of the table.
        void ClearTable() noexcept
            { m_table.clear(); }
        /** @brief Sets the font for the entire table.
            @param ft The font to apply.*/
        void SetTableFont(const wxFont& ft)
            {
            if (GetColumnCount() > 0)
                {
                for (auto& row : m_table)
                    {
                    for (auto& cell : row)
                        { cell.SetFont(ft); }
                    }
                }
            }
        /// @brief Sets the minimum percent of the drawing area's width that the
        ///     table should consume (between @c 0.0 to @c 1.0, representing 0% to 100%).
        /// @param percent The minimum percent of the area's width that the table should consume.
        void SetMinWidthProportion(const double percent)
            { m_minWidthProportion = std::clamp(percent, 0.0, 1.0); }
        /// @brief Sets the minimum percent of the drawing area's height that the
        ///     table should consume (between @c 0.0 to @c 1.0, representing 0% to 100%).
        /// @param percent The minimum percent of the area's height that the table should consume.
        void SetMinHeightProportion(const double percent)
            { m_minHeightProportion = std::clamp(percent, 0.0, 1.0); }

        /// @returns The pen used to highlight specific cells (e.g., outliers).
        [[nodiscard]] wxPen& GetHighlightPen() noexcept
            { return m_highlightPen; }
        /// @}

        /// @name Row & Column Functions
        /// @brief Functions for editing rows and columns.
        /// @{

        /// @returns The number of rows.
        /// @warning This will include the first row which contains the original dataset's
        ///     column names (unless it was transposed in the call to SetData()).
        [[nodiscard]] size_t GetRowCount() const noexcept
            { return m_table.size(); }
        /// @returns The number of columns.
        /// @warning If the imported file was transposed, then this will also include
        ///     the first column which contains the dataset's original column names.
        [[nodiscard]] size_t GetColumnCount() const noexcept
            { return (GetRowCount() == 0) ? 0 : m_table[0].size(); }

        /// @brief Inserts an empty row at the given index.
        /// @details For example, an index of @c 0 will insert the row at the
        ///     top of the table.
        /// @note If the table's size has not been established yet (via SetData()
        ///     or SetTableSize()), then calls to this will ignored since the
        ///     number of columns is unknown.
        /// @param rowIndex Where to insert the row.
        /// @param rowName An optional value for the left-most column of the new
        ///     row, representing a name for the row.
        void InsertRow(const size_t rowIndex, std::optional<wxString> rowName = std::nullopt)
            {
            if (GetColumnCount())
                {
                auto insertedRow = m_table.insert(m_table.cbegin() +
                        // clamp indices going beyond the row count to m_table.cend()
                        std::clamp<size_t>(rowIndex, 0, GetRowCount()),
                    std::vector<TableCell>(GetColumnCount(), TableCell()));
                if (rowName.has_value())
                    { insertedRow->at(0).m_value = rowName.value(); }
                }
            }
        /// @brief Inserts an empty column at the given index.
        /// @details For example, an index of @c 0 will insert the column at the
        ///     left side of the table.
        /// @note If the table's size has not been established yet (via SetData()
        ///     or SetTableSize()), then calls to this will ignored since there will
        ///     be no rows to insert columns into.
        /// @param colIndex Where to insert the column.
        /// @param colName An optional value for the first row of the new
        ///     column, representing a name for the column.
        void InsertColumn(const size_t colIndex, std::optional<wxString> colName = std::nullopt)
            {
            if (GetColumnCount())
                {
                for (auto& row : m_table)
                    {
                    row.insert(row.cbegin() +
                        // clamp indices going beyond the column count to row.cend()
                        std::clamp<size_t>(colIndex, 0, row.size()),
                        TableCell());
                    }
                if (colName.has_value())
                    { m_table[0][colIndex].m_value = colName.value(); }
                }
            }
        /** @brief Adds an aggregate (e.g., total) row to the end of the table.
            @param aggInfo Which type of aggregation to use in the column.
            @param rowName An optional value for the first row of the new
                row, representing a name for the row.\n
                This will be overwritten by a calculated value if the left-most column is not text.
            @param rowIndex Where to (optionally) insert the column. The default
                is to insert as the last row.
            @param bkColor An optional background for the row.
            @note This should be called after all data has been set because the
                the aggregation values are calculated as this function is called.
            @sa InsertRowTotals() for a simplified way to insert a total row
                (as well as subtotal rows).*/
        void InsertAggregateRow(const AggregateInfo& aggInfo,
                                std::optional<wxString> rowName = std::nullopt,
                                std::optional<size_t> rowIndex = std::nullopt,
                                std::optional<wxColour> bkColor = std::nullopt);
        /** @brief Adds an aggregate (e.g., total) column into the table.
            @param aggInfo Which type of aggregation to use in the column.
            @param colName An optional value for the first row of the new
                column, representing a name for the column.\n
                This will be overwritten by a calculated value if the top row is not text.
            @param colIndex Where to (optionally) insert the column. The default
                is to insert as the last column.
            @param bkColor An optional background for the column.
            @note This should be called after all data has been set because the
                the aggregation values are calculated as this function is called.*/
        void InsertAggregateColumn(const AggregateInfo& aggInfo,
                                   std::optional<wxString> colName = std::nullopt,
                                   std::optional<size_t> colIndex = std::nullopt,
                                   std::optional<wxColour> bkColor = std::nullopt);
        /** @brief Inserts total (and possibly subtotal) rows into a table.
            @details If the first column contains grouped labels (see GroupColumn())
                and the second column contains labels, then subtotal rows will be inserted
                beneath each parent group. Also, a grand total row will be inserted at the
                bottom of the table.\n
                Otherwise, a single total row will be inserted at the bottom for all rows.
            @param bkColor An optional background for the row(s).*/
        void InsertRowTotals(std::optional<wxColour> bkColor = std::nullopt);

        /// @brief Sets the background color for a given row.
        /// @param row The row to change.
        /// @param color The background color to apply to the row.
        /// @param startColumn An optional column in the row to start from.\n
        ///     The default is to start from the first column.
        /// @param endColumn An optional column in the row to end at.\n
        ///     The default is to end at the last column.
        /// @note This will have no effect until the table's dimensions have been specified
        ///     via SetData() or SetTableSize().
        void SetRowBackgroundColor(const size_t row, const wxColour color,
                                   std::optional<size_t> startColumn = std::nullopt,
                                   std::optional<size_t> endColumn = std::nullopt);
        /// @brief Sets the background color for a given column.
        /// @param column The column to change.
        /// @param color The background color to apply to the column.
        /// @note This will have no effect until the table's dimensions have been specified
        ///     via SetData() or SetTableSize().
        void SetColumnBackgroundColor(const size_t column, const wxColour color)
            {
            if (GetRowCount() && column < GetColumnCount())
                {
                for (auto& row : m_table)
                    {
                    if (column < row.size())
                        { row[column].m_bgColor = color; }
                    }
                }
            }

        /** @brief Makes the specified row use a bold font.
            @param row The row to make bold.*/
        void BoldRow(const size_t row)
            {
            if (row < GetRowCount())
                {
                auto& currentRow = m_table[row];
                for (auto& cell : currentRow)
                    { cell.GetFont().MakeBold(); }
                }
            }
        /** @brief Makes the specified column use a bold font.
            @param column The column to make bold.*/
        void BoldColumn(const size_t column)
            {
            if (GetColumnCount() > 0)
                {
                for (auto& row : m_table)
                    {
                    if (column < row.size())
                        { row[column].GetFont().MakeBold(); }
                    }
                }
            }

        /** @brief Sets the specified row's precision.
            @param row The row to edit.
            @param precision The precision for the row.*/
        void SetRowPrecision(const size_t row, const uint8_t precision)
            {
            if (row < GetRowCount())
                {
                auto& currentRow = m_table[row];
                for (auto& cell : currentRow)
                    { cell.m_precision = precision; }
                }
            }
        /** @brief Sets the specified column's precision.
            @param column The column to edit.
            @param precision The precision for the column.*/
        void SetColumnPrecision(const size_t column, const uint8_t precision)
            {
            if (GetColumnCount() > 0)
                {
                for (auto& row : m_table)
                    {
                    if (column < row.size())
                        { row[column].m_precision = precision; }
                    }
                }
            }

        /** @brief Makes the specified row's cells have horizontally centered content.
            @param row The row to have horizontally centered cell content.*/
        void CenterRowHorizontally(const size_t row)
            {
            if (row < GetRowCount())
                {
                auto& currentRow = m_table[row];
                for (auto& cell : currentRow)
                    { cell.m_horizontalCellAlignment = PageHorizontalAlignment::Centered; }
                }
            }
        /** @brief Makes the specified column's cells have horizontally centered content.
            @param column The column to have horizontally centered cell content.*/
        void CenterColumnHorizontally(const size_t column)
            {
            if (GetColumnCount() > 0)
                {
                for (auto& row : m_table)
                    {
                    if (column < row.size())
                        {
                        row[column].m_horizontalCellAlignment = PageHorizontalAlignment::Centered;
                        }
                    }
                }
            }

        /** @brief Across a given row, combines consecutive cells with the same label
                into one cell.
            @details For example, if a top row has three consecutive cells displaying "FY1982,"
                then this will combine them one with "FY1982" centered in it.\n
                This can be useful for showing grouped data.
            @param row The row to combine cells within.*/
        void GroupRow(const size_t row);
        /** @brief Down a given column, combines consecutive cells with the same label
                into one cell.
            @details For example, if the far-left column has three consecutive cells
                displaying "Business," then this will combine them one with "Business"
                centered in it.\n
                This can be useful for showing grouped data.
            @param column The column to combine cells within.*/
        void GroupColumn(const size_t column);

        /** @brief Finds the outliers from the specified column.
            @details This can be used for highlighting outliers (and possibly annotating them).
            @sa AddCellAnnotation().
            @param column The column to review.
            @param outlierThreshold The z-score threshold for determining if a value
                is an outlier. Normally this should be @c 3.0 (representing a value
                being three standard deviations from the mean). A lower value will
                be more liberal in classifying a value as an outlier; a higher value
                will be more strict.
            @returns The cell positions of the outliers.
            @warning This should not be called on columns with subtotal rows
                (see InsertAggregateRow()), as these will this will break the outlier calculation.
                If you must run this against such a column, be sure to remove these rows
                from the returned set of cell positions.\n
                Also, any changes to the structure of the table (inserting more rows or columns)
                will make the returned postions incorrect. This should be
                called after all structural changes to the table.*/
        std::vector<CellPosition> GetOutliers(const size_t column,
                                              const double outlierThreshold = 3.0);

        /// @brief Applies rows of alternating colors ("zebra stripes") to the table.
        /// @param alternateColor The background color to apply to ever other row.
        /// @param startRow The row to start from (default is @c 0).
        /// @param startColumn An optional column in the row to start from.\n
        ///     The default is to start from the first column.
        /// @param endColumn An optional column in the row to end at.\n
        ///     The default is to end at the last column.
        /// @note This will have no effect until the table's dimensions have been specified
        ///     via SetData() or SetTableSize().
        void ApplyAlternateRowColors(const wxColour alternateColor,
                                     const size_t startRow = 0, 
                                     std::optional<size_t> startColumn = std::nullopt,
                                     std::optional<size_t> endColumn = std::nullopt);
        /// @}

        /// @name Cell Functions
        /// @brief Functions for editing cells.
        /// @{

        /// @brief Accesses the cell at a given position.
        /// @param row The row index of the cell.
        /// @param column The column index of the cell.
        /// @returns The requested cell.
        /// @throws std::runtime_error If row or column index is out of range, throws an exception.\n
        ///     The exception's @c what() message is UTF-8 encoded, so pass it to
        ///     @c wxString::FromUTF8() when formatting it for an error message.
        [[nodiscard]] TableCell& GetCell(const size_t row, const size_t column);

        /// @brief Highlights the specified cells and adds a note pointing to them.
        /// @param cellNote Information about the cell(s) to highlight,
        ///     the note, and where to place it relative to the table.
        void AddCellAnnotation(const CellAnnotation& cellNote);
        /// @}

        /// @private
        void AddCellAnnotation(CellAnnotation&& cellNote);
        /// @private
        [[deprecated("Tables do not support legends.")]]
        [[nodiscard]] std::shared_ptr<GraphItems::Label> CreateLegend(
            [[maybe_unused]] const LegendOptions& options) override
            { return nullptr; }
    private:
        void RecalcSizes(wxDC& dc) final;

        /** @brief Determines which gutter a note should go into.
            @details Notes will have its gutter specified, but the tables page
                placement may conflict with this value. This function will determine
                if there is such a conflict and return the appropriate gutter.
            @param note The annotation to review.
            @returns The gutter that the note should go into.*/
        [[nodiscard]] Side DeduceGutterSide(const CellAnnotation& note) const noexcept
            {
            return ((note.m_side == Side::Right &&
                     GetPageHorizontalAlignment() != PageHorizontalAlignment::RightAligned) ||
                    // left side, but table is left aligned and there is no space for it
                    (note.m_side == Side::Left &&
                     GetPageHorizontalAlignment() == PageHorizontalAlignment::LeftAligned)) ?
                    Side::Right : Side::Left;
            }

        /** @brief Calculates the table's cell dimensions, overall width and height,
                adjusted drawing area. This drawing area includes the table and its
                gutter annotations.
            @param[out] columnWidths The calculated column widths.
            @param[out] rowHeights The calculated row heights.
            @param[in,out] drawArea The initial and updated drawing area for the table.
            @param dc The DC to measure text labels with.
            @returns The size of the table proper (i.e., the cells, but not outer annotations).\n
                Note that the size of @c drawArea may be larger if the table includes
                annotations along the gutters.*/
        [[nodiscard]] wxSize CalculateTableSize(std::vector<wxCoord>& columnWidths,
                                                std::vector<wxCoord>& rowHeights,
                                                wxRect& drawArea, wxDC& dc) const;

        /// @returns The area of a given cell if found; otherwise, an invalid rect.
        /// @param row The row index of the cell.
        /// @param column The column index of the cell.
        /// @warning This should only be called during or after a call to RecalcSizes().
        [[nodiscard]] wxRect GetCachedCellRect(const size_t row, const size_t column);

        /// @returns If a cell is being eclipsed vertically by the cell above it,
        ///     then return that cell. Otherwise, return @c std::nullopt.
        /// @param row The row index of the cell.
        /// @param column The column index of the cell.
        [[nodiscard]] std::optional<TableCell> GetParentRowWiseCell(const size_t row,
                                                                    const size_t column);
        /// @returns If a cell is being eclipsed horizontally by the cell left of it,
        ///     then return that cell. Otherwise, return @c std::nullopt.
        /// @param row The row index of the cell.
        /// @param column The column index of the cell.
        [[nodiscard]] std::optional<TableCell> GetParentColumnWiseCell(const size_t row,
                                                                       const size_t column);

        /** @brief Calculates an aggregation from a series of values.
            @param aggInfo Which type of aggregate calculation to perform.
            @param aggCell The cell to place the result.
            @param values The values for the calculation.*/
        void CalculateAggregate(const AggregateInfo& aggInfo, TableCell& aggCell,
                                const std::vector<double>& values);

        // DIPs for annotation connection lines and space between lines
        static constexpr wxCoord m_labelSpacingFromLine{ 5 };
        static constexpr wxCoord m_connectionOverhangWidth{ 10 };

        std::vector<std::vector<TableCell>> m_table;
        std::optional<double> m_minWidthProportion;
        std::optional<double> m_minHeightProportion;

        std::vector<CellAnnotation> m_cellAnnotations;

        wxPen m_highlightPen{ wxPen(*wxRED_PEN) };

        // cached values
        std::vector<std::vector<wxRect>> m_cachedCellRects;
        };
    }

/** @}*/

#endif //__WISTERIA_TABLE_H__
