/******************************************************************************\
# C - update                                     #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/20 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/23 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

/*
# GITHUB RELEASE CHECK, DOWNLOAD, SHA-256 CHECK AND THE EXE SWAP. SEE UPDATE.H
# FOR THE STEPS. IT ONLY RUNS WHEN PICCELL STARTS, BEFORE ITS WINDOW OPENS:
# THE CHECK RUNS IN A THREAD, AND A SMALL WINDOW SAYS WHAT HAPPENS WHEN IT
# TAKES LONGER THAN A MOMENT.
*/

/* **************************** [v] INCLUDES [v] **************************** */
#include <windows.h>
/******* """""""""""
 * #define COLOR_GRAYTEXT
 * #define COLOR_WINDOW
 * #define COLOR_WINDOWTEXT
 * #define CALLBACK
 * #define CP_UTF8
 * #define CREATE_ALWAYS
 * #define CREATE_NEW
 * #define DT_END_ELLIPSIS
 * #define DT_LEFT
 * #define DT_NOPREFIX
 * #define DT_SINGLELINE
 * #define DT_VCENTER
 * #define ERROR_ALREADY_EXISTS
 * #define FALSE
 * #define FILE_ATTRIBUTE_TEMPORARY
 * #define FILE_FLAG_DELETE_ON_CLOSE
 * #define FILE_SHARE_READ
 * #define GENERIC_READ
 * #define GENERIC_WRITE
 * #define GWL_STYLE
 * #define InterlockedCompareExchange
 * #define InterlockedExchange
 * #define InterlockedExchangeAdd
 * #define INVALID_FILE_ATTRIBUTES
 * #define INVALID_HANDLE_VALUE
 * #define LANG_TURKISH
 * #define LOGPIXELSY
 * #define MAKEINTRESOURCEW(I)
 * #define MAX_PATH
 * #define MOVEFILE_COPY_ALLOWED
 * #define MOVEFILE_REPLACE_EXISTING
 * #define MOVEFILE_WRITE_THROUGH
 * #define OPEN_EXISTING
 * #define PM_REMOVE
 * #define PRIMARYLANGID(L)
 * #define QS_ALLINPUT
 * #define SPI_GETNONCLIENTMETRICS
 * #define SPI_GETWORKAREA
 * #define SW_SHOWNORMAL
 * #define SYNCHRONIZE
 * #define TRANSPARENT
 * #define WAIT_OBJECT_0
 * #define WINAPI
 * #define WM_CLOSE
 * #define WM_ERASEBKGND
 * #define WM_PAINT
 * #define WM_USER
 * #define WS_CAPTION
 * #define WS_CHILD
 * #define WS_CLIPCHILDREN
 * #define WS_EX_APPWINDOW
 * #define WS_POPUP
 * #define WS_SYSMENU
 * #define WS_VISIBLE
 * typedef	DWORD;
 * typedef	HANDLE;
 * typedef	HDC;
 * typedef	HFONT;
 * typedef	HGDIOBJ;
 * typedef	HINSTANCE;
 * typedef	HWND;
 * typedef	LONG;
 * typedef	LONG_PTR;
 * typedef	LRESULT;
 * typedef	MSG;
 * typedef	NONCLIENTMETRICSW;
 * typedef	PAINTSTRUCT;
 * typedef	PROCESS_INFORMATION;
 * typedef	RECT;
 * typedef	STARTUPINFOW;
 * typedef	UINT_PTR;
 * typedef	WCHAR;
 * typedef	WIN32_FIND_DATAW;
 * typedef	WNDCLASSEXW;
 * BOOL		AdjustWindowRectEx(LPRECT, DWORD, BOOL, DWORD);
 * HDC		BeginPaint(HWND, LPPAINTSTRUCT);
 * BOOL		CloseHandle(HANDLE);
 * BOOL		CopyFileW(LPCWSTR, LPCWSTR, BOOL);
 * HANDLE	CreateFileW(
 * 	LPCWSTR,
 * 	DWORD,
 * 	DWORD,
 * 	LPSECURITY_ATTRIBUTES,
 * 	DWORD,
 * 	DWORD,
 * 	HANDLE
 * );
 * HFONT	CreateFontIndirectW(const LOGFONTW *);
 * HANDLE	CreateMutexW(LPSECURITY_ATTRIBUTES, BOOL, LPCWSTR);
 * BOOL		CreateProcessW(
 * 	LPCWSTR,
 * 	LPWSTR,
 * 	LPSECURITY_ATTRIBUTES,
 * 	LPSECURITY_ATTRIBUTES,
 * 	BOOL,
 * 	DWORD,
 * 	LPVOID,
 * 	LPCWSTR,
 * 	LPSTARTUPINFOW,
 * 	LPPROCESS_INFORMATION
 * );
 * HANDLE	CreateThread(
 * 	LPSECURITY_ATTRIBUTES,
 * 	SIZE_T,
 * 	LPTHREAD_START_ROUTINE,
 * 	LPVOID,
 * 	DWORD,
 * 	LPDWORD
 * );
 * HWND		CreateWindowExW(
 * 	DWORD,
 * 	LPCWSTR,
 * 	LPCWSTR,
 * 	DWORD,
 * 	int,
 * 	int,
 * 	int,
 * 	int,
 * 	HWND,
 * 	HMENU,
 * 	HINSTANCE,
 * 	LPVOID
 * );
 * LRESULT	DefWindowProcW(HWND, UINT, WPARAM, LPARAM);
 * BOOL		DeleteFileW(LPCWSTR);
 * BOOL		DeleteObject(HGDIOBJ);
 * BOOL		DestroyWindow(HWND);
 * LRESULT	DispatchMessageW(const MSG *);
 * int		DrawTextW(HDC, LPCWSTR, int, LPRECT, UINT);
 * BOOL		EndPaint(HWND, const PAINTSTRUCT *);
 * int		FillRect(HDC, const RECT *, HBRUSH);
 * BOOL		FindClose(HANDLE);
 * HANDLE	FindFirstFileW(LPCWSTR, LPWIN32_FIND_DATAW);
 * BOOL		FindNextFileW(HANDLE, LPWIN32_FIND_DATAW);
 * BOOL		GetClientRect(HWND, LPRECT);
 * LPWSTR	GetCommandLineW(void);
 * DWORD	GetCurrentProcessId(void);
 * HDC		GetDC(HWND);
 * int		GetDeviceCaps(HDC, int);
 * DWORD	GetFileAttributesW(LPCWSTR);
 * DWORD	GetLastError(void);
 * DWORD	GetModuleFileNameW(HMODULE, LPWSTR, DWORD);
 * HMODULE	GetModuleHandleW(LPCWSTR);
 * DWORD	GetSysColor(int);
 * HBRUSH	GetSysColorBrush(int);
 * DWORD	GetTempPathW(DWORD, LPWSTR);
 * DWORD	GetTickCount(void);
 * LANGID	GetUserDefaultUILanguage(void);
 * LONG_PTR	GetWindowLongPtrW(HWND, int);
 * BOOL		InvalidateRect(HWND, const RECT *, BOOL);
 * BOOL		IsWindowVisible(HWND);
 * HCURSOR	LoadCursorW(HINSTANCE, LPCWSTR);
 * HICON	LoadIconW(HINSTANCE, LPCWSTR);
 * int		lstrcmpW(LPCWSTR, LPCWSTR);
 * LPWSTR	lstrcpynW(LPWSTR, LPCWSTR, int);
 * LPWSTR	lstrcpyW(LPWSTR, LPCWSTR);
 * int		lstrlenW(LPCWSTR);
 * BOOL		MoveFileExW(LPCWSTR, LPCWSTR, DWORD);
 * DWORD	MsgWaitForMultipleObjects(
 * 	DWORD,
 * 	const HANDLE *,
 * 	BOOL,
 * 	DWORD,
 * 	DWORD
 * );
 * int		MulDiv(int, int, int);
 * int		MultiByteToWideChar(UINT, DWORD, LPCCH, int, LPWSTR, int);
 * HANDLE	OpenProcess(DWORD, BOOL, DWORD);
 * BOOL		PeekMessageW(LPMSG, HWND, UINT, UINT, UINT);
 * BOOL		ReadFile(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
 * ATOM		RegisterClassExW(const WNDCLASSEXW *);
 * int		ReleaseDC(HWND, HDC);
 * HGDIOBJ	SelectObject(HDC, HGDIOBJ);
 * LRESULT	SendMessageW(HWND, UINT, WPARAM, LPARAM);
 * int		SetBkMode(HDC, int);
 * BOOL		SetRect(LPRECT, int, int, int, int);
 * COLORREF	SetTextColor(HDC, COLORREF);
 * LONG_PTR	SetWindowLongPtrW(HWND, int, LONG_PTR);
 * BOOL		ShowWindow(HWND, int);
 * void		Sleep(DWORD);
 * BOOL		SystemParametersInfoW(UINT, UINT, PVOID, UINT);
 * BOOL		TranslateMessage(const MSG *);
 * DWORD	WaitForSingleObject(HANDLE, DWORD);
 * BOOL		WriteFile(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
 */
