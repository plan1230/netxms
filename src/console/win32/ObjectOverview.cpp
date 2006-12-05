// ObjectOverview.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectOverview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define X_MARGIN     5
#define Y_MARGIN     5


/////////////////////////////////////////////////////////////////////////////
// CObjectOverview

CObjectOverview::CObjectOverview()
{
   m_pObject = NULL;
}

CObjectOverview::~CObjectOverview()
{
}


BEGIN_MESSAGE_MAP(CObjectOverview, CWnd)
	//{{AFX_MSG_MAP(CObjectOverview)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_SET_OBJECT, OnSetObject)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CObjectOverview message handlers

BOOL CObjectOverview::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         LoadCursor(NULL, IDC_ARROW),
                                         (HBRUSH)(COLOR_WINDOW + 1), NULL);
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CObjectOverview::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create fonts
   m_fontNormal.CreateFont(-MulDiv(7, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, _T("MS Sans Serif"));
   m_fontBold.CreateFont(-MulDiv(7, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, _T("MS Sans Serif"));
   m_fontHeading.CreateFont(-MulDiv(10, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                            0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                            VARIABLE_PITCH | FF_DONTCARE, _T("MS Sans Serif"));

   GetClientRect(&rect);
   rect.top += 50;
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_NOCOLUMNHEADER, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(1, _T("Value"), LVCFMT_LEFT, 400);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	
	return 0;
}


//
// NXCM_SET_OBJECT message handler
//

void CObjectOverview::OnSetObject(WPARAM wParam, NXC_OBJECT *pObject)
{
   m_pObject = pObject;
   Refresh();
}


//
// WM_PAINT message handler
//

void CObjectOverview::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

   if (m_pObject != NULL)
   {
      RECT rect;
      DWORD dwImageIndex;
      int h;

      // Draw icon and objet's name right to it
      dc.FillSolidRect(X_MARGIN - 1, Y_MARGIN - 1, 34, 34, g_statusColorTable[m_pObject->iStatus]);
      dwImageIndex = GetObjectImageIndex(m_pObject);
      g_pObjectNormalImageList->Draw(&dc, dwImageIndex, CPoint(X_MARGIN, Y_MARGIN), ILD_TRANSPARENT);
      dc.SelectObject(&m_fontNormal);
      dc.SetBkColor(RGB(255, 255, 255));
      GetClientRect(&rect);
      h = rect.bottom;
      rect.left = X_MARGIN + 37;
      rect.top = Y_MARGIN;
      rect.bottom = Y_MARGIN + 40;
      rect.right -= X_MARGIN;
      dc.DrawText(m_pObject->szName, -1, &rect, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);

      rect.left = X_MARGIN;
      rect.top = Y_MARGIN + 45;
      rect.bottom = rect.top + 20;
      DrawHeading(dc, _T("Attributes"), &m_fontHeading, &rect, RGB(0, 0, 255), RGB(128, 128, 255));

      rect.top = GetWindowSize(&m_wndListCtrl).cy + Y_MARGIN + 70 + 10;
      rect.bottom = rect.top + 20;
      DrawHeading(dc, _T("Comments"), &m_fontHeading, &rect, RGB(0, 0, 255), RGB(128, 128, 255));

      if (m_pObject->pszComments != NULL)
      {
         rect.top = rect.bottom + 5;
         rect.bottom = h;
         rect.left += 2;
         rect.right -= 2;
         dc.DrawText(m_pObject->pszComments, _tcslen(m_pObject->pszComments), &rect,
                     DT_END_ELLIPSIS | DT_NOPREFIX | DT_WORDBREAK);
      }
   }
   else
   {
      dc.SelectObject(m_fontNormal);
      dc.SetBkColor(RGB(255, 255, 255));
      dc.TextOut(X_MARGIN, Y_MARGIN, _T("No object selected"), 18);
   }
}


//
// WM_SIZE message handler
//

void CObjectOverview::OnSize(UINT nType, int cx, int cy) 
{
   RECT rect;

	CWnd::OnSize(nType, cx, cy);

   m_wndListCtrl.SetWindowPos(NULL, X_MARGIN, Y_MARGIN + 70, cx - X_MARGIN * 2, max(50, (cy - Y_MARGIN - 70) / 2), SWP_NOZORDER);
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.SetColumnWidth(1, rect.right - 100);
}


//
// Refresh view
//

void CObjectOverview::Refresh()
{
   TCHAR szTemp[256];

   m_wndListCtrl.DeleteAllItems();

   InsertItem(_T("ID"), m_pObject->dwId);
   InsertItem(_T("Class"), g_szObjectClass[m_pObject->iClass]);
   if (m_pObject->dwIpAddr != 0)
      InsertItem(_T("IP Address"), IpToStr(m_pObject->dwIpAddr, szTemp));

   // Class-specific information
   switch(m_pObject->iClass)
   {
      case OBJECT_SUBNET:
         InsertItem(_T("Network Mask"), IpToStr(m_pObject->subnet.dwIpNetMask, szTemp));
         break;
      case OBJECT_NODE:
         if (m_pObject->node.dwFlags & NF_IS_NATIVE_AGENT)
         {
            InsertItem(_T("NetXMS Agent"), _T("Active"));
            InsertItem(_T("Agent Version"), m_pObject->node.szAgentVersion);
            InsertItem(_T("Platform Name"), m_pObject->node.szPlatformName);
         }
         else
         {
            InsertItem(_T("NetXMS Agent"), _T("Inactive"));
         }
         if (m_pObject->node.dwFlags & NF_IS_SNMP)
         {
            InsertItem(_T("SNMP Agent"), _T("Active"));
            InsertItem(_T("SNMP OID"), m_pObject->node.szObjectId);
         }
         else
         {
            InsertItem(_T("SNMP Agent"), _T("Inactive"));
         }
         InsertItem(_T("Node Type"), (TCHAR *)CodeToText(m_pObject->node.dwNodeType, g_ctNodeType));
         break;
      case OBJECT_INTERFACE:
         if (m_pObject->dwIpAddr != 0)
            InsertItem(_T("IP Netmask"), IpToStr(m_pObject->iface.dwIpNetMask, szTemp));
         InsertItem(_T("Index"), m_pObject->iface.dwIfIndex);
         InsertItem(_T("Type"), 
                    m_pObject->iface.dwIfType > MAX_INTERFACE_TYPE ?
                           _T("Unknown") : g_szInterfaceTypes[m_pObject->iface.dwIfType]);
         BinToStr(m_pObject->iface.bMacAddr, MAC_ADDR_LENGTH, szTemp);
         InsertItem(_T("MAC Address"), szTemp);
         break;
      default:
         break;
   }

   Invalidate();
}


//
// Insert item into list control
//

void CObjectOverview::InsertItem(TCHAR *pszName, TCHAR *pszValue)
{
   int nItem;

   nItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pszName);
   if (nItem != -1)
      m_wndListCtrl.SetItemText(nItem, 1, pszValue);
}

void CObjectOverview::InsertItem(TCHAR *pszName, DWORD dwValue)
{
   TCHAR szBuffer[64];

   _stprintf(szBuffer, _T("%u"), dwValue);
   InsertItem(pszName, szBuffer);
}
