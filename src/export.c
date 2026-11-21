/******************************************************************************\
# C - export                                     #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/19 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/21 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

/* **************************** [v] INCLUDES [v] **************************** */
#include <stdarg.h>
/******* """"""""""
 * #define va_end(ap)
 * #define va_start(ap, param)
 * typedef	va_list;
 */
#include <stdio.h>
/******* """""""""
 * typedef	FILE;
 * int		ferror(FILE *);
 * size_t	fwrite(const void *, size_t, size_t, FILE *);
 * int		vsnprintf(char *, size_t, const char *, va_list);
 */
#include <stdlib.h>
/******* """"""""""
 * void		*calloc(size_t, size_t);
 * void		free(void *);
 * void		*realloc(void *, size_t);
 * double	strtod(const char *, char **);
 */
#include <string.h>
/******* """"""""""
 * #define NULL
 * typedef	size_t;
 * void		*memcpy(void *, const void *, size_t);
 * void		*memset(void *, int, size_t);
 * char		*strchr(const char *, int);
 * int		strcmp(const char *, const char *);
 * size_t	strlen(const char *);
 * char		*strpbrk(const char *, const char *);
 * char		*strrchr(const char *, int);
 */
#include <time.h>
/******* """"""""
 * struct tm;
 * typedef		time_t;
 * struct tm	*localtime(const time_t *);
 * time_t		time(time_t *);
 */
#include "layout.h"
/******* """"""""""
 * struct S_TABLE;
 * int	LAYOUT_WRITE(const struct S_TABLE *, FILE *);
 */
#include "export.h"
/******* """"""""""
 * #define EXPORT_CSV
 * #define EXPORT_PDF
 * #define EXPORT_TSV
 * #define EXPORT_XLSX
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define KIND_TEXT 0
#define KIND_NUMBER 1
#define KIND_DATE 2
#define KIND_PERCENT 3

/* XLSX CELL STYLES (THE ORDER OF <cellXfs> IN STYLES.XML) */
#define STYLE_GENERAL 0
#define STYLE_DATE 1 /* NUMBER FORMAT 14: THE SHORT DATE OF THE PC */
#define STYLE_PERCENT 2 /* 0% */
#define STYLE_PERCENT_2 3 /* 0.00% */
#define STYLE_GROUPED 4 /* #,##0 */
#define STYLE_GROUPED_2 5 /* #,##0.00 */
#define STYLE_DECIMALS_2 6 /* 0.00 */
#define STYLE_HEADER 7 /* BOLD */

#define PDF_MARGIN 36.0 /* HALF AN INCH */
#define PDF_PAD 3.0 /* SPACE BETWEEN A CELL'S BORDER AND ITS TEXT */
#define PDF_SIZE 10.0 /* LETTER SIZE, MADE SMALLER DOWN TO PDF_SIZE_MIN */
#define PDF_SIZE_MIN 6.0

struct	S_BUF
{
	char	*DATA;
	size_t	SIZE;
	size_t	ROOM;
	int		FAILED;
};

/* WHAT A CELL HOLDS, AS EXCEL WOULD TAKE IT */
struct	S_VALUE
{
	int		KIND;
	int		STYLE;
	double	NUMBER;
};

/*
# THE WIDTHS OF HELVETICA (1/1000 OF THE LETTER SIZE) FOR BYTES 32 TO 255 OF
# WINDOWS-1254, THE TURKISH CODE PAGE THE PDF TEXT IS WRITTEN IN.
*/
static const short	G__WIDTH__[224] = {
	278, 278, 355, 556, 556, 889, 667, 191, 333, 333, 389, 584, 278, 333,
	278, 278, 556, 556, 556, 556, 556, 556, 556, 556, 556, 556, 278, 278,
	584, 584, 584, 556, 1015, 667, 667, 722, 722, 667, 611, 778, 722, 278,
	500, 667, 556, 833, 722, 778, 667, 778, 722, 667, 611, 722, 667, 944,
	667, 667, 611, 278, 278, 278, 469, 556, 333, 556, 556, 500, 556, 556,
	278, 556, 556, 222, 222, 500, 222, 833, 556, 556, 556, 556, 333, 500,
	278, 556, 500, 722, 500, 500, 500, 334, 260, 334, 584, 0, 556, 0,
	222, 556, 333, 1000, 556, 556, 333, 1000, 667, 333, 1000, 0, 0, 0,
	0, 222, 222, 333, 333, 350, 556, 1000, 333, 1000, 500, 333, 944, 0,
	0, 667, 278, 333, 556, 556, 556, 556, 260, 556, 333, 737, 370, 556,
	584, 333, 737, 552, 400, 549, 333, 333, 333, 576, 537, 333, 333, 333,
	365, 556, 834, 834, 834, 611, 667, 667, 667, 667, 667, 667, 1000, 722,
	667, 667, 667, 667, 278, 278, 278, 278, 778, 722, 778, 778, 778, 778,
	778, 584, 778, 722, 722, 722, 722, 278, 667, 611, 556, 556, 556, 556,
	556, 556, 889, 500, 556, 556, 556, 556, 278, 278, 278, 278, 556, 556,
	556, 556, 556, 556, 556, 549, 611, 556, 556, 556, 556, 278, 500, 500
};

/* THE SAME FOR HELVETICA-BOLD (THE HEADER ROW) */
static const short	G__BOLD__[224] = {
	278, 333, 474, 556, 556, 889, 722, 238, 333, 333, 389, 584, 278, 333,
	278, 278, 556, 556, 556, 556, 556, 556, 556, 556, 556, 556, 333, 333,
	584, 584, 584, 611, 975, 722, 722, 722, 722, 667, 611, 778, 722, 278,
	556, 722, 611, 833, 722, 778, 667, 778, 722, 667, 611, 722, 667, 944,
	667, 667, 611, 333, 278, 333, 584, 556, 333, 556, 611, 556, 611, 556,
	333, 611, 611, 278, 278, 556, 278, 889, 611, 611, 611, 611, 389, 556,
	333, 611, 556, 778, 556, 556, 500, 389, 280, 389, 584, 0, 556, 0,
	278, 556, 500, 1000, 556, 556, 333, 1000, 667, 333, 1000, 0, 0, 0,
	0, 278, 278, 500, 500, 350, 556, 1000, 333, 1000, 556, 333, 944, 0,
	0, 667, 278, 333, 556, 556, 556, 556, 280, 556, 333, 737, 370, 556,
	584, 333, 737, 552, 400, 549, 333, 333, 333, 576, 556, 333, 333, 333,
	365, 556, 834, 834, 834, 611, 722, 722, 722, 722, 722, 722, 1000, 722,
	667, 667, 667, 667, 278, 278, 278, 278, 778, 722, 778, 778, 778, 778,
	778, 584, 778, 722, 722, 722, 722, 278, 667, 611, 556, 556, 556, 556,
	556, 556, 889, 556, 556, 556, 556, 556, 278, 278, 278, 278, 611, 611,
	611, 611, 611, 611, 611, 549, 611, 611, 611, 611, 611, 278, 556, 556
};

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          BUFFER                          |  \\//  */
/*   \/   #==========================================================#   \/   */
static void
	PUT(struct S_BUF *B, const void *DATA, size_t SIZE)
{
	if (B->FAILED)
		return ;

	if (B->SIZE + SIZE > B->ROOM)
	{
		size_t	ROOM;
		char	*BIGGER;

		ROOM = B->ROOM * 2 + 4096;

		while (ROOM < B->SIZE + SIZE)
			ROOM *= 2;

		BIGGER = (char *)realloc(B->DATA, ROOM);

		if (!BIGGER)
		{
			B->FAILED = 1;
			return ;
		}

		B->DATA = BIGGER;
		B->ROOM = ROOM;
	}

	memcpy(B->DATA + B->SIZE, DATA, SIZE);
	B->SIZE += SIZE;
}

