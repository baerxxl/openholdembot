#include "stdafx.h"
#include "updialog.h"
#include "debug.h"

CUPDialog::_tagInitCommonControls CUPDialog::m_InitCommonControls;	//Static variable to facilitate the call of InitCommonControlsEx() function Only Once Per App

CUPDialog::_tagInitCommonControls::_tagInitCommonControls()
{
    __SEH_HEADER

    INITCOMMONCONTROLSEX icce;

    icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icce.dwICC	= CUPDIALOG_CONTROL_CLASSES;

    InitCommonControlsEx(&icce);
	
	__SEH_LOGFATAL("CUPDialog::_tagInitCommonControls : \n");
}

CUPDialog::CUPDialog(HWND hParentWnd,LP_CUPDIALOG_USERPROC lpUserProc,LPVOID lpUserProcParam,LPCTSTR lpszDlgTitle/*=_T("Please Wait..")*/,bool bAllowCancel/*=true*/)
{
    __SEH_SET_EXCEPTION_HANDLER(MyUnHandledExceptionFilter);

    __SEH_HEADER

    m_hThread = NULL;							//No Thread Yet !!

    m_hParentWnd = hParentWnd;					//Needed to Create the DialogBox - DlgProc asks this as Parameter

    m_bAllowCancel = bAllowCancel;				//Is Dialog Terminatable ??

    m_ThreadData.pUserProcParam	= lpUserProcParam;	//We send this as Parameter to the UserProc

    m_ThreadData.m_lpUserProc	= lpUserProc;	//The actual User Procedure

    ZeroMemory(m_szDialogCaption,sizeof(m_szDialogCaption));

    _tcsncpy(m_szDialogCaption,lpszDlgTitle,(sizeof(m_szDialogCaption)/sizeof(m_szDialogCaption[0]))-1);
	
	__SEH_LOGFATAL("CUPDialog::Constructor : \n");
}

CUPDialog::~CUPDialog(void)
{
    __SEH_HEADER

    Cleanup();			//It is possible that the Dialog object can be destroyed while Thread is still running..!!
	
	__SEH_LOGFATAL("CUPDialog::Destructor : \n");
}