#include <commctrl.h>
/******* """"""""""""
 * #define ICC_PROGRESS_CLASS
 * #define PBM_SETMARQUEE
 * #define PBM_SETPOS
 * #define PBM_SETRANGE32
 * #define PBS_MARQUEE
 * #define PROGRESS_CLASSW
 * typedef	INITCOMMONCONTROLSEX;
 * BOOL	InitCommonControlsEx(const INITCOMMONCONTROLSEX *);
 */
#include <winhttp.h>
/******* """""""""""
 * #define INTERNET_SCHEME_HTTPS
 * #define WINHTTP_ACCESS_TYPE_DEFAULT_PROXY
 * #define WINHTTP_ADDREQ_FLAG_ADD
 * #define WINHTTP_DEFAULT_ACCEPT_TYPES
 * #define WINHTTP_FLAG_SECURE
 * #define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
 * #define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
 * #define WINHTTP_HEADER_NAME_BY_INDEX
 * #define WINHTTP_NO_ADDITIONAL_HEADERS
 * #define WINHTTP_NO_HEADER_INDEX
 * #define WINHTTP_NO_PROXY_BYPASS
 * #define WINHTTP_NO_PROXY_NAME
 * #define WINHTTP_NO_REFERER
 * #define WINHTTP_NO_REQUEST_DATA
 * #define WINHTTP_OPTION_SECURE_PROTOCOLS
 * #define WINHTTP_QUERY_CONTENT_LENGTH
 * #define WINHTTP_QUERY_FLAG_NUMBER
 * #define WINHTTP_QUERY_STATUS_CODE
 * typedef		HINTERNET;
 * typedef		URL_COMPONENTS;
 * BOOL			WinHttpAddRequestHeaders(HINTERNET, LPCWSTR, DWORD, DWORD);
 * BOOL			WinHttpCloseHandle(HINTERNET);
 * HINTERNET	WinHttpConnect(HINTERNET, LPCWSTR, INTERNET_PORT, DWORD);
 * BOOL			WinHttpCrackUrl(LPCWSTR, DWORD, DWORD, LPURL_COMPONENTS);
 * HINTERNET	WinHttpOpen(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
 * HINTERNET	WinHttpOpenRequest(
 * 	HINTERNET,
 * 	LPCWSTR,
 * 	LPCWSTR,
 * 	LPCWSTR,
 * 	LPCWSTR,
 * 	LPCWSTR *,
 * 	DWORD
 * );
 * BOOL			WinHttpQueryHeaders(
 * 	HINTERNET,
 * 	DWORD,
 * 	LPCWSTR,
 * 	LPVOID,
 * 	LPDWORD,
 * 	LPDWORD
 * );
 * BOOL			WinHttpReadData(HINTERNET, LPVOID, DWORD, LPDWORD);
 * BOOL			WinHttpReceiveResponse(HINTERNET, LPVOID);
 * BOOL			WinHttpSendRequest(
 * 	HINTERNET,
 * 	LPCWSTR,
 * 	DWORD,
 * 	LPVOID,
 * 	DWORD,
 * 	DWORD,
 * 	DWORD_PTR
 * );
 * BOOL			WinHttpSetOption(HINTERNET, DWORD, LPVOID, DWORD);
 * BOOL			WinHttpSetTimeouts(HINTERNET, int, int, int, int);
 */
#include <bcrypt.h>
/******* """"""""""
 * #define BCRYPT_SHA256_ALGORITHM
 * typedef	BCRYPT_ALG_HANDLE;
 * typedef	BCRYPT_HASH_HANDLE;
 * NTSTATUS	BCryptCloseAlgorithmProvider(BCRYPT_ALG_HANDLE, ULONG);
 * NTSTATUS	BCryptCreateHash(
 * 	BCRYPT_ALG_HANDLE,
 * 	BCRYPT_HASH_HANDLE *,
 * 	PUCHAR,
 * 	ULONG,
 * 	PUCHAR,
 * 	ULONG,
 * 	ULONG
 * );
 * NTSTATUS	BCryptDestroyHash(BCRYPT_HASH_HANDLE);
 * NTSTATUS	BCryptFinishHash(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
 * NTSTATUS	BCryptHashData(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
 * NTSTATUS	BCryptOpenAlgorithmProvider(
 * 	BCRYPT_ALG_HANDLE *,
 * 	LPCWSTR,
 * 	LPCWSTR,
 * 	ULONG
 * );
 */
#include <stdio.h>
/******* """""""""
 * int	snprintf(char *, size_t, const char *, ...);
 * int	swprintf(wchar_t *, size_t, const wchar_t *, ...);
 */
#include <stdlib.h>
/******* """"""""""
 * int				atoi(const char *);
 * void				free(void *);
 * void				*malloc(size_t);
 * void				*realloc(void *, size_t);
 * unsigned long	strtoul(const char *, char **, int);
 */
#include <string.h>
/******* """"""""""
 * #define NULL
 * typedef	size_t;
 * int		_stricmp(const char *, const char *);
 * int		_strnicmp(const char *, const char *, size_t);
 * void		*memcpy(void *, const void *, size_t);
 * void		*memset(void *, int, size_t);
 * size_t	strcspn(const char *, const char *);
 * size_t	strlen(const char *);
 * int		strncmp(const char *, const char *, size_t);
 * size_t	strspn(const char *, const char *);
 * char		*strstr(const char *, const char *);
 * wchar_t	*wcsrchr(const wchar_t *, wchar_t);
 */
#include <stdint.h>
/******* """"""""""
 * typedef	uint8_t;
 */
#include "version.h"
/******* """""""""""
 * #define PICCELL_VERSION_A
 * #define PICCELL_VERSION_B
 * #define PICCELL_VERSION_C
 * #define PICCELL_VERSION_D
 * #define PICCELL_VERSION_TEXT
 */
#include "update.h"
/******* """"""""""
 * #define UPDATE_ASSET
 * #define UPDATE_REPO
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define HTTP_AGENT L"PICCELL/" L"" PICCELL_VERSION_TEXT
#define BODY_MAX (2U << 20) /* A RELEASE PAGE IS NEVER THAT BIG */
#define WAIT_SHOW_MS 800 /* THE WAITING WINDOW ONLY SHOWS AFTER THIS LONG */
#define WAIT_TICK_MS 100 /* IT LOOKS AT WHAT HAPPENS THIS OFTEN */
#define CLEAN_TRIES 40 /* X 250 MS: THE OLD EXE MAY STILL BE CLOSING */

#define STEP_LOOK 0 /* ASKING GITHUB */
#define STEP_DOWNLOAD 1 /* THE NEW EXE IS COMING */
#define STEP_INSTALL 2 /* THE EXES ARE SWAPPED */

