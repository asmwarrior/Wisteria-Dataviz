/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_PROCONROADMAP_H__
#define __WISTERIA_PROCONROADMAP_H__

#include "roadmap.h"

namespace Wisteria::Graphs
    {
    /** @brief Pro & Con %Roadmap, which compares pros and cons (i.e., positive and negative
            sentiments) about a subject. This can be useful for surveys and SWOT
            (<b>S</b>trengths/<b>W</b>eaknesses/<b>O</b>pportunites/<b>T</b>hreats) analyses.
        @details This graphic displays a road leading towards a final goal.
            Along this road are "road stops" of variable size that cause the road to curve. These
            road stops represent the pros and cons from survey responses. Both the
            size of a road stop and the curve in the road next to it represent the number of times the
            pro or con was mentioned in the survey.\n
            \n
            The stops on the right side of the road are pros, the left ones are cons.
        @par Two Pro & Con Roadmaps stacked into a SWOT analysis report:

        @image html SWOTRoadmap.svg width=70%

        @par %Data:
            This plot accepts a Data::Dataset where one categorical column is the positive
            sentiments (pros), and another categorical is the negative sentiments (cons).
            Continuous columns can also be included which include the aggregated counts for
            each of the respective sentiments. (If these are not provided, then frequency
            counts of the sentiments are used.)\n
            \n
            The example below shows a @c Strength column that will have its categories aggregated
            by the graph. Meanwhile, the @c Weakness column will be loaded using the pre-calculated
            totals from the corresponding @c WeeknessTotals column.

            | Strength                 | Weekness            | WeeknessTotals |
            | :--                      | :--                 | --:            |
            | New features             | Cost                | 22             |
            | New features             | Missing features    | 12             |
            | Improved user management | Employee retraining | 12             |
            ...

        @par Missing Data:
            Missing sentiment labels or aggregate counts will result in list-wise deletion.

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the plot
         auto canvas = new Wisteria::Canvas{ this };
         canvas->SetFixedObjectsGridSize(3, 1);

         // strengths and weaknesses
            {
            auto swData = std::make_shared<Data::Dataset>();
            try
                {
                swData->ImportCSV(L"/home/mwalker/data/ERP Migration Survey.csv",
                    ImportInfo().
                    CategoricalColumns({
                        { L"Strength", CategoricalImportMethod::ReadAsStrings },
                        { L"Weakness", CategoricalImportMethod::ReadAsStrings }
                        }));
                }
            catch (const std::exception& err)
                {
                wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                             _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
                return;
                }

            auto roadmap = std::make_shared<ProConRoadmap>(canvas);
            roadmap->SetData(swData,
                             L"Strength", std::nullopt,
                             L"Weakness", std::nullopt, 2);
            roadmap->SetCanvasMargins(5, 5, 0, 5);
            roadmap->GetLeftYAxis().GetTitle().SetText(_("Strengths & Weaknesses"));
            roadmap->GetLeftYAxis().GetTitle().SetMinimumUserSizeDIPs(30, std::nullopt);
            // don't include the counts on the labels
            roadmap->SetMarkerLabelDisplay(Roadmap::MarkerLabelDisplay::Name);

            canvas->SetFixedObject(0, 0, roadmap);
            }

         // opportunities and threats
            {
            auto swData = std::make_shared<Data::Dataset>();
            try
                {
                swData->ImportCSV(L"/home/mwalker/data/ERP Migration Survey.csv",
                    ImportInfo().
                    CategoricalColumns({
                        { L"Opportunity", CategoricalImportMethod::ReadAsStrings },
                        { L"Threat", CategoricalImportMethod::ReadAsStrings }
                        }));
                }
            catch (const std::exception& err)
                {
                wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                             _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
                return;
                }

            auto roadmap = std::make_shared<ProConRoadmap>(canvas);
            roadmap->SetData(swData,
                             L"Opportunity", std::nullopt,
                             L"Threat", std::nullopt,
                             // ignore items that are only mentioned once
                             2);
            roadmap->SetCanvasMargins(0, 5, 5, 5);
            roadmap->GetLeftYAxis().GetTitle().SetText(_("Opportunities & Threats"));
            roadmap->GetLeftYAxis().GetTitle().SetMinimumUserSizeDIPs(30, std::nullopt);
            // add the default caption explaining how to read the graph
            roadmap->AddDefaultCaption();
            // don't include the counts on the labels
            roadmap->SetMarkerLabelDisplay(Roadmap::MarkerLabelDisplay::Name);

            canvas->SetFixedObject(1, 0, roadmap);

            // add the legend at the bottom (beneath the explanatory caption)
            roadmap->SetPositiveLegendLabel(_(L"Strengths & Opportunities"));
            roadmap->SetNegativeLegendLabel(_(L"Weaknesses & Threats"));
            auto legend = roadmap->CreateLegend(LegendCanvasPlacementHint::AboveOrBeneathGraph, true);
            const auto proportionForEachRoadMap =
                safe_divide<double>(1 - canvas->CalcMinHeightProportion(legend), 2);
            canvas->GetRowInfo(0).HeightProportion(proportionForEachRoadMap);
            canvas->GetRowInfo(1).HeightProportion(proportionForEachRoadMap);
            // calculate the canvas height that the legend needs and lock it
            canvas->GetRowInfo(2).
                HeightProportion(canvas->CalcMinHeightProportion(legend)).
                LockProportion(true);
            canvas->SetFixedObject(2, 0, legend);
            }

         // add a title with a green banner background and white font
         Label topTitle(GraphItemInfo(_("ERP Migration SWOT Analysis\n"
            "Employee Survey Results Regarding Proposed Migration to new ERP Software")).
            Padding(5, 5, 5, 5).
            ChildAlignment(RelativeAlignment::FlushLeft).
            FontColor(*wxWHITE).
            FontBackgroundColor(ColorBrewer::GetColor(Colors::Color::HunterGreen)));
         topTitle.GetHeaderInfo().Enable(true).FontColor(*wxWHITE).GetFont().MakeBold();
         canvas->GetTopTitles().push_back(topTitle);

         // make the canvas tall since we are stacking two graphs on top of each other
         canvas->SetCanvasMinHeightDIPs(canvas->GetDefaultCanvasHeightDIPs() * 2);
         // also, fit it to the entire page when printing (preferrably in portait)
         canvas->FitToPageWhenPrinting(true);
         
        @endcode
    */
    class ProConRoadmap final : public Roadmap
        {
    public:
        /** @brief Constructor.
            @param canvas The canvas to draw the graph on.*/
        explicit ProConRoadmap(Canvas* canvas) : Roadmap(canvas)
            {
            // showing counts, so don't display any values as negative
            SetMarkerLabelDisplay(MarkerLabelDisplay::NameAndAbsoluteValue);
            }

        /** @brief Sets the data.
            @param data The data to use for the graph.
            @param positiveColumnName The column containing the positive
                (i.e., pros, strengths, opportunities, etc.) labels.
            @param positiveValueColumnName The column containing the positive labels'
                aggregated totals.\n
                This is optional; if not used, the frequencies of the labels
                as they appear in the positive column are used.
            @param negativeColumnName The column containing the negative
                (i.e., cons, weaknesses, threats, etc.) labels.
            @param negativeValueColumnName The column containing the negative labels'
                aggregated totals.\n
                This is optional; if not used, the frequencies of the labels
                as they appear in the negative column are used.
            @param minimumCountForItem The minimum occurrence for an item to be included.\n
                This is useful for excluding items not mentioned often in the survey.
                The default is to include all items.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(std::shared_ptr<const Data::Dataset> data,
                     const wxString& positiveColumnName,
                     const std::optional<wxString>& positiveValueColumnName,
                     const wxString& negativeColumnName,
                     const std::optional<wxString>& negativeValueColumnName,
                     const std::optional<size_t>& minimumCountForItem);

        /// @brief Adds a caption explaining how to interpret the graph.
        void AddDefaultCaption() final;

        /** @brief Sets the legend label for positive influencers.
            @param label The label to display.*/
        void SetPositiveLegendLabel(const wxString& label)
            { m_positiveLabel = label; }
        /** @brief Sets the legend label for negative influencers.
            @param label The label to display.*/
        void SetNegativeLegendLabel(const wxString& label)
            { m_negativeLabel = label; }
    private:
        /// @returns The positive label used for the legend.
        [[nodiscard]] wxString GetPositiveLegendLabel() const final
            { return m_positiveLabel; }
        /// @returns The negative label used for the legend.
        [[nodiscard]] wxString GetNegativeLegendLabel() const final
            { return m_negativeLabel; }
        
        wxString m_positiveLabel{ _("Pro") };
        wxString m_negativeLabel{ _("Con") };
        };
    }

/** @}*/

#endif //__WISTERIA_PROCONROADMAP_H__
