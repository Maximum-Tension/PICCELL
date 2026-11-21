/******************************************************************************\
# H - winrt                                      #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/18 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/21 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

/*
# THE FEW WINDOWS RUNTIME (WINRT) INTERFACES PICCELL CALLS, WRITTEN OUT BY HAND:
# OLDER MINGW-W64 HAS NO HEADERS FOR THEM. THE IDS AND THE ORDER OF THE
# METHODS COME FROM THE WINDOWS METADATA (THE SAME AS MICROSOFT'S WINDOWS-RS).
# A METHOD PICCELL NEVER CALLS IS A PLAIN POINTER THAT ONLY KEEPS ITS PLACE;
# METHODS AFTER THE LAST ONE PICCELL CALLS ARE LEFT OUT.
*/

#ifndef WINRT_H
#define WINRT_H

/* **************************** [v] INCLUDES [v] **************************** */
#include <windows.h>
/******* """""""""""
 * #define STDMETHODCALLTYPE
 * typedef	BYTE;
 * typedef	HRESULT;
 * typedef	INT32;
 * typedef	UINT32;
 */
/* **************************** [^] INCLUDES [^] **************************** */

typedef struct S_HSTRING	*WINRT_STRING; /* A WINRT STRING (HSTRING) */

#define ASYNC_STARTED 0 /* ASYNCSTATUS */
#define ASYNC_COMPLETED 1
#define PIXEL_BGRA8 87 /* BITMAPPIXELFORMAT.BGRA8 */

struct	S_RECT /* WINDOWS.FOUNDATION.RECT */
{
	float	X;
	float	Y;
	float	WIDTH;
	float	HEIGHT;
};

/* THE FIRST SIX METHODS OF EVERY WINRT INTERFACE (IINSPECTABLE) */
#define INSPECTABLE_SLOTS \
	HRESULT	(STDMETHODCALLTYPE *QueryInterface)(void *, REFIID, void **);\
	ULONG	(STDMETHODCALLTYPE *AddRef)(void *);\
	ULONG	(STDMETHODCALLTYPE *Release)(void *);\
	void	*GetIids;\
	void	*GetRuntimeClassName;\
	void	*GetTrustLevel

struct	S_INSPECTABLE_VTBL
{
	INSPECTABLE_SLOTS;
};

struct	S_ASYNC_INFO_VTBL /* IASYNCINFO */
{
	INSPECTABLE_SLOTS;
	void	*get_Id;
	HRESULT	(STDMETHODCALLTYPE *get_Status)(void *, int *);
};

struct	S_ASYNC_OP_VTBL /* IASYNCOPERATION<T> */
{
	INSPECTABLE_SLOTS;
	void	*put_Completed;
	void	*get_Completed;
	HRESULT	(STDMETHODCALLTYPE *GetResults)(void *, void **);
};

struct	S_VECTOR_VIEW_VTBL /* IVECTORVIEW<T> */
{
	INSPECTABLE_SLOTS;
	HRESULT	(STDMETHODCALLTYPE *GetAt)(void *, UINT32, void **);
	HRESULT	(STDMETHODCALLTYPE *get_Size)(void *, UINT32 *);
};

struct	S_OCR_STATICS_VTBL /* IOCRENGINESTATICS */
{
	INSPECTABLE_SLOTS;
	HRESULT	(STDMETHODCALLTYPE *get_MaxImageDimension)(void *, UINT32 *);
	void	*get_AvailableRecognizerLanguages;
	void	*IsLanguageSupported;
	HRESULT	(STDMETHODCALLTYPE *TryCreateFromLanguage)(void *, void *, void **);
	HRESULT	(STDMETHODCALLTYPE *TryCreateFromUserProfileLanguages)(
		void *,
		void **
	);
};

struct	S_OCR_ENGINE_VTBL /* IOCRENGINE */
{
	INSPECTABLE_SLOTS;
	HRESULT	(STDMETHODCALLTYPE *RecognizeAsync)(void *, void *, void **);
};

struct	S_OCR_RESULT_VTBL /* IOCRRESULT */
{
	INSPECTABLE_SLOTS;
	HRESULT	(STDMETHODCALLTYPE *get_Lines)(void *, void **);
};

struct	S_OCR_LINE_VTBL /* IOCRLINE */
{
	INSPECTABLE_SLOTS;
	HRESULT	(STDMETHODCALLTYPE *get_Words)(void *, void **);
};

struct	S_OCR_WORD_VTBL /* IOCRWORD */
{
	INSPECTABLE_SLOTS;
	HRESULT	(STDMETHODCALLTYPE *get_BoundingRect)(void *, struct S_RECT *);
	HRESULT	(STDMETHODCALLTYPE *get_Text)(void *, WINRT_STRING *);
};

struct	S_BITMAP_STATICS_VTBL /* ISOFTWAREBITMAPSTATICS */
{
	INSPECTABLE_SLOTS;
	void	*Copy;
	void	*Convert;
	void	*ConvertWithAlpha;
	HRESULT	(STDMETHODCALLTYPE *CreateCopyFromBuffer)(
		void *,
		void *,
		int,
		INT32,
		INT32,
		void **
	);
};

struct	S_BUFFER_STATICS_VTBL /* ICRYPTOGRAPHICBUFFERSTATICS */
{
	INSPECTABLE_SLOTS;
	void	*Compare;
	void	*GenerateRandom;
	void	*GenerateRandomNumber;
	HRESULT	(STDMETHODCALLTYPE *CreateFromByteArray)(
		void *,
		UINT32,
		BYTE *,
		void **
	);
};

struct	S_LANGUAGE_FACTORY_VTBL /* ILANGUAGEFACTORY */
{
	INSPECTABLE_SLOTS;
	HRESULT	(STDMETHODCALLTYPE *CreateLanguage)(void *, WINRT_STRING, void **);
};

/* AN OBJECT IS A POINTER TO ITS TABLE OF METHODS */
#define WINRT_CALL(TYPE, OBJECT) \
	(((const struct TYPE * const *)(OBJECT))[0])

#endif /* !WINRT_H */
