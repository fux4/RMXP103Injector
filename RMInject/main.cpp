// mydll.cpp : Defines the entry point for the DLL application.
//
#include <Windows.h>
#include <Windowsx.h>
#include <stdio.h>
#include <mmsystem.h>
#include <io.h>
#include <string>
#include <vector>
#include "atlimage.h"
#include "ApiHook.h"
using namespace std;

#define FUX2_SCROLL 0x16666

string workDir;
HINSTANCE hInstance;
MSG msg;

CImage tcimg;
long picWidth = 1;
long picHeight = 1;

int screenWidth = 1;
int screenHeight = 1;
int selIndex = 0;
int startIndex = 0;
int pageMax = 40;
int lineHeight = 12;
int scrollHeight = 480;
int lastIndex = 0;

static TCHAR szWindowName[] = TEXT ("fux2PicWindow") ;
static TCHAR szAppName[] = TEXT ("fux2PicSelector") ;
static TCHAR szPictureName[] = TEXT ("fux2PicShow") ;

RECT drawRect;
RECT picRect;

bool isClickOk = false;

HFONT font = CreateFont(12,0,0,0,100,FALSE,FALSE,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FF_SWISS, L"宋体");

HBRUSH SYSTEM_COLOR = CreateSolidBrush(RGB(127,157,185));
HBRUSH BLACK_COLOR = CreateSolidBrush(RGB(0,0,0));
HBRUSH BLUE_COLOR = CreateSolidBrush(RGB(49,106,197));
HBRUSH BACK_COLOR = CreateSolidBrush(RGB(236,233,216));
HBRUSH SCROLL_BACK_COLOR = CreateSolidBrush(RGB(0xCC,0xCC,0xCC));

HWND ButtonOkHwnd = NULL;
HWND ButtonCancelHwnd = NULL;
HWND PictureBoxHwnd = NULL;
HWND hScroll = NULL;

vector<string> fileList;
vector<string> fullList;

TCHAR L_TempBmpPath[MAX_PATH];   //这里被定义，保证MAX_PATH内存大小足够大

void p(long v){
	char c[10];
	sprintf(c,"%x",v);
	MessageBoxA(0,c,"1",0);
}

void p(const char* c){
	MessageBoxA(0,c,"1",0);
}

void g(HWND hwnd,long v){
	char c[10];
	sprintf(c,"%d",v);
	SetWindowTextA(hwnd,c);
}

int getItemStartIndex(int postion)
{
    return max(min(postion,lastIndex),0);
}

wchar_t *STRING2TCHAR(string str)
{
	const char *buffer = str.c_str();
    size_t len = strlen(buffer);
    size_t wlen = MultiByteToWideChar(CP_ACP, 0, buffer, int(len), NULL, 0);
    wchar_t *wBuf = new wchar_t[wlen + 1];
    MultiByteToWideChar(CP_ACP, 0, buffer, int(len), wBuf, int(wlen));
	wBuf[wlen] = 0;
    return wBuf;
}

string TCHAR2STRING(TCHAR *str)
{
	int iLen = WideCharToMultiByte(CP_ACP, 0,str, -1, NULL, 0, NULL, NULL);
	char* chRtn =new char[iLen*sizeof(char)];
	WideCharToMultiByte(CP_ACP, 0, str, -1, chRtn, iLen, NULL, NULL);
	chRtn[iLen] = 0;
	return string(chRtn);
}