#define STATE_WORK 0 /* THE CHECK RUNS; CLOSING THE WINDOW STILL SKIPS IT */
#define STATE_SWAP 1 /* THE EXES ARE BEING SWAPPED: TOO LATE TO SKIP */
#define STATE_SKIP 2 /* THE WAITING WINDOW WAS CLOSED: NO UPDATE THIS TIME */
#define STATE_STARTED 3 /* THE NEW EXE RUNS: THIS ONE CLOSES */
#define STATE_NONE 4 /* NO UPDATE */

#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 0X00000800 /* OLD MINGW HEADERS */
#endif /* !WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 */

#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3 0X00002000 /* OLD MINGW HEADERS */
#endif /* !WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3 */

#ifndef PBS_MARQUEE
#define PBS_MARQUEE 0X08 /* OLD MINGW HEADERS */
#endif /* !PBS_MARQUEE */

#ifndef PBM_SETMARQUEE
#define PBM_SETMARQUEE (WM_USER + 10) /* OLD MINGW HEADERS */
#endif /* !PBM_SETMARQUEE */

/* THE ANSWER OF THE RELEASE PAGE, ENDING WITH 0 */
struct	S_BODY
{
	char	*DATA;
	size_t	LEN;
	size_t	ROOM;
};

static volatile LONG	G__STATE__ = STATE_WORK;
static volatile LONG	G__STEP__ = STEP_LOOK;
static volatile LONG	G__GOT__; /* BYTES OF THE NEW EXE SO FAR */
static volatile LONG	G__TOTAL__; /* ITS SIZE, 0 = NOT KNOWN */
static WCHAR			G__TAG__[64]; /* THE RELEASE NAME, FOR THE WINDOW */
static WCHAR			G__LINE__[160]; /* WHAT THE WAITING WINDOW SAYS */
static HWND				G__BAR__; /* ITS PROGRESS BAR */
static HFONT			G__FONT__;
static DWORD			G__START__; /* WHEN THE CHECK STARTED */
static int				G__DPI__ = 96;
static int				G__ENGLISH__;

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                           HTTP                           |  \\//  */
/*   \/   #==========================================================#   \/   */
/* 1 = ADDED. THE BODY ALWAYS ENDS WITH 0 */
static int
	BODY_ADD(struct S_BODY *BODY, const char *DATA, size_t SIZE)
{
	if (BODY->LEN + SIZE + 1 > BODY->ROOM)
	{
		size_t	ROOM;
		char	*MORE;

		ROOM = BODY->ROOM * 2 + SIZE + 4096;
		MORE = (char *)realloc(BODY->DATA, ROOM);

		if (!MORE)
			return (0);

		BODY->DATA = MORE;
		BODY->ROOM = ROOM;
	}

	memcpy(BODY->DATA + BODY->LEN, DATA, SIZE);
	BODY->LEN += SIZE;
	BODY->DATA[BODY->LEN] = '\0';
	return (1);
}

/*
# GITHUB ONLY TALKS TLS 1.2 AND NEWER. WINDOWS 7 GIVES WINHTTP ONLY TLS 1.0
# UNLESS THE PROGRAM ASKS FOR MORE, AND EVEN THEN ONLY WITH UPDATE KB3140245.
# WINDOWS 10 AND 11 ALREADY USE TLS 1.2 AND 1.3; ASKING CHANGES NOTHING THERE.
# AN OLDER WINDOWS THAT DOES NOT KNOW TLS 1.3 REFUSES THE FIRST ASK.
*/
static void
	ASK_FOR_TLS12(HINTERNET SESSION)
{
	DWORD	PROTOCOLS;

	PROTOCOLS = (
		WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 |
		WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
	);

	if (
		!WinHttpSetOption(
			SESSION,
			WINHTTP_OPTION_SECURE_PROTOCOLS,
			&PROTOCOLS,
			sizeof(PROTOCOLS)
		)
	)
	{
		PROTOCOLS = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
		WinHttpSetOption(
			SESSION,
			WINHTTP_OPTION_SECURE_PROTOCOLS,
			&PROTOCOLS,
			sizeof(PROTOCOLS)
		);
	}
}

