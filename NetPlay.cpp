#pragma comment(lib, "wsock32.lib")

#include "DebugOut.h"

#include "NetPlay.h"

#define	CLOSESOCKET(soc) if((soc)!=INVALID_SOCKET){::closesocket((soc));(soc)=INVALID_SOCKET;}

CNetPlay	NetPlay;

CNetPlay::CNetPlay()
{
	m_hWnd = m_hWndMsg = NULL;
	m_bConnect = FALSE;

	m_SocketConnect = INVALID_SOCKET; //수로 표현되는 소켓의 핸들 값을 지정하기 위해 정의된 자료형의 이름
	m_SocketData = INVALID_SOCKET; //실패시 반환한다 
}

CNetPlay::~CNetPlay()
{
	// 우선순위(?)
	Release();
}

BOOL	CNetPlay::Initialize( HWND hWnd )
{
	// 우선순위(?)
	Release();

	// WinSock DLL초기화
	if( ::WSAStartup( MAKEWORD(1,1), &m_WSAdata ) )
		return	FALSE;

	// 버전의 차이 
	if( m_WSAdata.wVersion != MAKEWORD(1,1) ) {
		::WSACleanup(); //소켓이 리셋되고 닫힘
		return	FALSE;
	}

	m_hWnd = hWnd;
	return	TRUE;
}
void	CNetPlay::Release() //초기화(?)
{
	Disconnect();

	if( m_hWnd ) {
		::WSACleanup();
		m_hWnd = NULL;
	}
}

BOOL	CNetPlay::Connect( BOOL bServer, const char* IP, unsigned short Port )
{
	if( !m_hWnd )
		return	FALSE;

	m_bServer = bServer;

	if( bServer ) {
	// Server
		// 소켓 생성 연결알림
		if( m_SocketConnect == INVALID_SOCKET ) {
			m_SocketConnect = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
			if( m_SocketConnect == INVALID_SOCKET ) {
				DEBUGOUT( "CNetPlay:socket failed.\n" );
				return	FALSE;
			}
		}
		// ポートと結びつける
		struct sockaddr_in addr;
		::ZeroMemory( &addr, sizeof(addr) );
		addr.sin_family      = AF_INET;
		addr.sin_addr.s_addr = ::htonl( INADDR_ANY );
		addr.sin_port        = ::htons( Port );
		if( ::bind( m_SocketConnect, (struct sockaddr *)&addr, sizeof(addr) ) == SOCKET_ERROR ) {
			DEBUGOUT( "CNetPlay:bind failed.\n" );
			CLOSESOCKET( m_SocketConnect );
			return	FALSE;
		}
		// 接続要求イベントの設定
		if( ::WSAAsyncSelect( m_SocketConnect, m_hWnd, WM_NETPLAY, FD_ACCEPT ) == SOCKET_ERROR ) {
			DEBUGOUT( "CNetPlay:WSAAsyncSelect failed.\n" );
			CLOSESOCKET( m_SocketConnect );
			return	FALSE;
		}
		// 接続要求受付開始
		if( ::listen( m_SocketConnect, 1 ) == SOCKET_ERROR ) {
			DEBUGOUT( "CNetPlay:listen failed.\n" );
			CLOSESOCKET( m_SocketConnect );
			return	FALSE;
		}
	} else {
	// Client
		// IPアドレス？
		unsigned long IP_address = ::inet_addr( IP );
		if( IP_address == INADDR_NONE ) {
			DEBUGOUT( "CNetPlay:IPアドレスが不正です。\"%s\"\n", IP );
			return	FALSE;
		}

		// データ通信ソケット作成
		m_SocketData = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		if( m_SocketData == INVALID_SOCKET ) {
			DEBUGOUT( "CNetPlay:socket failed.\n" );
			return	FALSE;
		}
		// Nagleアルゴリズムの無効化
		BOOL	bOpt = TRUE;
		if( ::setsockopt( m_SocketData, IPPROTO_TCP, TCP_NODELAY, (const char*)&bOpt, sizeof(bOpt) ) == SOCKET_ERROR ) {
			DEBUGOUT( "CNetPlay:setsockopt failed.\n" );
			CLOSESOCKET( m_SocketData );
			return	FALSE;
		}
		// ブロッキングモード設定
		BOOL	bArg = TRUE;
		if( ::ioctlsocket( m_SocketData, FIONBIO, (unsigned long*)&bArg ) == SOCKET_ERROR ) {
			DEBUGOUT( "CNetPlay:ioctlsocket failed.\n" );
			CLOSESOCKET( m_SocketData );
			return	FALSE;
		}
		// 接続完了イベントの設定
		if( ::WSAAsyncSelect( m_SocketData, m_hWnd, WM_NETPLAY, FD_CONNECT ) == SOCKET_ERROR ) {
			DEBUGOUT( "CNetPlay:WSAAsyncSelect failed.\n" );
			CLOSESOCKET( m_SocketData );
			return	FALSE;
		}
		// 接続を要求する
		struct sockaddr_in addr;
		::ZeroMemory( &addr, sizeof(addr) );
		addr.sin_family      = AF_INET;
		addr.sin_addr.s_addr = IP_address;
		addr.sin_port        = ::htons( Port );
		if( ::connect( m_SocketData, (struct sockaddr *)&addr, sizeof(addr) ) == SOCKET_ERROR ) {
			if( ::WSAGetLastError() != WSAEWOULDBLOCK ) {
				DEBUGOUT( "CNetPlay:connect failed.\n" );
				CLOSESOCKET( m_SocketData );
				return	FALSE;
			}
		}
	}

	return	TRUE;
}