void getAllFiles(char* nowName)  
{
	long hFile = NULL;
	struct _finddata_t fileinfo;
	string pn;
	pn.assign(workDir).append("\\Graphics\\Pictures\\*");
	fileList.push_back("(无)");
	if((hFile = _findfirst(pn.c_str(),&fileinfo)) !=  -1)
	{
		do  
		{
			if(!(fileinfo.attrib &  _A_SUBDIR) && (fileinfo.size>0))
			{
				string filename = fileinfo.name;
				string suffixStr = filename.substr(filename.find_last_of('.') + 1);
				if(suffixStr.compare("jpg")==0||suffixStr.compare("png")==0||suffixStr.compare("bmp")==0){
					string atemp;
					atemp.assign(workDir).append("\\Graphics\\Pictures\\").append(fileinfo.name);
					fullList.push_back(atemp);
					fileinfo.name[filename.find_last_of('.')] = 0;
					if(strcmp(fileinfo.name,nowName)==0){
						selIndex = fileList.size();
					}
					fileList.push_back(fileinfo.name);
				}
			}
		}while(_findnext(hFile, &fileinfo)==0);
		_findclose(hFile); 
	} 
}

static void mlCGFillColor(HDC hdc, RECT *r, unsigned int color)
{
    SetBkColor(hdc, color);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, r, 0, 0, 0);
}

void loadPicFromRM(){
	tcimg.Destroy();
	if(selIndex>0){
		tcimg.Load(STRING2TCHAR(fullList[selIndex-1]));
		picWidth = tcimg.GetWidth();
		picHeight = tcimg.GetHeight();
		if (tcimg.GetBPP() == 32)
		{
			for (long i = 0; i < picWidth; i++){
				for (long j = 0; j < picHeight; j++)
				{
					byte *pByte = (byte *)tcimg.GetPixelAddress(i, j);
					pByte[0] = pByte[0] * pByte[3] / 255;
					pByte[1] = pByte[1] * pByte[3] / 255;
					pByte[2] = pByte[2] * pByte[3] / 255;
				}
			}
		}
	}
	InvalidateRect(PictureBoxHwnd, &picRect, true);
}

void updateView(HDC hdc){
	SelectObject(hdc,font);
	SetBkColor(hdc,RGB(0,255,0));
	SetBkMode(hdc,TRANSPARENT);
	HPEN gPen=CreatePen(PS_SOLID,1,RGB(0,0,0));
    HPEN oPen=(HPEN)SelectObject(hdc,gPen);
	RECT rect;
	// filename
	rect.left = 16;
	rect.right = 162;
	int tmpVar = fileList.size();
	for(unsigned int i=startIndex;i<startIndex+pageMax;i++){
		if(i<tmpVar){
			rect.top = 16+(i-startIndex)*lineHeight;
			rect.bottom = rect.top + lineHeight-1;
			if(selIndex==i){
				gPen=CreatePen(PS_DOT,1,RGB(206,149,58));
				oPen=(HPEN)SelectObject(hdc,gPen);
				SelectObject(hdc,BLUE_COLOR);
				Rectangle(hdc,rect.left,rect.top,rect.right,rect.bottom);
				SetTextColor(hdc,RGB(255,255,255));
			}else{
				SetTextColor(hdc,RGB(0,0,0));
			}
			wchar_t *tmpStr = STRING2TCHAR(fileList[i]);
			DrawText(hdc,tmpStr,wcslen(tmpStr),&rect,DT_SINGLELINE);
			delete tmpStr;
		}else{
			break;
		}
	}
}

void initializeUI(HDC hdc){
	RECT rect;
	rect.left = 14;
	rect.top = 14;
	rect.right = 180;
	rect.bottom = 498;
	FrameRect(hdc,&rect,SYSTEM_COLOR);
}

