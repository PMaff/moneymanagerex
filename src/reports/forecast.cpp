/*******************************************************
 Copyright (C) 2016 Guan Lisheng (guanlisheng@gmail.com)
 Copyright (C) 2021 Mark Whalley (mark@ipx.co.uk)

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ********************************************************/

#include "images_list.h"
#include "forecast.h"
#include "mmex.h"
#include "mmframe.h"
#include "util.h"
#include "reports/htmlbuilder.h"
#include "model/Model_Checking.h"

class mm_html_template;

mmReportForecast::mmReportForecast(): mmPrintableBase(wxTRANSLATE("Forecast"))
{
    setReportParameters(Reports::ForecastReport);
}

mmReportForecast::~mmReportForecast()
{
}

wxString mmReportForecast::getHTMLText()
{
    // Grab the data
    std::map<wxString, std::pair<double, double> > amount_by_day;
    Model_Checking::Data_Set all_trans;
    
    if (m_date_range && m_date_range->is_with_date()) {
        all_trans = Model_Checking::instance().find(DB_Table_CHECKINGACCOUNT_V1::TRANSDATE(m_date_range->start_date().FormatISODate(), GREATER_OR_EQUAL)
            , DB_Table_CHECKINGACCOUNT_V1::TRANSDATE(m_date_range->end_date().FormatISODate(), LESS_OR_EQUAL));
    }
    else {
        all_trans = Model_Checking::instance().all();
    }

    for (const auto & trx : all_trans)
    {
        if (Model_Checking::type(trx) == Model_Checking::TRANSFER || Model_Checking::foreignTransactionAsTransfer(trx))
            continue;

        amount_by_day[trx.TRANSDATE].first += Model_Checking::withdrawal(trx, -1);
        amount_by_day[trx.TRANSDATE].second += Model_Checking::deposit(trx, -1);
    }

    

    // Build the report
    mmHTMLBuilder hb;
    hb.init();
    hb.addReportHeader(getReportTitle(), m_date_range->startDay());
    hb.DisplayDateHeading(m_date_range->start_date(), m_date_range->end_date(), m_date_range->is_with_date());

    GraphData gd;
    GraphSeries gsWithdrawal, gsDeposit;
    for (const auto & kv : amount_by_day)
    {
        gd.labels.push_back(kv.first);
        //wxLogDebug(" Values = %d, %d", kv.second.first, kv.second.second);
        gsWithdrawal.values.push_back(kv.second.first);
        gsDeposit.values.push_back(kv.second.second);
    }
    gsDeposit.name = _("Deposit");
    gd.series.push_back(gsDeposit);
    gsWithdrawal.name = _("Withdrawal");
    gd.series.push_back(gsWithdrawal);

    gd.type = GraphData::LINE_DATETIME;
    gd.colors = { mmThemeMetaColour(meta::COLOR_REPORT_CREDIT)
                    , mmThemeMetaColour(meta::COLOR_REPORT_DEBIT) };
    hb.addChart(gd);

    hb.end();

    return hb.getHTMLText();
}