/*
# GET URL. THE BODY GOES INTO BODY OR, WHEN FILE_HANDLE IS GIVEN, INTO THAT
# FILE. REDIRECTS ARE FOLLOWED (A RELEASE DOWNLOAD MOVES TO ANOTHER SERVER).
# RETURNS THE HTTP STATUS, 0 WHEN THERE WAS NO ANSWER AT ALL.
*/
static DWORD
	HTTP_GET(
		const WCHAR *URL,
		struct S_BODY *BODY,
		HANDLE FILE_HANDLE,
		int API,
		int TIMEOUT
	)
{
	URL_COMPONENTS	PARTS;
	HINTERNET		SESSION;
	HINTERNET		CONNECT;
	HINTERNET		REQUEST;
	WCHAR			HOST[256];
	WCHAR			PATH[2048];
	DWORD			STATUS;
	DWORD			FLAGS;

	memset(&PARTS, 0, sizeof(PARTS));
	PARTS.dwStructSize = sizeof(PARTS);
	PARTS.lpszHostName = HOST;
	PARTS.dwHostNameLength = 256;
	PARTS.lpszUrlPath = PATH;
	PARTS.dwUrlPathLength = 2048;

	if (!WinHttpCrackUrl(URL, 0, 0, &PARTS))
		return (0);

	STATUS = 0;
	SESSION = WinHttpOpen(
		HTTP_AGENT,
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS,
		0
	);

	if (!SESSION)
		return (0);

	ASK_FOR_TLS12(SESSION);
	WinHttpSetTimeouts(SESSION, 3000, 3000, 3000, TIMEOUT);
	CONNECT = WinHttpConnect(SESSION, HOST, PARTS.nPort, 0);
	REQUEST = NULL;

	if (PARTS.nScheme == INTERNET_SCHEME_HTTPS)
		FLAGS = WINHTTP_FLAG_SECURE;
	else
		FLAGS = 0;

	if (CONNECT)
		REQUEST = WinHttpOpenRequest(
			CONNECT,
			L"GET",
			PATH,
			NULL,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			FLAGS
		);

	if (REQUEST && API)
		WinHttpAddRequestHeaders(
			REQUEST,
			L"Accept: application/vnd.github+json\r\n"
			L"X-GitHub-Api-Version: 2022-11-28",
			(DWORD)-1,
			WINHTTP_ADDREQ_FLAG_ADD
		);

	if (
		REQUEST &&
		WinHttpSendRequest(
			REQUEST,
			WINHTTP_NO_ADDITIONAL_HEADERS,
			0,
			WINHTTP_NO_REQUEST_DATA,
			0,
			0,
			0
		) &&
		WinHttpReceiveResponse(REQUEST, NULL)
	)
	{
		char	PIECE[16384];
		DWORD	SIZE;
		DWORD	GOT;
		int		OK;

		SIZE = sizeof(STATUS);
		WinHttpQueryHeaders(
			REQUEST,
			WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
			WINHTTP_HEADER_NAME_BY_INDEX,
			&STATUS,
			&SIZE,
			WINHTTP_NO_HEADER_INDEX
		);
		OK = (STATUS == 200);

		if (OK && FILE_HANDLE)
		{
			DWORD	LENGTH;

			SIZE = sizeof(LENGTH);

			if (
				WinHttpQueryHeaders(
					REQUEST,
					WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
					WINHTTP_HEADER_NAME_BY_INDEX,
					&LENGTH,
					&SIZE,
					WINHTTP_NO_HEADER_INDEX
				)
			)
				InterlockedExchange(&G__TOTAL__, (LONG)LENGTH);
		}

		/* A CLOSED WAITING WINDOW STOPS IT AT THE NEXT PIECE */
		while (
			OK &&
			G__STATE__ != STATE_SKIP &&
			WinHttpReadData(REQUEST, PIECE, sizeof(PIECE), &GOT) &&
			GOT
		)
		{
			DWORD	DONE;

			if (FILE_HANDLE)
			{
				OK = (
					WriteFile(FILE_HANDLE, PIECE, GOT, &DONE, NULL) &&
					DONE == GOT
				);
				InterlockedExchangeAdd(&G__GOT__, (LONG)GOT);
			}
			else if (BODY->LEN + GOT > BODY_MAX)
				OK = 0;
			else
				OK = BODY_ADD(BODY, PIECE, GOT);
		}

		if (!OK || G__STATE__ == STATE_SKIP)
			STATUS = 0;
	}

	if (REQUEST)
		WinHttpCloseHandle(REQUEST);

	if (CONNECT)
		WinHttpCloseHandle(CONNECT);

	WinHttpCloseHandle(SESSION);
	return (STATUS);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                           HTTP                           |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                   READING THE RELEASE                    |  \\//  */
/*   \/   #==========================================================#   \/   */
/* THE STRING VALUE OF THE NEXT "KEY" AFTER FROM. RETURNS WHERE IT ENDS. */
static const char
	*JSON_STRING(
		const char *FROM,
		const char *KEY,
		char *OUTPUT,
		size_t CAP
	)
{
	char		NEEDLE[64];
	const char	*P;
	size_t		N;

	snprintf(NEEDLE, sizeof(NEEDLE), "\"%s\"", KEY);
	P = strstr(FROM, NEEDLE);

	if (!P)
		return (NULL);

	P += strlen(NEEDLE);

	while (*P == ' ' || *P == '\t' || *P == '\r' || *P == '\n' || *P == ':')
		++P;

	if (*P != '"') /* NULL OR NOT A STRING */
	{
		OUTPUT[0] = '\0';
		return (P);
	}

	++P;
	N = 0;

	while (*P && *P != '"' && N + 1 < CAP)
		OUTPUT[N++] = *P++;

	OUTPUT[N] = '\0';
	return (P);
}

/*
# FINDS THE PICCELL.EXE ASSET: ITS DOWNLOAD URL ENDS WITH "/PICCELL.EXE", AND
# ITS "DIGEST" IS THE ONE BETWEEN THE PREVIOUS ASSET'S URL AND THIS URL.
*/
static int
	FIND_ASSET(
		const char *JSON,
		char *URL,
		size_t URL_CAP,
		char *SHA,
		size_t SHA_CAP
	)
{
	const char		*P;
	const char		*KEY_AT;
	const char		*LAST_END;
	const size_t	NAME_LEN = strlen(UPDATE_ASSET);

	LAST_END = JSON;
	P = JSON;

	while ((KEY_AT = strstr(P, "\"browser_download_url\"")) != NULL)
	{
		const char	*D;
		char		VALUE[600];
		char		DIGEST[100];
		size_t		LEN;
		const char	*const AFTER = JSON_STRING(
			KEY_AT,
			"browser_download_url",
			VALUE,
			sizeof(VALUE)
		);

		if (!AFTER)
			return (0);

		LEN = strlen(VALUE);

		if (
			LEN > NAME_LEN &&
			VALUE[LEN - NAME_LEN - 1] == '/' &&
			_stricmp(VALUE + LEN - NAME_LEN, UPDATE_ASSET) == 0
		)
		{
			DIGEST[0] = '\0';

			for (
				D = strstr(LAST_END, "\"digest\"");
				D && D < KEY_AT;
				D = strstr(D + 1, "\"digest\"")
			)
				JSON_STRING(D, "digest", DIGEST, sizeof(DIGEST));

			snprintf(URL, URL_CAP, "%s", VALUE);

			if (
				_strnicmp(DIGEST, "sha256:", 7) != 0 ||
				strlen(DIGEST + 7) != 64
			)
				return (0); /* GITHUB GAVE NO SHA-256: DO NOT TRUST IT */

			snprintf(SHA, SHA_CAP, "%s", DIGEST + 7);
			return (1);
		}

		LAST_END = AFTER;
		P = AFTER;
	}

	return (0);
}

/*
# "20.26.11.20-MK1151" -> 1 WHEN ITS DATE IS THE SAME AS THIS EXE'S VERSION OR
# NEWER. ONLY THE FIRST FOUR NUMBERS COUNT; WHAT COMES AFTER THEM (LIKE
# "-MK1151") CAN BE ANYTHING. SAME DAY RELEASES ARE TOLD APART BY SHA-256.
*/
static int
	NOT_OLDER(const char *TAG)
{
	const int	MINE[4] = {
		PICCELL_VERSION_A,
		PICCELL_VERSION_B,
		PICCELL_VERSION_C,
		PICCELL_VERSION_D
	};
	int			THEIRS[4];
	int			I;
	const char	*P;

	P = TAG;

	while (*P && (*P < '0' || *P > '9'))
		++P;

	for (I = 0; I < 4; ++I)
	{
		THEIRS[I] = atoi(P);

		while (*P >= '0' && *P <= '9')
			++P;

		if (*P == '.')
			++P;
	}

	for (I = 0; I < 4; ++I)
		if (THEIRS[I] != MINE[I])
			return (THEIRS[I] > MINE[I]);

	return (1);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                   READING THE RELEASE                    |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          FILES                           |  \\//  */
/*   \/   #==========================================================#   \/   */
/* SHA-256 OF A FILE AS 64 LOWER CASE HEX LETTERS */
static int
	FILE_SHA256(const WCHAR *PATH, char *OUTPUT)
{
	static const char	HEX[] = "0123456789abcdef";
	BCRYPT_ALG_HANDLE	ALG;
	HANDLE				FILE_HANDLE;
	uint8_t				DIGEST[32];
	uint8_t				*PIECE;
	int					OK;
	int					I;

	OK = 0;
	FILE_HANDLE = CreateFileW(
		PATH,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);

	if (FILE_HANDLE == INVALID_HANDLE_VALUE)
		return (0);

	PIECE = (uint8_t *)malloc(1 << 16);

	if (
		PIECE &&
		BCryptOpenAlgorithmProvider(&ALG, BCRYPT_SHA256_ALGORITHM, NULL, 0) == 0
	)
	{
		BCRYPT_HASH_HANDLE	HASH;

		if (BCryptCreateHash(ALG, &HASH, NULL, 0, NULL, 0, 0) == 0)
		{
			DWORD	GOT;

			OK = 1;

			while (
				OK &&
				ReadFile(FILE_HANDLE, PIECE, 1 << 16, &GOT, NULL) &&
				GOT
			)
				OK = (BCryptHashData(HASH, PIECE, GOT, 0) == 0);

			OK = OK && BCryptFinishHash(HASH, DIGEST, sizeof(DIGEST), 0) == 0;
			BCryptDestroyHash(HASH);
		}

		BCryptCloseAlgorithmProvider(ALG, 0);
	}

	free(PIECE);
	CloseHandle(FILE_HANDLE);

	for (I = 0; OK && I < 32; ++I)
	{
		OUTPUT[I * 2] = HEX[DIGEST[I] >> 4];
		OUTPUT[I * 2 + 1] = HEX[DIGEST[I] & 15];
	}

	OUTPUT[64] = '\0';
	return (OK);
}

/* UTF-8 TO A NEW UTF-16 STRING (FREE IT). NULL WHEN IT FAILS */
static WCHAR
	*TO_WIDE(const char *TEXT)
{
	WCHAR		*WIDE;
	const int	CHARS = MultiByteToWideChar(CP_UTF8, 0, TEXT, -1, NULL, 0);

	if (CHARS <= 0)
		return (NULL);

	WIDE = (WCHAR *)malloc((size_t)CHARS * sizeof(WCHAR));

	if (WIDE)
		MultiByteToWideChar(CP_UTF8, 0, TEXT, -1, WIDE, CHARS);

	return (WIDE);
}

/* "%TMP%" WITHOUT THE LAST "\" */
static void
	TEMP_DIR(WCHAR *OUTPUT)
{
	DWORD	LEN;

	LEN = GetTempPathW(MAX_PATH, OUTPUT);

	while (LEN && OUTPUT[LEN - 1] == L'\\')
		OUTPUT[--LEN] = L'\0';
}

/* CAN WE REPLACE FILES NEXT TO THIS EXE? (NOT IN PROGRAM FILES) */
static int
	CAN_WRITE_HERE(const WCHAR *EXE)
{
	WCHAR	TEST[MAX_PATH + 32];
	WCHAR	*SLASH;
	HANDLE	FILE_HANDLE;

	swprintf(TEST, MAX_PATH + 32, L"%ls", EXE);
	SLASH = wcsrchr(TEST, L'\\');

	if (!SLASH)
		return (0);

	swprintf(
		SLASH + 1,
		(size_t)(TEST + MAX_PATH + 32 - SLASH - 1),
		L"PICCELL-%lu.tmp",
		(unsigned long)GetCurrentProcessId()
	);
	FILE_HANDLE = CreateFileW(
		TEST,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_NEW,
		FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
		NULL
	);

	if (FILE_HANDLE == INVALID_HANDLE_VALUE)
		return (0);

	CloseHandle(FILE_HANDLE);
	return (1);
}

/*
# AN EXE NEXT TO "SRC\PICCELL.H" IS A TEST BUILD IN THE SOURCE FOLDER. IT MUST
# NOT REPLACE ITSELF WITH THE RELEASE THE MOMENT IT IS STARTED.
*/
static int
	IS_SOURCE_FOLDER(const WCHAR *EXE)
{
	WCHAR	PATH[MAX_PATH + 16];
	WCHAR	*SLASH;

	lstrcpynW(PATH, EXE, MAX_PATH);
	SLASH = wcsrchr(PATH, L'\\');

	if (!SLASH)
		return (0);

	lstrcpyW(SLASH + 1, L"src\\piccell.h");
	return (GetFileAttributesW(PATH) != INVALID_FILE_ATTRIBUTES);
}

/*
# DELETES "<DIR>\<PATTERN>" AND RETURNS HOW MANY ARE STILL THERE: A RUNNING
# EXE, OR A FILE THAT IS STILL BEING WRITTEN, CAN NOT BE DELETED.
*/
static int
	DELETE_ALL(const WCHAR *DIR, const WCHAR *PATTERN)
{
	WIN32_FIND_DATAW	FD;
	HANDLE				FIND;
	WCHAR				PATH[MAX_PATH * 2];
	int					LEFT;

	swprintf(PATH, MAX_PATH * 2, L"%ls\\%ls", DIR, PATTERN);
	FIND = FindFirstFileW(PATH, &FD);

	if (FIND == INVALID_HANDLE_VALUE)
		return (0);

	LEFT = 0;

	do
	{
		swprintf(PATH, MAX_PATH * 2, L"%ls\\%ls", DIR, FD.cFileName);

		if (!DeleteFileW(PATH))
			++LEFT;
	}
	while (FindNextFileW(FIND, &FD));

	FindClose(FIND);
	return (LEFT);
}

/* THE FOLDER OF THIS EXE, WITHOUT THE LAST "\". 0 = UNKNOWN */
static int
	EXE_DIR(WCHAR *OUTPUT)
{
	WCHAR	*SLASH;

	OUTPUT[0] = L'\0';
	GetModuleFileNameW(NULL, OUTPUT, MAX_PATH);
	SLASH = wcsrchr(OUTPUT, L'\\');

	if (!SLASH)
		return (0);

	*SLASH = L'\0';
	return (1);
}

/* OLD EXES AN UPDATE LEFT BEHIND (SEE SWAP_AND_START): HOW MANY ARE LEFT */
static int
	DELETE_OLD(void)
{
	WCHAR	DIR[MAX_PATH];
	int		LEFT;

	TEMP_DIR(DIR);
	LEFT = DELETE_ALL(DIR, L"PICCELL-OLD-*.exe");

	if (EXE_DIR(DIR))
		LEFT += DELETE_ALL(DIR, L"PICCELL-OLD-*.exe");

	return (LEFT);
}

/* THE COMMAND LINE WITHOUT THE EXE NAME AND WITHOUT "-update-finish" */
static const WCHAR
	*ARGUMENTS(void)
{
	static const WCHAR	FLAG[] = L"-update-finish";
	const WCHAR			*P;
	int					I;

	P = GetCommandLineW();

	if (*P == L'"')
	{
		++P;

		while (*P && *P != L'"')
			++P;

		if (*P)
			++P;
	}
	else
		while (*P && *P != L' ' && *P != L'\t')
			++P;

	while (*P == L' ' || *P == L'\t')
		++P;

	for (I = 0; FLAG[I] && P[I] == FLAG[I]; ++I)
	{}

	if (!FLAG[I] && (!P[I] || P[I] == L' ' || P[I] == L'\t'))
		P += I;

	while (*P == L' ' || *P == L'\t')
		++P;

	return (P);
}

static void
	NOTE_PATH(DWORD PID, WCHAR *OUTPUT)
{
	WCHAR	DIR[MAX_PATH];

	TEMP_DIR(DIR);
	swprintf(
		OUTPUT,
		MAX_PATH * 2,
		L"%ls\\PICCELL-UPDATE-%lu.txt",
		DIR,
		(unsigned long)PID
	);
}

/* STARTS "EXE" WITH THE GIVEN ARGUMENTS. FLAGS GO TO CREATEPROCESSW */
static int
	START(
		const WCHAR *EXE,
		const WCHAR *FLAG,
		const WCHAR *REST,
		DWORD FLAGS,
		PROCESS_INFORMATION *PI
	)
{
	STARTUPINFOW	SI;
	WCHAR			*COMMAND;
	int				OK;
	const size_t	CHARS = (
		(size_t)lstrlenW(EXE) +
		(size_t)lstrlenW(FLAG) +
		(size_t)lstrlenW(REST) +
		8
	);

	COMMAND = (WCHAR *)malloc(CHARS * sizeof(WCHAR));

	if (!COMMAND)
		return (0);

	if (*FLAG)
		swprintf(COMMAND, CHARS, L"\"%ls\" %ls %ls", EXE, FLAG, REST);
	else
		swprintf(COMMAND, CHARS, L"\"%ls\" %ls", EXE, REST);

	memset(&SI, 0, sizeof(SI));
	SI.cb = sizeof(SI);
	memset(PI, 0, sizeof(*PI));
	OK = CreateProcessW(
		NULL,
		COMMAND,
		NULL,
		NULL,
		FALSE,
		FLAGS,
		NULL,
		NULL,
		&SI,
		PI
	);
	free(COMMAND);
	return (OK);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          FILES                           |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                        THE UPDATE                        |  \\//  */
/*   \/   #==========================================================#   \/   */
/*
# THE LATEST RELEASE. 1 = UPDATE: ITS PICCELL.EXE HAS ANOTHER SHA-256 THAN THE
# RUNNING EXE, AND ITS DATE IS NOT OLDER. 0 = NOTHING TO DO OR GITHUB COULD NOT
# BE ASKED. THE RUNNING EXE IS HASHED, SO A PACKED AND SIGNED RELEASE MATCHES
# ITS OWN DIGEST EXACTLY.
*/
static int
	LATEST_RELEASE(
		char *TAG,
		size_t TAG_CAP,
		char *URL,
		size_t URL_CAP,
		char *SHA,
		size_t SHA_CAP
	)
{
	struct S_BODY	BODY;
	WCHAR			EXE[MAX_PATH];
	char			MINE[80];
	int				OK;

	memset(&BODY, 0, sizeof(BODY));

	if (
		HTTP_GET(
			L"https://api.github.com/repos/" UPDATE_REPO L"/releases/latest",
			&BODY,
			NULL,
			1,
			5000
		) != 200 ||
		!BODY.DATA
	)
	{
		free(BODY.DATA);
		return (0);
	}

	OK = (
		JSON_STRING(BODY.DATA, "tag_name", TAG, TAG_CAP) &&
		NOT_OLDER(TAG) &&
		FIND_ASSET(BODY.DATA, URL, URL_CAP, SHA, SHA_CAP)
	);
	free(BODY.DATA);

	if (!OK)
		return (0);

	GetModuleFileNameW(NULL, EXE, MAX_PATH);

	if (FILE_SHA256(EXE, MINE) && _stricmp(MINE, SHA) == 0)
		return (0); /* THIS IS ALREADY THAT EXACT FILE */

	return (1);
}

/* DOWNLOADS URL TO TARGET AND KEEPS IT ONLY WHEN ITS SHA-256 IS RIGHT */
static int
	DOWNLOAD_VERIFIED(const char *URL, const char *SHA, const WCHAR *TARGET)
{
	WCHAR			*WIDE_URL;
	char			GOT[80];
	int				OK;
	const HANDLE	FILE_HANDLE = CreateFileW(
		TARGET,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		0,
		NULL
	);

	if (FILE_HANDLE == INVALID_HANDLE_VALUE)
		return (0);

	WIDE_URL = TO_WIDE(URL);
	OK = WIDE_URL && HTTP_GET(WIDE_URL, NULL, FILE_HANDLE, 0, 60000) == 200;
	free(WIDE_URL);
	CloseHandle(FILE_HANDLE);
	OK = OK && FILE_SHA256(TARGET, GOT) && _stricmp(GOT, SHA) == 0;

	if (!OK)
		DeleteFileW(TARGET);

	return (OK);
}

/*
# THE USER'S WAY: THE RUNNING EXE MOVES ITSELF TO %TMP% (WINDOWS LETS A
# RUNNING EXE BE RENAMED, NOT DELETED), THE NEW EXE TAKES ITS NAME, AND THE
# NEW ONE STARTS AND DELETES THE OLD ONE (UPDATE_CLEAN). WHEN %TMP% IS ON
# ANOTHER DRIVE, A RUNNING EXE CAN NOT GO THERE, SO IT IS RENAMED NEXT TO
# ITSELF INSTEAD. WHEN A STEP FAILS, THE OLD EXE GOES BACK TO ITS NAME AND
# JUST OPENS ITS WINDOW: PICCELL.EXE IS NEVER LOST.
# 1 = THE NEW EXE RUNS NOW.
*/
static int
	SWAP_AND_START(const WCHAR *NEW_EXE)
{
	PROCESS_INFORMATION	PI;
	WCHAR				SELF[MAX_PATH];
	WCHAR				DIR[MAX_PATH];
	WCHAR				OLD[MAX_PATH * 2];
	const unsigned long	PID = GetCurrentProcessId();
	const unsigned long	ID = GetTickCount();

	GetModuleFileNameW(NULL, SELF, MAX_PATH);
	TEMP_DIR(DIR);
	swprintf(
		OLD,
		MAX_PATH * 2,
		L"%ls\\PICCELL-OLD-%lu-%lu.exe",
		DIR,
		PID,
		ID
	);

	if (!MoveFileExW(SELF, OLD, MOVEFILE_WRITE_THROUGH))
	{
		if (!EXE_DIR(DIR))
		{
			DeleteFileW(NEW_EXE);
			return (0);
		}

		swprintf(
			OLD,
			MAX_PATH * 2,
			L"%ls\\PICCELL-OLD-%lu-%lu.exe",
			DIR,
			PID,
			ID
		);

		if (!MoveFileExW(SELF, OLD, MOVEFILE_WRITE_THROUGH))
		{
			DeleteFileW(NEW_EXE);
			return (0);
		}
	}

	if (
		!MoveFileExW(
			NEW_EXE,
			SELF,
			MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH
		)
	)
	{
		if (!MoveFileExW(OLD, SELF, MOVEFILE_WRITE_THROUGH))
			CopyFileW(OLD, SELF, FALSE);

		DeleteFileW(NEW_EXE);
		return (0);
	}

	/* IT DID NOT START: IT IS IN PLACE FOR NEXT TIME, THIS ONE OPENS NOW */
	if (!START(SELF, L"-updated", ARGUMENTS(), 0, &PI))
		return (0);

	CloseHandle(PI.hThread);
	CloseHandle(PI.hProcess);
	return (1);
}

/* THE WHOLE UPDATE. 1 = THE NEW EXE RUNS NOW, 0 = NO UPDATE */
static int
	CHECK_AND_SWAP(void)
{
	WCHAR	DIR[MAX_PATH];
	WCHAR	NEW_EXE[MAX_PATH * 2];
	char	TAG[64];
	char	URL[600];
	char	SHA[80];

	TEMP_DIR(DIR);
	DELETE_ALL(DIR, L"PICCELL-NEW-*.exe"); /* A DOWNLOAD THAT WAS CUT SHORT */

	if (!LATEST_RELEASE(TAG, sizeof(TAG), URL, sizeof(URL), SHA, sizeof(SHA)))
		return (0);

	if (MultiByteToWideChar(CP_UTF8, 0, TAG, -1, G__TAG__, 64) <= 0)
		G__TAG__[0] = L'\0';

	InterlockedExchange(&G__STEP__, STEP_DOWNLOAD);
	swprintf(
		NEW_EXE,
		MAX_PATH * 2,
		L"%ls\\PICCELL-NEW-%lu.exe",
		DIR,
		(unsigned long)GetTickCount()
	);

	if (!DOWNLOAD_VERIFIED(URL, SHA, NEW_EXE))
		return (0);

	/* THE WAITING WINDOW WAS CLOSED MEANWHILE: NO UPDATE THIS TIME */
	if (
		InterlockedCompareExchange(&G__STATE__, STATE_SWAP, STATE_WORK) !=
		STATE_WORK
	)
	{
		DeleteFileW(NEW_EXE);
		return (0);
	}

	InterlockedExchange(&G__STEP__, STEP_INSTALL);
	return (SWAP_AND_START(NEW_EXE));
}

/* THE THREAD OF THE CHECK. PARAMETER: THE MUTEX, CLOSED WHEN IT ENDS */
static DWORD WINAPI
	UPDATE_WORK(LPVOID PARAMETER)
{
	if (CHECK_AND_SWAP())
		InterlockedExchange(&G__STATE__, STATE_STARTED);
	else
	{
		InterlockedCompareExchange(&G__STATE__, STATE_NONE, STATE_WORK);
		InterlockedCompareExchange(&G__STATE__, STATE_NONE, STATE_SWAP);
	}

	CloseHandle((HANDLE)PARAMETER);
	return (0);
}

/* PARAMETER: HOW MANY TIMES TO TRY, 250 MS APART */
static DWORD WINAPI
	CLEAN_THREAD(LPVOID PARAMETER)
{
	const int	TRIES = (int)(UINT_PTR)PARAMETER;
	int			I;

	for (I = 0; I < TRIES && DELETE_OLD(); ++I)
		Sleep(250);

	return (0);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                        THE UPDATE                        |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                    THE WAITING WINDOW                    |  \\//  */
/*   \/   #==========================================================#   \/   */
/*
# WHILE THE CHECK RUNS IN ITS THREAD, THE MAIN THREAD WAITS HERE. WHEN IT
# TAKES LONGER THAN WAIT_SHOW_MS (A SLOW NETWORK, OR A DOWNLOAD), A SMALL
# WINDOW SAYS WHAT HAPPENS, SO NOBODY THINKS PICCELL DID NOT OPEN. CLOSING IT
# SKIPS THE UPDATE THIS TIME AND PICCELL OPENS AS IT IS.
*/
#define SCALE(N) MulDiv((N), G__DPI__, 96)

/* THE LINE THE WINDOW SHOWS NOW */
static void
	WAIT_TEXT(WCHAR *OUTPUT, size_t CAP)
{
	const LONG	STEP = G__STEP__;
	const LONG	TOTAL = G__TOTAL__;
	const int	PERCENT = (int)(((long long)G__GOT__ * 100) / (TOTAL + !TOTAL));

	if (STEP == STEP_INSTALL && G__ENGLISH__)
		swprintf(OUTPUT, CAP, L"Installing PICCELL %ls...", G__TAG__);
	else if (STEP == STEP_INSTALL)
		swprintf(OUTPUT, CAP, L"PICCELL %ls kuruluyor...", G__TAG__);
	else if (STEP == STEP_DOWNLOAD && TOTAL > 0 && G__ENGLISH__)
		swprintf(
			OUTPUT,
			CAP,
			L"Downloading PICCELL %ls... %d%%",
			G__TAG__,
			PERCENT
		);
	else if (STEP == STEP_DOWNLOAD && TOTAL > 0)
		swprintf(
			OUTPUT,
			CAP,
			L"PICCELL %ls indiriliyor... %%%d",
			G__TAG__,
			PERCENT
		);
	else if (STEP == STEP_DOWNLOAD && G__ENGLISH__)
		swprintf(OUTPUT, CAP, L"Downloading PICCELL %ls...", G__TAG__);
	else if (STEP == STEP_DOWNLOAD)
		swprintf(OUTPUT, CAP, L"PICCELL %ls indiriliyor...", G__TAG__);
	else if (G__ENGLISH__)
		swprintf(OUTPUT, CAP, L"Looking for a new version...");
	else
		swprintf(OUTPUT, CAP, L"Yeni s\u00FCr\u00FCm aran\u0131yor...");
}

/* MOVING WHILE THE SIZE IS NOT KNOWN, THEN HOW MUCH IS DOWNLOADED */
static void
	WAIT_BAR(void)
{
	LONG_PTR	STYLE;
	const LONG	TOTAL = G__TOTAL__;
	const int	EXACT = (G__STEP__ == STEP_DOWNLOAD && TOTAL > 0);

	if (!G__BAR__)
		return ;

	STYLE = GetWindowLongPtrW(G__BAR__, GWL_STYLE);

	if (EXACT && (STYLE & PBS_MARQUEE))
	{
		SendMessageW(G__BAR__, PBM_SETMARQUEE, FALSE, 0);
		SetWindowLongPtrW(G__BAR__, GWL_STYLE, STYLE & ~(LONG_PTR)PBS_MARQUEE);
		SendMessageW(G__BAR__, PBM_SETRANGE32, 0, 1000);
	}
	else if (!EXACT && !(STYLE & PBS_MARQUEE))
	{
		SetWindowLongPtrW(G__BAR__, GWL_STYLE, STYLE | PBS_MARQUEE);
		SendMessageW(G__BAR__, PBM_SETMARQUEE, TRUE, 30);
	}

	if (EXACT)
		SendMessageW(
			G__BAR__,
			PBM_SETPOS,
			(WPARAM)(((long long)G__GOT__ * 1000) / TOTAL),
			0
		);
}

static void
	WAIT_PAINT(HWND WINDOW)
{
	PAINTSTRUCT	PS;
	RECT		AREA;
	RECT		LINE;
	HGDIOBJ		OLD_FONT;
	const HDC	DC = BeginPaint(WINDOW, &PS);
	const UINT	FLAGS = (
		DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX
	);

	GetClientRect(WINDOW, &AREA);
	FillRect(DC, &AREA, GetSysColorBrush(COLOR_WINDOW));
	SetBkMode(DC, TRANSPARENT);
	OLD_FONT = NULL;

	if (G__FONT__)
		OLD_FONT = SelectObject(DC, G__FONT__);

	SetTextColor(DC, GetSysColor(COLOR_WINDOWTEXT));
	SetRect(&LINE, SCALE(16), SCALE(12), AREA.right - SCALE(16), SCALE(38));
	DrawTextW(DC, G__LINE__, -1, &LINE, FLAGS);

	if (G__STEP__ != STEP_INSTALL)
	{
		SetTextColor(DC, GetSysColor(COLOR_GRAYTEXT));
		SetRect(&LINE, SCALE(16), SCALE(66), AREA.right - SCALE(16), SCALE(92));

		if (G__ENGLISH__)
			DrawTextW(DC, L"Close this window to skip it.", -1, &LINE, FLAGS);
		else
			DrawTextW(
				DC,
				L"Atlamak i\u00E7in bu pencereyi kapat\u0131n.",
				-1,
				&LINE,
				FLAGS
			);
	}

	if (OLD_FONT)
		SelectObject(DC, OLD_FONT);

	EndPaint(WINDOW, &PS);
}

static LRESULT CALLBACK
	WAIT_PROC(HWND WINDOW, UINT MESSAGE, WPARAM WP, LPARAM LP)
{
	switch (MESSAGE)
	{
		case (WM_PAINT):
		{
			WAIT_PAINT(WINDOW);
		}
		return (0);
		case (WM_ERASEBKGND):
		{}
		return (1); /* WM_PAINT FILLS IT: NO FLICKER */
		case (WM_CLOSE):
		{
			/* SKIP THE UPDATE; TOO LATE ONCE THE EXES ARE BEING SWAPPED */
			InterlockedCompareExchange(&G__STATE__, STATE_SKIP, STATE_WORK);
		}
		return (0);
	}

	return (DefWindowProcW(WINDOW, MESSAGE, WP, LP));
}

/* NEW TEXT, THE BAR, AND THE WINDOW SHOWS ONCE IT TAKES LONG ENOUGH */
static void
	WAIT_TICK(HWND WINDOW)
{
	WCHAR	TEXT[160];

	if (!WINDOW)
		return ;

	WAIT_TEXT(TEXT, 160);

	if (lstrcmpW(TEXT, G__LINE__))
	{
		lstrcpynW(G__LINE__, TEXT, 160);
		InvalidateRect(WINDOW, NULL, FALSE);
	}

	WAIT_BAR();

	if (!IsWindowVisible(WINDOW) && GetTickCount() - G__START__ >= WAIT_SHOW_MS)
		ShowWindow(WINDOW, SW_SHOWNORMAL);
}

/* THE WINDOW, HIDDEN FOR NOW, IN THE MIDDLE OF THE SCREEN. NULL = NONE */
static HWND
	WAIT_OPEN(void)
{
	WNDCLASSEXW				CLASS;
	INITCOMMONCONTROLSEX	CONTROLS;
	NONCLIENTMETRICSW		METRICS;
	RECT					FRAME;
	RECT					WORK;
	HWND					WINDOW;
	HDC						SCREEN;
	int						WIDTH;
	int						HEIGHT;
	const DWORD				STYLE = (
		WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN
	);
	const HINSTANCE			SELF = GetModuleHandleW(NULL);

	G__ENGLISH__ = PRIMARYLANGID(GetUserDefaultUILanguage()) != LANG_TURKISH;
	WAIT_TEXT(G__LINE__, 160);
	SCREEN = GetDC(NULL);

	if (SCREEN)
	{
		G__DPI__ = GetDeviceCaps(SCREEN, LOGPIXELSY);
		ReleaseDC(NULL, SCREEN);
	}

	if (G__DPI__ < 96)
		G__DPI__ = 96;

	memset(&METRICS, 0, sizeof(METRICS));
	METRICS.cbSize = sizeof(METRICS);

	if (
		SystemParametersInfoW(
			SPI_GETNONCLIENTMETRICS,
			sizeof(METRICS),
			&METRICS,
			0
		)
	)
		G__FONT__ = CreateFontIndirectW(&METRICS.lfMessageFont);

	CONTROLS.dwSize = sizeof(CONTROLS);
	CONTROLS.dwICC = ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&CONTROLS);
	memset(&CLASS, 0, sizeof(CLASS));
	CLASS.cbSize = sizeof(CLASS);
	CLASS.lpfnWndProc = WAIT_PROC;
	CLASS.hInstance = SELF;
	CLASS.hIcon = LoadIconW(SELF, MAKEINTRESOURCEW(1));
	CLASS.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512)); /* ARROW */
	CLASS.lpszClassName = L"PICCELL-UPDATE";
	RegisterClassExW(&CLASS);
	SetRect(&FRAME, 0, 0, SCALE(360), SCALE(104));
	AdjustWindowRectEx(&FRAME, STYLE, FALSE, WS_EX_APPWINDOW);
	WIDTH = FRAME.right - FRAME.left;
	HEIGHT = FRAME.bottom - FRAME.top;

	if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &WORK, 0))
		SetRect(&WORK, 0, 0, 1024, 768);

	WINDOW = CreateWindowExW(
		WS_EX_APPWINDOW,
		L"PICCELL-UPDATE",
		L"PICCELL",
		STYLE,
		WORK.left + (WORK.right - WORK.left - WIDTH) / 2,
		WORK.top + (WORK.bottom - WORK.top - HEIGHT) / 2,
		WIDTH,
		HEIGHT,
		NULL,
		NULL,
		SELF,
		NULL
	);

	if (!WINDOW)
		return (NULL);

	G__BAR__ = CreateWindowExW(
		0,
		PROGRESS_CLASSW,
		NULL,
		WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
		SCALE(16),
		SCALE(44),
		SCALE(328),
		SCALE(14),
		WINDOW,
		NULL,
		SELF,
		NULL
	);

	if (G__BAR__)
		SendMessageW(G__BAR__, PBM_SETMARQUEE, TRUE, 30);

	return (WINDOW);
}