void createTools(HWND hwnd,WPARAM wParam, LPARAM lParam){
	ButtonOkHwnd = ::CreateWindowEx(4,
			L"Button", 
			TEXT ("确定"),
			0x50010000 | BS_DEFPUSHBUTTON,
			676, 508,
			75, 21,
			hwnd, NULL, hInstance, NULL);
	SendMessage( ButtonOkHwnd, WM_SETFONT, (WPARAM)font, MAKELONG(TRUE,0) );
	ButtonCancelHwnd = ::CreateWindowEx(4,
			L"Button", 
			TEXT ("取消"),
			0x50010000,
			757, 508,
			75, 21,
			hwnd, NULL, hInstance, NULL);
	SendMessage( ButtonCancelHwnd, WM_SETFONT, (WPARAM)font, MAKELONG(TRUE,0) );
	HWND box2 = CreateWindowEx(0x204,
			L"Static", 
			TEXT (""),
			0x50000004,
			188, 14,
			644, 484,
			hwnd, NULL, hInstance, NULL);
	PictureBoxHwnd = ::CreateWindowEx(0,
			szPictureName, 
			szPictureName,
			0x50000000,
			0, 0,
			640, 480,
			box2, NULL, hInstance, NULL);
	hScroll = CreateWindow(szAppName, szAppName, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                  162, 16, 16, 480,
                                  hwnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)//消息的处理程序
 
{
    HDC hdc;
    PAINTSTRUCT ps;
	int cx,cy;
    switch (message)
    {
	case FUX2_SCROLL:
		InvalidateRect(hwnd, &drawRect, false);
		hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc,&drawRect,BACK_COLOR);
		updateView(hdc);
        EndPaint (hwnd, &ps);
		return 0;
	case WM_LBUTTONDOWN:
		cx = GET_X_LPARAM(lParam);
		cy = GET_Y_LPARAM(lParam);
		if(cx>=drawRect.left&&cx<=drawRect.right&&cy>=drawRect.top&&cy<=drawRect.bottom){
			selIndex = startIndex + (cy-drawRect.top)/lineHeight;
			InvalidateRect(hwnd, &drawRect, false);
			hdc = BeginPaint(hwnd, &ps);
			FillRect(hdc,&drawRect,BACK_COLOR);
			updateView(hdc);
			EndPaint (hwnd, &ps);
			loadPicFromRM();
		}
		return 0;
	case WM_LBUTTONDBLCLK:
		isClickOk = true;
		return DefWindowProc (hwnd, WM_CLOSE, wParam, lParam);
	case WM_CREATE:
		createTools(hwnd,wParam, lParam);
		return DefWindowProc (hwnd, message, wParam, lParam);
	case WM_MOUSEWHEEL:
        PostMessage(hScroll, WM_MOUSEWHEEL, wParam, lParam);
        return 0;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
		initializeUI(hdc);
		updateView(hdc);
        EndPaint (hwnd, &ps);
        return 0 ;
	case WM_COMMAND:
		if((HWND)lParam==ButtonOkHwnd){
			isClickOk = true;
			return DefWindowProc (hwnd, WM_CLOSE, wParam, lParam);
		}else if((HWND)lParam==ButtonCancelHwnd){
			return DefWindowProc (hwnd, WM_CLOSE, wParam, lParam);
		}
	case WM_KEYDOWN:
		if(wParam==VK_ESCAPE)
        {
			return DefWindowProc (hwnd, WM_CLOSE, wParam, lParam);
		}
		return DefWindowProc (hwnd, message, wParam, lParam);
    case WM_DESTROY:
		PostMessage(hwnd,WM_QUIT,wParam,lParam);
        return 0;
    }
    return DefWindowProc (hwnd, message, wParam, lParam);
}

CApiHook ClickPicHooker;

HWND getHookParent(){
	DWORD tid = GetCurrentThreadId();
	HWND hand = GetWindow(GetForegroundWindow(), 0);
	char className[50];
    while(hand){
		if(tid == GetWindowThreadProcessId(hand, 0)){
			memset(className,0,11);
			GetClassNameA(hand, className, 12);
			if(strcmp(className,"#32770")==0) break;
		}
		hand = GetWindow(hand, 2);
	}
	return hand;
}

