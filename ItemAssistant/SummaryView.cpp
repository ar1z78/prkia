#include "StdAfx.h"
#include "SummaryView.h"
#include "DBManager.h"
#include "DataModel.h"
#include "CharacterSummaryFormatter.h"
#include "StringUtil.h"
#include <tchar.h>
namespace std {
    typedef basic_string<TCHAR> tstring;
}


namespace aoia { namespace sv {

    SummaryView::SummaryView(sqlite::IDBPtr db, aoia::IGuiServicesPtr gui)
        : m_db(db)
        , m_webview(_T(""))
        , m_gui(gui)
    {
    }

    SummaryView::~SummaryView()
    {
    }

    LRESULT SummaryView::OnCreate( LPCREATESTRUCT createStruct )
    {
        m_webview.Create(m_hWnd);

        return 0;
    }

    LRESULT SummaryView::OnSize( UINT wParam, CSize newSize )
    {
        CRect r( CPoint( 0, 0 ), newSize );
        m_webview.SetWindowPos(NULL, r.left, r.top, r.Width(), r.Height(), SWP_NOZORDER | SWP_NOACTIVATE | SWP_DEFERERASE | SWP_NOSENDCHANGING);

        return 0;
    }

    LRESULT SummaryView::OnRefresh( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
    {
        UpdateSummary();
        return 0;
    }

    LRESULT SummaryView::OnHelp( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
    {
        m_gui->ShowHelp(_T("summary"));
        return 0;
    }

    void SummaryView::OnActive( bool doActivation )
    {
        if (doActivation) {
            UpdateSummary();
        }
    }

    void SummaryView::UpdateSummary()
    {
        DataModelPtr model(DataModelPtr(new DataModel(m_db)));

        std::tstringstream contentHtml;

        if (model->getItemCount() != 0)
        {
            CharacterSummaryFormatter formatter(model);
            contentHtml << formatter.toString();
        }

        std::tstring htmlTemplate = GetHtmlTemplate();
        StringUtil::replace_all(htmlTemplate, _T("%SUMMARY%"), contentHtml.str());

        m_webview.SetHTML(htmlTemplate);
    }

    std::tstring SummaryView::GetHtmlTemplate()
    {
        if (!m_template.empty()) {
            return m_template;
        }

        HRSRC hrsrc = ::FindResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_TEMPLATE), RT_HTML);
        DWORD size = ::SizeofResource(_Module.GetResourceInstance(), hrsrc);
        HGLOBAL hGlobal = ::LoadResource(_Module.GetResourceInstance(), hrsrc);
        void * pData = ::LockResource(hGlobal);

        if (pData) {
            std::string raw((char*)pData, size);
            m_template = from_ascii_copy(raw);
        }

        return m_template;
    }

}}  // end of namespace
