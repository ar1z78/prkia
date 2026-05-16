#include "stdafx.h"
#include "DataGridControl.h"
#include <AOManager.h>

namespace aoia {

    DataGridControl::DataGridControl()
    {
        AtlInitCommonControls(ICC_LISTVIEW_CLASSES);
    }


    DataGridControl::~DataGridControl()
    {
    }


    void DataGridControl::setModel(IDataGridModelPtr model)
    {
        // Clean out any previous data
        while (m_listView.GetHeader().GetItemCount())
        {
            m_listView.DeleteColumn(0);
        }
        m_listView.DeleteAllItems();

        if (m_model)
        {
            m_model->disconnect(m_addSignalConnection);
            m_model->disconnect(m_removeSignalConnection);
            m_model->disconnect(m_clearSignalConnection);
            m_model->disconnect(m_updateSignalConnection);
        }
        m_model = model;
        if (m_model)
        {
            
            m_addSignalConnection = m_model->connectItemAdded([this](unsigned int index) { 
                this->onItemAdded(index); 
            });
            m_removeSignalConnection = m_model->connectItemRemoved([this](unsigned int index) { 
                this->onItemRemoved(index); 
            });
            m_clearSignalConnection = m_model->connectAllRemoved([this]() { 
                this->onAllItemsRemoved(); 
            });
            m_updateSignalConnection = m_model->connectCollectionUpdated([this]() { 
                this->onAllItemsUpdated(); 
            });
        }

        updateColumns();
        m_listView.SetItemCountEx(m_model->getItemCount(), 0);
    }


    aoia::IDataGridModelPtr DataGridControl::getModel() const
    {
        return m_model;
    }


    LRESULT DataGridControl::onCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        DefWindowProc(uMsg, wParam, lParam);
        m_listView = m_hWnd;
        m_listView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        return 0;
    }


    LRESULT DataGridControl::onGetDispInfo(int wParam, LPNMHDR lParam, BOOL &bHandled)
    {
        NMLVDISPINFO *pdi = (NMLVDISPINFO*)lParam;
        if (pdi->item.mask & LVIF_TEXT)
        {
            std::tstring text = m_model->getItemProperty(pdi->item.iItem, pdi->item.iSubItem);
            StrCpyN(pdi->item.pszText, text.c_str(), min((int)(text.length()) + 1, pdi->item.cchTextMax));
        }
        return 0;
    }


    void DataGridControl::onAllItemsUpdated()
    {
        m_listView.SetItemCountEx(m_model->getItemCount(), 0);
        m_listView.Invalidate(FALSE);
    }


    void DataGridControl::onItemAdded(unsigned int index)
    {
        if (m_listView.GetItemCount() == 0) {
            updateColumns();
        }
        m_listView.InsertItem(index, LPSTR_TEXTCALLBACK);
        autosizeColumnsUseHeader();
    }


    void DataGridControl::onItemRemoved(unsigned int index)
    {
        m_listView.DeleteItem(index);
        if (m_listView.GetItemCount() == 0) {
            updateColumns();
        }
    }


    void DataGridControl::onAllItemsRemoved()
    {
        m_listView.DeleteAllItems();
        updateColumns();
    }


    void DataGridControl::updateColumns()
    {
        while (m_listView.GetHeader().GetItemCount()) {
            m_listView.DeleteColumn(0);
        }
        for (unsigned int i = 0; i < m_model->getColumnCount(); ++i) {
            m_listView.InsertColumn(i, m_model->getColumnName(i).c_str());
            m_listView.SetColumnWidth(i, 100);  // Default 100px width for all columns.
        }
    }


    void DataGridControl::autosizeColumnsUseHeader()
    {
        int numCol = m_listView.GetHeader().GetItemCount();
        for (int i = 0; i < numCol; ++i)
        {
            m_listView.SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);            
        }
    }


    void DataGridControl::autosizeColumnsUseData(bool onlyVisible)
    {
        if (!onlyVisible)
        {
/*
			int numCol = m_listView.GetHeader().GetItemCount();
            for (int i = 0; i < numCol; ++i)
            {
                m_listView.SetColumnWidth(i, LVSCW_AUTOSIZE);
            }
*/
            // Loop all rows and calculate width on those.
            unsigned int firstRow = m_listView.GetTopIndex();
            unsigned int maxRow = m_listView.GetItemCount();
            TCHAR buffer[MAX_PATH];

            for (unsigned int i = 0; i < m_model->getColumnCount(); ++i) 
            {            
                unsigned int maxWidth = m_listView.GetStringWidth(m_model->getColumnName(i).c_str());
                for (unsigned int rowIndex = firstRow; rowIndex < maxRow; rowIndex++)
                {
                    m_listView.GetItemText(rowIndex, i, buffer, 200);
                    maxWidth = max(maxWidth, (unsigned int)m_listView.GetStringWidth(buffer));
                }
                m_listView.SetColumnWidth(i, maxWidth + 15);
            }

		}
        else 
        {
            // Loop the visible rows and calculate width on those only.
            unsigned int numRows = m_listView.GetCountPerPage();
            unsigned int firstRow = m_listView.GetTopIndex();
            unsigned int maxRow = m_listView.GetItemCount();
            TCHAR buffer[MAX_PATH];

            for (unsigned int i = 0; i < m_model->getColumnCount(); ++i) 
            {            
                unsigned int maxWidth = m_listView.GetStringWidth(m_model->getColumnName(i).c_str());
                for (unsigned int rowIndex = firstRow; (numRows-- > 0) && (rowIndex < maxRow); rowIndex++)
                {
                    m_listView.GetItemText(rowIndex, i, buffer, 200);
                    maxWidth = max(maxWidth, (unsigned int)m_listView.GetStringWidth(buffer));
                }
                m_listView.SetColumnWidth(i, max(maxWidth + 15, (unsigned int)m_listView.GetColumnWidth(i)));
            }
        }
    }


    unsigned int DataGridControl::getSelectedCount() const
    {
        return m_listView.GetSelectedCount();
    }


	void DataGridControl::setSelectedItems(unsigned int containerId) {
		
		int lastId = -1;
		do
		{
			lastId = m_listView.GetNextItem(lastId, LVNI_ALL);
			if (lastId > -1) { 
				//LVITEM item;
				//int data = item.GetItemData(lastId);
				
				m_listView.SetItemState(13, 1, LVIS_SELECTED);
				m_listView.SetSelectionMark(13);

			}
		}
		while(lastId > -1);

		
	}
    std::set<unsigned int> DataGridControl::getSelectedItems() const
    {
        std::set<unsigned int> result;
        
        int lastId = -1;
        do
        {
            lastId = m_listView.GetNextItem(lastId, LVNI_SELECTED);
            if (lastId > -1)
            {
                result.insert(lastId);
            }
        }
        while(lastId > -1);

        return result;
    }


    void DataGridControl::ClearSelection()
    {

        m_listView.SetItemState(-1, 0, LVIS_SELECTED);
    }