static void
	PUTS(struct S_BUF *B, const char *TEXT)
{
	PUT(B, TEXT, strlen(TEXT));
}

static void
	PUTF(struct S_BUF *B, const char *FORMAT, ...)
{
	char	LINE[512];
	va_list	ARGS;
	int		N;

	va_start(ARGS, FORMAT);
	N = vsnprintf(LINE, sizeof(LINE), FORMAT, ARGS);
	va_end(ARGS);

	if (N < 0 || N >= (int)sizeof(LINE))
	{
		B->FAILED = 1;
		return ;
	}

	PUT(B, LINE, (size_t)N);
}

/* THE BUFFER TO THE FILE AND FREED. 1 = WRITTEN */
static int
	FLUSH(struct S_BUF *B, FILE *OUTPUT)
{
	int	OK;

	OK = !B->FAILED;

	if (OK && B->SIZE)
		OK = fwrite(B->DATA, 1, B->SIZE, OUTPUT) == B->SIZE;

	free(B->DATA);
	memset(B, 0, sizeof(*B));
	return (OK && !ferror(OUTPUT));
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          BUFFER                          |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          VALUES                          |  \\//  */
/*   \/   #==========================================================#   \/   */
static int
	IS_DIGIT(int C)
{
	return (C >= '0' && C <= '9');
}

/* DAYS FROM 1899-12-30 (DAY 0 OF EXCEL) TO A DATE, -1 WHEN NO SUCH DATE */
static long
	EXCEL_DAY(int YEAR, int MONTH, int DAY)
{
	static const int	DAYS[12] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};
	const int			LEAP = (YEAR % 4 == 0 && YEAR % 100) || YEAR % 400 == 0;
	long				Y;
	long				ERA;
	long				YOE;
	long				DOY;
	long				DOE;
	int					LIMIT;

	if (YEAR < 1900 || YEAR > 9999 || MONTH < 1 || MONTH > 12 || DAY < 1)
		return (-1);

	LIMIT = DAYS[MONTH - 1] + (MONTH == 2 && LEAP);

	if (DAY > LIMIT)
		return (-1);

	/* DAYS FROM 1970-01-01 (THE CIVIL CALENDAR, MARCH FIRST) */
	Y = YEAR - (MONTH <= 2);
	ERA = Y / 400;
	YOE = Y - ERA * 400;
	DOY = (153 * (MONTH + 12 * (MONTH <= 2) - 3) + 2) / 5 + DAY - 1;
	DOE = YOE * 365 + YOE / 4 - YOE / 100 + DOY;
	return (ERA * 146097 + DOE - 719468 + 25569);
}

/* "12.05.2026", "12/05/26", "1-5-2026". 1 = A DATE */
static int
	PARSE_DATE(const char *TEXT, int TURKISH, struct S_VALUE *V)
{
	int		PART[3];
	int		DIGITS[3];
	char	SEPARATOR;
	long	DAY;
	int		I;

	SEPARATOR = 0;

	for (I = 0; I < 3; ++I)
	{
		PART[I] = 0;
		DIGITS[I] = 0;

		while (IS_DIGIT(*TEXT) && DIGITS[I] < 4)
		{
			PART[I] = PART[I] * 10 + (*TEXT++ - '0');
			++DIGITS[I];
		}

		if (!DIGITS[I])
			return (0);

		if (I == 2)
			break ;

		if (!SEPARATOR && (*TEXT == '.' || *TEXT == '/' || *TEXT == '-'))
			SEPARATOR = *TEXT;

		if (*TEXT != SEPARATOR)
			return (0);

		++TEXT;
	}

	if (*TEXT || DIGITS[0] > 2 || DIGITS[1] > 2 || DIGITS[2] == 3)
		return (0);

	if (DIGITS[2] <= 2)
		PART[2] += 2000;

	/* DAY FIRST, UNLESS ONLY THE OTHER WAY IS A DATE (ENGLISH "05/13/2026") */
	DAY = EXCEL_DAY(PART[2], PART[1], PART[0]);

	if (DAY < 0 && !TURKISH)
		DAY = EXCEL_DAY(PART[2], PART[0], PART[1]);

	if (DAY < 0)
		return (0);

	V->KIND = KIND_DATE;
	V->STYLE = STYLE_DATE;
	V->NUMBER = (double)DAY;
	return (1);
}

/* 1 = EVERY GROUP AFTER THE FIRST ONE HAS 3 DIGITS, THE FIRST ONE 1 TO 3 */
static int
	GROUPS_OF_3(const char *FROM, const char *TO, char MARK)
{
	const char	*P;
	int			RUN;
	int			FIRST;

	RUN = 0;
	FIRST = 1;

	for (P = FROM; P < TO; ++P)
	{
		if (*P != MARK)
		{
			++RUN;
			continue ;
		}

		if ((FIRST && (RUN < 1 || RUN > 3)) || (!FIRST && RUN != 3))
			return (0);

		FIRST = 0;
		RUN = 0;
	}

	return (!FIRST && RUN == 3);
}

