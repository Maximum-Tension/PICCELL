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
# FOR THE STEPS. IT IS INWAY'S UPDATER, MADE FOR A WINDOW: THE CHECK RUNS IN A
# THREAD, AND THE SWAP WAITS UNTIL THE WINDOW CLOSES.
*/

/* **************************** [v] INCLUDES [v] **************************** */
#include <windows.h>
/******* """""""""""
 * #define CP_UTF8
 * #define CREATE_ALWAYS
 * #define CREATE_NEW
 * #define CREATE_SUSPENDED
 * #define FALSE
 * #define FILE_ATTRIBUTE_TEMPORARY
 * #define FILE_FLAG_DELETE_ON_CLOSE
 * #define FILE_SHARE_READ
 * #define GENERIC_READ
 * #define GENERIC_WRITE
 * #define InterlockedCompareExchange
 * #define InterlockedExchange
 * #define INVALID_FILE_ATTRIBUTES
 * #define INVALID_HANDLE_VALUE
 * #define MAX_PATH
 * #define MOVEFILE_COPY_ALLOWED
 * #define MOVEFILE_REPLACE_EXISTING
 * #define MOVEFILE_WRITE_THROUGH
 * #define OPEN_EXISTING
 * #define SYNCHRONIZE
 * typedef	DWORD;
 * typedef	HANDLE;
 * typedef	LONG;
 * typedef	PROCESS_INFORMATION;
 * typedef	STARTUPINFOW;
 * typedef	WCHAR;
 * typedef	WIN32_FIND_DATAW;
 * BOOL		CloseHandle(HANDLE);
 * HANDLE	CreateFileW(
 * 	LPCWSTR,
 * 	DWORD,
 * 	DWORD,
 * 	LPSECURITY_ATTRIBUTES,
 * 	DWORD,
 * 	DWORD,
 * 	HANDLE
 * );
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
 * BOOL		DeleteFileW(LPCWSTR);
 * BOOL		FindClose(HANDLE);
 * HANDLE	FindFirstFileW(LPCWSTR, LPWIN32_FIND_DATAW);
 * BOOL		FindNextFileW(HANDLE, LPWIN32_FIND_DATAW);
 * LPWSTR	GetCommandLineW(void);
 * DWORD	GetCurrentProcessId(void);
 * DWORD	GetFileAttributesW(LPCWSTR);
 * DWORD	GetModuleFileNameW(HMODULE, LPWSTR, DWORD);
 * DWORD	GetTempPathW(DWORD, LPWSTR);
 * DWORD	GetTickCount(void);
 * LPWSTR	lstrcpynW(LPWSTR, LPCWSTR, int);
 * LPWSTR	lstrcpyW(LPWSTR, LPCWSTR);
 * int		lstrlenW(LPCWSTR);
 * BOOL		MoveFileExW(LPCWSTR, LPCWSTR, DWORD);
 * int		MultiByteToWideChar(UINT, DWORD, LPCCH, int, LPWSTR, int);
 * HANDLE	OpenProcess(DWORD, BOOL, DWORD);
 * BOOL		ReadFile(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
 * DWORD	ResumeThread(HANDLE);
 * void		Sleep(DWORD);
 * DWORD	WaitForSingleObject(HANDLE, DWORD);
 * int		WideCharToMultiByte(
 * 	UINT,
 * 	DWORD,
 * 	LPCWCH,
 * 	int,
 * 	LPSTR,
 * 	int,
 * 	LPCCH,
 * 	LPBOOL
 * );
 * BOOL		WriteFile(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
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
 * #define UPDATE_NONE
 * #define UPDATE_READY
 * #define UPDATE_REPO
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define HTTP_AGENT L"PICCELL/" L"" PICCELL_VERSION_TEXT
#define BODY_MAX (2U << 20) /* A RELEASE PAGE IS NEVER THAT BIG */

#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 0X00000800 /* OLD MINGW HEADERS */
#endif /* !WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 */

#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3 0X00002000 /* OLD MINGW HEADERS */
#endif /* !WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3 */

/* THE ANSWER OF THE RELEASE PAGE, ENDING WITH 0 */
struct	S_BODY
{
	char	*DATA;
	size_t	LEN;
	size_t	ROOM;
};

/* UPDATE_READY ONCE G__NEW_EXE__ AND G__TAG__ ARE SET */
static volatile LONG	G__STATE__ = UPDATE_NONE;
static WCHAR			G__NEW_EXE__[MAX_PATH * 2]; /* THE CHECKED DOWNLOAD */
static char				G__TAG__[64]; /* ITS RELEASE NAME */

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
	WinHttpSetTimeouts(SESSION, 5000, 5000, 8000, TIMEOUT);
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

		while (
			OK &&
			WinHttpReadData(REQUEST, PIECE, sizeof(PIECE), &GOT) &&
			GOT
		)
		{
			DWORD	DONE;

			if (FILE_HANDLE)
				OK = (
					WriteFile(FILE_HANDLE, PIECE, GOT, &DONE, NULL) &&
					DONE == GOT
				);
			else if (BODY->LEN + GOT > BODY_MAX)
				OK = 0;
			else
				OK = BODY_ADD(BODY, PIECE, GOT);
		}

		if (!OK)
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