void ClickHooker(){
	DWORD oldNameHwnd;
	__asm{
		mov eax,[ecx+0x20]
		mov oldNameHwnd,eax
	}
	HWND parentWindow = getHookParent();
	if(IsWindowEnabled(parentWindow)){
		startIndex = 0;
		selIndex = 0;
		long width = 866,height=570;
		long x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
		long y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
		int txtLen = GetWindowTextLengthA((HWND)oldNameHwnd);
		char txtName[50];
		GetWindowTextA((HWND)oldNameHwnd,txtName,txtLen);
		txtName[txtLen]=0;
		vector<string> fl;
		fileList.swap(fl);
		vector<string> sl;
		fullList.swap(sl);
		TCHAR workSpaceBuf[MAX_PATH];
		GetCurrentDirectory(MAX_PATH,workSpaceBuf);
		workDir = TCHAR2STRING(workSpaceBuf);
		getAllFiles(txtName);
		lastIndex = fileList.size()-pageMax;

		startIndex = getItemStartIndex(selIndex);
		loadPicFromRM();
		isClickOk = false;
		HWND hwnd = ::CreateWindowEx(0x10501,
			szWindowName, 
			TEXT ("图片选择"),
			0x94C800C4,
			x, y,
			width, height,
			parentWindow, NULL, hInstance, NULL);
		if(hwnd){
			RECT rcClient;
			GetClientRect(hwnd,&rcClient);
			screenWidth = rcClient.right - rcClient.left;
			screenHeight = rcClient.bottom - rcClient.top;
			EnableWindow(parentWindow,false);
			ShowWindow(hwnd, SW_SHOW) ;
			UpdateWindow(hwnd);
			while (GetMessage(&msg, NULL, 0, 0)>0)
			{
				TranslateMessage (&msg) ;
				DispatchMessage (&msg) ;
			}
			EnableWindow(parentWindow,true);
			if(GetActiveWindow()!=parentWindow){
				SetActiveWindow(parentWindow);
			}
			if (hwnd)
			{
				DestroyWindow(hwnd);
				hwnd = NULL;
			}
			if(isClickOk){
				SetWindowTextA((HWND)oldNameHwnd,fileList[selIndex].c_str());
			}
		}else{
			MessageBoxA( parentWindow, "创建窗口失败!", "Faild", MB_ICONINFORMATION );
		}
	}
}