LRESULT DataGridControl::onCustomDraw(int wParam, LPNMHDR lParam, BOOL &bHandled)
{
    LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

    switch(pLVCD->nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT:
            return CDRF_NOTIFYSUBITEMDRAW;

        case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
        {
            if (pLVCD->iSubItem == 2) // Your Value Column
            {
                std::tstring text = m_model->getItemProperty(pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem);
                
                size_t firstDigit = text.find_first_of(_T("0123456789"));
                long long val = 0;
                if (firstDigit != std::tstring::npos) {
                    val = _tstoi64(text.c_str() + firstDigit);
                }

                // 1. Get settings from AOManager locally
                SellerSettings s = AOManager::instance().getSellerSettings();
                
                // 2. Local Hex to COLORREF helper
                auto parseHex = [](std::tstring hex) -> COLORREF {
                    TCHAR* end;
                    unsigned long v = _tcstoul(hex.c_str(), &end, 16);
                    return RGB((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
                };

                COLORREF cLow = parseHex(s.colorLow);
                COLORREF cMed = parseHex(s.colorMed);
                COLORREF cHigh = parseHex(s.colorHigh);

                // 3. Apply Gradient Math
                if (val >= s.priceMax) {
                    pLVCD->clrText = cHigh;
                }
                else if (val <= 0) {
                    pLVCD->clrText = cLow;
                }
                else {
                    float t = (float)val / (float)s.priceMax;
                    
                    auto mix = [](COLORREF c1, COLORREF c2, float f) {
                        return RGB(
                            GetRValue(c1) + (int)((GetRValue(c2) - GetRValue(c1)) * f),
                            GetGValue(c1) + (int)((GetGValue(c2) - GetGValue(c1)) * f),
                            GetBValue(c1) + (int)((GetBValue(c2) - GetBValue(c1)) * f)
                        );
                    };

                    if (t < 0.5f) {
                        pLVCD->clrText = mix(cLow, cMed, t * 2.0f);
                    } else {
                        pLVCD->clrText = mix(cMed, cHigh, (t - 0.5f) * 2.0f);
                    }
                }
                return CDRF_NEWFONT;
            }
            else
            {
                pLVCD->clrText = GetSysColor(COLOR_WINDOWTEXT);
                return CDRF_NEWFONT;
            }
        }
    }
    return CDRF_DODEFAULT;
}


}   // namespace