/* UTF-16 TO A NEW UTF-8 STRING (FREE IT). NULL WHEN IT FAILS */
static char
	*TO_UTF8(const WCHAR *TEXT)
{
	char		*BYTES;
	const int	SIZE = WideCharToMultiByte(
		CP_UTF8,
		0,
		TEXT,
		-1,
		NULL,
		0,
		NULL,
		NULL
	);

	if (SIZE <= 0)
		return (NULL);

	BYTES = (char *)malloc((size_t)SIZE);

	if (BYTES)
		WideCharToMultiByte(CP_UTF8, 0, TEXT, -1, BYTES, SIZE, NULL, NULL);

	return (BYTES);
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

/* LEFTOVERS OF AN EARLIER UPDATE THAT COULD NOT BE MOVED OR WAS CUT SHORT */
static void
	CLEAN_TEMP(void)
{
	WIN32_FIND_DATAW	FD;
	HANDLE				FIND;
	WCHAR				DIR[MAX_PATH];
	WCHAR				PATH[MAX_PATH * 2];

	TEMP_DIR(DIR);
	swprintf(PATH, MAX_PATH * 2, L"%ls\\PICCELL-NEW-*.exe", DIR);
	FIND = FindFirstFileW(PATH, &FD);

	if (FIND == INVALID_HANDLE_VALUE)
		return ;

	do
	{
		swprintf(PATH, MAX_PATH * 2, L"%ls\\%ls", DIR, FD.cFileName);
		DeleteFileW(PATH); /* FAILS QUIETLY WHILE IT RUNS */
	}
	while (FindNextFileW(FIND, &FD));

	FindClose(FIND);
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

/* STARTS "EXE" WITH THE GIVEN ARGUMENTS. FLAGS: CREATE_SUSPENDED OR 0 */
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
			8000
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

int
	UPDATE_FIND(char *TAG, size_t CAP)
{
	WCHAR	EXE[MAX_PATH];
	WCHAR	DIR[MAX_PATH];
	char	NAME[64];
	char	URL[600];
	char	SHA[80];

	TAG[0] = '\0';
	CLEAN_TEMP();
	GetModuleFileNameW(NULL, EXE, MAX_PATH);

	if (
		IS_SOURCE_FOLDER(EXE) ||
		!CAN_WRITE_HERE(EXE) ||
		!LATEST_RELEASE(
			NAME,
			sizeof(NAME),
			URL,
			sizeof(URL),
			SHA,
			sizeof(SHA)
		)
	)
		return (UPDATE_NONE);

	TEMP_DIR(DIR);
	swprintf(
		G__NEW_EXE__,
		MAX_PATH * 2,
		L"%ls\\PICCELL-NEW-%lu.exe",
		DIR,
		(unsigned long)GetTickCount()
	);

	if (!DOWNLOAD_VERIFIED(URL, SHA, G__NEW_EXE__))
		return (UPDATE_NONE);

	snprintf(G__TAG__, sizeof(G__TAG__), "%s", NAME);
	snprintf(TAG, CAP, "%s", NAME);
	InterlockedExchange(&G__STATE__, UPDATE_READY);
	return (UPDATE_READY);
}

int
	UPDATE_APPLY(int OPEN_AGAIN)
{
	PROCESS_INFORMATION	PI;
	WCHAR				EXE[MAX_PATH];
	WCHAR				NOTE[MAX_PATH * 2];
	char				*OLD;
	HANDLE				FILE_HANDLE;

	/* ONLY ONCE: "RESTART NOW", THEN THE WINDOW CLOSES TOO */
	if (
		InterlockedCompareExchange(&G__STATE__, UPDATE_NONE, UPDATE_READY) !=
		UPDATE_READY
	)
		return (0);

	GetModuleFileNameW(NULL, EXE, MAX_PATH);

	if (
		!START(
			G__NEW_EXE__,
			L"-update-finish",
			ARGUMENTS(),
			CREATE_SUSPENDED,
			&PI
		)
	)
	{
		DeleteFileW(G__NEW_EXE__);
		return (0);
	}

	NOTE_PATH(PI.dwProcessId, NOTE);
	OLD = TO_UTF8(EXE);
	FILE_HANDLE = CreateFileW(
		NOTE,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		0,
		NULL
	);

	if (FILE_HANDLE != INVALID_HANDLE_VALUE && OLD)
	{
		char	TEXT[MAX_PATH * 4 + 160];
		DWORD	DONE;

		/* PID OF THIS EXE, ITS PATH, THE RELEASE NAME, OPEN OR QUIET */
		if (OPEN_AGAIN)
			snprintf(
				TEXT,
				sizeof(TEXT),
				"%lu\r\n%s\r\n%s\r\nopen\r\n",
				(unsigned long)GetCurrentProcessId(),
				OLD,
				G__TAG__
			);
		else
			snprintf(
				TEXT,
				sizeof(TEXT),
				"%lu\r\n%s\r\n%s\r\nquiet\r\n",
				(unsigned long)GetCurrentProcessId(),
				OLD,
				G__TAG__
			);

		WriteFile(FILE_HANDLE, TEXT, (DWORD)strlen(TEXT), &DONE, NULL);
	}

	if (FILE_HANDLE != INVALID_HANDLE_VALUE)
		CloseHandle(FILE_HANDLE);

	free(OLD);
	ResumeThread(PI.hThread);
	CloseHandle(PI.hThread);
	CloseHandle(PI.hProcess);
	return (1); /* THE NEW ONE WAITS FOR THIS ONE TO CLOSE */
}

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
/*  //\\  |                        THE UPDATE                        |  //\\  */
/* '""""' #==========================================================# '""""' */