LRESULT CALLBACK scrollWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT r;
    static int dragState=0;
	static int clickY=0;
	static int clickIndex=0;

    int accumDelta;
    switch (message) {
    case WM_LBUTTONDOWN:
		if(fileList.size()<=pageMax){ return 0;}
		dragState = 1;
		clickY = GET_Y_LPARAM(lParam);
		clickIndex = startIndex;
		PostMessage(hwnd, WM_PAINT, wParam, lParam);
		InvalidateRect(hwnd, NULL, false);
        return 0;
    case WM_MOUSEMOVE:
		if(fileList.size()<=pageMax){ return 0;}
        if (dragState == 1) {
            SetCapture(hwnd);
            dragState = 2;
        }
        else if (dragState == 2) {
            if (!(wParam & MK_LBUTTON)) {
                dragState = 0;
                if (GetCapture() == hwnd)
                    ReleaseCapture();
				startIndex = clickIndex;
				PostMessage(hwnd, WM_PAINT, wParam, lParam);
				InvalidateRect(hwnd, NULL, false);
            }
            else {
				int nowY = GET_Y_LPARAM(lParam);
				int rem = startIndex;
				int offset = (nowY - clickY)*fileList.size();
				startIndex = getItemStartIndex(clickIndex+offset/scrollHeight);
				if(startIndex!=rem){
					InvalidateRect(hwnd, NULL, false);
					PostMessage(hwnd, WM_PAINT, wParam, lParam);
					PostMessage(GetParent(hwnd), FUX2_SCROLL, 0, 0);
				}
            }
        }
        return 0;
    case WM_LBUTTONUP:
		if(fileList.size()<=pageMax){ return 0;}
        if (dragState == 2) 
            ReleaseCapture();
        if (dragState) {
            dragState = 0;
			PostMessage(hwnd, WM_PAINT, wParam, lParam);
            InvalidateRect(hwnd, NULL, false);
        }
        return 0;
    case WM_MOUSEWHEEL:
		if(fileList.size()<=pageMax){ return 0;}
		accumDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
		startIndex = getItemStartIndex(startIndex-accumDelta);
		InvalidateRect(hwnd, NULL, false);
		PostMessage(hwnd, WM_PAINT, wParam, lParam);
		PostMessage(GetParent(hwnd), FUX2_SCROLL, 0, 0);
        return 0;
    case WM_PAINT:
		if (fileList.size()>pageMax) {
			int s;
			int v;
			hdc = BeginPaint(hwnd, &ps);
			GetClientRect(hwnd, &r);
			HPEN gPen=CreatePen(PS_SOLID,1,RGB(0,0,0));
			HPEN oPen=(HPEN)SelectObject(hdc,gPen);
			SelectObject(hdc,SCROLL_BACK_COLOR);
			Rectangle(hdc,r.left,r.top,r.right,r.bottom);
			SelectObject(hdc,oPen);

            s = max(pageMax*scrollHeight/fileList.size(), 20);

            v = min(startIndex*scrollHeight/fileList.size(),scrollHeight-s);
            r.left++;
            r.right--;
            r.top = v ;
            r.bottom = r.top + s;

            mlCGFillColor(hdc, &r, dragState ? 0x999999 : 0x666666e);
            InflateRect(&r, -1, -1);
            mlCGFillColor(hdc, &r, dragState ? 0x666666e : 0x999999);

			EndPaint(hwnd, &ps);
		}

        return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK pictureWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    switch (message) {
    case WM_PAINT:
		if(!tcimg.IsNull()){
			hdc = BeginPaint(hwnd, &ps);
			tcimg.Draw(hdc, 0, 0);
			tcimg.Destroy();
			EndPaint(hwnd, &ps);
		}
        return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

void RegisterController(){
	WNDCLASS wndclass;
	wndclass.style			= 0;
	wndclass.lpfnWndProc	= scrollWndProc ;
	wndclass.cbClsExtra		= 0 ;
	wndclass.cbWndExtra		= 0 ;
	wndclass.hInstance		= hInstance ;
	wndclass.hIcon			= 0 ;
	wndclass.hCursor		= 0 ;
	wndclass.hbrBackground	= 0;
	wndclass.lpszMenuName	= NULL ;
	wndclass.lpszClassName	= szAppName ;
	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT ("注册控件失败!"),szAppName, MB_ICONERROR) ;
		ClickPicHooker.SetHookOff();
		return;
	}
	wndclass.lpszClassName	= szWindowName;
	wndclass.lpfnWndProc	= WndProc;
	wndclass.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.hbrBackground	= BACK_COLOR;
	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT ("注册控件失败!"),szAppName, MB_ICONERROR) ;
		ClickPicHooker.SetHookOff();
		return;
	}
	wndclass.lpszClassName	= szPictureName;
	wndclass.lpfnWndProc	= pictureWndProc;
	wndclass.style			= 0;
	wndclass.hbrBackground	= BLUE_COLOR;
	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT ("注册控件失败!"),szAppName, MB_ICONERROR) ;
		ClickPicHooker.SetHookOff();
		return;
	}
	drawRect.left = 16;
	drawRect.top = 16;
	drawRect.right = 162;
	drawRect.bottom = 496;
	picRect.left = 0;
	picRect.right = 640;
	picRect.top = 0;
	picRect.bottom = 480;
}

void InjectAll(){
	hInstance = (HINSTANCE)GetModuleHandle(NULL);
	ClickPicHooker.Initialize((FARPROC)((long)hInstance+0x70510),(FARPROC)ClickHooker);
	ClickPicHooker.SetHookOn();
	RegisterController();
}

BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch ( ul_reason_for_call )
    {
        case DLL_PROCESS_ATTACH:
        {
            InjectAll();
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            //MessageBoxA( NULL, "DLL已从目标进程卸载。", "信息", MB_ICONINFORMATION );
            break;
        }
    }
    return TRUE;
}