void	CNetPlay::Disconnect()
{
	// ソケットをシャットダウンして破棄
	if( m_SocketConnect != INVALID_SOCKET ) {
//		::shutdown( m_SocketConnect, SD_BOTH );	// WS2以降らしい…
		CLOSESOCKET( m_SocketConnect );
	}
	if( m_SocketData != INVALID_SOCKET ) {
//		::shutdown( m_SocketData, SD_BOTH );	// WS2以降らしい…
		CLOSESOCKET( m_SocketData );
	}

	m_bConnect = FALSE;
}

INT	CNetPlay::Send( unsigned char* pBuf, int size )
{
	if( !m_hWnd || !m_bConnect || m_SocketData == INVALID_SOCKET )
		return	-1L;

	if( ::send( m_SocketData, (char*)pBuf, size, 0 ) == SOCKET_ERROR ) {
		DEBUGOUT( "CNetPlay:send failed. code=%d\n", ::WSAGetLastError() );
		Disconnect();
		return	-1L;
	}

	return	0L;
}

INT	CNetPlay::Recv( unsigned char* pBuf, int size )
{
	if( !m_hWnd || !m_bConnect || m_SocketData == INVALID_SOCKET )
		return	-1L;

	unsigned long	len = 0;
	if( ::ioctlsocket( m_SocketData, FIONREAD, (unsigned long*)&len ) == SOCKET_ERROR ) {
		DEBUGOUT( "CNetPlay:ioctlsocket failed.\n" );
		Disconnect();
		return	-1L;
	}

	if( !len ) {
		return	0L;
	} else {
		if( ::recv( m_SocketData, (char*)pBuf, size, 0 ) == SOCKET_ERROR ) {
			DEBUGOUT( "CNetPlay:recv failed.\n" );
			Disconnect();
			return	-1L;
		}
	}

	return	size;
}

INT	CNetPlay::RecvTime( unsigned char* pBuf, int size, unsigned long timeout )
{
	INT	ret;
	unsigned long dwTimeOut;
	dwTimeOut = ::timeGetTime();
	while( (ret = NetPlay.Recv( pBuf, size )) == 0 ) {
		// 固まらない措置
		::Sleep( 0 );
		// タイムアウトのチェック
		if( (::timeGetTime()-dwTimeOut) > timeout ) {
			return	-1;
		}
	}
	return	ret;
}

