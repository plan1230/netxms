// DeploymentView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DeploymentView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Constants
//

#define STATUS_HEIGHT         34
#define PROGRESS_CTRL_TOP     20
#define PROGRESS_CTRL_HEIGHT  9


/////////////////////////////////////////////////////////////////////////////
// CDeploymentView

IMPLEMENT_DYNCREATE(CDeploymentView, CMDIChildWnd)

CDeploymentView::CDeploymentView()
{
   m_dwRqId = 0;
   m_bFinished = FALSE;
}

CDeploymentView::~CDeploymentView()
{
}


BEGIN_MESSAGE_MAP(CDeploymentView, CMDIChildWnd)
	//{{AFX_MSG_MAP(CDeploymentView)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_START_DEPLOYMENT, OnStartDeployment)
   ON_MESSAGE(WM_DEPLOYMENT_INFO, OnDeploymentInfo)
   ON_MESSAGE(WM_DEPLOYMENT_FINISHED, OnDeploymentFinished)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeploymentView message handlers

BOOL CDeploymentView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CDeploymentView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create font
   m_font.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                     0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                     VARIABLE_PITCH | FF_DONTCARE, "Verdana");

   // Create progress bar
   GetClientRect(&rect);
   rect.left += 4;
   rect.right -= 4;
   rect.top = PROGRESS_CTRL_TOP;
   rect.bottom = rect.top + PROGRESS_CTRL_HEIGHT;
   m_wndProgressCtrl.Create(WS_CHILD | WS_VISIBLE, rect, this, ID_PROGRESS_CTRL);

   // Create list view inside window
   GetClientRect(&rect);
   rect.top += STATUS_HEIGHT + 1;
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.InsertColumn(0, _T("Node"), LVCFMT_LEFT, 120);
   m_wndListCtrl.InsertColumn(1, _T("Status"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(2, _T("Message"), LVCFMT_LEFT, 250);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

   m_imageList.Create(g_pObjectSmallImageList);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);
	
	return 0;
}


//
// Deployment worker thread
//

static DWORD WINAPI DeploymentThread(DEPLOYMENT_JOB *pJob)
{
   DWORD dwResult;

   dwResult = NXCDeployPackage(g_hSession, pJob->dwPkgId, pJob->dwNumObjects,
                               pJob->pdwObjectList, pJob->pdwRqId);
   PostMessage(pJob->hWnd, WM_DEPLOYMENT_FINISHED, 0, dwResult);
   safe_free(pJob->pdwObjectList);
   free(pJob);
   return 0;
}


//
// WM_START_DEPLOYMENT message handler
//

void CDeploymentView::OnStartDeployment(WPARAM wParam, DEPLOYMENT_JOB *pJob)
{
   DWORD dwThreadId;
   HANDLE hThread;

   m_wndProgressCtrl.SetRange32(0, pJob->dwNumObjects);
   m_wndProgressCtrl.SetStep(1);
   m_wndProgressCtrl.SetPos(0);

   pJob->hWnd = m_hWnd;
   pJob->pdwRqId = &m_dwRqId;
   hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DeploymentThread,
                          pJob, 0, &dwThreadId);
   if (hThread != NULL)
   {
      CloseHandle(hThread);
   }
}


//
// WM_DEPLOYMENT_INFO message handler
//

void CDeploymentView::OnDeploymentInfo(DWORD dwRqId, NXC_DEPLOYMENT_STATUS *pInfo)
{
   if (dwRqId == m_dwRqId)
   {
      LVFINDINFO lvfi;
      int iItem;

      // Find node's record
      lvfi.flags = LVFI_PARAM;
      lvfi.lParam = pInfo->dwNodeId;
      iItem = m_wndListCtrl.FindItem(&lvfi, -1);
      if (iItem == -1)
      {
         NXC_OBJECT *pObject;

         // Create new record
         pObject = NXCFindObjectById(g_hSession, pInfo->dwNodeId);
         if (pObject != NULL)
         {
            iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pObject->szName,
                                             GetObjectImageIndex(pObject));
         }
         else
         {
            iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, _T("<invalid>"), -1);
         }
         m_wndListCtrl.SetItemData(iItem, pInfo->dwNodeId);
         m_wndProgressCtrl.SetRange32(0, m_wndListCtrl.GetItemCount());
      }
      m_wndListCtrl.SetItemText(iItem, 1, g_szDeploymentStatus[pInfo->dwStatus]);
      m_wndListCtrl.SetItemText(iItem, 2, 
         (pInfo->dwStatus == DEPLOYMENT_STATUS_FAILED) ? pInfo->pszErrorMessage : _T(""));

      if ((pInfo->dwStatus == DEPLOYMENT_STATUS_FAILED) ||
          (pInfo->dwStatus == DEPLOYMENT_STATUS_COMPLETED))
         m_wndProgressCtrl.StepIt();
   }
}


//
// WM_PAINT message handler
//

void CDeploymentView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
   RECT rect;
   CPen pen, *pOldPen;
   CFont *pOldFont;

   // Draw separator between status box and node list
   pen.CreatePen(PS_SOLID, 0, RGB(127, 127, 127));
   pOldPen = dc.SelectObject(&pen);

   GetClientRect(&rect);
   dc.MoveTo(0, STATUS_HEIGHT);
   dc.LineTo(rect.right, STATUS_HEIGHT);

   dc.SelectObject(pOldPen);

   // Show current status
   pOldFont = dc.SelectObject(&m_font);
   dc.TextOut(4, 4, m_bFinished ? _T("Job finished") : _T("Job running "), 12);
   dc.SelectObject(pOldFont);
}


//
// WM_SIZE messahe handler
//

void CDeploymentView::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);

   m_wndProgressCtrl.SetWindowPos(NULL, 4, PROGRESS_CTRL_TOP, cx - 8, PROGRESS_CTRL_HEIGHT, SWP_NOZORDER);
   m_wndListCtrl.SetWindowPos(NULL, 0, STATUS_HEIGHT + 1, cx, cy - STATUS_HEIGHT, SWP_NOZORDER);
}


//
// WM_DEPLOYMENT_FINISHED message handler
//

void CDeploymentView::OnDeploymentFinished(WPARAM wParam, LPARAM lParam)
{
   m_bFinished = TRUE;
   InvalidateRect(NULL);
   if (lParam == RCC_SUCCESS)
      MessageBox(_T("Deployment job finished successfully"), _T("Information"), MB_OK | MB_ICONINFORMATION);
   else
      theApp.ErrorBox(lParam, _T("Deployment job failed: %s"));
}