/* UNTIL THE CHECK ENDS OR THE WINDOW IS CLOSED */
static void
	WAIT_LOOP(HANDLE THREAD, HWND WINDOW)
{
	MSG		MESSAGE;
	DWORD	WOKE;

	for (;;)
	{
		WOKE = MsgWaitForMultipleObjects(
			1,
			&THREAD,
			FALSE,
			WAIT_TICK_MS,
			QS_ALLINPUT
		);

		if (WOKE == WAIT_OBJECT_0 || G__STATE__ == STATE_SKIP)
			return ;

		while (PeekMessageW(&MESSAGE, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&MESSAGE);
			DispatchMessageW(&MESSAGE);
		}

		WAIT_TICK(WINDOW);
	}
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                    THE WAITING WINDOW                    |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                         THE API                          |  \\//  */
/*   \/   #==========================================================#   \/   */
int
	UPDATE_AT_START(void)
{
	WCHAR	EXE[MAX_PATH];
	HANDLE	MUTEX;
	HANDLE	THREAD;
	HWND	WINDOW;

	GetModuleFileNameW(NULL, EXE, MAX_PATH);

	if (IS_SOURCE_FOLDER(EXE) || !CAN_WRITE_HERE(EXE))
		return (0);

	/* TWO PICCELLS OPENED AT ONCE: ONLY ONE OF THEM UPDATES */
	MUTEX = CreateMutexW(NULL, FALSE, L"Local\\PICCELL-UPDATE");

	if (!MUTEX)
		return (0);

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		CloseHandle(MUTEX);
		return (0);
	}

	G__START__ = GetTickCount();
	THREAD = CreateThread(NULL, 0, UPDATE_WORK, MUTEX, 0, NULL);

	if (!THREAD)
	{
		CloseHandle(MUTEX);
		return (0);
	}

	WINDOW = WAIT_OPEN();
	WAIT_LOOP(THREAD, WINDOW);

	if (WINDOW)
		DestroyWindow(WINDOW);

	if (G__FONT__)
		DeleteObject(G__FONT__);

	G__BAR__ = NULL;
	G__FONT__ = NULL;
	CloseHandle(THREAD); /* A SKIPPED CHECK ENDS BY ITSELF IN THE BACKGROUND */
	return (G__STATE__ == STATE_STARTED);
}