HRESULT	CNetPlay::WndProc( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
	// エラー？
	if( WSAGETSELECTERROR(lParam) ) {
#if	0
		switch( WSAGETSELECTERROR(lParam) ) {
			case	WSAEINTR:
				MessageBox( hWnd, "関数呼び出しが中断された。\n", "ERROR", MB_OK );
				break;
			case	WSAEBADF:
				MessageBox( hWnd, "無効なファイルハンドル。\n", "ERROR", MB_OK );
				break;
			case	WSAEACCES:
				MessageBox( hWnd, "アクセスが拒否された。\n", "ERROR", MB_OK );
				break;
			case	WSAEFAULT:
				MessageBox( hWnd, "無効なバッファアドレスです。\n", "ERROR", MB_OK );
				break;
			case	WSAEINVAL:
				MessageBox( hWnd, "無効な引数が渡された。\n", "ERROR", MB_OK );
				break;
			case	WSAEMFILE:
				MessageBox( hWnd, "使用中のソケットの数が多すぎる。\n", "ERROR", MB_OK );
				break;
			case	WSAEWOULDBLOCK:
				MessageBox( hWnd, "操作はブロッキングされる。\n", "ERROR", MB_OK );
				break;
			case	WSAEINPROGRESS:
				MessageBox( hWnd, "既にブロッキング手続きが実行されている。\n", "ERROR", MB_OK );
				break;
			case	WSAEALREADY:
				MessageBox( hWnd, "要求された操作は既に実行中、または実行済み。\n", "ERROR", MB_OK );
				break;
			case	WSAENOTSOCK:
				MessageBox( hWnd, "指定されたソケットが無効である。\n", "ERROR", MB_OK );
				break;
//			case	WSAEDESTADDREQ:
//				MessageBox( hWnd, "操作の実行に送信先アドレスが必要。\n", "ERROR", MB_OK );
//				break;
			case	WSAEMSGSIZE:
				MessageBox( hWnd, "メッセージサイズが大きすぎる。\n", "ERROR", MB_OK );
				break;
			case	WSAEPROTOTYPE:
				MessageBox( hWnd, "ソケットは要求されたプロトコルに適合していない。\n", "ERROR", MB_OK );
				break;
			case	WSAENOPROTOOPT:
				MessageBox( hWnd, "不正なプロトコルオプション。\n", "ERROR", MB_OK );
				break;
			case	WSAEPROTONOSUPPORT:
				MessageBox( hWnd, "プロトコルがサポートされていない。\n", "ERROR", MB_OK );
				break;
			case	WSAESOCKTNOSUPPORT:
				MessageBox( hWnd, "指定されたソケットタイプはサポートされていない。\n", "ERROR", MB_OK );
				break;
			case	WSAEOPNOTSUPP:
				MessageBox( hWnd, "要求された操作はサポートされていない。\n", "ERROR", MB_OK );
				break;
			case	WSAEPFNOSUPPORT:
				MessageBox( hWnd, "プロトコルファミリがサポートされていない。\n", "ERROR", MB_OK );
				break;
			case	WSAEAFNOSUPPORT:
				MessageBox( hWnd, "アドレスファミリがサポートされていない。\n", "ERROR", MB_OK );
				break;

			case	WSAEADDRINUSE:
				MessageBox( hWnd, "アドレスは既に使用中である。\n", "ERROR", MB_OK );
				break;
			case	WSAEADDRNOTAVAIL:
				MessageBox( hWnd, "無効なネットワークアドレス。\n", "ERROR", MB_OK );
				break;
			case	WSAENETDOWN:
				MessageBox( hWnd, "ネットワークがダウンしている。\n", "ERROR", MB_OK );
				break;
			case	WSAENETUNREACH:
				MessageBox( hWnd, "指定されたネットワークホストに到達できない。\n", "ERROR", MB_OK );
				break;
			case	WSAENETRESET:
				MessageBox( hWnd, "ネットワーク接続が破棄された。\n", "ERROR", MB_OK );
				break;

			case	WSAECONNRESET:
				MessageBox( hWnd, "ネットワーク接続が相手によって破棄された。\n", "ERROR", MB_OK );
				break;
			case	WSAENOBUFS:
				MessageBox( hWnd, "バッファが不足している。\n", "ERROR", MB_OK );
				break;
			case	WSAEISCONN:
				MessageBox( hWnd, "ソケットは既に接続されている。\n", "ERROR", MB_OK );
				break;
			case	WSAENOTCONN:
				MessageBox( hWnd, "ソケットは接続されていない。\n", "ERROR", MB_OK );
				break;
			case	WSAESHUTDOWN:
				MessageBox( hWnd, "ソケットはシャットダウンされている。\n", "ERROR", MB_OK );
				break;
			case	WSAETOOMANYREFS:
				MessageBox( hWnd, "参照の数が多すぎる。\n", "ERROR", MB_OK );
				break;

			case	WSAETIMEDOUT:
				MessageBox( hWnd, "接続要求がタイムアウトした。\n", "ERROR", MB_OK );
				break;
			case	WSAECONNREFUSED:
				MessageBox( hWnd, "接続が拒否された。\n", "ERROR", MB_OK );
				break;
			case	WSAELOOP:
				MessageBox( hWnd, "ループ。\n", "ERROR", MB_OK );
				break;
			case	WSAENAMETOOLONG:
				MessageBox( hWnd, "名前が長すぎる。\n", "ERROR", MB_OK );
				break;
			case	WSAEHOSTDOWN:
				MessageBox( hWnd, "ホストがダウンしている。\n", "ERROR", MB_OK );
				break;
			case	WSAEHOSTUNREACH:
				MessageBox( hWnd, "ホストへの経路がない。\n", "ERROR", MB_OK );
				break;
			case	WSAENOTEMPTY:
				MessageBox( hWnd, "ディレクトリが空ではない。\n", "ERROR", MB_OK );
				break;
			case	WSAEPROCLIM:
				MessageBox( hWnd, "ユーザーの数が多すぎる。\n", "ERROR", MB_OK );
				break;
			case	WSAEDQUOT:
				MessageBox( hWnd, "ディスククォータ。\n", "ERROR", MB_OK );
				break;
			case	WSAESTALE:
				MessageBox( hWnd, "実行しようとした操作は廃止されている。\n", "ERROR", MB_OK );
				break;


			case	WSAEREMOTE:
				MessageBox( hWnd, "リモート。\n", "ERROR", MB_OK );
				break;
			case	WSASYSNOTREADY:
				MessageBox( hWnd, "ネットワークサブシステムが利用できない。\n", "ERROR", MB_OK );
				break;
			case	WSAVERNOTSUPPORTED:
				MessageBox( hWnd, "Winsock.dllのバージョンが範囲外である。\n", "ERROR", MB_OK );
				break;
			case	WSANOTINITIALISED:
				MessageBox( hWnd, "WinSockシステムが初期化されていない。\n", "ERROR", MB_OK );
				break;
			case	WSAEDISCON:
				MessageBox( hWnd, "シャットダウン処理中。\n", "ERROR", MB_OK );
				break;
			case	WSAENOMORE:
				MessageBox( hWnd, "データはこれ以上存在しない。\n", "ERROR", MB_OK );
				break;
			case	WSAECANCELLED:
				MessageBox( hWnd, "操作は取り消された。\n", "ERROR", MB_OK );
				break;

			case	WSAEINVALIDPROCTABLE:
				MessageBox( hWnd, "サービスプロバイダの関数テーブルが無効。\n", "ERROR", MB_OK );
				break;
			case	WSAEINVALIDPROVIDER:
				MessageBox( hWnd, "サービスプロバイダが無効。\n", "ERROR", MB_OK );
				break;
			case	WSAEPROVIDERFAILEDINIT:
				MessageBox( hWnd, "サービスプロバイダの初期化に失敗した。\n", "ERROR", MB_OK );
				break;
			case	WSASYSCALLFAILURE:
				MessageBox( hWnd, "システムコールに失敗した。\n", "ERROR", MB_OK );
				break;
			case	WSASERVICE_NOT_FOUND:
				MessageBox( hWnd, "サービスが見つからない。\n", "ERROR", MB_OK );
				break;
			case	WSATYPE_NOT_FOUND:
				MessageBox( hWnd, "タイプが見つからない。\n", "ERROR", MB_OK );
				break;
			case	WSA_E_NO_MORE:
				MessageBox( hWnd, "データはこれ以上存在しない。\n", "ERROR", MB_OK );
				break;
			case	WSA_E_CANCELLED:
				MessageBox( hWnd, "検索がキャンセルされた。\n", "ERROR", MB_OK );
				break;
			case	WSAEREFUSED:
				MessageBox( hWnd, "操作は拒否された。\n", "ERROR", MB_OK );
				break;
			case	WSAHOST_NOT_FOUND:
				MessageBox( hWnd, "ホストが見つからない。\n", "ERROR", MB_OK );
				break;
			case	WSATRY_AGAIN:
				MessageBox( hWnd, "指定されたホストが見つからない、またはサービスの異常。\n", "ERROR", MB_OK );
				break;
			case	WSANO_RECOVERY:
				MessageBox( hWnd, "回復不能なエラーが発生した。\n", "ERROR", MB_OK );
				break;
			case	WSANO_DATA:
				MessageBox( hWnd, "要求されたタイプのデータレコードが見つからない。\n", "ERROR", MB_OK );
				break;

		}
#endif
		Disconnect();
		if( m_hWndMsg ) {
			::PostMessage( m_hWndMsg, WM_NETPLAY_CLOSE, 0, 0 );
		}
		return	0L;
	}

	BOOL	bOpt;
	BOOL	bArg;

	switch( WSAGETSELECTEVENT(lParam) ) {
		case	FD_ACCEPT:
			DEBUGOUT( "Accepting...." );
			sockaddr_in	addr;
			int	len;
			len = sizeof(addr);
			m_SocketData = ::accept( m_SocketConnect, (sockaddr*)&addr, &len );
			if( m_SocketData == INVALID_SOCKET ) {
				DEBUGOUT( "failed.\n" );
				CLOSESOCKET( m_SocketConnect );
				if( m_hWndMsg ) {
					::PostMessage( m_hWndMsg, WM_NETPLAY_ERROR, 0, 0 );
				}
				return	0L;
			}
			DEBUGOUT( "done.\n" );

			// Nagleアルゴリズムの無効化
			bOpt = TRUE;
			if( ::setsockopt( m_SocketData, IPPROTO_TCP, TCP_NODELAY, (const char*)&bOpt, sizeof(bOpt) ) == SOCKET_ERROR ) {
				DEBUGOUT( "CNetPlay:setsockopt failed.\n" );
				CLOSESOCKET( m_SocketData );
				if( m_hWndMsg ) {
					::PostMessage( m_hWndMsg, WM_NETPLAY_ERROR, 0, 0 );
				}
				return	0L;
			}
			// ブロッキングモード設定
			bArg = TRUE;
			if( ::ioctlsocket( m_SocketData, FIONBIO, (unsigned long*)&bArg ) == SOCKET_ERROR ) {
				DEBUGOUT( "CNetPlay:ioctlsocket failed.\n" );
				CLOSESOCKET( m_SocketData );
				if( m_hWndMsg ) {
					::PostMessage( m_hWndMsg, WM_NETPLAY_ERROR, 0, 0 );
				}
				return	0L;
			}
			::WSAAsyncSelect( m_SocketData, m_hWnd, WM_NETPLAY, FD_CLOSE );
			m_bConnect = TRUE;
			if( m_hWndMsg ) {
				::PostMessage( m_hWndMsg, WM_NETPLAY_ACCEPT, 0, 0 );
			}
			break;
		case	FD_CONNECT:
			DEBUGOUT( "Connection done.\n" );
			::WSAAsyncSelect( m_SocketData, m_hWnd, WM_NETPLAY, FD_CLOSE );
			m_bConnect = TRUE;
			if( m_hWndMsg ) {
				::PostMessage( m_hWndMsg, WM_NETPLAY_CONNECT, 0, 0 );
			}
			break;
		case	FD_CLOSE:
			DEBUGOUT( "Connection close.\n" );
			Disconnect();
			if( m_hWndMsg ) {
				::PostMessage( m_hWndMsg, WM_NETPLAY_CLOSE, 0, 0 );
			}
			break;
	}

	return	0L;
}

