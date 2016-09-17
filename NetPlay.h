//
// NET PLAY CLASSSSSSSSSSSSSS
//
#ifndef	__CNETPLAY_INCLUDED__
#define	__CNETPLAY_INCLUDED__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <winsock.h>

#include <string>
using namespace std;
//WM_APP 설명 http://www.soen.kr/lecture/win32api/reference/Message/WM_APP.htm
#define	WM_NETPLAY		(WM_APP+100)

#define	WM_NETPLAY_ACCEPT	(WM_APP+110)
#define	WM_NETPLAY_CONNECT	(WM_APP+111)
#define	WM_NETPLAY_CLOSE	(WM_APP+112)
#define	WM_NETPLAY_ERROR	(WM_APP+113)

class	CNetPlay
{
public:
	CNetPlay();
	~CNetPlay();

	// 초기화 / 오픈(개방)
	BOOL	Initialize( HWND hWnd );
	void	Release();

	// NETPLAY가 가능한지 확인 
	BOOL	IsNetPlay() { return m_hWnd?TRUE:FALSE; }
	// 연결중인가 ? 
	BOOL	IsConnect() { return m_hWnd?m_bConnect:FALSE; }
	// 연결중인가 ? 
	BOOL	IsServer() { return m_bServer; }

	// 통신지연시간 
	void	SetLatency( INT nLatency ) { m_nLatency = nLatency; }
	INT	GetLatency() { return m_nLatency; }

	// 비동기 처리 메시지 반송을할때 윈도우의 설정
	void	SetMsgWnd( HWND hWnd ) { m_hWndMsg = hWnd; }

	// 연결 및 분리
	BOOL	Connect( BOOL bServer, const char* IP, unsigned short Port );
	void	Disconnect();

	// 데이터 전송 0 : 수신 데이터 대기 
	//1 이상 : 수신 데이터가 존재함  
	//0 미만 : 연결 조각이나 오류
	INT	Send( unsigned char* pBuf, int size );
	// 데이터 수신
	// 0 : 수신 데이터 대기 1이상 : 수신 데이터 있는 0미만 : 접속 부족이나 오류
	// 타임 아웃이 없음 
	INT	Recv( unsigned char* pBuf, int size );
	// 타임 아웃이 존재함
	INT	RecvTime( unsigned char* pBuf, int size, unsigned long timeout );

	// Windows 메시지 절차 
	//WndProc - 메시지 처리 함수 , WPARAM - 값을 넘김 LPARAM - 값 말고도 포인터로도 넘김
	HRESULT	WndProc( HWND hWnd, WPARAM wParam, LPARAM lParam );
protected:
	// 멤버 변수
	HWND	m_hWnd;
	HWND	m_hWndMsg;

	BOOL	m_bServer;
	BOOL	m_bConnect;	// 연결 체크
	INT	m_nLatency;

	// WINSOCK
	WSADATA	m_WSAdata;
	SOCKET	m_SocketConnect;
	SOCKET	m_SocketData;
private:
};

extern	CNetPlay	NetPlay;

#endif	// !__CNETPLAY_INCLUDED__