/*
# "1.234,56" (TURKISH) OR "1,234.56": WHEN BOTH MARKS ARE THERE THE LAST ONE
# IS THE DECIMAL MARK. ONE MARK ALONE: A THOUSANDS MARK WHEN EVERY GROUP
# AFTER IT HAS 3 DIGITS AND IT IS NOT THE DECIMAL MARK OF THE LANGUAGE.
*/
static int
	PARSE_NUMBER(const char *TEXT, int TURKISH, struct S_VALUE *V)
{
	const char	*START;
	const char	*END;
	const char	*P;
	char		CLEAN[64];
	char		DECIMAL;
	char		GROUP;
	int			DOTS;
	int			COMMAS;
	int			DIGITS;
	int			LEN;
	int			AFTER;

	START = TEXT;

	if (*START == '-' || *START == '+')
		++START;

	DOTS = 0;
	COMMAS = 0;
	DIGITS = 0;

	for (END = START; IS_DIGIT(*END) || *END == '.' || *END == ','; ++END)
	{
		DOTS += *END == '.';
		COMMAS += *END == ',';
		DIGITS += IS_DIGIT(*END);
	}

	if (
		*END ||
		!DIGITS ||
		DIGITS > 15 ||
		!IS_DIGIT(START[0]) ||
		!IS_DIGIT(END[-1]) ||
		(START[0] == '0' && IS_DIGIT(START[1]))
	)
		return (0);

	DECIMAL = 0;
	GROUP = 0;

	if (DOTS && COMMAS)
	{
		DECIMAL = ',';

		if (strrchr(START, '.') > strrchr(START, ','))
			DECIMAL = '.';

		GROUP = (char)('.' + ',' - DECIMAL);
	}
	else if (DOTS + COMMAS)
	{
		const char	MARK = (char)(DOTS ? '.' : ',');
		const char	OWN = (char)(TURKISH ? ',' : '.');

		if (MARK != OWN && GROUPS_OF_3(START, END, MARK))
			GROUP = MARK;
		else if (DOTS + COMMAS == 1)
			DECIMAL = MARK;
		else if (GROUPS_OF_3(START, END, MARK))
			GROUP = MARK;
		else
			return (0);
	}

	if (DECIMAL && (DOTS + COMMAS > 1) && !GROUP)
		return (0);

	/* THE THOUSANDS MARKS ARE ONLY IN THE WHOLE PART: "1.234,56" */
	if (GROUP && DECIMAL)
	{
		const char	*WHOLE_END = strrchr(START, DECIMAL);

		if (strchr(WHOLE_END, GROUP) || !GROUPS_OF_3(START, WHOLE_END, GROUP))
			return (0);
	}

	LEN = 0;
	AFTER = -1;

	if (*TEXT == '-')
		CLEAN[LEN++] = '-';

	for (P = START; P < END; ++P)
	{
		if (*P == GROUP)
			continue ;

		if (*P == DECIMAL)
		{
			CLEAN[LEN++] = '.';
			AFTER = 0;
			continue ;
		}

		CLEAN[LEN++] = *P;
		AFTER += AFTER >= 0;
	}

	CLEAN[LEN] = '\0';
	V->KIND = KIND_NUMBER;
	V->NUMBER = strtod(CLEAN, NULL);
	V->STYLE = STYLE_GENERAL;

	if (GROUP && AFTER > 0)
		V->STYLE = STYLE_GROUPED_2;
	else if (GROUP)
		V->STYLE = STYLE_GROUPED;
	else if (AFTER == 2)
		V->STYLE = STYLE_DECIMALS_2;

	return (1);
}

/* WHAT A CELL HOLDS: A DATE, A PERCENT, A NUMBER OR TEXT */
static void
	PARSE_VALUE(const char *TEXT, int TURKISH, struct S_VALUE *V)
{
	char	CELL[64];
	size_t	LEN;

	V->KIND = KIND_TEXT;
	V->STYLE = STYLE_GENERAL;
	V->NUMBER = 0.0;

	while (*TEXT == ' ')
		++TEXT;

	LEN = strlen(TEXT);

	while (LEN && TEXT[LEN - 1] == ' ')
		--LEN;

	if (!LEN || LEN >= sizeof(CELL))
		return ;

	memcpy(CELL, TEXT, LEN);
	CELL[LEN] = '\0';

	if (PARSE_DATE(CELL, TURKISH, V))
		return ;

	/* "15%", "% 15" AND "15 %" */
	if (CELL[LEN - 1] == '%' || CELL[0] == '%')
	{
		char	*NUMBER;

		if (CELL[0] == '%')
			NUMBER = CELL + 1;
		else
		{
			CELL[LEN - 1] = '\0';
			NUMBER = CELL;
		}

		while (*NUMBER == ' ')
			++NUMBER;

		LEN = strlen(NUMBER);

		while (LEN && NUMBER[LEN - 1] == ' ')
			NUMBER[--LEN] = '\0';

		if (PARSE_NUMBER(NUMBER, TURKISH, V))
		{
			V->KIND = KIND_PERCENT;
			V->STYLE = STYLE_PERCENT;

			if (strpbrk(NUMBER, ".,") && V->NUMBER != (double)(long)V->NUMBER)
				V->STYLE = STYLE_PERCENT_2;

			V->NUMBER /= 100.0;
		}

		return ;
	}

	PARSE_NUMBER(CELL, TURKISH, V);
}