void
	UPDATE_CLEAN(int UPDATED)
{
	HANDLE	THREAD;
	int		TRIES;

	TRIES = 1;

	if (UPDATED)
		TRIES = CLEAN_TRIES;

	THREAD = CreateThread(
		NULL,
		0,
		CLEAN_THREAD,
		(LPVOID)(UINT_PTR)TRIES,
		0,
		NULL
	);

	if (THREAD)
		CloseHandle(THREAD);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                         THE API                          |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                       THE OLD WAY                        |  \\//  */
/*   \/   #==========================================================#   \/   */
/*
# PICCELL 20.26.11.23 AND OLDER UPDATED WHILE THEIR WINDOW WAS OPEN: THEY
# START THE NEW EXE IN %TMP% WITH "-update-finish" AND A NOTE THAT SAYS WHERE
# THE OLD EXE IS. SO THIS STAYS, OR THEIR LAST UPDATE COULD NOT FINISH.
*/
void
	UPDATE_FINISH(void)
{
	PROCESS_INFORMATION	PI;
	WCHAR				NOTE[MAX_PATH * 2];
	WCHAR				SELF[MAX_PATH];
	WCHAR				*OLD;
	char				TEXT[MAX_PATH * 4 + 160];
	char				*LINE;
	char				*NEXT;
	HANDLE				FILE_HANDLE;
	HANDLE				PROCESS;
	DWORD				GOT;
	DWORD				PID;
	int					OPEN_AGAIN;
	int					I;
	int					GONE;

	NOTE_PATH(GetCurrentProcessId(), NOTE);
	FILE_HANDLE = CreateFileW(
		NOTE,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);

	if (FILE_HANDLE == INVALID_HANDLE_VALUE)
		return ;

	GOT = 0;
	ReadFile(FILE_HANDLE, TEXT, sizeof(TEXT) - 1, &GOT, NULL);
	CloseHandle(FILE_HANDLE);
	DeleteFileW(NOTE);
	TEXT[GOT] = '\0';
	/* LINE 1: PID. LINE 2: THE OLD EXE. LINE 3: THE TAG. LINE 4: OPEN/QUIET */
	PID = (DWORD)strtoul(TEXT, &LINE, 10);

	while (*LINE == '\r' || *LINE == '\n')
		++LINE;

	NEXT = LINE + strcspn(LINE, "\r\n");

	if (*NEXT)
		*NEXT++ = '\0';

	NEXT += strspn(NEXT, "\r\n");
	NEXT += strcspn(NEXT, "\r\n"); /* THE TAG IS ONLY FOR PEOPLE READING IT */
	NEXT += strspn(NEXT, "\r\n");
	OPEN_AGAIN = (strncmp(NEXT, "open", 4) == 0);
	OLD = TO_WIDE(LINE);

	if (!OLD || !*OLD)
	{
		free(OLD);
		return ;
	}

	PROCESS = OpenProcess(SYNCHRONIZE, FALSE, PID);

	if (PROCESS)
	{
		WaitForSingleObject(PROCESS, 15000);
		CloseHandle(PROCESS);
	}

	GetModuleFileNameW(NULL, SELF, MAX_PATH);
	/*
	# REPLACE_EXISTING DELETES THE OLD EXE AND PUTS THIS ONE IN ITS PLACE IN
	# ONE STEP, SO THE OLD ONE IS NEVER LOST WHEN THE MOVE FAILS. ON ANOTHER
	# DRIVE IT COPIES AND THE TEMP FILE IS CLEANED UP AT THE NEXT CHECK.
	*/
	GONE = 0;

	for (I = 0; I < 50 && !GONE; ++I)
	{
		GONE = MoveFileExW(
			SELF,
			OLD,
			MOVEFILE_REPLACE_EXISTING |
			MOVEFILE_COPY_ALLOWED |
			MOVEFILE_WRITE_THROUGH
		);

		if (!GONE)
			Sleep(100); /* THE OLD ONE MAY STILL BE CLOSING */
	}

	/*
	# THE NEW EXE, OR THE OLD ONE WHEN THE MOVE FAILED: BOTH ARE AT OLD NOW.
	# "-updated" WHEN IT REALLY IS THE NEW ONE. AFTER A FAILED MOVE THE OLD ONE
	# OPENS WITH "-noupdate", OR IT WOULD FIND THE SAME UPDATE AND CLOSE AGAIN
	# AND AGAIN. THE NEXT TIME IT IS OPENED IT TRIES ONCE MORE.
	*/
	if (OPEN_AGAIN)
	{
		const WCHAR	*FLAG;

		if (GONE)
			FLAG = L"-updated";
		else
			FLAG = L"-noupdate";

		if (START(OLD, FLAG, ARGUMENTS(), 0, &PI))
		{
			CloseHandle(PI.hThread);
			CloseHandle(PI.hProcess);
		}
	}

	free(OLD);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                       THE OLD WAY                        |  //\\  */
/* '""""' #==========================================================# '""""' */