void CUPDialog::Cleanup()
{
    __SEH_HEADER

    m_ThreadData.bTerminate = true;

    if (m_ThreadData.bAlive)		//If associated Thread is still alive Terminate It
    {
        Sleep(CUPDIALOG_TERMINATE_DELAY);
        DWORD dwExitCode = 0;
        if (GetExitCodeThread(m_hThread,&dwExitCode) && dwExitCode == STILL_ACTIVE)
            TerminateThread(m_hThread, IDCANCEL);
    }

    if (m_hThread)
    {
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    m_ThreadData.bAlive = false;
    m_ThreadData.bTerminate = false;
    m_ThreadData.hThreadWnd = NULL;
	
	__SEH_LOGFATAL("CUPDialog::Cleanup : \n");
}

INT_PTR CUPDialog::DoModal()
{
    __SEH_HEADER

    Cleanup();		//If this is not first time, we had better Terminate any previous instance Threads !!

    return DialogBoxParam(NULL,MAKEINTRESOURCE(IDD),m_hParentWnd,ProgressDlgProc,(LPARAM)this);
	
	__SEH_LOGFATAL("CUPDialog::DoModal : \n");
}

static DWORD WINAPI ThreadProc(LPVOID lpThreadParameter)	//Calls the User Progress Procedure
{
    __SEH_HEADER

    LPPROGRESSTHREADDATA pThreadData = (LPPROGRESSTHREADDATA) lpThreadParameter;

    pThreadData->bAlive = true;

    INT_PTR nResult = IDCANCEL;

    if (pThreadData->bTerminate == true)
        goto TerminateThreadProc;

    nResult = (true == (*pThreadData->m_lpUserProc)((CUPDUPDATA*)lpThreadParameter)) ? IDOK : IDCANCEL;

TerminateThreadProc:

    pThreadData->bAlive = false;

    if (pThreadData->bTerminate == false)
        ::PostMessage(pThreadData->hThreadWnd,PROGRESSTHREADDATA::WM_PROGRESSTHREADCOMPLETED,MAKEWPARAM(nResult,0),0);

    return 0;
	
	__SEH_LOGFATAL("CUPDialog, ThreadProc : \n");
}

INT_PTR CALLBACK ProgressDlgProc(HWND hDlg,UINT Message,WPARAM wParam,LPARAM lParam)
{
    __SEH_HEADER

    switch (Message)
    {
    case WM_INITDIALOG:
    {
        CUPDialog *pProgressDialog = (CUPDialog*) lParam;

        if (pProgressDialog->m_bAllowCancel == false)
            SendMessage(hDlg,PROGRESSTHREADDATA::WM_DISABLECONTROLS,wParam,lParam);

        SendMessage(GetDlgItem(hDlg,CUPDIALOG_TEXTBOX_ID),WM_SETTEXT,0,(LPARAM)_T(""));

        SendMessage(GetDlgItem(hDlg,CUPDIALOG_PROGRESSBAR_ID),PBM_SETPOS,0,0);

        SendMessage(hDlg,WM_SETTEXT,0,(LPARAM)pProgressDialog->m_szDialogCaption);

        SetWindowLongPtr(hDlg,GWL_USERDATA,(LONG_PTR)pProgressDialog);

        ((LPPROGRESSTHREADDATA)(LPVOID)(&pProgressDialog->m_ThreadData))->hThreadWnd = hDlg;

        DWORD dwThreadId = 0;

        pProgressDialog->m_hThread = CreateThread(NULL,NULL,ThreadProc,&pProgressDialog->m_ThreadData,0,&dwThreadId);

        if (pProgressDialog->m_hThread == NULL)	EndDialog(hDlg,IDCANCEL);

        return TRUE;
    }
    case WM_COMMAND:
    {
        if (CUPDIALOG_CANCELBUTTON_ID == LOWORD(wParam))
        {
            SendMessage(hDlg,PROGRESSTHREADDATA::WM_CANCELPROGRESSTHREAD,0,0);
            return TRUE;
        }
        break;
    }
    case WM_SYSCOMMAND:
    {
        if (SC_CLOSE == wParam)
        {
            SendMessage(hDlg,PROGRESSTHREADDATA::WM_CANCELPROGRESSTHREAD,0,0);
            return TRUE;
        }
        break;
    }
    case PROGRESSTHREADDATA::WM_DISABLECONTROLS:
    {
        EnableMenuItem(GetSystemMenu(hDlg,false),SC_CLOSE,MF_DISABLED|MF_GRAYED|MF_BYCOMMAND);
        EnableWindow(GetDlgItem(hDlg,CUPDIALOG_CANCELBUTTON_ID),false);
        return TRUE;
    }
    case PROGRESSTHREADDATA::WM_PROGRESSTHREADCOMPLETED:		//wParam = IDOK or IDCANCEL depending on the Success of Thread
    {
        EndDialog(hDlg, wParam);
        return TRUE;
    }
    case PROGRESSTHREADDATA::WM_PROGRESSTEXTUPDATE:				//lParam = ProgressText;
    {
        SendMessage(GetDlgItem(hDlg,CUPDIALOG_TEXTBOX_ID),WM_SETTEXT,0,lParam);
        return TRUE;
    }
    case PROGRESSTHREADDATA::WM_PROGRESSBARUPDATE:				//wParam = % Progress;
    {
        SendMessage(GetDlgItem(hDlg,CUPDIALOG_PROGRESSBAR_ID),PBM_SETPOS,wParam,0);
        return TRUE;
    }
    case PROGRESSTHREADDATA::WM_CANCELPROGRESSTHREAD:			//Enough to Signal the Thread - Actual Handle Would be Closed in the Dialog Destructor
    {
        {
            LPPROGRESSTHREADDATA pThreadData = (LPPROGRESSTHREADDATA)(LPVOID)(&((CUPDialog*)GetWindowLongPtr(hDlg,GWL_USERDATA))->m_ThreadData);
            pThreadData->bTerminate = true;
            SendMessage(GetDlgItem(hDlg,CUPDIALOG_TEXTBOX_ID),WM_SETTEXT,0,(LPARAM)(_T("Termination Initiated..")));
            SendMessage(hDlg,PROGRESSTHREADDATA::WM_DISABLECONTROLS,wParam,lParam);
            if (pThreadData->bAlive)
                Sleep(CUPDIALOG_TERMINATE_DELAY);
            if (pThreadData->bAlive)
                SendMessage(GetDlgItem(hDlg,CUPDIALOG_TEXTBOX_ID),WM_SETTEXT,0,(LPARAM)(_T("Termination Complete ..Shutting Down !!")));
            if (pThreadData->bAlive)
                Sleep(CUPDIALOG_TERMINATE_DELAY);
            EndDialog(hDlg, MAKEWPARAM(IDCANCEL,1));
        }
        return TRUE;
    }
    }
    return FALSE;
	
	__SEH_LOGFATAL("CUPDialog, ProgressDlgProc : \n");
}