int
	EXPORT_NUMBER(const char *TEXT, int TURKISH, double *VALUE)
{
	struct S_VALUE	V;

	PARSE_VALUE(TEXT, TURKISH, &V);
	*VALUE = V.NUMBER;
	return (V.KIND != KIND_TEXT);
}
/*
# 1 = THE FIRST ROW NAMES THE COLUMNS: TWO OR MORE CELLS, ALL TEXT, ABOVE A
# COLUMN THAT HOLDS NUMBERS FURTHER DOWN.
*/
static int
	HAS_HEADER(const struct S_TABLE *T, int TURKISH)
{
	int	FILLED;
	int	C;

	FILLED = 0;

	if (T->ROWS < 2)
		return (0);

	for (C = 0; C < T->COLS; ++C)
	{
		const char	*CELL = T->CELL[C];
		double		V;

		if (!CELL || !CELL[0])
			continue ;

		if (EXPORT_NUMBER(CELL, TURKISH, &V))
			return (0);

		++FILLED;
	}

	for (C = 0; C < T->COLS && FILLED >= 2; ++C)
	{
		int	R;

		if (!T->CELL[C])
			continue ;

		for (R = 1; R < T->ROWS; ++R)
		{
			const char	*CELL = T->CELL[R * T->COLS + C];
			double		V;

			if (CELL && EXPORT_NUMBER(CELL, TURKISH, &V))
				return (1);
		}
	}

	return (0);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          VALUES                          |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                         TSV, CSV                         |  \\//  */
/*   \/   #==========================================================#   \/   */
/* A CSV CELL: IN QUOTES WHEN IT HOLDS THE SEPARATOR, A QUOTE OR A NEW LINE */
static void
	CSV_CELL(struct S_BUF *B, const char *CELL, char SEPARATOR)
{
	const size_t	LEN = strlen(CELL);

	if (
		!strchr(CELL, SEPARATOR) &&
		!strpbrk(CELL, "\"\r\n") &&
		(!LEN || (CELL[0] != ' ' && CELL[LEN - 1] != ' '))
	)
	{
		PUTS(B, CELL);
		return ;
	}

	PUT(B, "\"", 1);

	for (; *CELL; ++CELL)
	{
		if (*CELL == '"')
			PUT(B, "\"", 1);

		PUT(B, CELL, 1);
	}

	PUT(B, "\"", 1);
}

/* UTF-8 WITH A BOM, SO THAT EXCEL READS THE TURKISH LETTERS RIGHT */
static int
	WRITE_CSV(const struct S_TABLE *T, int TURKISH, FILE *OUTPUT)
{
	struct S_BUF	B;
	char			SEPARATOR;
	int				R;

	memset(&B, 0, sizeof(B));
	SEPARATOR = ',';

	if (TURKISH)
		SEPARATOR = ';';

	PUTS(&B, "\xEF\xBB\xBF");

	for (R = 0; R < T->ROWS; ++R)
	{
		int	C;

		for (C = 0; C < T->COLS; ++C)
		{
			const char	*CELL = T->CELL[R * T->COLS + C];

			if (C)
				PUT(&B, &SEPARATOR, 1);

			if (CELL)
				CSV_CELL(&B, CELL, SEPARATOR);
		}

		PUTS(&B, "\r\n");
	}

	return (FLUSH(&B, OUTPUT));
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                         TSV, CSV                         |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                           XLSX                           |  \\//  */
/*   \/   #==========================================================#   \/   */
static unsigned long
	CRC32(const void *DATA, size_t SIZE)
{
	static unsigned long	TABLE[256];
	const unsigned char		*P = (const unsigned char *)DATA;
	unsigned long			CRC;
	size_t					I;

	if (!TABLE[1])
	{
		unsigned long	N;

		for (N = 0; N < 256; ++N)
		{
			unsigned long	C;
			int				K;

			C = N;

			for (K = 0; K < 8; ++K)
			{
				if (C & 1)
					C = 0XEDB88320UL ^ (C >> 1);
				else
					C >>= 1;
			}

			TABLE[N] = C;
		}
	}

	CRC = 0XFFFFFFFFUL;

	for (I = 0; I < SIZE; ++I)
		CRC = TABLE[(CRC ^ P[I]) & 0XFF] ^ (CRC >> 8);

	return (CRC ^ 0XFFFFFFFFUL);
}

static void
	PUT16(struct S_BUF *B, unsigned long V)
{
	unsigned char	BYTES[2];

	BYTES[0] = (unsigned char)(V & 0XFF);
	BYTES[1] = (unsigned char)((V >> 8) & 0XFF);
	PUT(B, BYTES, 2);
}

static void
	PUT32(struct S_BUF *B, unsigned long V)
{
	PUT16(B, V & 0XFFFF);
	PUT16(B, (V >> 16) & 0XFFFF);
}

/*
# A ZIP WITHOUT COMPRESSION ("STORED"), WHICH EXCEL OPENS LIKE ANY OTHER.
# DIRECTORY: THE CENTRAL LIST, FILLED AS THE FILES GO IN.
*/
struct	S_ZIP
{
	struct S_BUF	DEST;
	struct S_BUF	DIRECTORY;
	unsigned long	TIME; /* MS-DOS TIME AND DATE */
	unsigned long	DATE;
	int				FILES;
};

static void
	ZIP_ADD(struct S_ZIP *Z, const char *NAME, const struct S_BUF *FILE_DATA)
{
	const unsigned long	OFFSET = (unsigned long)Z->DEST.SIZE;
	const unsigned long	CRC = CRC32(FILE_DATA->DATA, FILE_DATA->SIZE);
	const unsigned long	SIZE = (unsigned long)FILE_DATA->SIZE;
	const size_t		NAME_LEN = strlen(NAME);

	if (FILE_DATA->FAILED)
		Z->DEST.FAILED = 1;

	PUT32(&Z->DEST, 0X04034B50UL);
	PUT16(&Z->DEST, 20); /* VERSION NEEDED: 2.0 */
	PUT16(&Z->DEST, 0); /* FLAGS */
	PUT16(&Z->DEST, 0); /* STORED */
	PUT16(&Z->DEST, Z->TIME);
	PUT16(&Z->DEST, Z->DATE);
	PUT32(&Z->DEST, CRC);
	PUT32(&Z->DEST, SIZE);
	PUT32(&Z->DEST, SIZE);
	PUT16(&Z->DEST, (unsigned long)NAME_LEN);
	PUT16(&Z->DEST, 0); /* NO EXTRA FIELD */
	PUT(&Z->DEST, NAME, NAME_LEN);
	PUT(&Z->DEST, FILE_DATA->DATA, FILE_DATA->SIZE);
	PUT32(&Z->DIRECTORY, 0X02014B50UL);
	PUT16(&Z->DIRECTORY, 20); /* MADE BY: 2.0 */
	PUT16(&Z->DIRECTORY, 20);
	PUT16(&Z->DIRECTORY, 0);
	PUT16(&Z->DIRECTORY, 0);
	PUT16(&Z->DIRECTORY, Z->TIME);
	PUT16(&Z->DIRECTORY, Z->DATE);
	PUT32(&Z->DIRECTORY, CRC);
	PUT32(&Z->DIRECTORY, SIZE);
	PUT32(&Z->DIRECTORY, SIZE);
	PUT16(&Z->DIRECTORY, (unsigned long)NAME_LEN);
	PUT16(&Z->DIRECTORY, 0); /* EXTRA */
	PUT16(&Z->DIRECTORY, 0); /* COMMENT */
	PUT16(&Z->DIRECTORY, 0); /* DISK */
	PUT16(&Z->DIRECTORY, 0); /* INTERNAL ATTRIBUTES */
	PUT32(&Z->DIRECTORY, 0); /* EXTERNAL ATTRIBUTES */
	PUT32(&Z->DIRECTORY, OFFSET);
	PUT(&Z->DIRECTORY, NAME, NAME_LEN);
	++Z->FILES;
}

static int
	ZIP_END(struct S_ZIP *Z, FILE *OUTPUT)
{
	const unsigned long	START = (unsigned long)Z->DEST.SIZE;
	const unsigned long	SIZE = (unsigned long)Z->DIRECTORY.SIZE;

	if (Z->DIRECTORY.FAILED)
		Z->DEST.FAILED = 1;

	PUT(&Z->DEST, Z->DIRECTORY.DATA, Z->DIRECTORY.SIZE);
	PUT32(&Z->DEST, 0X06054B50UL);
	PUT16(&Z->DEST, 0);
	PUT16(&Z->DEST, 0);
	PUT16(&Z->DEST, (unsigned long)Z->FILES);
	PUT16(&Z->DEST, (unsigned long)Z->FILES);
	PUT32(&Z->DEST, SIZE);
	PUT32(&Z->DEST, START);
	PUT16(&Z->DEST, 0);
	free(Z->DIRECTORY.DATA);
	return (FLUSH(&Z->DEST, OUTPUT));
}

/* TEXT WITH & < > " MADE SAFE FOR XML; CONTROL CHARACTERS LEFT OUT */
static void
	PUT_XML(struct S_BUF *B, const char *TEXT)
{
	for (; *TEXT; ++TEXT)
	{
		const unsigned char	C = (unsigned char)*TEXT;

		if (C == '&')
			PUTS(B, "&amp;");
		else if (C == '<')
			PUTS(B, "&lt;");
		else if (C == '>')
			PUTS(B, "&gt;");
		else if (C == '"')
			PUTS(B, "&quot;");
		else if (C >= 0X20 || C == '\t' || C == '\n')
			PUT(B, TEXT, 1);
	}
}

/* THE NAME OF A COLUMN: A, B, ... Z, AA, AB ... */
static void
	COLUMN_NAME(int COL, char *DEST)
{
	char	REVERSED[8];
	int		N;
	int		I;

	N = 0;
	++COL;

	while (COL > 0 && N < 7)
	{
		REVERSED[N++] = (char)('A' + (COL - 1) % 26);
		COL = (COL - 1) / 26;
	}

	for (I = 0; I < N; ++I)
		DEST[I] = REVERSED[N - 1 - I];

	DEST[N] = '\0';
}

/* LETTERS (NOT BYTES) OF A UTF-8 TEXT */
static int
	LETTER_COUNT(const char *TEXT)
{
	int	N;

	N = 0;

	for (; *TEXT; ++TEXT)
		N += ((unsigned char)*TEXT & 0XC0) != 0X80;

	return (N);
}

static void
	SHEET_XML(struct S_BUF *B, const struct S_TABLE *T, int TURKISH)
{
	const int	HEADER = HAS_HEADER(T, TURKISH);
	int			R;
	int			C;

	PUTS(
		B,
		"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\r\n"
		"<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml"
		"/2006/main\">"
	);

	/* THE HEADER ROW STAYS IN SIGHT WHILE SCROLLING */
	if (HEADER)
		PUTS(
			B,
			"<sheetViews><sheetView workbookViewId=\"0\"><pane ySplit=\"1\" "
			"topLeftCell=\"A2\" activePane=\"bottomLeft\" state=\"frozen\"/>"
			"</sheetView></sheetViews>"
		);

	/* COLUMN WIDTHS FROM THE LONGEST CELL (IN CHARACTERS) */
	if (T->COLS)
		PUTS(B, "<cols>");

	for (C = 0; C < T->COLS; ++C)
	{
		int	WIDEST;

		WIDEST = 0;

		for (R = 0; R < T->ROWS; ++R)
		{
			const char	*CELL = T->CELL[R * T->COLS + C];

			if (CELL && LETTER_COUNT(CELL) > WIDEST)
				WIDEST = LETTER_COUNT(CELL);
		}

		if (WIDEST < 6)
			WIDEST = 6;

		if (WIDEST > 60)
			WIDEST = 60;

		PUTF(
			B,
			"<col min=\"%d\" max=\"%d\" width=\"%.1f\" customWidth=\"1\"/>",
			C + 1,
			C + 1,
			WIDEST * 1.15 + 2.0
		);
	}

	if (T->COLS)
		PUTS(B, "</cols>");

	PUTS(B, "<sheetData>");

	for (R = 0; R < T->ROWS; ++R)
	{
		PUTF(B, "<row r=\"%d\">", R + 1);

		for (C = 0; C < T->COLS; ++C)
		{
			const char		*CELL = T->CELL[R * T->COLS + C];
			struct S_VALUE	V;
			char			NAME[8];

			if (!CELL || !CELL[0])
				continue ;

			COLUMN_NAME(C, NAME);
			PARSE_VALUE(CELL, TURKISH, &V);

			if (V.KIND == KIND_TEXT)
			{
				PUTF(B, "<c r=\"%s%d\" t=\"inlineStr\"", NAME, R + 1);

				if (HEADER && !R)
					PUTF(B, " s=\"%d\"", STYLE_HEADER);

				PUTS(B, "><is>");
				PUTS(B, "<t xml:space=\"preserve\">");
				PUT_XML(B, CELL);
				PUTS(B, "</t></is></c>");
				continue ;
			}

			PUTF(B, "<c r=\"%s%d\"", NAME, R + 1);

			if (V.STYLE)
				PUTF(B, " s=\"%d\"", V.STYLE);

			PUTF(B, "><v>%.15g</v></c>", V.NUMBER);
		}

		PUTS(B, "</row>");
	}

	PUTS(B, "</sheetData></worksheet>");
}

static int
	WRITE_XLSX(const struct S_TABLE *T, int TURKISH, FILE *OUTPUT)
{
	static const char	TYPES[] =
		"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\r\n"
		"<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/"
		"content-types\">"
		"<Default Extension=\"rels\" ContentType=\"application/"
		"vnd.openxmlformats-package.relationships+xml\"/>"
		"<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
		"<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/"
		"vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
		"<Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\""
		"application/vnd.openxmlformats-officedocument.spreadsheetml."
		"worksheet+xml\"/>"
		"<Override PartName=\"/xl/styles.xml\" ContentType=\"application/"
		"vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>"
		"</Types>";
	static const char	RELS[] =
		"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\r\n"
		"<Relationships xmlns=\"http://schemas.openxmlformats.org/package/"
		"2006/relationships\">"
		"<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/"
		"officeDocument/2006/relationships/officeDocument\" "
		"Target=\"xl/workbook.xml\"/>"
		"</Relationships>";
	static const char	WORKBOOK[] =
		"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\r\n"
		"<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/"
		"2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/"
		"officeDocument/2006/relationships\">"
		"<sheets><sheet name=\"PICCELL\" sheetId=\"1\" r:id=\"rId1\"/></sheets>"
		"</workbook>";
	static const char	WORKBOOK_RELS[] =
		"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\r\n"
		"<Relationships xmlns=\"http://schemas.openxmlformats.org/package/"
		"2006/relationships\">"
		"<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/"
		"officeDocument/2006/relationships/worksheet\" "
		"Target=\"worksheets/sheet1.xml\"/>"
		"<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/"
		"officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>"
		"</Relationships>";
	/* THE ORDER OF <cellXfs> IS THE ORDER OF THE STYLE_ NUMBERS */
	static const char	STYLES[] =
		"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\r\n"
		"<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/"
		"2006/main\">"
		"<fonts count=\"2\"><font><sz val=\"11\"/><name val=\"Calibri\"/>"
		"<family val=\"2\"/></font><font><b/><sz val=\"11\"/>"
		"<name val=\"Calibri\"/><family val=\"2\"/></font></fonts>"
		"<fills count=\"2\"><fill><patternFill patternType=\"none\"/></fill>"
		"<fill><patternFill patternType=\"gray125\"/></fill></fills>"
		"<borders count=\"1\"><border><left/><right/><top/><bottom/>"
		"<diagonal/></border></borders>"
		"<cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" "
		"fillId=\"0\" borderId=\"0\"/></cellStyleXfs>"
		"<cellXfs count=\"8\">"
		"<xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" "
		"xfId=\"0\"/>"
		"<xf numFmtId=\"14\" fontId=\"0\" fillId=\"0\" borderId=\"0\" "
		"xfId=\"0\" applyNumberFormat=\"1\"/>"
		"<xf numFmtId=\"9\" fontId=\"0\" fillId=\"0\" borderId=\"0\" "
		"xfId=\"0\" applyNumberFormat=\"1\"/>"
		"<xf numFmtId=\"10\" fontId=\"0\" fillId=\"0\" borderId=\"0\" "
		"xfId=\"0\" applyNumberFormat=\"1\"/>"
		"<xf numFmtId=\"3\" fontId=\"0\" fillId=\"0\" borderId=\"0\" "
		"xfId=\"0\" applyNumberFormat=\"1\"/>"
		"<xf numFmtId=\"4\" fontId=\"0\" fillId=\"0\" borderId=\"0\" "
		"xfId=\"0\" applyNumberFormat=\"1\"/>"
		"<xf numFmtId=\"2\" fontId=\"0\" fillId=\"0\" borderId=\"0\" "
		"xfId=\"0\" applyNumberFormat=\"1\"/>"
		"<xf numFmtId=\"0\" fontId=\"1\" fillId=\"0\" borderId=\"0\" "
		"xfId=\"0\" applyFont=\"1\"/>"
		"</cellXfs>"
		"<cellStyles count=\"1\"><cellStyle name=\"Normal\" xfId=\"0\" "
		"builtinId=\"0\"/></cellStyles>"
		"</styleSheet>";
	struct S_ZIP		Z;
	struct S_BUF		PART;
	struct tm			*NOW;
	time_t				SECONDS;

	memset(&Z, 0, sizeof(Z));
	SECONDS = time(NULL);
	NOW = localtime(&SECONDS);

	if (NOW && NOW->tm_year >= 80)
	{
		Z.TIME = (unsigned long)(
			(NOW->tm_hour << 11) | (NOW->tm_min << 5) | (NOW->tm_sec / 2)
		);
		Z.DATE = (unsigned long)(
			((NOW->tm_year - 80) << 9) | ((NOW->tm_mon + 1) << 5) | NOW->tm_mday
		);
	}

	memset(&PART, 0, sizeof(PART));
	PART.DATA = (char *)TYPES;
	PART.SIZE = sizeof(TYPES) - 1;
	ZIP_ADD(&Z, "[Content_Types].xml", &PART);
	PART.DATA = (char *)RELS;
	PART.SIZE = sizeof(RELS) - 1;
	ZIP_ADD(&Z, "_rels/.rels", &PART);
	PART.DATA = (char *)WORKBOOK;
	PART.SIZE = sizeof(WORKBOOK) - 1;
	ZIP_ADD(&Z, "xl/workbook.xml", &PART);
	PART.DATA = (char *)WORKBOOK_RELS;
	PART.SIZE = sizeof(WORKBOOK_RELS) - 1;
	ZIP_ADD(&Z, "xl/_rels/workbook.xml.rels", &PART);
	PART.DATA = (char *)STYLES;
	PART.SIZE = sizeof(STYLES) - 1;
	ZIP_ADD(&Z, "xl/styles.xml", &PART);
	memset(&PART, 0, sizeof(PART));
	SHEET_XML(&PART, T, TURKISH);
	ZIP_ADD(&Z, "xl/worksheets/sheet1.xml", &PART);
	free(PART.DATA);
	return (ZIP_END(&Z, OUTPUT));
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                           XLSX                           |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                           PDF                            |  \\//  */
/*   \/   #==========================================================#   \/   */
/*
# ONE UTF-8 CHARACTER TO WINDOWS-1254 ("?" WHEN IT HAS NONE). RETURNS THE
# BYTES READ.
*/
static int
	TO_1254(const char *TEXT, unsigned char *DEST)
{
	static const unsigned short	HIGH[32] = {
		0X20AC, 0, 0X201A, 0X0192, 0X201E, 0X2026, 0X2020, 0X2021,
		0X02C6, 0X2030, 0X0160, 0X2039, 0X0152, 0, 0, 0,
		0, 0X2018, 0X2019, 0X201C, 0X201D, 0X2022, 0X2013, 0X2014,
		0X02DC, 0X2122, 0X0161, 0X203A, 0X0153, 0, 0, 0X0178
	};
	const unsigned char			*P = (const unsigned char *)TEXT;
	unsigned long				CODE;
	int							LEN;
	int							I;

	if (P[0] < 0X80)
	{
		*DEST = P[0];
		return (1);
	}

	LEN = 1;
	CODE = P[0];

	if ((P[0] & 0XE0) == 0XC0 && (P[1] & 0XC0) == 0X80)
	{
		CODE = ((P[0] & 0X1FUL) << 6) | (P[1] & 0X3F);
		LEN = 2;
	}
	else if (
		(P[0] & 0XF0) == 0XE0 &&
		(P[1] & 0XC0) == 0X80 &&
		(P[2] & 0XC0) == 0X80
	)
	{
		CODE = ((P[0] & 0X0FUL) << 12) | ((P[1] & 0X3FUL) << 6) | (P[2] & 0X3F);
		LEN = 3;
	}
	else if ((P[0] & 0XF8) == 0XF0)
	{
		for (LEN = 1; LEN < 4 && (P[LEN] & 0XC0) == 0X80; ++LEN)
		{}

		*DEST = '?';
		return (LEN);
	}

	*DEST = '?';

	/* THE SIX TURKISH LETTERS TAKE THE PLACES OF Ð Ý Þ ð ý þ */
	if (CODE == 0X011E)
		*DEST = 0XD0;
	else if (CODE == 0X0130)
		*DEST = 0XDD;
	else if (CODE == 0X015E)
		*DEST = 0XDE;
	else if (CODE == 0X011F)
		*DEST = 0XF0;
	else if (CODE == 0X0131)
		*DEST = 0XFD;
	else if (CODE == 0X015F)
		*DEST = 0XFE;
	else if (
		CODE >= 0XA0 &&
		CODE <= 0XFF &&
		CODE != 0XD0 &&
		CODE != 0XDD &&
		CODE != 0XDE &&
		CODE != 0XF0 &&
		CODE != 0XFD &&
		CODE != 0XFE
	)
		*DEST = (unsigned char)CODE;

	for (I = 0; I < 32 && *DEST == '?' && CODE >= 0X100; ++I)
		if (HIGH[I] == CODE)
			*DEST = (unsigned char)(0X80 + I);

	return (LEN);
}

/* THE WIDTH OF A UTF-8 TEXT IN POINTS, AT LETTER SIZE SIZE (BOLD: 1) */
static double
	TEXT_WIDTH(const char *TEXT, double SIZE, int BOLD)
{
	long	SUM;

	SUM = 0;

	while (*TEXT)
	{
		unsigned char	B;

		TEXT += TO_1254(TEXT, &B);

		if (B >= 32 && BOLD)
			SUM += G__BOLD__[B - 32];
		else if (B >= 32)
			SUM += G__WIDTH__[B - 32];
	}

	return ((double)SUM * SIZE / 1000.0);
}

/* A PDF STRING: ( ) AND \ ESCAPED, BYTES ABOVE 126 AS \OCTAL */
static void
	PUT_PDF_TEXT(struct S_BUF *B, const char *TEXT)
{
	PUT(B, "(", 1);

	while (*TEXT)
	{
		unsigned char	C;

		TEXT += TO_1254(TEXT, &C);

		if (C == '(' || C == ')' || C == '\\')
			PUTF(B, "\\%c", C);
		else if (C < 32 || C > 126)
			PUTF(B, "\\%03o", C);
		else
			PUT(B, &C, 1);
	}

	PUT(B, ")", 1);
}

/* THE PAGES OF THE TABLE AND WHAT ONE PAGE NEEDS TO DRAW ITSELF */
struct	S_PDF
{
	const struct S_TABLE	*T;
	int						TURKISH;
	double					WIDTH_PT;
	double					HEIGHT_PT;
	double					SIZE; /* LETTER SIZE */
	double					ROW_H;
	double					*COL_W;
	int						HEADER; /* 1 = ROW 0 IS ON TOP OF EVERY PAGE */
	int						ROWS_PER_PAGE;
	int						PAGES;
};

/* COLUMN WIDTHS, LETTER SIZE AND PAGE SIZE (A4, ACROSS WHEN IT IS WIDE) */
static int
	PDF_LAYOUT(struct S_PDF *P)
{
	double	NATURAL;
	double	ROOM;
	int		C;

	P->COL_W = (double *)calloc((size_t)P->T->COLS + 1, sizeof(double));

	if (!P->COL_W)
		return (0);

	NATURAL = 0.0;

	for (C = 0; C < P->T->COLS; ++C)
	{
		int	R;

		P->COL_W[C] = 18.0;

		for (R = 0; R < P->T->ROWS; ++R)
		{
			const char	*CELL = P->T->CELL[R * P->T->COLS + C];
			double		W;

			if (!CELL)
				continue ;

			W = TEXT_WIDTH(CELL, PDF_SIZE, P->HEADER && !R) + 2.0 * PDF_PAD;

			if (W > P->COL_W[C])
				P->COL_W[C] = W;
		}

		NATURAL += P->COL_W[C];
	}

	P->WIDTH_PT = 595.0;
	P->HEIGHT_PT = 842.0;

	if (NATURAL > P->WIDTH_PT - 2.0 * PDF_MARGIN)
	{
		P->WIDTH_PT = 842.0;
		P->HEIGHT_PT = 595.0;
	}

	ROOM = P->WIDTH_PT - 2.0 * PDF_MARGIN;
	P->SIZE = PDF_SIZE;

	/* TOO WIDE: SMALLER LETTERS FIRST, THEN THE WIDEST COLUMNS ARE CUT */
	if (NATURAL > ROOM)
	{
		double	F;

		F = ROOM / NATURAL;

		if (F < PDF_SIZE_MIN / PDF_SIZE)
			F = PDF_SIZE_MIN / PDF_SIZE;

		P->SIZE = PDF_SIZE * F;

		for (C = 0; C < P->T->COLS; ++C)
			P->COL_W[C] *= F;

		NATURAL *= F;
	}

	if (NATURAL > ROOM)
		for (C = 0; C < P->T->COLS; ++C)
			P->COL_W[C] *= ROOM / NATURAL;

	P->ROW_H = P->SIZE * 1.7;
	P->ROWS_PER_PAGE = (int)((P->HEIGHT_PT - 2.0 * PDF_MARGIN) / P->ROW_H);
	P->ROWS_PER_PAGE -= P->HEADER;

	if (P->ROWS_PER_PAGE < 1)
		P->ROWS_PER_PAGE = 1;

	P->PAGES = (P->T->ROWS - P->HEADER + P->ROWS_PER_PAGE - 1);
	P->PAGES /= P->ROWS_PER_PAGE;

	if (P->PAGES < 1)
		P->PAGES = 1;

	return (1);
}

/* THE TEXT OF ONE ROW OF THE TABLE ON LINE LINE OF THE PAGE */
static void
	PDF_ROW(const struct S_PDF *P, int ROW, int LINE, struct S_BUF *B)
{
	const double	TOP = P->HEIGHT_PT - PDF_MARGIN;
	const double	BASE = TOP - (LINE + 1) * P->ROW_H + P->ROW_H * 0.32;
	const int		BOLD = P->HEADER && !ROW;
	double			X;
	int				C;

	X = PDF_MARGIN;

	for (C = 0; C < P->T->COLS; ++C)
	{
		const char	*CELL = P->T->CELL[ROW * P->T->COLS + C];
		double		AT;
		double		NUMBER;

		if (CELL && CELL[0])
		{
			AT = X + PDF_PAD;

			if (!BOLD && EXPORT_NUMBER(CELL, P->TURKISH, &NUMBER))
				AT = X + P->COL_W[C] - PDF_PAD - TEXT_WIDTH(CELL, P->SIZE, 0);

			/* A TEXT TOO LONG FOR ITS CELL IS CUT AT THE BORDER */
			PUTF(
				B,
				"q %.2f %.2f %.2f %.2f re W n BT /F%d %.2f Tf %.2f %.2f Td ",
				X + 0.5,
				BASE - P->ROW_H * 0.32,
				P->COL_W[C] - 1.0,
				P->ROW_H,
				1 + BOLD,
				P->SIZE,
				AT,
				BASE
			);
			PUT_PDF_TEXT(B, CELL);
			PUTS(B, " Tj ET Q\n");
		}

		X += P->COL_W[C];
	}
}

/*
# THE DRAWING OF ONE PAGE: THE HEADER ROW (GRAY), EVERY OTHER ROW A LITTLE
# GRAY, A LIGHT GRID, AND THE TEXT, NUMBERS TO THE RIGHT.
*/
static void
	PDF_PAGE(const struct S_PDF *P, int PAGE, struct S_BUF *B)
{
	const int		FIRST = P->HEADER + PAGE * P->ROWS_PER_PAGE;
	const double	TOP = P->HEIGHT_PT - PDF_MARGIN;
	double			TABLE_W;
	double			X;
	int				LINES;
	int				LAST;
	int				R;
	int				C;

	LAST = FIRST + P->ROWS_PER_PAGE;

	if (LAST > P->T->ROWS)
		LAST = P->T->ROWS;

	LINES = P->HEADER + LAST - FIRST;
	TABLE_W = 0.0;

	for (C = 0; C < P->T->COLS; ++C)
		TABLE_W += P->COL_W[C];

	if (P->HEADER)
		PUTF(
			B,
			"0.85 g %.2f %.2f %.2f %.2f re f\n",
			PDF_MARGIN,
			TOP - P->ROW_H,
			TABLE_W,
			P->ROW_H
		);

	PUTS(B, "0.95 g\n");

	for (R = P->HEADER + 1; R < LINES; R += 2)
		PUTF(
			B,
			"%.2f %.2f %.2f %.2f re\n",
			PDF_MARGIN,
			TOP - (R + 1) * P->ROW_H,
			TABLE_W,
			P->ROW_H
		);

	PUTS(B, "f\n0.6 G 0.5 w\n");

	for (R = 0; R <= LINES; ++R)
		PUTF(
			B,
			"%.2f %.2f m %.2f %.2f l\n",
			PDF_MARGIN,
			TOP - R * P->ROW_H,
			PDF_MARGIN + TABLE_W,
			TOP - R * P->ROW_H
		);

	X = PDF_MARGIN;

	for (C = 0; C <= P->T->COLS; ++C)
	{
		PUTF(
			B,
			"%.2f %.2f m %.2f %.2f l\n",
			X,
			TOP,
			X,
			TOP - LINES * P->ROW_H
		);

		if (C < P->T->COLS)
			X += P->COL_W[C];
	}

	PUTS(B, "S\n0 g\n");

	if (P->HEADER)
		PDF_ROW(P, 0, 0, B);

	for (R = FIRST; R < LAST; ++R)
		PDF_ROW(P, R, P->HEADER + R - FIRST, B);

	/* THE PAGE NUMBER WHEN THERE ARE SEVERAL */
	if (P->PAGES > 1)
		PUTF(
			B,
			"BT /F1 8 Tf %.2f %.2f Td (%d / %d) Tj ET\n",
			P->WIDTH_PT / 2.0 - 10.0,
			PDF_MARGIN / 2.0,
			PAGE + 1,
			P->PAGES
		);
}

/* ONE OF THE TWO FONTS (OBJECT 3 HELVETICA, 4 HELVETICA-BOLD) */
static void
	PDF_FONT(struct S_BUF *B, int BOLD)
{
	const short	*WIDTHS = G__WIDTH__;
	int			I;

	if (BOLD)
		WIDTHS = G__BOLD__;

	PUTF(B, "%d 0 obj\n<< /Type /Font /Subtype /Type1 ", 3 + BOLD);

	if (BOLD)
		PUTS(B, "/BaseFont /Helvetica-Bold ");
	else
		PUTS(B, "/BaseFont /Helvetica ");

	PUTS(
		B,
		"/Encoding << /Type /Encoding /BaseEncoding /WinAnsiEncoding "
		"/Differences [ 208 /Gbreve 221 /Idotaccent /Scedilla 240 /gbreve "
		"253 /dotlessi /scedilla ] >>\n/FirstChar 32 /LastChar 255 /Widths ["
	);

	for (I = 0; I < 224; ++I)
	{
		if (I % 16)
			PUTF(B, " %d", WIDTHS[I]);
		else
			PUTF(B, "\n%d", WIDTHS[I]);
	}

	PUTS(B, " ] >>\nendobj\n");
}

/*
# OBJECTS: 1 CATALOG, 2 PAGES, 3 AND 4 FONTS, THEN A PAGE AND ITS DRAWING
# FOR EVERY PAGE (5 + 2 * I AND 6 + 2 * I).
*/
static int
	WRITE_PDF(const struct S_TABLE *T, int TURKISH, FILE *OUTPUT)
{
	struct S_PDF	P;
	struct S_BUF	B;
	struct S_BUF	DRAW;
	long			*AT;
	long			XREF;
	int				OBJECTS;
	int				I;

	memset(&P, 0, sizeof(P));
	memset(&B, 0, sizeof(B));
	P.T = T;
	P.TURKISH = TURKISH;
	P.HEADER = HAS_HEADER(T, TURKISH);

	if (!PDF_LAYOUT(&P))
		return (0);

	OBJECTS = 4 + 2 * P.PAGES;
	AT = (long *)calloc((size_t)OBJECTS + 1, sizeof(long));

	if (!AT)
	{
		free(P.COL_W);
		return (0);
	}

	PUTS(&B, "%PDF-1.4\n%\xE2\xE3\xCF\xD3\n");
	AT[1] = (long)B.SIZE;
	PUTS(&B, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");
	AT[2] = (long)B.SIZE;
	PUTS(&B, "2 0 obj\n<< /Type /Pages /Kids [");

	for (I = 0; I < P.PAGES; ++I)
		PUTF(&B, " %d 0 R", 5 + 2 * I);

	PUTF(&B, " ] /Count %d >>\nendobj\n", P.PAGES);
	AT[3] = (long)B.SIZE;
	PDF_FONT(&B, 0);
	AT[4] = (long)B.SIZE;
	PDF_FONT(&B, 1);

	for (I = 0; I < P.PAGES; ++I)
	{
		AT[5 + 2 * I] = (long)B.SIZE;
		PUTF(
			&B,
			"%d 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 %.0f %.0f] "
			"/Resources << /Font << /F1 3 0 R /F2 4 0 R >> >> "
			"/Contents %d 0 R >>\nendobj\n",
			5 + 2 * I,
			P.WIDTH_PT,
			P.HEIGHT_PT,
			6 + 2 * I
		);
		memset(&DRAW, 0, sizeof(DRAW));
		PDF_PAGE(&P, I, &DRAW);
		AT[6 + 2 * I] = (long)B.SIZE;
		PUTF(
			&B,
			"%d 0 obj\n<< /Length %lu >>\nstream\n",
			6 + 2 * I,
			(unsigned long)DRAW.SIZE
		);
		PUT(&B, DRAW.DATA, DRAW.SIZE);

		if (DRAW.FAILED)
			B.FAILED = 1;

		free(DRAW.DATA);
		PUTS(&B, "\nendstream\nendobj\n");
	}

	XREF = (long)B.SIZE;
	PUTF(&B, "xref\n0 %d\n0000000000 65535 f \n", OBJECTS + 1);

	for (I = 1; I <= OBJECTS; ++I)
		PUTF(&B, "%010ld 00000 n \n", AT[I]);

	PUTF(
		&B,
		"trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%ld\n%%%%EOF\n",
		OBJECTS + 1,
		XREF
	);
	free(AT);
	free(P.COL_W);
	return (FLUSH(&B, OUTPUT));
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                           PDF                            |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                        PUBLIC API                        |  \\//  */
/*   \/   #==========================================================#   \/   */
int
	EXPORT_KIND(const char *NAME)
{
	const char	*DOT = strrchr(NAME, '.');
	char		EXT[8];
	int			I;

	if (!DOT || strlen(DOT + 1) >= sizeof(EXT))
		return (EXPORT_TSV);

	for (I = 0; DOT[1 + I]; ++I)
	{
		EXT[I] = DOT[1 + I];

		if (EXT[I] >= 'A' && EXT[I] <= 'Z')
			EXT[I] = (char)(EXT[I] - 'A' + 'a');
	}

	EXT[I] = '\0';

	if (strcmp(EXT, "csv") == 0)
		return (EXPORT_CSV);

	if (strcmp(EXT, "xlsx") == 0)
		return (EXPORT_XLSX);

	if (strcmp(EXT, "pdf") == 0)
		return (EXPORT_PDF);

	return (EXPORT_TSV);
}

int
	EXPORT_WRITE(
		const struct S_TABLE *TABLE,
		int KIND,
		int TURKISH,
		FILE *OUTPUT
	)
{
	switch (KIND)
	{
		case (EXPORT_CSV):
		{}
		return (WRITE_CSV(TABLE, TURKISH, OUTPUT));
		case (EXPORT_XLSX):
		{}
		return (WRITE_XLSX(TABLE, TURKISH, OUTPUT));
		case (EXPORT_PDF):
		{}
		return (WRITE_PDF(TABLE, TURKISH, OUTPUT));
		default:
		{}
		return (LAYOUT_WRITE(TABLE, OUTPUT));
	}
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                        PUBLIC API                        |  //\\  */
/* '""""' #==========================================================# '""""' */
