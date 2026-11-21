/******************************************************************************\
# C - hand                                       #       Maximum Tension       #
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
#include <math.h>
/******* """"""""
 * float	expf(float);
 * float	fabsf(float);
 * float	floorf(float);
 * float	fmaxf(float, float);
 * double	fmin(double, double);
 * float	fminf(float, float);
 * double	log(double);
 * float	log1pf(float);
 */
#include <stdio.h>
/******* """""""""
 * int	snprintf(char *, size_t, const char *, ...);
 */
#include <stdlib.h>
/******* """"""""""
 * void	*calloc(size_t, size_t);
 * void	free(void *);
 * void	*malloc(size_t);
 * void	qsort(void *, size_t, size_t, int (*)(const void *, const void *));
 * void	*realloc(void *, size_t);
 */
#include <string.h>
/******* """"""""""
 * #define NULL
 * typedef	size_t;
 * void		*memcpy(void *, const void *, size_t);
 * void		*memmove(void *, const void *, size_t);
 * void		*memset(void *, int, size_t);
 * char		*strchr(const char *, int);
 * int		strcmp(const char *, const char *);
 * char		*strcpy(char *, const char *);
 * size_t	strcspn(const char *, const char *);
 * size_t	strlen(const char *);
 * int		strncmp(const char *, const char *, size_t);
 * char		*strpbrk(const char *, const char *);
 * char		*strstr(const char *, const char *);
 */
#include "piccell.h"
/******* """""""""
 * struct S_WORD;
 */
#include "layout.h"
/******* """"""""""
 * struct S_TABLE;
 * int		LAYOUT_APPEND(struct S_TABLE *, struct S_TABLE *);
 * int		LAYOUT_BUILD(const struct S_WORD *, int, struct S_TABLE *);
 * void	LAYOUT_FREE(struct S_TABLE *);
 * int		LAYOUT_GRID(
 * 	const struct S_WORD *,
 * 	const int *,
 * 	int,
 * 	const float *,
 * 	int,
 * 	struct S_TABLE *
 * );
 */
#include "page.h"
/******* """"""""
 * struct S_LINE;
 * void	PAGE_FREE(struct S_LINE *, int);
 * int	PAGE_LETTER_HEIGHT(void);
 * int	PAGE_LINES(const unsigned char *, int, int, struct S_LINE **);
 * int	PAGE_RULES(const float **);
 * void	PAGE_TURN(float *, float *);
 */
#include "reader.h"
/******* """"""""""
 * struct S_READER;
 * const char	*READER_CHAR(const struct S_READER *, int);
 * int			READER_CLASSES(const struct S_READER *);
 * float		*READER_RUN(const struct S_READER *, const float *, int, int *);
 */
#include "ctc.h"
/******* """""""
 * struct S_GLYPH;
 * struct S_GRAMMAR;
 * double	CTC_GRAMMAR(
 * 	const float *,
 * 	int,
 * 	int,
 * 	int,
 * 	const struct S_GRAMMAR *,
 * 	struct S_GLYPH *,
 * 	int *,
 * 	int,
 * 	int *
 * );
 * int		CTC_GREEDY(
 * 	const float *,
 * 	int,
 * 	int,
 * 	int,
 * 	const unsigned char *,
 * 	struct S_GLYPH *,
 * 	int
 * );
 * double	CTC_SCORE(const float *, int, int, int, const int *, int);
 */
#include "words.h"
/******* """""""""
 * const char *const	WORDS_BUILT_IN[];
 * const int			WORDS_BUILT_IN_COUNT;
 */
#include "hand.h"
/******* """"""""
 * struct S_HAND;
 */
/* **************************** [^] INCLUDES [^] **************************** */

#ifdef _WIN32 /* THE LINES ARE READ ON EVERY PROCESSOR AT THE SAME TIME */
#include <windows.h>
/******* """""""""""
 * #define INFINITE
 * #define InterlockedIncrement
 * #define WINAPI
 * typedef	DWORD;
 * typedef	HANDLE;
 * typedef	LONG;
 * typedef	LPVOID;
 * typedef	SYSTEM_INFO;
 * BOOL		CloseHandle(HANDLE);
 * HANDLE	CreateThread(
 * 	LPSECURITY_ATTRIBUTES,
 * 	SIZE_T,
 * 	LPTHREAD_START_ROUTINE,
 * 	LPVOID,
 * 	DWORD,
 * 	LPDWORD
 * );
 * void		GetSystemInfo(LPSYSTEM_INFO);
 * DWORD	WaitForSingleObject(HANDLE, DWORD);
 */
#endif /* _WIN32 */

#define CELL_MAX 128 /* BYTES OF ONE CELL */
#define FIELDS_MAX 8 /* COLUMNS OF A LIST LINE (THE GRAMMAR FITS 64 NODES) */
#define COLS_MAX 16 /* COLUMNS OF THE TABLE OF A LIST */
#define GLYPHS_MAX 512 /* GLYPHS OF ONE LINE */
#define CLASSES_MAX 256
#define PAGE_SIDE_MAX 2400 /* BIGGER PHOTOS ARE MADE SMALLER FIRST */
#define LETTER_MAX 30 /* LETTERS TALLER THAN THIS: THE PAGE IS MADE SMALLER */
#define LETTER_GOAL 20
#define LETTER_LEAST 9 /* LETTERS SMALLER THAN THIS: THE PAGE IS MADE BIGGER */
#define LIST_COST 9.0 /* A LIST LINE: ITS DASHES COST LESS THAN THIS */
#define LINE_MARGIN 3.0 /* FEWER FIELDS WIN WHEN THEY FIT THIS MUCH BETTER */
#define NEIGHBOUR_PRIOR 4.0 /* THE SAME CODE AS THE LINE ABOVE OR BELOW */
#define WHOLE_SHARE 0.7F /* 70% WHOLE AMOUNTS: A "." IN ONE IS A SPECK */
#define CODE_CHOICES_MAX 16 /* THE MOST CODES ONE CELL MAY TAKE */
#define CODE_DOUBT 2.0 /* A CODE THAT FITS THIS MUCH WORSE THAN READ: MARKED */
#define SAME_HAND_REACH 2 /* A COLUMN WORD TAKES A LONE READING THIS CLOSE */
#define UPPER_DOUBT 1.0 /* ALL CAPITALS NEED THIS MUCH BETTER INK */
#define CASE_CAP 3.0 /* THE MOST ONE CELL LOSES FOR A CASE (CHOOSE_CASE_OF) */
#define DATE_COST 6.0 /* A TITLE IS READ AS A DATE WHEN IT COSTS THIS LITTLE */
#define DASH_DOUBT 0.5 /* A "-" THE PAGE FOUND COSTS AT MOST THIS */
#define DASH_TOLERANCE 1.0 /* A LINE VOTES FOR A DASH THAT COSTS LESS */
#define PRIOR 1.5 /* WEIGHT OF "THIS WORD COMES AGAIN AND AGAIN" */
#define CODE_PRIOR 0.7 /* THE SAME FOR CODES */
#define WORD_BONUS 4.5 /* WEIGHT OF "THIS IS A KNOWN WORD" */
#define CASE_BONUS 0.5 /* A KNOWN WORD AS IT IS KNOWN ("TL") */
#define PREFIX_COST -8.0F /* A WORD BEFORE A TOTAL IS RARE */
#define OTHER_UNIT_COST -4.0F /* A UNIT THAT IS NOT IN THE LIST */
#define OTHER_LETTER_COST -1.5F /* ... AND EVERY LETTER OF IT */
#define TOTAL_FIT 12.0 /* A LINE WITHOUT DIGITS MAY BE THE TOTAL FOR THIS */
#define STRICT_TOTAL 15.0 /* A TOTAL WITH A UNIT OF THE LIST MAY COST MORE */
#define STRICT_ROW 10.0 /* THE SAME FOR A LINE OF THE LIST */
#define LETTER_COST 3.0 /* A DIGIT CODE MAY COST THIS MUCH PER LETTER */
#define UNITS_MAX 3
#define UNIT_LETTERS_MAX 24
#define SUM_LOSS 12.0 /* A SUM FIX MAY COST AT MOST THIS MUCH FIT */
#define TOTAL_TRUST 5.0 /* CHANGING THE TOTAL COSTS THIS MUCH MORE */
#define CHECK_LOG -0.7 /* A CELL LESS LIKELY THAN 1 IN 2 IS MARKED */
#define CHECK_LOG_NUMBER -2.5 /* THE SAME FOR CODES AND NUMBERS */
#define SHAPE_PAIRED 1 /* A COLUMN OF CODES THAT ARE A LETTER AND A DIGIT */
#define SHAPE_LEAD 2 /* A COLUMN OF CODES THAT START WITH A LETTER */
#define SHAPE_TWO_HOURS 4 /* A COLUMN OF TIMES LIKE "08:30" */
#define TIME_FIT 8.0 /* A CELL OF A COLUMN OF TIMES: AT MOST THIS WORSE */
#define DIGIT_FIT 6.0 /* A CELL OF A COLUMN OF DIGITS: THIS FOR EVERY GLYPH */
#define ROW_LINES_MAX 128 /* THE MOST ROWS OF A DRAWN TABLE */
#define EDGES_MAX 32 /* THE MOST COLUMNS OF A DRAWN TABLE, LESS ONE */
#define BLOCK_GAP 2.0F /* A GAP OF 2 LINES PARTS THE PAGE (TWO TABLES) */
#define CELL_GAP 10 /* A GAP OF 10 STEPS (1.25 LETTERS) PARTS TWO CELLS */
#define NUMBER_GAP 6 /* STEPS: A SHORTER SPACE IN A NUMBER IS NO SPACE */
#define WORD_MARGIN 2.0 /* A WORD CHOSEN BY LESS THAN THIS IS MARKED */
#define USER_WORDS_MAX 4096
#define JUDGE_LINES 12 /* LINES READ TO JUDGE WHAT THE OCR SAW */
#define JUDGE_WORST 10.0 /* THE MOST A CHARACTER OF THE OCR MAY COST */

#define KIND_FREE 0 /* ANYTHING BUT A DASH */
#define KIND_TEXT 1
#define KIND_CODE 2 /* DIGITS AND LETTERS, NO SPACES */
#define KIND_NUMBER 3 /* DIGITS, . AND , */
#define KIND_NUMBER_UNIT 4 /* "12 TOP": A NUMBER, THEN A WORD */
#define KIND_NUMBER_KNOWN_UNIT 5 /* "12 TOP" WITH A UNIT OF THE LIST */
#define KIND_TIME 6 /* "11:30" (A COLUMN OF A DRAWN TABLE) */

/* THE TAG OF A NODE: FIELD * 4 + PART */
#define PART_SPACE 0 /* SPACES AND DASHES: IN NO CELL */
#define PART_MAIN 1
#define PART_UNIT 2
#define PART_PREFIX 3 /* "TOTAL:" BEFORE A TOTAL */

struct	S_READ
{
	struct S_LINE	*LINE;
	float			*LOGP;
	int				STEPS;
	double			BEST; /* SCORE OF THE BEST CLASS AT EVERY STEP */
	double			COST[FIELDS_MAX]; /* COST OF A LIST LINE OF K + 1 FIELDS */
	int				COUNT;
	struct S_GLYPH	GLYPH[GLYPHS_MAX];
	int				TAG[GLYPHS_MAX]; /* -1 = NO GRAMMAR */
	int				FIELDS; /* 0 = NOT A LIST LINE */
	int				TOTAL; /* 1 = THE TOTAL UNDER A LIST */
	int				DROP; /* 1 = NO TEXT ON IT */
	int				DONE; /* 1 = READ */
	int				KINDS[FIELDS_MAX]; /* OF ITS FIELDS, AS LAST READ */
	double			SCORE; /* OF THAT READING */
};

/* A PAGE: ITS LINES, AND WHAT THE READER SAW ON THEM */
struct	S_HAND
{
	struct S_LINE	*LINES;
	struct S_READ	*READS;
	int				COUNT;
	float			SCALE; /* PICTURE PIXELS PER PIXEL OF THE LINES */
};

struct	S_CELL
{
	int		LINE; /* -1 = EMPTY */
	int		FROM; /* STEPS OF THE LINE */
	int		TO;
	int		CHECK; /* 1 = NOT SURE */
	char	TEXT[CELL_MAX];
};

/* ONE CLASS LIST PER LETTER OF A UNIT: THE LETTER IN BOTH CASES */
struct	S_UNITS
{
	int				COUNT;
	const char		*WORD[UNITS_MAX];
	int				LETTERS[UNITS_MAX];
	unsigned char	MASK[UNIT_LETTERS_MAX][CLASSES_MAX];
};

/* THE BEST GUESS FOR A CELL SO FAR (SEE TRY_GUESS) */
struct	S_GUESS
{
	double	SCORE;
	double	SECOND; /* THE BEST SCORE OF ANOTHER WORD */
	char	TEXT[CELL_MAX];
	char	BARE[CELL_MAX];
};

/* A KNOWN WORD, AND THE SAME WITHOUT CASE AND SPACES (SEE BARE) */
struct	S_KNOWN
{
	const char	*WORD;
	char		BARE[CELL_MAX];
};

/* THE WORDS OF A COLUMN (OR A PAGE) AND HOW MANY TIMES EACH CAME */
struct	S_VOCAB
{
	int		COUNT;
	char	(*WORD)[CELL_MAX];
	int		*HITS;
	int		*CLOSE; /* A KNOWN WORD ONE LETTER AWAY (G__KNOWN__), -1 = NONE */
};

static struct S_READER	*G__READER__;
static int				G__CLASSES__;
static int				G__DASH__; /* THE CLASS OF "-" */
static int				G__SPACE__;
static struct S_KNOWN	*G__KNOWN__; /* BUILT IN, THEN ADDED */
static int				G__KNOWN_COUNT__;
static int				G__KNOWN_ROOM__;
/* ONE BYTE PER CLASS: 1 = ALLOWED */
static unsigned char	G__NOT_DASH__[CLASSES_MAX];
static unsigned char	G__TEXT__[CLASSES_MAX];
static unsigned char	G__ALNUM__[CLASSES_MAX];
static unsigned char	G__DIGIT__[CLASSES_MAX];
static unsigned char	G__NUMBER__[CLASSES_MAX];
static unsigned char	G__LETTER__[CLASSES_MAX];
static unsigned char	G__SPACE_ONLY__[CLASSES_MAX];
static unsigned char	G__DASH_ONLY__[CLASSES_MAX];
static unsigned char	G__DATE_MARK__[CLASSES_MAX]; /* "." "/" "-" */
static unsigned char	G__TIME_MARK__[CLASSES_MAX]; /* ":" "." */
/* THE UNITS OF THE LIST BEING READ (KIND_NUMBER_KNOWN_UNIT) */
static const struct S_UNITS	*G__UNITS__;
/* THE SAME LETTER IN THE OTHER CASE ("a" > "A"), 0 = NONE */
static int				G__TWIN__[CLASSES_MAX];

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                       SMALL THINGS                       |  \\//  */
/*   \/   #==========================================================#   \/   */
static int
	IS_DIGIT_CLASS(int K)
{
	const char	*C = READER_CHAR(G__READER__, K);

	return (C[0] >= '0' && C[0] <= '9' && !C[1]);
}

static int
	IS_LETTER_CLASS(int K)
{
	const unsigned char	*C;

	C = (const unsigned char *)READER_CHAR(G__READER__, K);
	return (
		(C[0] >= 'A' && C[0] <= 'Z') ||
		(C[0] >= 'a' && C[0] <= 'z') ||
		C[0] >= 0XC0
	);
}

static int	CLASS_OF(const char *TEXT, int *LEN);
static int	CHANGE_CASE(const char *TEXT, int UP, int DOTTED, char *DEST);
static int	MAKE_VOCAB(struct S_VOCAB *V, struct S_CELL *const *CELLS, int N);
static void	FREE_VOCAB(struct S_VOCAB *V);
static double	LOOSE_FIT(
	const struct S_READ *R,
	const struct S_CELL *C,
	const char *TEXT,
	int NO_SPACE
);

/* THE CLASS LISTS (MASKS) OF THE GRAMMARS, AND THE TWINS */
static void
	MAKE_MASKS(void)
{
	int	K;

	memset(G__NOT_DASH__, 0, sizeof(G__NOT_DASH__));
	memset(G__TEXT__, 0, sizeof(G__TEXT__));
	memset(G__ALNUM__, 0, sizeof(G__ALNUM__));
	memset(G__DIGIT__, 0, sizeof(G__DIGIT__));
	memset(G__NUMBER__, 0, sizeof(G__NUMBER__));
	memset(G__LETTER__, 0, sizeof(G__LETTER__));
	memset(G__SPACE_ONLY__, 0, sizeof(G__SPACE_ONLY__));
	memset(G__DASH_ONLY__, 0, sizeof(G__DASH_ONLY__));
	memset(G__DATE_MARK__, 0, sizeof(G__DATE_MARK__));
	memset(G__TIME_MARK__, 0, sizeof(G__TIME_MARK__));

	for (K = 1; K < G__CLASSES__; ++K)
	{
		const char	*C = READER_CHAR(G__READER__, K);
		const int	DIGIT = IS_DIGIT_CLASS(K);
		const int	LETTER = IS_LETTER_CLASS(K);

		G__NOT_DASH__[K] = K != G__DASH__;
		G__DIGIT__[K] = (unsigned char)DIGIT;
		G__LETTER__[K] = (unsigned char)LETTER;
		G__ALNUM__[K] = (unsigned char)(DIGIT || LETTER);
		G__NUMBER__[K] = DIGIT || strcmp(C, ".") == 0 || strcmp(C, ",") == 0;
		G__SPACE_ONLY__[K] = K == G__SPACE__;
		G__DASH_ONLY__[K] = K == G__DASH__;
		G__DATE_MARK__[K] = C[0] && !C[1] && strchr("./-", C[0]) != NULL;
		G__TIME_MARK__[K] = C[0] && !C[1] && strchr(":.", C[0]) != NULL;
		G__TEXT__[K] = LETTER || (C[0] && strchr(" .,'/&()+\"", C[0]));
		G__TWIN__[K] = 0;

		if (LETTER)
		{
			char	OTHER[4];
			int		LEN;
			int		TWIN;

			CHANGE_CASE(C, 1, 0, OTHER);

			if (strcmp(OTHER, C) == 0)
				CHANGE_CASE(C, 0, 0, OTHER);

			TWIN = CLASS_OF(OTHER, &LEN);

			if (TWIN != K && LEN == (int)strlen(OTHER))
				G__TWIN__[K] = TWIN;
		}
	}
}

void
	HAND_SET_READER(struct S_READER *READER)
{
	int	K;

	G__READER__ = READER;
	G__CLASSES__ = 0;
	G__DASH__ = -1;
	G__SPACE__ = -1;

	if (!READER || READER_CLASSES(READER) > CLASSES_MAX)
		return ;

	G__CLASSES__ = READER_CLASSES(READER);

	for (K = 1; K < G__CLASSES__; ++K)
	{
		if (strcmp(READER_CHAR(READER, K), "-") == 0)
			G__DASH__ = K;

		if (strcmp(READER_CHAR(READER, K), " ") == 0)
			G__SPACE__ = K;
	}

	MAKE_MASKS();
}

/* THE CLASS OF THE UTF-8 CHARACTER AT *TEXT (AND ITS LENGTH), 0 = UNKNOWN */
static int
	CLASS_OF(const char *TEXT, int *LEN)
{
	int	K;

	for (K = 1; K < G__CLASSES__; ++K)
	{
		const char		*C = READER_CHAR(G__READER__, K);
		const size_t	N = strlen(C);

		if (N && strncmp(TEXT, C, N) == 0)
		{
			*LEN = (int)N;
			return (K);
		}
	}

	*LEN = 1;
	return (0);
}

/* TEXT TO CLASSES. RETURNS THEIR NUMBER; -1 WHEN A CHARACTER IS UNKNOWN */
static int
	TO_CLASSES(const char *TEXT, int *DEST, int MOST)
{
	int	COUNT;

	COUNT = 0;

	while (*TEXT && COUNT < MOST)
	{
		int	LEN;
		int	K;

		K = CLASS_OF(TEXT, &LEN);

		if (!K)
			return (-1);

		DEST[COUNT++] = K;
		TEXT += LEN;
	}

	return (COUNT);
}

/* THE TEXT OF GLYPHS FROM ... TO (BOTH INCLUDED), SPACES AT THE ENDS DROPPED */
static void
	GLYPH_TEXT(
		const struct S_GLYPH *G,
		int FROM,
		int TO,
		char *DEST,
		size_t SIZE
	)
{
	size_t	LEN;
	int		I;

	LEN = 0;
	DEST[0] = '\0';

	while (FROM <= TO && G[FROM].CLASS == G__SPACE__)
		++FROM;

	while (TO >= FROM && G[TO].CLASS == G__SPACE__)
		--TO;

	for (I = FROM; I <= TO; ++I)
	{
		const char		*C = READER_CHAR(G__READER__, G[I].CLASS);
		const size_t	N = strlen(C);

		/* NO DOUBLE SPACES */
		if (G[I].CLASS == G__SPACE__ && LEN && DEST[LEN - 1] == ' ')
			continue ;

		if (LEN + N + 1 > SIZE)
			break ;

		memcpy(DEST + LEN, C, N);
		LEN += N;
		DEST[LEN] = '\0';
	}
}

/*
# ONE UTF-8 CHARACTER IN ANOTHER CASE. UP: 1 = TO UPPER CASE, 0 = TO LOWER.
# DOTTED: 1 = "i" BECOMES "İ" (TURKISH), 0 = "I". WRITES IT TO OUT AND
# RETURNS THE BYTES READ FROM TEXT.
*/
static int
	CHANGE_CASE(const char *TEXT, int UP, int DOTTED, char *DEST)
{
	static const char *const	PAIRS[] = {
		"Ç", "ç", "Ğ", "ğ", "Ö", "ö", "Ş", "ş", "Ü", "ü", "İ", "i", "I", "ı"
	};
	const unsigned char			C = (unsigned char)TEXT[0];
	int							P;

	DEST[1] = '\0';
	DEST[2] = '\0';

	if (C == 'i' && UP && DOTTED)
	{
		strcpy(DEST, "İ");
		return (1);
	}

	if (C < 0X80)
	{
		DEST[0] = (char)C;

		if (UP && C >= 'a' && C <= 'z')
			DEST[0] = (char)(C - 'a' + 'A');

		if (!UP && C >= 'A' && C <= 'Z')
			DEST[0] = (char)(C - 'A' + 'a');

		return (1);
	}

	for (P = 0; P < 14; P += 2)
	{
		/* "İ" AND "ı" ARE TURNED AROUND ONLY ONE WAY ("İ" > "i", "ı" > "I") */
		if (!UP && strncmp(TEXT, PAIRS[P], 2) == 0 && P != 12)
		{
			strcpy(DEST, PAIRS[P + 1]);
			return (2);
		}

		if (UP && strncmp(TEXT, PAIRS[P + 1], 2) == 0 && P != 10)
		{
			strcpy(DEST, PAIRS[P]);
			return (2);
		}
	}

	DEST[0] = TEXT[0];
	DEST[1] = TEXT[1];
	return (1 + ((C & 0XE0) == 0XC0));
}

/*
# A WORD IN ONE OF FIVE CASES: 0 AS WRITTEN, 1 LOWER, 2 UPPER, 3 UPPER WITH
# "İ", 4 THE FIRST LETTER UPPER AND THE REST LOWER.
*/
static void
	CASE_VARIANT(const char *WORD, int MODE, char *DEST, size_t SIZE)
{
	size_t	LEN;
	int		FIRST;

	LEN = 0;
	FIRST = 1;

	if (MODE == 0)
	{
		snprintf(DEST, SIZE, "%s", WORD);
		return ;
	}

	while (*WORD)
	{
		char	ONE[4];
		int		UP;

		UP = MODE == 2 || MODE == 3 || (MODE == 4 && FIRST);
		WORD += CHANGE_CASE(WORD, UP, MODE == 3, ONE);
		FIRST = ONE[0] == ' ';

		if (LEN + strlen(ONE) + 1 > SIZE)
			break ;

		strcpy(DEST + LEN, ONE);
		LEN += strlen(ONE);
	}

	DEST[LEN] = '\0';
}

/* THE SAME WORD WITHOUT CASE, SPACES, DOTS AND APOSTROPHES ("I" IS "i") */
static void
	BARE(const char *TEXT, char *DEST, size_t SIZE)
{
	size_t	LEN;

	LEN = 0;

	while (*TEXT)
	{
		char	ONE[4];

		if (*TEXT == ' ' || *TEXT == '.' || *TEXT == '\'')
		{
			++TEXT;
			continue ;
		}

		TEXT += CHANGE_CASE(TEXT, 0, 0, ONE);

		if (strcmp(ONE, "ı") == 0)
			strcpy(ONE, "i");

		if (LEN + strlen(ONE) + 1 > SIZE)
			break ;

		strcpy(DEST + LEN, ONE);
		LEN += strlen(ONE);
	}

	DEST[LEN] = '\0';
}

/* ONE MORE KNOWN WORD (WORD MUST STAY: A BUILT-IN OR A COPY). 1 = ADDED */
static int
	ADD_KNOWN(const char *WORD)
{
	if (G__KNOWN_COUNT__ >= USER_WORDS_MAX + WORDS_BUILT_IN_COUNT)
		return (0);

	if (G__KNOWN_COUNT__ >= G__KNOWN_ROOM__)
	{
		const int		ROOM = G__KNOWN_ROOM__ * 2 + 512;
		struct S_KNOWN	*BIGGER;

		BIGGER = (struct S_KNOWN *)realloc(
			G__KNOWN__,
			(size_t)ROOM * sizeof(*BIGGER)
		);

		if (!BIGGER)
			return (0);

		G__KNOWN__ = BIGGER;
		G__KNOWN_ROOM__ = ROOM;
	}

	G__KNOWN__[G__KNOWN_COUNT__].WORD = WORD;
	BARE(WORD, G__KNOWN__[G__KNOWN_COUNT__].BARE, CELL_MAX);
	++G__KNOWN_COUNT__;
	return (1);
}

/* THE BUILT-IN WORDS, THE FIRST TIME THEY ARE NEEDED */
static void
	KNOWN_START(void)
{
	int	I;

	if (G__KNOWN_COUNT__)
		return ;

	for (I = 0; I < WORDS_BUILT_IN_COUNT; ++I)
		ADD_KNOWN(WORDS_BUILT_IN[I]);
}

void
	HAND_ADD_WORDS(const char *TEXT)
{
	KNOWN_START();

	while (*TEXT)
	{
		size_t	LEN;
		char	*COPY;

		while (*TEXT && strchr(" \t\r\n,;", *TEXT))
			++TEXT;

		LEN = strcspn(TEXT, "\r\n,;");

		while (LEN && strchr(" \t", TEXT[LEN - 1]))
			--LEN;

		if (!LEN || LEN >= CELL_MAX)
		{
			TEXT += LEN;
			continue ;
		}

		COPY = (char *)malloc(LEN + 1);

		if (!COPY)
			return ;

		memcpy(COPY, TEXT, LEN);
		COPY[LEN] = '\0';

		if (!ADD_KNOWN(COPY))
		{
			free(COPY);
			return ;
		}

		TEXT += LEN;
	}
}

/* EDIT DISTANCE OF TWO BARE WORDS (BYTES), CUT AT 64 BYTES */
static int
	DISTANCE(const char *A, const char *B)
{
	int	ROW[65];
	int	LA;
	int	LB;
	int	I;
	int	J;

	LA = (int)strlen(A);
	LB = (int)strlen(B);

	if (LA > 64)
		LA = 64;

	if (LB > 64)
		LB = 64;

	for (J = 0; J <= LB; ++J)
		ROW[J] = J;

	for (I = 1; I <= LA; ++I)
	{
		int	DIAG;

		DIAG = ROW[0];
		ROW[0] = I;

		for (J = 1; J <= LB; ++J)
		{
			const int	UP = ROW[J];
			int			BEST;

			BEST = DIAG + (A[I - 1] != B[J - 1]);

			if (UP + 1 < BEST)
				BEST = UP + 1;

			if (ROW[J - 1] + 1 < BEST)
				BEST = ROW[J - 1] + 1;

			DIAG = UP;
			ROW[J] = BEST;
		}
	}

	return (ROW[LB]);
}

/* THE SPACES OF A CODE OR A NUMBER LEFT OUT ("1 345" IS "1345") */
static void
	NO_SPACES(char *TEXT)
{
	char	*TO;

	for (TO = TEXT; *TEXT; ++TEXT)
		if (*TEXT != ' ')
			*TO++ = *TEXT;

	*TO = '\0';
}

/* 1 = ONLY DIGITS, DOTS AND COMMAS */
static int
	IS_NUMBER_TEXT(const char *TEXT)
{
	if (!*TEXT)
		return (0);

	for (; *TEXT; ++TEXT)
		if ((*TEXT < '0' || *TEXT > '9') && *TEXT != '.' && *TEXT != ',')
			return (0);

	return (1);
}

/* THE WHOLE NUMBER OF A CELL ("1.250" IS 1250), -1 WHEN IT IS NOT ONE */
static long
	WHOLE_NUMBER(const char *TEXT)
{
	long	V;
	int		DIGITS;

	V = 0;
	DIGITS = 0;

	for (; *TEXT; ++TEXT)
	{
		if (*TEXT == '.' && DIGITS)
			continue ;

		if (*TEXT < '0' || *TEXT > '9' || V > 100000000L)
			return (-1);

		V = V * 10 + (*TEXT - '0');
		++DIGITS;
	}

	if (!DIGITS)
		return (-1);

	return (V);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                       SMALL THINGS                       |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                         READING                          |  \\//  */
/*   \/   #==========================================================#   \/   */
/* A PICTURE MADE SMALLER BY F (EVERY F X F BLOCK AVERAGED). NULL = NO MEMORY */
static unsigned char
	*SHRINK(const unsigned char *GRAY, int W, int H, int F, int *NW, int *NH)
{
	unsigned char	*DEST;
	int				X;
	int				Y;

	*NW = W / F;
	*NH = H / F;

	if (*NW < 1 || *NH < 1)
		return (NULL);

	DEST = (unsigned char *)malloc((size_t)*NW * (size_t)*NH);

	if (!DEST)
		return (NULL);

	for (Y = 0; Y < *NH; ++Y)
	{
		for (X = 0; X < *NW; ++X)
		{
			int	SUM;
			int	DY;

			SUM = 0;

			for (DY = 0; DY < F; ++DY)
			{
				const unsigned char	*ROW = GRAY + (size_t)(Y * F + DY) * W;
				int					DX;

				for (DX = 0; DX < F; ++DX)
					SUM += ROW[X * F + DX];
			}

			DEST[(size_t)Y * *NW + X] = (unsigned char)(SUM / (F * F));
		}
	}

	return (DEST);
}

/*
# THE CUBIC WEIGHT (A = -0.5, AS PHOTO PROGRAMS USE) OF A PIXEL D AWAY: IT
# KEEPS EDGES SHARP, WHERE AN AVERAGE OF NEIGHBOURS WOULD BLUR THEM
*/
static float
	CUBIC(float D)
{
	if (D < 0.0F)
		D = -D;

	if (D <= 1.0F)
		return ((1.5F * D - 2.5F) * D * D + 1.0F);

	if (D < 2.0F)
		return (((-0.5F * D + 2.5F) * D - 4.0F) * D + 2.0F);

	return (0.0F);
}

/*
# ONE DIRECTION OF GROW: COUNT VALUES (STEP APART) BECOME 2 * COUNT. EVERY
# NEW VALUE IS A QUARTER OF A PIXEL BEFORE OR AFTER AN OLD ONE.
*/
static void
	GROW_PASS(
		const float *SOURCE,
		int COUNT,
		int STEP,
		float *TARGET,
		int TARGET_STEP
	)
{
	int	I;

	for (I = 0; I < COUNT * 2; ++I)
	{
		const float	AT = (float)I / 2.0F - 0.25F;
		const int	BASE = (int)floorf(AT);
		const float	T = AT - (float)BASE;
		float		SUM;
		int			K;

		SUM = 0.0F;

		for (K = -1; K <= 2; ++K)
		{
			int	J;

			J = BASE + K;

			if (J < 0)
				J = 0;

			if (J >= COUNT)
				J = COUNT - 1;

			SUM += SOURCE[J * STEP] * CUBIC((float)K - T);
		}

		TARGET[I * TARGET_STEP] = SUM;
	}
}

/* A PICTURE MADE TWICE AS BIG, WITH CUBIC WEIGHTS. NULL = NO MEMORY */
static unsigned char
	*GROW(const unsigned char *GRAY, int W, int H)
{
	unsigned char	*DEST;
	float			*ROW;
	float			*WIDE;
	float			*TALL;
	int				X;
	int				Y;

	DEST = (unsigned char *)malloc((size_t)W * 2 * (size_t)H * 2);
	ROW = (float *)malloc((size_t)W * sizeof(float));
	WIDE = (float *)malloc((size_t)W * 2 * (size_t)H * sizeof(float));
	TALL = (float *)malloc((size_t)H * 2 * sizeof(float));

	if (!DEST || !ROW || !WIDE || !TALL)
	{
		free(DEST);
		free(ROW);
		free(WIDE);
		free(TALL);
		return (NULL);
	}

	for (Y = 0; Y < H; ++Y)
	{
		for (X = 0; X < W; ++X)
			ROW[X] = (float)GRAY[(size_t)Y * W + X];

		GROW_PASS(ROW, W, 1, WIDE + (size_t)Y * W * 2, 1);
	}

	for (X = 0; X < W * 2; ++X)
	{
		GROW_PASS(WIDE + X, H, W * 2, TALL, 1);

		for (Y = 0; Y < H * 2; ++Y)
		{
			float	V;

			V = TALL[Y] + 0.5F;

			if (V < 0.0F)
				V = 0.0F;

			if (V > 255.0F)
				V = 255.0F;

			DEST[(size_t)Y * W * 2 + X] = (unsigned char)V;
		}
	}

	free(ROW);
	free(WIDE);
	free(TALL);
	return (DEST);
}

/*
# A "-" THAT THE PAGE FOUND (A LONE FLAT STROKE) IS A LIKELY READING ALL
# ALONG ITS COLUMNS, EVEN WHERE THE READER SAW ONLY A GAP. A STEP COVERS 4
# COLUMNS; IT IS ON THE DASH WHEN ITS MIDDLE IS.
*/
static void
	SEE_DASHES(struct S_READ *R)
{
	int	D;

	for (D = 0; D < R->LINE->DASHES; ++D)
	{
		int	T;

		for (
			T = (R->LINE->DASH_FROM[D] + 1) / 4;
			4 * T + 2 < R->LINE->DASH_TO[D] && T < R->STEPS;
			++T
		)
		{
			float	*ROW = R->LOGP + (size_t)T * G__CLASSES__;
			float	MAX;
			int		K;

			MAX = ROW[0];

			for (K = 1; K < G__CLASSES__; ++K)
				if (ROW[K] > MAX)
					MAX = ROW[K];

			if (ROW[G__DASH__] < MAX - (float)DASH_DOUBT)
				ROW[G__DASH__] = MAX - (float)DASH_DOUBT;
		}
	}
}

/* ONE LINE: THE READER, THE BEST CLASS AT EVERY STEP, AND ITS SCORE */
static void
	READ_ONE(struct S_READ *R)
{
	int	T;
	int	I;

	if (R->DONE)
		return ;

	R->DONE = 1;
	R->LOGP = READER_RUN(G__READER__, R->LINE->INK, R->LINE->W, &R->STEPS);
	R->COUNT = 0;
	R->BEST = 0.0;

	if (!R->LOGP)
		return ;

	SEE_DASHES(R);
	R->COUNT = CTC_GREEDY(
		R->LOGP,
		G__CLASSES__,
		0,
		R->STEPS,
		NULL,
		R->GLYPH,
		GLYPHS_MAX
	);

	for (I = 0; I < R->COUNT; ++I)
		R->TAG[I] = -1;

	for (T = 0; T < R->STEPS; ++T)
	{
		const float	*ROW = R->LOGP + (size_t)T * G__CLASSES__;
		float		MAX;
		int			K;

		MAX = ROW[0];

		for (K = 1; K < G__CLASSES__; ++K)
			if (ROW[K] > MAX)
				MAX = ROW[K];

		R->BEST += MAX;
	}
}

#ifdef _WIN32
struct	S_JOB
{
	struct S_READ	*READS;
	const int		*WHICH;
	int				COUNT;
	volatile LONG	NEXT;
};

static DWORD WINAPI
	READ_THREAD(LPVOID PARAMETER)
{
	struct S_JOB	*J;
	LONG			I;

	J = (struct S_JOB *)PARAMETER;

	while ((I = InterlockedIncrement(&J->NEXT) - 1) < J->COUNT)
		READ_ONE(&J->READS[J->WHICH[I]]);

	return (0);
}
#endif /* _WIN32 */

/* THE LINES WHICH[0 ... COUNT - 1], ON EVERY PROCESSOR AT THE SAME TIME */
static void
	READ_LINES(struct S_READ *READS, const int *WHICH, int COUNT)
{
#ifdef _WIN32
	struct S_JOB	J;
	SYSTEM_INFO		INFO;
	HANDLE			THREADS[16];
	int				N;
	int				I;

	GetSystemInfo(&INFO);
	N = (int)INFO.dwNumberOfProcessors - 1; /* THIS THREAD WORKS TOO */

	if (N > 16)
		N = 16;

	if (N > COUNT - 1)
		N = COUNT - 1;

	J.READS = READS;
	J.WHICH = WHICH;
	J.COUNT = COUNT;
	J.NEXT = 0;

	for (I = 0; I < N; ++I)
		THREADS[I] = CreateThread(NULL, 0, READ_THREAD, &J, 0, NULL);

	READ_THREAD(&J);

	for (I = 0; I < N; ++I)
	{
		if (THREADS[I])
		{
			WaitForSingleObject(THREADS[I], INFINITE);
			CloseHandle(THREADS[I]);
		}
	}
#else /* !_WIN32 */
	int	I;

	for (I = 0; I < COUNT; ++I)
		READ_ONE(&READS[WHICH[I]]);
#endif /* _WIN32 */
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                         READING                          |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                         GRAMMARS                         |  \\//  */
/*   \/   #==========================================================#   \/   */
/* A NEW NODE (THE GRAMMARS HERE NEVER NEED MORE THAN CTC_NODES_MAX) */
static int
	NODE(struct S_GRAMMAR *G, int TAG)
{
	G->TAG[G->NODES] = TAG;
	G->FINAL[G->NODES] = 0;
	return (G->NODES++);
}

static void
	ARC(struct S_GRAMMAR *G, int FROM, int TO, const unsigned char *ALLOWED)
{
	G->ARC[G->ARCS].FROM = FROM;
	G->ARC[G->ARCS].TO = TO;
	G->ARC[G->ARCS].ALLOWED = ALLOWED;
	G->ARC[G->ARCS].COST = 0.0F;
	++G->ARCS;
}

/*
# A NUMBER AND ONE OF THE UNITS OF THE LIST (G__UNITS__), SPELLED OUT LETTER
# BY LETTER: "12 Top" CAN NOT BE READ AS "12707". T: FIELD * 4. EXITS GETS
# THE LAST NODE OF EVERY UNIT AND THE SPACES AFTER THEM (AT MOST 4).
*/
static int
	ADD_KNOWN_UNIT(struct S_GRAMMAR *G, int ENTRY, int T, int *EXITS)
{
	const int	MAIN = NODE(G, T + PART_MAIN);
	const int	SPACE = NODE(G, T + PART_SPACE);
	const int	AFTER = NODE(G, T + PART_SPACE);
	int			USED;
	int			N;
	int			K;

	ARC(G, ENTRY, MAIN, G__DIGIT__);
	ARC(G, MAIN, MAIN, G__NUMBER__);
	ARC(G, MAIN, SPACE, G__SPACE_ONLY__);
	ARC(G, SPACE, SPACE, G__SPACE_ONLY__);
	ARC(G, AFTER, AFTER, G__SPACE_ONLY__);
	USED = 0;
	N = 0;

	for (K = 0; K < G__UNITS__->COUNT && N < 3; ++K)
	{
		int	PREV;
		int	L;

		PREV = -1;

		for (L = 0; L < G__UNITS__->LETTERS[K]; ++L)
		{
			const unsigned char	*MASK = G__UNITS__->MASK[USED + L];
			const int			NEXT = NODE(G, T + PART_UNIT);

			if (PREV < 0)
			{
				ARC(G, MAIN, NEXT, MASK);
				ARC(G, SPACE, NEXT, MASK);
			}
			else
				ARC(G, PREV, NEXT, MASK);

			PREV = NEXT;
		}

		ARC(G, PREV, AFTER, G__SPACE_ONLY__);
		EXITS[N++] = PREV;
		USED += G__UNITS__->LETTERS[K];
	}

	EXITS[N++] = AFTER;
	return (N);
}

/*
# ONE FIELD OF A LIST LINE AFTER NODE ENTRY (AT MOST 4 NODES AND 10 ARCS,
# OR WITH THE UNITS OF THE LIST, 3 NODES MORE THAN THEIR LETTERS).
# EXITS GETS THE NODES IT MAY END AT; RETURNS THEIR NUMBER.
*/
static int
	ADD_FIELD(struct S_GRAMMAR *G, int ENTRY, int FIELD, int KIND, int *EXITS)
{
	const int	T = FIELD * 4;
	int			MAIN;
	int			SPACE;
	int			UNIT;
	int			AFTER;

	if (KIND != KIND_FREE)
		ARC(G, ENTRY, ENTRY, G__SPACE_ONLY__);

	switch (KIND)
	{
		case (KIND_TEXT):
		{
			MAIN = NODE(G, T + PART_MAIN);
			ARC(G, ENTRY, MAIN, G__TEXT__);
			ARC(G, MAIN, MAIN, G__TEXT__);
			EXITS[0] = MAIN;
		}
		return (1);
		case (KIND_CODE):
		case (KIND_NUMBER):
		{
			const unsigned char	*FIRST = G__ALNUM__;
			const unsigned char	*NEXT = G__ALNUM__;

			if (KIND == KIND_NUMBER)
			{
				FIRST = G__DIGIT__;
				NEXT = G__NUMBER__;
			}

			MAIN = NODE(G, T + PART_MAIN);
			ARC(G, ENTRY, MAIN, FIRST);
			ARC(G, MAIN, MAIN, NEXT);
			ARC(G, MAIN, MAIN, G__SPACE_ONLY__);
			EXITS[0] = MAIN;
		}
		return (1);
		case (KIND_NUMBER_UNIT):
		{
			MAIN = NODE(G, T + PART_MAIN);
			SPACE = NODE(G, T + PART_SPACE);
			UNIT = NODE(G, T + PART_UNIT);
			AFTER = NODE(G, T + PART_SPACE);
			ARC(G, ENTRY, MAIN, G__DIGIT__);
			ARC(G, MAIN, MAIN, G__NUMBER__);
			ARC(G, MAIN, SPACE, G__SPACE_ONLY__);
			ARC(G, SPACE, SPACE, G__SPACE_ONLY__);
			ARC(G, MAIN, UNIT, G__LETTER__);
			ARC(G, SPACE, UNIT, G__LETTER__);
			ARC(G, UNIT, UNIT, G__LETTER__);
			ARC(G, UNIT, AFTER, G__SPACE_ONLY__);
			ARC(G, AFTER, AFTER, G__SPACE_ONLY__);
			EXITS[0] = MAIN;
			EXITS[1] = SPACE;
			EXITS[2] = UNIT;
			EXITS[3] = AFTER;
		}
		return (4);
		case (KIND_NUMBER_KNOWN_UNIT):
		{}
		return (ADD_KNOWN_UNIT(G, ENTRY, T, EXITS));
		default:
		{
			MAIN = NODE(G, T + PART_MAIN);
			ARC(G, ENTRY, MAIN, G__NOT_DASH__);
			ARC(G, MAIN, MAIN, G__NOT_DASH__);
			EXITS[0] = MAIN;
		}
		return (1);
	}
}

/* A LIST LINE: FIELDS OF THESE KINDS WITH A DASH BETWEEN EVERY TWO */
static void
	LIST_GRAMMAR(struct S_GRAMMAR *G, int FIELDS, const int *KINDS)
{
	int	EXITS[4];
	int	ENTRY;
	int	F;

	G->NODES = 0;
	G->ARCS = 0;
	ENTRY = NODE(G, PART_SPACE);

	for (F = 0; F < FIELDS; ++F)
	{
		const int	N = ADD_FIELD(G, ENTRY, F, KINDS[F], EXITS);
		int			I;

		if (F == FIELDS - 1)
		{
			for (I = 0; I < N; ++I)
				G->FINAL[EXITS[I]] = 1;

			break ;
		}

		ENTRY = NODE(G, (F + 1) * 4 + PART_SPACE);

		for (I = 0; I < N; ++I)
			ARC(G, EXITS[I], ENTRY, G__DASH_ONLY__);
	}
}

/*
# A TOTAL: MAYBE A WORD ("TOTAL:"), A NUMBER, MAYBE A UNIT. THE UNITS OF THE
# LIST ("Top") ARE SPELLED OUT LETTER BY LETTER, SO THE INK OF THE NUMBER
# CAN NOT BE TAKEN FOR A UNIT ("170 Top" IS NOT "1 tata"); ANY OTHER UNIT
# COSTS MORE.
*/
static void
	TOTAL_GRAMMAR(struct S_GRAMMAR *G, struct S_UNITS *U, int ONLY_UNITS)
{
	int	PREFIX;
	int	MAIN;
	int	SPACE;
	int	FREE;
	int	AFTER;
	int	USED;
	int	K;

	G->NODES = 0;
	G->ARCS = 0;
	NODE(G, PART_SPACE);
	PREFIX = NODE(G, PART_PREFIX);
	MAIN = NODE(G, PART_MAIN);
	SPACE = NODE(G, PART_SPACE);
	FREE = NODE(G, PART_UNIT);
	AFTER = NODE(G, PART_SPACE);
	ARC(G, 0, 0, G__SPACE_ONLY__);
	ARC(G, 0, MAIN, G__DIGIT__);
	ARC(G, MAIN, MAIN, G__NUMBER__);
	ARC(G, MAIN, SPACE, G__SPACE_ONLY__);
	ARC(G, SPACE, SPACE, G__SPACE_ONLY__);
	ARC(G, AFTER, AFTER, G__SPACE_ONLY__);
	G->FINAL[AFTER] = 1;
	USED = 0;

	/* STRICT: A NUMBER AND ONE OF THE UNITS, NOTHING ELSE */
	if (!ONLY_UNITS)
	{
		ARC(G, 0, PREFIX, G__LETTER__);
		G->ARC[G->ARCS - 1].COST = PREFIX_COST;
		ARC(G, PREFIX, PREFIX, G__TEXT__);
		ARC(G, PREFIX, MAIN, G__DIGIT__);
		ARC(G, MAIN, FREE, G__LETTER__);
		G->ARC[G->ARCS - 1].COST = OTHER_UNIT_COST;
		ARC(G, SPACE, FREE, G__LETTER__);
		G->ARC[G->ARCS - 1].COST = OTHER_UNIT_COST;
		ARC(G, FREE, FREE, G__LETTER__);
		G->ARC[G->ARCS - 1].COST = OTHER_LETTER_COST;
		ARC(G, FREE, AFTER, G__SPACE_ONLY__);
		G->FINAL[MAIN] = 1;
		G->FINAL[SPACE] = 1;
		G->FINAL[FREE] = 1;
	}


	for (K = 0; K < U->COUNT; ++K)
	{
		int	PREV;
		int	L;

		PREV = -1;

		for (L = 0; L < U->LETTERS[K]; ++L)
		{
			const int	NEXT = NODE(G, PART_UNIT);

			if (PREV < 0)
			{
				ARC(G, MAIN, NEXT, U->MASK[USED + L]);
				ARC(G, SPACE, NEXT, U->MASK[USED + L]);
			}
			else
				ARC(G, PREV, NEXT, U->MASK[USED + L]);

			PREV = NEXT;
		}

		G->FINAL[PREV] = 1;
		ARC(G, PREV, AFTER, G__SPACE_ONLY__);
		USED += U->LETTERS[K];
	}
}

/*
# THE UNITS OF A COLUMN: THE WORDS THAT COME TWO TIMES OR MORE, MOST FIRST,
# AS LONG AS THEIR LETTERS FIT THE GRAMMAR. 0 = NONE.
*/
static int
	MAKE_UNITS(struct S_CELL **CELLS, int COUNT, struct S_UNITS *U)
{
	struct S_VOCAB	V;
	int				USED;
	int				I;

	U->COUNT = 0;
	USED = 0;

	if (!MAKE_VOCAB(&V, CELLS, COUNT))
		return (0);

	while (U->COUNT < UNITS_MAX)
	{
		const char	*P;
		int			BEST;
		int			L;
		int			K;

		BEST = -1;

		for (K = 0; K < V.COUNT; ++K)
			if (V.HITS[K] >= 2 && (BEST < 0 || V.HITS[K] > V.HITS[BEST]))
				BEST = K;

		if (BEST < 0)
			break ;

		V.HITS[BEST] = 0;
		L = 0;

		for (P = V.WORD[BEST]; *P && USED + L < UNIT_LETTERS_MAX; ++L)
		{
			int	LEN;
			int	K2;

			K2 = CLASS_OF(P, &LEN);
			memset(U->MASK[USED + L], 0, CLASSES_MAX);

			if (!K2)
				break ;

			U->MASK[USED + L][K2] = 1;
			U->MASK[USED + L][G__TWIN__[K2]] = 1;
			U->MASK[USED + L][0] = 0;
			P += LEN;
		}

		if (*P || !L)
			continue ;

		U->WORD[U->COUNT] = V.WORD[BEST];
		U->LETTERS[U->COUNT] = L;
		++U->COUNT;
		USED += L;
	}

	/* V IS FREED: THE WORDS ARE TAKEN FROM THE CELLS THAT HOLD THEM */
	for (I = 0; I < U->COUNT; ++I)
	{
		char	B[CELL_MAX];
		int		K;

		BARE(U->WORD[I], B, sizeof(B));

		for (K = 0; K < COUNT; ++K)
		{
			char	A[CELL_MAX];

			BARE(CELLS[K]->TEXT, A, sizeof(A));

			if (CELLS[K]->LINE >= 0 && strcmp(A, B) == 0)
			{
				U->WORD[I] = CELLS[K]->TEXT;
				break ;
			}
		}
	}

	FREE_VOCAB(&V);
	return (U->COUNT);
}

/* THE LINE READ AGAIN WITH THE GRAMMAR: ITS GLYPHS AND TAGS ARE REPLACED */
static double
	READ_GRAMMAR(struct S_READ *R, const struct S_GRAMMAR *G)
{
	struct S_GLYPH	GLYPH[GLYPHS_MAX];
	int				TAG[GLYPHS_MAX];
	double			SCORE;
	int				N;

	SCORE = CTC_GRAMMAR(
		R->LOGP,
		G__CLASSES__,
		0,
		R->STEPS,
		G,
		GLYPH,
		TAG,
		GLYPHS_MAX,
		&N
	);

	if (SCORE > -1E29)
	{
		memcpy(R->GLYPH, GLYPH, (size_t)N * sizeof(GLYPH[0]));
		memcpy(R->TAG, TAG, (size_t)N * sizeof(TAG[0]));
		R->COUNT = N;
	}

	return (SCORE);
}

/* THE SCORE OF THE BEST WAY THE GRAMMAR ALLOWS, THE LINE LEFT AS IT IS */
static double
	SCORE_GRAMMAR(const struct S_READ *R, const struct S_GRAMMAR *G)
{
	int	N;

	return (
		CTC_GRAMMAR(
			R->LOGP,
			G__CLASSES__,
			0,
			R->STEPS,
			G,
			NULL,
			NULL,
			0,
			&N
		)
	);
}

/* WHAT A LIST LINE OF 1, 2, ... FIELDS COSTS THIS LINE (0 = FITS AS READ) */
static void
	LIST_COSTS(struct S_READ *R)
{
	struct S_GRAMMAR	G;
	int					KINDS[FIELDS_MAX];
	int					K;

	memset(KINDS, 0, sizeof(KINDS));

	for (K = 0; K < FIELDS_MAX; ++K)
	{
		LIST_GRAMMAR(&G, K + 1, KINDS);
		R->COST[K] = R->BEST - SCORE_GRAMMAR(R, &G);
	}
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                         GRAMMARS                         |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          CELLS                           |  \\//  */
/*   \/   #==========================================================#   \/   */
/*
# THE STEPS OF GLYPHS FROM ... TO: FROM THE MIDDLE OF THE GAP BEFORE THE
# FIRST ONE TO THE MIDDLE OF THE GAP AFTER THE LAST ONE.
*/
static void
	GLYPH_SPAN(const struct S_READ *R, int FROM, int TO, int *SF, int *ST)
{
	*SF = 0;
	*ST = R->STEPS;

	if (FROM > 0)
		*SF = (R->GLYPH[FROM - 1].LAST + R->GLYPH[FROM].FIRST + 1) / 2;

	if (TO < R->COUNT - 1)
		*ST = (R->GLYPH[TO].LAST + R->GLYPH[TO + 1].FIRST + 1) / 2;

	if (*ST <= *SF)
		*ST = *SF + 1;
}

/* A CELL FROM GLYPHS FROM ... TO, SPACES AT ITS ENDS LEFT OUT */
static void
	MAKE_CELL(
		const struct S_READ *R,
		int LINE,
		int FROM,
		int TO,
		struct S_CELL *C
	)
{
	while (FROM <= TO && R->GLYPH[FROM].CLASS == G__SPACE__)
		++FROM;

	while (TO >= FROM && R->GLYPH[TO].CLASS == G__SPACE__)
		--TO;

	C->LINE = -1;
	C->TEXT[0] = '\0';

	if (FROM > TO)
		return ;

	C->LINE = LINE;
	GLYPH_SPAN(R, FROM, TO, &C->FROM, &C->TO);
	GLYPH_TEXT(R->GLYPH, FROM, TO, C->TEXT, sizeof(C->TEXT));
}

/* THE CELL OF ONE FIELD (PART -1: ALL OF IT BUT ITS SPACES AND DASHES) */
static void
	FIELD_CELL(
		const struct S_READ *R,
		int LINE,
		int FIELD,
		int PART,
		struct S_CELL *C
	)
{
	int	FROM;
	int	TO;
	int	I;

	FROM = R->COUNT;
	TO = -1;

	for (I = 0; I < R->COUNT; ++I)
	{
		const int	F = R->TAG[I] / 4;
		const int	P = R->TAG[I] % 4;

		if (
			R->TAG[I] < 0 ||
			F != FIELD ||
			P == PART_SPACE ||
			(PART >= 0 && P != PART)
		)
			continue ;

		if (I < FROM)
			FROM = I;

		TO = I;
	}

	MAKE_CELL(R, LINE, FROM, TO, C);
}

/* HOW WELL TEXT FITS THE INK OF A CELL (LOG) */
static double
	FIT(const struct S_READ *R, const struct S_CELL *C, const char *TEXT)
{
	int	LABELS[CELL_MAX];
	int	N;

	N = TO_CLASSES(TEXT, LABELS, CELL_MAX);

	if (N < 0)
		return (-1E30);

	return (
		CTC_SCORE(R->LOGP, G__CLASSES__, C->FROM, C->TO, LABELS, N)
	);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          CELLS                           |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          WORDS                           |  \\//  */
/*   \/   #==========================================================#   \/   */
/*
# THE KNOWN WORD THAT A TEXT WHICH IS NOT KNOWN MISSES BY ONE LETTER
# ("Craam" > "Cream"), FOR WORDS OF 4 LETTERS OR MORE. -1 = NONE.
*/
static int
	NEAR_KNOWN(const char *TEXT)
{
	char	B[CELL_MAX];
	int		FOUND;
	int		I;

	KNOWN_START();
	BARE(TEXT, B, sizeof(B));
	FOUND = -1;

	if (strlen(B) < 4)
		return (-1);

	for (I = 0; I < G__KNOWN_COUNT__; ++I)
	{
		const int	D = DISTANCE(G__KNOWN__[I].BARE, B);

		if (D == 0)
			return (-1);

		if (D == 1 && FOUND < 0)
			FOUND = I;
	}

	return (FOUND);
}

/* THE CELL TEXTS AND HOW MANY TIMES EACH CAME (CASE AND SPACES LEFT OUT) */
static int
	MAKE_VOCAB(struct S_VOCAB *V, struct S_CELL *const *CELLS, int COUNT)
{
	int	I;

	V->COUNT = 0;
	V->WORD = malloc((size_t)(COUNT + 1) * sizeof(*V->WORD));
	V->HITS = (int *)calloc((size_t)COUNT + 1, sizeof(int));
	V->CLOSE = (int *)calloc((size_t)COUNT + 1, sizeof(int));

	if (!V->WORD || !V->HITS || !V->CLOSE)
	{
		free(V->WORD);
		free(V->HITS);
		free(V->CLOSE);
		memset(V, 0, sizeof(*V));
		return (0);
	}

	for (I = 0; I < COUNT; ++I)
	{
		char	B[CELL_MAX];
		int		K;

		if (CELLS[I]->LINE < 0 || !CELLS[I]->TEXT[0])
			continue ;

		BARE(CELLS[I]->TEXT, B, sizeof(B));

		for (K = 0; K < V->COUNT; ++K)
		{
			char	BK[CELL_MAX];

			BARE(V->WORD[K], BK, sizeof(BK));

			if (strcmp(B, BK) == 0)
				break ;
		}

		if (K == V->COUNT)
			strcpy(V->WORD[V->COUNT++], CELLS[I]->TEXT);

		/* THE SAME WORD WITHOUT ITS STRAY SPACES ("vi zon" > "vizon") */
		if (strlen(CELLS[I]->TEXT) < strlen(V->WORD[K]))
			strcpy(V->WORD[K], CELLS[I]->TEXT);

		V->HITS[K] += 1;
	}

	for (I = 0; I < V->COUNT; ++I)
		V->CLOSE[I] = NEAR_KNOWN(V->WORD[I]);

	return (1);
}

static void
	FREE_VOCAB(struct S_VOCAB *V)
{
	free(V->WORD);
	free(V->HITS);
	free(V->CLOSE);
}

/*
# HOW OFTEN A GUESS COUNTS AS SEEN IN A COLUMN. A KNOWN WORD ALSO GETS THE
# TIMES OF ITS NEAR MISSES; A NEAR MISS GETS NONE: A WORD READ AS "Craam"
# THIRTEEN TIMES IS "Cream" READ THE SAME WRONG WAY THIRTEEN TIMES.
*/
static int
	SEEN(const struct S_VOCAB *V, const char *TEXT)
{
	char	B[CELL_MAX];
	int		KNOWN;
	int		SUM;
	int		I;

	if (!V)
		return (0);

	BARE(TEXT, B, sizeof(B));
	KNOWN = -1;

	KNOWN_START();

	for (I = 0; I < G__KNOWN_COUNT__ && KNOWN < 0; ++I)
		if (strcmp(B, G__KNOWN__[I].BARE) == 0)
			KNOWN = I;

	SUM = 0;

	for (I = 0; I < V->COUNT; ++I)
	{
		char	BI[CELL_MAX];

		BARE(V->WORD[I], BI, sizeof(BI));

		if (strcmp(BI, B) == 0 && V->CLOSE[I] < 0)
			SUM += V->HITS[I];
		else if (KNOWN >= 0 && V->CLOSE[I] == KNOWN)
			SUM += V->HITS[I];
	}

	return (SUM);
}

/* HOW MANY TIMES A TEXT CAME IN THE VOCABULARY (0 = NEVER) */
static int
	HITS_OF(const struct S_VOCAB *V, const char *TEXT)
{
	char	B[CELL_MAX];
	int		K;

	BARE(TEXT, B, sizeof(B));

	for (K = 0; V && K < V->COUNT; ++K)
	{
		char	BK[CELL_MAX];

		BARE(V->WORD[K], BK, sizeof(BK));

		if (strcmp(B, BK) == 0)
			return (V->HITS[K]);
	}

	return (0);
}

/* 1 = A KNOWN WORD (BUILT IN OR GIVEN), IN ANY CASE */
static int
	IS_KNOWN(const char *TEXT)
{
	char	B[CELL_MAX];
	int		I;

	KNOWN_START();
	BARE(TEXT, B, sizeof(B));

	for (I = 0; I < G__KNOWN_COUNT__; ++I)
		if (strcmp(B, G__KNOWN__[I].BARE) == 0)
			return (1);

	return (0);
}

/* HOW FAR A GUESS MAY BE FROM WHAT WAS READ */
static int
	DISTANCE_LIMIT(const char *BARE_TEXT)
{
	const size_t	LEN = strlen(BARE_TEXT);

	if (LEN <= 4)
		return (1);

	if (LEN <= 8)
		return (2);

	return (3);
}

/*
# THE STEPS OF A CELL WITH EVERY LETTER AS LIKELY AS ITSELF OR ITS TWIN IN
# THE OTHER CASE: HANDS WRITE "p" AND "P" ALIKE, SO THE WORD IS CHOSEN
# FIRST, AND ITS CASE AFTER THAT. NULL = NO MEMORY.
*/
static float
	*FOLDED(const struct S_READ *R, const struct S_CELL *C, int NO_SPACE)
{
	const int	STEPS = C->TO - C->FROM;
	float		*DEST;
	int			T;

	DEST = (float *)malloc((size_t)STEPS * G__CLASSES__ * sizeof(float));

	if (!DEST)
		return (NULL);

	for (T = 0; T < STEPS; ++T)
	{
		const float	*ROW = R->LOGP + (size_t)(C->FROM + T) * G__CLASSES__;
		float		*TO = DEST + (size_t)T * G__CLASSES__;
		int			K;

		for (K = 0; K < G__CLASSES__; ++K)
		{
			const float	A = ROW[K];
			float		B;

			TO[K] = A;

			if (!G__TWIN__[K])
				continue ;

			B = ROW[G__TWIN__[K]];

			if (A > B)
				TO[K] = A + log1pf(expf(B - A));
			else
				TO[K] = B + log1pf(expf(A - B));
		}

		/* A SPACE THAT DOES NOT COUNT IS LIKE A "NOTHING" */
		if (NO_SPACE)
		{
			const float	A = TO[0];
			const float	B = ROW[G__SPACE__];

			if (A > B)
				TO[0] = A + log1pf(expf(B - A));
			else
				TO[0] = B + log1pf(expf(A - B));
		}
	}

	return (DEST);
}

/*
# THE SCORE OF ONE GUESS ON THE FOLDED STEPS. G KEEPS THE BEST ONE, AND THE
# BEST SCORE OF ANOTHER WORD (HOW SURE THE CHOICE IS).
*/
static void
	TRY_GUESS(
		const float *FOLD,
		int STEPS,
		const char *GUESS,
		double BONUS,
		struct S_GUESS *G
	)
{
	char	B[CELL_MAX];
	int		LABELS[CELL_MAX];
	double	SCORE;
	int		SAME;
	int		N;

	N = TO_CLASSES(GUESS, LABELS, CELL_MAX);

	if (N < 0)
		return ;

	SCORE = CTC_SCORE(FOLD, G__CLASSES__, 0, STEPS, LABELS, N) + BONUS;
	BARE(GUESS, B, sizeof(B));
	SAME = strcmp(B, G->BARE) == 0;

	if (SCORE > G->SCORE)
	{
		if (!SAME)
			G->SECOND = G->SCORE;

		G->SCORE = SCORE;
		snprintf(G->TEXT, CELL_MAX, "%s", GUESS);
		strcpy(G->BARE, B);
	}
	else if (!SAME && SCORE > G->SECOND)
		G->SECOND = SCORE;
}

/*
# THE CASE OF A WORD: AS GIVEN (WHEN AS_GIVEN), ALL LOWER, ALL UPPER (WITH
# "I" OR "İ") OR THE FIRST LETTER UPPER, WHICHEVER FITS THE INK BEST.
*/
static void
	CHOOSE_CASE(
		const struct S_READ *R,
		const struct S_CELL *C,
		const char *WORD,
		int AS_GIVEN,
		char *DEST
	)
{
	double	BEST_SCORE;
	int		MODE;

	BEST_SCORE = -1E300;
	snprintf(DEST, CELL_MAX, "%s", WORD);

	for (MODE = !AS_GIVEN; MODE < 5; ++MODE)
	{
		char	VARIANT[CELL_MAX];
		double	SCORE;

		CASE_VARIANT(WORD, MODE, VARIANT, sizeof(VARIANT));
		SCORE = FIT(R, C, VARIANT);

		if (SCORE > BEST_SCORE)
		{
			BEST_SCORE = SCORE;
			strcpy(DEST, VARIANT);
		}
	}
}

/* 1 = THE TEXT HAS ONE OF THE USUAL CASES (SEE CHOOSE_CASE) */
static int
	USUAL_CASE(const char *TEXT)
{
	int	MODE;

	for (MODE = 1; MODE < 5; ++MODE)
	{
		char	VARIANT[CELL_MAX];

		CASE_VARIANT(TEXT, MODE, VARIANT, sizeof(VARIANT));

		if (strcmp(VARIANT, TEXT) == 0)
			return (1);
	}

	return (0);
}

/* THE KNOWN WORD WITH THE SAME BARE FORM ("g ray" > "Gray"), OR NULL */
static const char
	*KNOWN_SPELLING(const char *TEXT)
{
	char	B[CELL_MAX];
	int		I;

	KNOWN_START();
	BARE(TEXT, B, sizeof(B));

	for (I = 0; I < G__KNOWN_COUNT__; ++I)
		if (strcmp(B, G__KNOWN__[I].BARE) == 0)
			return (G__KNOWN__[I].WORD);

	return (NULL);
}

/*
# A WORD THAT IS NOT KNOWN, AT MOST SAME_HAND_REACH LETTERS AWAY FROM A WORD
# THAT COMES 3 TIMES OR MORE IN ITS COLUMN (2 FOR A KNOWN WORD) AND 3 TIMES
# AS OFTEN AS ITSELF, IS THAT WORD, HOWEVER BADLY IT FITS: THE SAME HAND
# WROTE IT AGAIN ("Crarm" IN A COLUMN OF "Cream"). LESS THAN HALF OF THE
# LETTERS OF THAT WORD MAY BE DIFFERENT ("'zon" IS "vizon").
# WHEN MORE THAN ONE IS CLOSE, THE ONE THAT FITS BEST.
*/
static void
	SAME_HAND(
		const float *FOLD,
		int STEPS,
		const struct S_VOCAB *V,
		int OWN_HITS,
		struct S_CELL *C,
		struct S_GUESS *G
	)
{
	struct S_GUESS	H;
	char			READ_BARE[CELL_MAX];
	int				I;

	BARE(C->TEXT, READ_BARE, sizeof(READ_BARE));
	H.SCORE = -1E300;
	H.SECOND = -1E300;
	H.TEXT[0] = '\0';
	H.BARE[0] = '\0';

	for (I = 0; V && I < V->COUNT; ++I)
	{
		char	B[CELL_MAX];
		int		NEED;
		int		APART;

		NEED = 3 - IS_KNOWN(V->WORD[I]);

		if (OWN_HITS >= 2 && NEED < 3 * OWN_HITS)
			NEED = 3 * OWN_HITS;

		if (V->HITS[I] < NEED)
			continue ;

		BARE(V->WORD[I], B, sizeof(B));
		APART = DISTANCE(B, READ_BARE);

		if (
			strcmp(B, READ_BARE) == 0 ||
			APART > SAME_HAND_REACH ||
			APART * 2 >= (int)strlen(B)
		)
			continue ;

		TRY_GUESS(FOLD, STEPS, V->WORD[I], 0.0, &H);
	}

	if (!H.TEXT[0])
		return ;

	*G = H;
	C->CHECK = H.SCORE - H.SECOND < WORD_MARGIN;
}

/*
# THE WORD A CELL TAKES: THE GUESS THAT FITS ITS INK BEST, WHAT WAS READ, A
# WORD THAT COMES AGAIN AND AGAIN IN ITS COLUMN, OR A KNOWN WORD, WHEN THAT
# IS CLOSE TO WHAT WAS READ. FIT + PRIOR * LOG(TIMES IN THE COLUMN) +
# WORD_BONUS WHEN IT IS A KNOWN WORD. A WORD THAT COMES 3 TIMES OR MORE MAY
# BE FURTHER AWAY (HALF OF ITS LETTERS, ROUNDED UP). CASE DOES NOT COUNT.
*/
static void
	CHOOSE_WORD(
		const struct S_READ *R,
		struct S_CELL *C,
		const struct S_VOCAB *V
	)
{
	struct S_GUESS	G;
	const char		*SPELLING;
	char			READ_BARE[CELL_MAX];
	float			*FOLD;
	double			BONUS;
	int				OWN_HITS;
	int				STEPS;
	int				LIMIT;
	int				I;

	FOLD = FOLDED(R, C, 0);

	if (!FOLD)
		return ;

	STEPS = C->TO - C->FROM;
	BARE(C->TEXT, READ_BARE, sizeof(READ_BARE));
	LIMIT = DISTANCE_LIMIT(READ_BARE);
	G.SCORE = -1E300;
	G.SECOND = -1E300;
	G.TEXT[0] = '\0';
	G.BARE[0] = '\0';
	OWN_HITS = HITS_OF(V, C->TEXT);
	BONUS = PRIOR * log((double)SEEN(V, C->TEXT) + 1.0);
	BONUS += WORD_BONUS * IS_KNOWN(C->TEXT);
	TRY_GUESS(FOLD, STEPS, C->TEXT, BONUS, &G);

	for (I = 0; V && I < V->COUNT; ++I)
	{
		char	B[CELL_MAX];
		int		REACH;

		if (V->HITS[I] < 2)
			continue ;

		BARE(V->WORD[I], B, sizeof(B));
		REACH = LIMIT;

		if (V->HITS[I] >= 3 && ((int)strlen(B) + 1) / 2 > REACH)
			REACH = ((int)strlen(B) + 1) / 2;

		if (DISTANCE(B, READ_BARE) > REACH)
			continue ;

		BONUS = PRIOR * log((double)SEEN(V, V->WORD[I]) + 1.0);
		BONUS += WORD_BONUS * IS_KNOWN(V->WORD[I]);
		TRY_GUESS(FOLD, STEPS, V->WORD[I], BONUS, &G);
	}

	KNOWN_START();

	for (I = 0; I < G__KNOWN_COUNT__; ++I)
	{
		const char	*W = G__KNOWN__[I].WORD;

		if (DISTANCE(G__KNOWN__[I].BARE, READ_BARE) > LIMIT)
			continue ;

		BONUS = PRIOR * log((double)SEEN(V, W) + 1.0) + WORD_BONUS;
		TRY_GUESS(FOLD, STEPS, W, BONUS, &G);
	}

	/*
	# NOT SURE: ANOTHER WORD WAS CLOSE, OR A WORD THAT IS NEITHER KNOWN NOR
	# SEEN AGAIN IN THE COLUMN STAYED AS IT WAS READ, AND IT FITS BADLY
	*/
	C->CHECK = G.SCORE - G.SECOND < WORD_MARGIN;

	if (
		strcmp(G.BARE, READ_BARE) == 0 &&
		!IS_KNOWN(C->TEXT) &&
		strlen(C->TEXT) >= 4
	)
		SAME_HAND(FOLD, STEPS, V, OWN_HITS, C, &G);

	free(FOLD);

#ifdef HAND_DEBUG
	fprintf(stderr, "  word %-14s > %-14s %.2f\n", C->TEXT, G.TEXT, G.SCORE);
#endif /* HAND_DEBUG */

	if (!G.TEXT[0])
		return ;

	if (
		strcmp(G.BARE, READ_BARE) == 0 &&
		OWN_HITS < 2 &&
		!IS_KNOWN(C->TEXT) &&
		LOOSE_FIT(R, C, C->TEXT, 0) < CHECK_LOG
	)
		C->CHECK = 1;

	/*
	# A KNOWN WORD IS WRITTEN AS IT IS KNOWN, ANY OTHER AS IN ITS COLUMN,
	# WITHOUT STRAY SPACES ("no te" IS "note" WHEN "note" IS IN THE COLUMN)
	*/
	SPELLING = KNOWN_SPELLING(G.TEXT);

	for (I = 0; !SPELLING && V && I < V->COUNT; ++I)
	{
		char	B[CELL_MAX];

		BARE(V->WORD[I], B, sizeof(B));

		if (strcmp(B, G.BARE) == 0)
			SPELLING = V->WORD[I];
	}

	if (SPELLING)
		snprintf(C->TEXT, sizeof(C->TEXT), "%s", SPELLING);
	else
		strcpy(C->TEXT, G.TEXT);
}

/* 1 = CELL J HAS THE WORD B (BARE) */
static int
	CELL_HAS(struct S_CELL *const *CELLS, int J, const char *B)
{
	char	BJ[CELL_MAX];

	if (CELLS[J]->LINE < 0 || !CELLS[J]->TEXT[0])
		return (0);

	BARE(CELLS[J]->TEXT, BJ, sizeof(BJ));
	return (strcmp(B, BJ) == 0);
}

/*
# THE CASE OF ONE WORD ALL OVER A COLUMN: THE CASE ("Cream", "cream",
# "CREAM") THAT FITS THE INK OF ITS CELLS TOGETHER BEST, FROM CELL FIRST ON.
# A CELL LOSES AT MOST CASE_CAP FOR A CASE: INK THAT HARDLY SHOWS THE WORD
# DOES NOT DECIDE FOR ALL THE OTHERS. FITS: ROOM FOR 5 PER CELL.
*/
static void
	CHOOSE_CASE_OF(
		struct S_READ *READS,
		struct S_CELL **CELLS,
		int COUNT,
		int FIRST,
		double *FITS
	)
{
	char	VARIANT[5][CELL_MAX];
	char	B[CELL_MAX];
	double	BEST_SCORE;
	int		BEST;
	int		USE[5];
	int		MODE;
	int		SAME;
	int		J;

	BARE(CELLS[FIRST]->TEXT, B, sizeof(B));

	/* AS IT IS KNOWN (OR READ, IN A USUAL CASE), LOWER, UPPER, FIRST UP */
	for (MODE = 0; MODE < 5; ++MODE)
	{
		USE[MODE] = MODE > 0 || IS_KNOWN(CELLS[FIRST]->TEXT);
		USE[MODE] |= USUAL_CASE(CELLS[FIRST]->TEXT);
		CASE_VARIANT(CELLS[FIRST]->TEXT, MODE, VARIANT[MODE], CELL_MAX);
	}

	for (J = FIRST; J < COUNT; ++J)
	{
		double	TOP;

		if (!CELL_HAS(CELLS, J, B))
			continue ;

		TOP = -1E300;

		for (MODE = 0; MODE < 5; ++MODE)
		{
			if (!USE[MODE])
				continue ;

			FITS[J * 5 + MODE] = FIT(
				&READS[CELLS[J]->LINE],
				CELLS[J],
				VARIANT[MODE]
			);

			if (FITS[J * 5 + MODE] > TOP)
				TOP = FITS[J * 5 + MODE];
		}

		for (MODE = 0; MODE < 5; ++MODE)
		{
			if (!USE[MODE])
				continue ;

			FITS[J * 5 + MODE] -= TOP;

			if (FITS[J * 5 + MODE] < -CASE_CAP)
				FITS[J * 5 + MODE] = -CASE_CAP;
		}
	}

	BEST = 1;
	BEST_SCORE = -1E300;

	for (MODE = 0; MODE < 5; ++MODE)
	{
		double	SCORE;

		if (!USE[MODE])
			continue ;

		SCORE = 0.0;
		SAME = 0;

		for (J = FIRST; J < COUNT; ++J)
		{
			if (!CELL_HAS(CELLS, J, B))
				continue ;

			SCORE += FITS[J * 5 + MODE];
			++SAME;
		}

		/*
		# PER CELL. A KNOWN WORD AS IT IS KNOWN ("TL") UNLESS THE INK SAYS NO;
		# A KNOWN WORD SPELLED "Vizon" ONLY TELLS IT IS A WORD, NOT ITS CASE.
		*/
		SCORE /= SAME;

		if (
			MODE == 0 &&
			IS_KNOWN(CELLS[FIRST]->TEXT) &&
			strcmp(VARIANT[0], VARIANT[4]) != 0
		)
			SCORE += CASE_BONUS;

		/* ALL CAPITALS ONLY WHEN THE INK SAYS SO ("o" AND "O" LOOK ALIKE) */
		if (MODE == 2 || MODE == 3)
			SCORE -= UPPER_DOUBT;

		if (SCORE > BEST_SCORE)
		{
			BEST_SCORE = SCORE;
			BEST = MODE;
		}
	}

	for (J = COUNT - 1; J >= FIRST; --J)
		if (CELL_HAS(CELLS, J, B))
			strcpy(CELLS[J]->TEXT, VARIANT[BEST]);
}

/* THE SAME WORD HAS THE SAME CASE ALL OVER A COLUMN (SEE CHOOSE_CASE_OF) */
static void
	CHOOSE_CASES(struct S_READ *READS, struct S_CELL **CELLS, int COUNT)
{
	double	*FITS;
	int		I;

	FITS = (double *)malloc(((size_t)COUNT * 5 + 1) * sizeof(double));

	if (!FITS)
		return ;

	for (I = 0; I < COUNT; ++I)
	{
		char	B[CELL_MAX];
		int		J;

		if (CELLS[I]->LINE < 0 || !CELLS[I]->TEXT[0])
			continue ;

		/* THE FIRST CELL OF ITS WORD DECIDES FOR ALL OF THEM */
		BARE(CELLS[I]->TEXT, B, sizeof(B));

		for (J = 0; J < I && !CELL_HAS(CELLS, J, B); ++J)
		{}

		if (J == I)
			CHOOSE_CASE_OF(READS, CELLS, COUNT, I, FITS);
	}

	free(FITS);
}

/* ONE WORD CELL ALONE (A PAGE THAT IS NOT A LIST): ITS WORD, THEN ITS CASE */
static void
	CORRECT_WORD(
		const struct S_READ *R,
		struct S_CELL *C,
		const struct S_VOCAB *V
	)
{
	const char	*KNOWN;
	char		WORD[CELL_MAX];

	if (C->LINE < 0 || !C->TEXT[0])
		return ;

	CHOOSE_WORD(R, C, V);
	strcpy(WORD, C->TEXT);
	KNOWN = KNOWN_SPELLING(WORD);
	CHOOSE_CASE(R, C, WORD, KNOWN != NULL || USUAL_CASE(WORD), C->TEXT);
}

/*
# HOW LIKELY A TEXT IS ON THE INK OF A CELL (LOG), UPPER AND LOWER CASE
# TAKEN AS ONE; NO_SPACE: SPACES DO NOT COUNT, FOR CODES AND NUMBERS.
*/
static double
	LOOSE_FIT(
		const struct S_READ *R,
		const struct S_CELL *C,
		const char *TEXT,
		int NO_SPACE
	)
{
	char	COPY[CELL_MAX];
	int		LABELS[CELL_MAX];
	float	*FOLD;
	double	RESULT;
	int		N;

	snprintf(COPY, sizeof(COPY), "%s", TEXT);

	if (NO_SPACE)
		NO_SPACES(COPY);

	N = TO_CLASSES(COPY, LABELS, CELL_MAX);
	FOLD = FOLDED(R, C, NO_SPACE);
	RESULT = -1E30;

	if (N >= 0 && FOLD)
		RESULT = CTC_SCORE(FOLD, G__CLASSES__, 0, C->TO - C->FROM, LABELS, N);

	free(FOLD);
	return (RESULT);
}

/* 1 = THE TEXT OF A CELL IS NOT LIKELY ENOUGH (SEE LOOSE_FIT) */
static int
	DOUBT(const struct S_READ *R, const struct S_CELL *C, int NO_SPACE)
{
	if (C->LINE < 0 || !C->TEXT[0])
		return (0);

	/* (CODES AND NUMBERS ARE READ WITH RULES: THEY MAY FIT LESS WELL) */
	if (NO_SPACE)
		return (LOOSE_FIT(R, C, C->TEXT, 1) < CHECK_LOG_NUMBER);

	return (LOOSE_FIT(R, C, C->TEXT, 0) < CHECK_LOG);
}

int
	HAND_KNOWN(const char *TEXT)
{
	return (IS_KNOWN(TEXT));
}

/*
# THE WORD CELLS OF ONE COLUMN: THEIR WORDS, THEN ONE CASE PER WORD. TWO
# ROUNDS: THE SECOND ONE COUNTS THE WORDS AS THE FIRST ONE CORRECTED THEM
# ("Groy" IS "Gray", NOT "Grey", WHEN "Gray" CAME SIX TIMES).
*/
static void
	CORRECT_COLUMN(struct S_READ *READS, struct S_CELL **CELLS, int COUNT)
{
	struct S_VOCAB	V;
	char			(*READ)[CELL_MAX];
	int				ROUND;
	int				I;

	READ = malloc((size_t)(COUNT + 1) * sizeof(*READ));

	if (!READ)
		return ;

	for (I = 0; I < COUNT; ++I)
		strcpy(READ[I], CELLS[I]->TEXT);

	for (ROUND = 0; ROUND < 2; ++ROUND)
	{
		if (!MAKE_VOCAB(&V, CELLS, COUNT))
			break ;

		for (I = 0; I < COUNT; ++I)
		{
			if (CELLS[I]->LINE < 0 || !READ[I][0])
				continue ;

			strcpy(CELLS[I]->TEXT, READ[I]);
			CHOOSE_WORD(&READS[CELLS[I]->LINE], CELLS[I], &V);
		}

		FREE_VOCAB(&V);
	}

	free(READ);
	CHOOSE_CASES(READS, CELLS, COUNT);
}

/*
# THE CODES OF THE COLUMN THAT CELL I MAY REALLY HAVE: WHAT WAS READ, A CODE
# OF ANOTHER CELL ONE CHARACTER AWAY, OR TWO AWAY WHEN IT COMES TWICE OR MORE
# AND HAS 5 CHARACTERS OR MORE. INDEXES INTO V; RETURNS THEIR NUMBER.
*/
static int
	CODE_CHOICES(const struct S_VOCAB *V, const char *TEXT, int *DEST)
{
	int	COUNT;
	int	K;

	COUNT = 0;

	for (K = 0; K < V->COUNT && COUNT < CODE_CHOICES_MAX; ++K)
	{
		const int	APART = DISTANCE(V->WORD[K], TEXT);

		if (
			APART == 0 ||
			APART == 1 ||
			(APART == 2 && V->HITS[K] >= 2 && strlen(V->WORD[K]) >= 5)
		)
			DEST[COUNT++] = K;
	}

	return (COUNT);
}

/*
# A COLUMN OF CODES, ALL LINES TOGETHER: EVERY CELL TAKES ONE OF ITS CHOICES
# (CODE_CHOICES), FOR THE BEST SUM OF HOW WELL THE INK FITS, CODE_PRIOR *
# LOG(TIMES IN THE COLUMN), AND NEIGHBOUR_PRIOR FOR EVERY TWO LINES IN A
# ROW WITH THE SAME CODE: A LIST OFTEN HAS ONE CODE ON LINES NEXT TO EACH
# OTHER ("0098", "10078" UNDER "10098" ARE "10098"). CLOSE CODES ARE OFTEN
# REAL (1345, 1346), SO THE INK STILL DECIDES MOSTLY. A CELL THAT TOOK A
# CODE ITS INK FITS CODE_DOUBT WORSE THAN WHAT WAS READ IS MARKED.
*/
static void
	CORRECT_CODES(struct S_READ *READS, struct S_CELL **CELLS, int COUNT)
{
	const size_t	SLOTS = (size_t)COUNT * CODE_CHOICES_MAX + 1;
	struct S_VOCAB	V;
	int				*CHOICE;
	int				*CHOICES;
	int				*BACK;
	double			*FIT_OF;
	double			*SCORE;
	int				BEST;
	int				I;
	int				J;

	if (!MAKE_VOCAB(&V, CELLS, COUNT))
		return ;

	CHOICE = (int *)malloc(SLOTS * sizeof(int));
	CHOICES = (int *)calloc((size_t)COUNT + 1, sizeof(int));
	BACK = (int *)malloc(SLOTS * sizeof(int));
	FIT_OF = (double *)malloc(SLOTS * sizeof(double));
	SCORE = (double *)malloc(SLOTS * sizeof(double));

	if (!CHOICE || !CHOICES || !BACK || !FIT_OF || !SCORE)
	{
		free(CHOICE);
		free(CHOICES);
		free(BACK);
		free(FIT_OF);
		free(SCORE);
		FREE_VOCAB(&V);
		return ;
	}

	/* FORWARD: THE BEST SUM UP TO EVERY CHOICE OF EVERY CELL */
	for (I = 0; I < COUNT; ++I)
	{
		struct S_CELL	*C = CELLS[I];
		int				*MINE = &CHOICE[I * CODE_CHOICES_MAX];

		if (C->LINE < 0 || !C->TEXT[0])
			continue ;

		CHOICES[I] = CODE_CHOICES(&V, C->TEXT, MINE);

		for (J = 0; J < CHOICES[I]; ++J)
		{
			const int	K = MINE[J];
			double		BEST_BEFORE;
			int			P;

			FIT_OF[I * CODE_CHOICES_MAX + J] = LOOSE_FIT(
				&READS[C->LINE],
				C,
				V.WORD[K],
				1
			);
			BEST_BEFORE = 0.0;
			BACK[I * CODE_CHOICES_MAX + J] = -1;

			for (P = 0; I > 0 && P < CHOICES[I - 1]; ++P)
			{
				double	BEFORE;

				BEFORE = SCORE[(I - 1) * CODE_CHOICES_MAX + P];

				if (CHOICE[(I - 1) * CODE_CHOICES_MAX + P] == K)
					BEFORE += NEIGHBOUR_PRIOR;

				if (BACK[I * CODE_CHOICES_MAX + J] < 0 || BEFORE > BEST_BEFORE)
				{
					BEST_BEFORE = BEFORE;
					BACK[I * CODE_CHOICES_MAX + J] = P;
				}
			}

			SCORE[I * CODE_CHOICES_MAX + J] = (
				BEST_BEFORE +
				FIT_OF[I * CODE_CHOICES_MAX + J] +
				CODE_PRIOR * log((double)V.HITS[K] + 1.0)
			);
		}
	}

	/* BACK: FROM THE BEST CHOICE OF THE LAST CELL TO THE FIRST */
	BEST = -1;

	for (I = COUNT - 1; I >= 0; --I)
	{
		struct S_CELL	*C = CELLS[I];
		double			OWN;
		int				PICK;

		if (!CHOICES[I])
		{
			BEST = -1;
			continue ;
		}

		PICK = BEST;

		if (PICK < 0)
		{
			PICK = 0;

			for (J = 1; J < CHOICES[I]; ++J)
				if (
					SCORE[I * CODE_CHOICES_MAX + J] >
					SCORE[I * CODE_CHOICES_MAX + PICK]
				)
					PICK = J;
		}

		/* WHAT WAS READ IS ALWAYS ONE OF THE CHOICES */
		OWN = FIT_OF[I * CODE_CHOICES_MAX + PICK];

		for (J = 0; J < CHOICES[I]; ++J)
		{
			const int	K = CHOICE[I * CODE_CHOICES_MAX + J];

			if (DISTANCE(V.WORD[K], C->TEXT) == 0)
				OWN = FIT_OF[I * CODE_CHOICES_MAX + J];
		}

		C->CHECK = OWN - FIT_OF[I * CODE_CHOICES_MAX + PICK] > CODE_DOUBT;
		strcpy(C->TEXT, V.WORD[CHOICE[I * CODE_CHOICES_MAX + PICK]]);
		BEST = BACK[I * CODE_CHOICES_MAX + PICK];
	}

	free(CHOICE);
	free(CHOICES);
	free(BACK);
	free(FIT_OF);
	free(SCORE);
	FREE_VOCAB(&V);
}

/*
# HOW A WORD (BARE) IS SPELLED IN THE COLUMN OF CELL X, OR ELSE IN ANY OTHER
# CELL OF ALL; NULL = NOWHERE
*/
static const char
	*SAME_SPELLING(
		struct S_CELL *const *ALL,
		int N,
		int COLS,
		const struct S_CELL *CELLS,
		const struct S_CELL *X,
		const char *WORD
	)
{
	const char	*ANY;
	int			I;

	ANY = NULL;

	for (I = 0; I < N; ++I)
	{
		char	B[CELL_MAX];

		if (ALL[I] == X || ALL[I]->LINE < 0)
			continue ;

		BARE(ALL[I]->TEXT, B, sizeof(B));

		if (strcmp(B, WORD) != 0)
			continue ;

		if ((ALL[I] - CELLS) % COLS == (X - CELLS) % COLS)
			return (ALL[I]->TEXT);

		if (!ANY)
			ANY = ALL[I]->TEXT;
	}

	return (ANY);
}

/*
# THE WORD CELLS OF THE WHOLE LIST AGAINST EACH OTHER: A WORD READ ONCE IS
# A WORD THE SAME HAND WROTE AGAIN AND AGAIN IN ANOTHER COLUMN ("C(aam" IS
# "Cream"), SEE SAME_HAND. WORDY[C]: 1 = COLUMN C HOLDS WORDS.
*/
static void
	CORRECT_PAGE(
		struct S_READ *READS,
		int COUNT,
		struct S_CELL *CELLS,
		int COLS,
		const unsigned char *WORDY
	)
{
	struct S_CELL	**ALL;
	struct S_VOCAB	V;
	int				N;
	int				I;
	int				C;

	ALL = (struct S_CELL **)calloc((size_t)COUNT * COLS + 1, sizeof(*ALL));

	if (!ALL)
		return ;

	N = 0;

	for (I = 0; I < COUNT; ++I)
		for (C = 0; C < COLS; ++C)
			if (READS[I].FIELDS && WORDY[C])
				ALL[N++] = &CELLS[I * COLS + C];

	if (!MAKE_VOCAB(&V, ALL, N))
	{
		free(ALL);
		return ;
	}

	for (I = 0; I < N; ++I)
	{
		struct S_CELL	*X = ALL[I];
		struct S_GUESS	G;
		const char		*SPELLING;
		float			*FOLD;
		const int		OWN_HITS = HITS_OF(&V, X->TEXT);

		if (
			X->LINE < 0 ||
			strlen(X->TEXT) < 4 ||
			IS_KNOWN(X->TEXT) ||
			OWN_HITS >= 2
		)
			continue ;

		FOLD = FOLDED(&READS[X->LINE], X, 0);

		if (!FOLD)
			continue ;

		G.TEXT[0] = '\0';
		SAME_HAND(FOLD, X->TO - X->FROM, &V, OWN_HITS, X, &G);
		free(FOLD);

		if (!G.TEXT[0])
			continue ;

		/* SPELLED AS IN ITS OWN COLUMN, OR ELSE ANYWHERE ON THE PAGE */
		SPELLING = SAME_SPELLING(ALL, N, COLS, CELLS, X, G.BARE);

		if (!SPELLING)
			SPELLING = KNOWN_SPELLING(G.TEXT);

		if (SPELLING)
			snprintf(X->TEXT, CELL_MAX, "%s", SPELLING);
		else
			strcpy(X->TEXT, G.TEXT);
	}

	FREE_VOCAB(&V);
	free(ALL);
}

/* 1 = ONLY DIGITS */
static int
	ALL_DIGITS(const char *TEXT)
{
	if (!*TEXT)
		return (0);

	for (; *TEXT; ++TEXT)
		if (*TEXT < '0' || *TEXT > '9')
			return (0);

	return (1);
}

/*
# 1 = A SIZE, NOT A NUMBER: A DIGIT, THEN "x" (AFTER ' OR " MAYBE), THEN A
# DIGIT ("60x90", "2'3\"x8'", "240 X 305"). IT STAYS AS IT WAS READ.
*/
static int
	IS_SIZE(const char *TEXT)
{
	const char	*P;

	for (P = TEXT; *P; ++P)
	{
		const char	*BEFORE;
		const char	*AFTER;

		if (*P != 'x' && *P != 'X')
			continue ;

		BEFORE = P;

		while (
			BEFORE > TEXT &&
			(BEFORE[-1] == '\'' || BEFORE[-1] == '"' || BEFORE[-1] == ' ')
		)
			--BEFORE;

		AFTER = P + 1;

		while (*AFTER == ' ')
			++AFTER;

		if (
			BEFORE > TEXT &&
			BEFORE[-1] >= '0' &&
			BEFORE[-1] <= '9' &&
			*AFTER >= '0' &&
			*AFTER <= '9'
		)
			return (1);
	}

	return (0);
}

/*
# A COLUMN OF WHOLE AMOUNTS (WHOLE_SHARE OF THEM ONLY DIGITS): A "." OR ","
# IN ONE OF THEM IS A SPECK ("21." IS "21", "2.9" IS "29"). THE CELL IS
# READ AGAIN WITH DIGITS ONLY.
*/
static void
	WHOLE_AMOUNTS(struct S_READ *READS, struct S_CELL **CELLS, int COUNT)
{
	int	FILLED;
	int	WHOLE;
	int	I;

	FILLED = 0;
	WHOLE = 0;

	for (I = 0; I < COUNT; ++I)
	{
		if (CELLS[I]->LINE < 0 || !CELLS[I]->TEXT[0])
			continue ;

		++FILLED;
		WHOLE += ALL_DIGITS(CELLS[I]->TEXT);
	}

	if (FILLED < 3 || (float)WHOLE < WHOLE_SHARE * (float)FILLED)
		return ;

	for (I = 0; I < COUNT; ++I)
	{
		struct S_CELL	*C = CELLS[I];
		struct S_GLYPH	G[CELL_MAX];
		int				N;

		if (C->LINE < 0 || !C->TEXT[0] || ALL_DIGITS(C->TEXT))
			continue ;

		if (!strpbrk(C->TEXT, "0123456789") || strpbrk(C->TEXT, "/-"))
			continue ;

		N = CTC_GREEDY(
			READS[C->LINE].LOGP,
			G__CLASSES__,
			C->FROM,
			C->TO,
			G__DIGIT__,
			G,
			CELL_MAX
		);

		if (N > 0)
			GLYPH_TEXT(G, 0, N - 1, C->TEXT, sizeof(C->TEXT));
	}
}

/*
# THE NUMBERS OF A COLUMN SHOULD ADD UP TO THE TOTAL. WHEN THEY DO NOT, THE
# ONE NUMBER THAT WOULD MAKE THE SUM RIGHT WITH ONE DIGIT CHANGED, ADDED OR
# TAKEN AWAY, AND WHOSE INK FITS THAT BEST, IS CHANGED (WHEN IT FITS WELL
# ENOUGH). OR THE TOTAL ITSELF, WHEN IT WAS THE ONE THAT WAS READ WRONG.
*/
static void
	FIX_SUM(
		struct S_READ *READS,
		struct S_CELL **CELLS,
		int COUNT,
		struct S_CELL *TOTAL
	)
{
	struct S_CELL	*BEST_CELL;
	char			BEST_TEXT[CELL_MAX];
	char			TRY[CELL_MAX];
	double			BEST_LOSS;
	long			SUM;
	long			WANT;
	int				I;

	WANT = WHOLE_NUMBER(TOTAL->TEXT);
	SUM = 0;

	for (I = 0; I < COUNT; ++I)
	{
		const long	V = WHOLE_NUMBER(CELLS[I]->TEXT);

		if (CELLS[I]->LINE < 0)
			continue ;

		if (V < 0)
			return ;

		SUM += V;
	}

	if (WANT < 0 || SUM == WANT || SUM <= 0)
		return ;

	BEST_CELL = NULL;
	BEST_LOSS = SUM_LOSS;

	for (I = 0; I < COUNT; ++I)
	{
		struct S_CELL	*C = CELLS[I];
		const long		NEED = WANT - (SUM - WHOLE_NUMBER(C->TEXT));
		double			LOSS;

		if (C->LINE < 0 || NEED < 0)
			continue ;

		snprintf(TRY, sizeof(TRY), "%ld", NEED);

		if (DISTANCE(TRY, C->TEXT) > 1)
			continue ;

		LOSS = FIT(&READS[C->LINE], C, C->TEXT) - FIT(&READS[C->LINE], C, TRY);

#ifdef HAND_DEBUG
		fprintf(stderr, "  sum: %s > %s loss %.2f\n", C->TEXT, TRY, LOSS);
#endif /* HAND_DEBUG */

		if (LOSS < BEST_LOSS)
		{
			BEST_LOSS = LOSS;
			BEST_CELL = C;
			strcpy(BEST_TEXT, TRY);
		}
	}

	/* THE TOTAL READ WRONG? (THE WRITER'S OWN SUM IS USUALLY RIGHT) */
	snprintf(TRY, sizeof(TRY), "%ld", SUM);

	if (TOTAL->LINE >= 0 && DISTANCE(TRY, TOTAL->TEXT) <= 1)
	{
		const struct S_READ	*R = &READS[TOTAL->LINE];
		const double		NOW = FIT(R, TOTAL, TOTAL->TEXT);
		const double		LOSS = NOW - FIT(R, TOTAL, TRY) + TOTAL_TRUST;

#ifdef HAND_DEBUG
		fprintf(stderr, "  sum: total %s > %s %.2f\n", TOTAL->TEXT, TRY, LOSS);
#endif /* HAND_DEBUG */

		if (LOSS < BEST_LOSS)
		{
			BEST_LOSS = LOSS;
			BEST_CELL = TOTAL;
			strcpy(BEST_TEXT, TRY);
		}
	}

	if (BEST_CELL)
		strcpy(BEST_CELL->TEXT, BEST_TEXT);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          WORDS                           |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          LISTS                           |  \\//  */
/*   \/   #==========================================================#   \/   */
/*
# A LIST PAGE: MOST LINES ARE "A - B - C - D" WITH THE SAME NUMBER OF DASHES.
# EVERY LINE VOTES FOR THE NUMBER OF FIELDS THAT COSTS IT LEAST (THE BIGGER
# ONE WHEN TWO COST ABOUT THE SAME: A FAINT DASH IS STILL A DASH). 0 = NOT A
# LIST PAGE.
*/
static int
	LIST_FIELDS(struct S_READ *READS, int COUNT)
{
	int	VOTES[FIELDS_MAX + 1];
	int	FIELDS;
	int	LINES;
	int	TEXT_LINES;
	int	K;
	int	I;

	memset(VOTES, 0, sizeof(VOTES));
	TEXT_LINES = 0;

	for (I = 0; I < COUNT; ++I)
	{
		struct S_READ	*R = &READS[I];
		int				BEST;

		if (R->DROP)
			continue ;

		LIST_COSTS(R);
		BEST = 0;
		++TEXT_LINES;

		for (K = 1; K < FIELDS_MAX; ++K)
			if (R->COST[K] <= R->COST[BEST] + DASH_TOLERANCE)
				BEST = K;

		VOTES[BEST + 1] += 1;
	}

	FIELDS = 1;

	for (K = 2; K <= FIELDS_MAX; ++K)
		if (VOTES[K] > VOTES[FIELDS])
			FIELDS = K;

	/*
	# MORE FIELDS WIN WHEN A QUARTER OF THE LINES HAVE THEM: A SHORTER LINE
	# FITS INTO MORE COLUMNS (ONE LEFT EMPTY), A LONGER ONE DOES NOT FIT
	*/
	for (K = FIELDS + 1; K <= FIELDS_MAX && FIELDS >= 2; ++K)
		if (VOTES[K] >= 2 && VOTES[K] * 4 >= TEXT_LINES)
			FIELDS = K;

#ifdef HAND_DEBUG
	for (I = 0; I < COUNT; ++I)
	{
		char	T[512];

		GLYPH_TEXT(READS[I].GLYPH, 0, READS[I].COUNT - 1, T, sizeof(T));
		fprintf(stderr, "%2d", I);

		for (K = 0; K < FIELDS_MAX; ++K)
			fprintf(stderr, " %6.1f", READS[I].COST[K]);

		fprintf(stderr, "  %s\n", T);
	}

	fprintf(stderr, "fields %d\n", FIELDS);
#endif /* HAND_DEBUG */

	if (FIELDS < 2)
		return (0);

	LINES = 0;

	for (I = 0; I < COUNT; ++I)
		LINES += !READS[I].DROP && READS[I].COST[FIELDS - 1] < LIST_COST;

	if (LINES < 2 || LINES * 3 < TEXT_LINES)
		return (0);

	return (FIELDS);
}

/*
# THE LIST LINES: A LINE WITH AT LEAST ONE DASH THAT COSTS LESS THAN
# LIST_COST. IT TAKES THE MOST FIELDS (UP TO FIELDS) THAT COST AT MOST
# LINE_MARGIN MORE THAN ITS BEST: A FAINT DASH IS STILL A DASH, BUT
# "perşembe" IS NOT "perşe - mbe". FEWER FIELDS LEAVE A COLUMN EMPTY.
*/
static void
	MARK_LIST_LINES(struct S_READ *READS, int COUNT, int FIELDS)
{
	int	I;

	for (I = 0; I < COUNT; ++I)
	{
		struct S_READ	*R = &READS[I];
		int				BEST;
		int				K;

		R->FIELDS = 0;

		if (R->DROP)
			continue ;

		BEST = 1;

		for (K = 2; K < FIELDS; ++K)
			if (R->COST[K] < R->COST[BEST])
				BEST = K;

		if (R->COST[BEST] >= LIST_COST)
			continue ;

		R->FIELDS = BEST + 1;

		for (K = BEST + 1; K < FIELDS; ++K)
			if (R->COST[K] <= R->COST[BEST] + LINE_MARGIN)
				R->FIELDS = K + 1;

		/*
		# A LINE THAT DOES NOT FIT ALL FIELDS NEEDS DASHES THAT FIT BETTER
		# THAN NO DASHES AT ALL (A TITLE OR A TOTAL IS NOT A SHORT LIST LINE)
		*/
		if (R->COST[FIELDS - 1] >= LIST_COST && R->COST[BEST] >= R->COST[0])
			R->FIELDS = 0;
	}
}

/* THE COLUMN OF FIELD F OF A LINE OF N FIELDS (THE LAST ONE GOES LAST) */
static int
	COLUMN_OF(int F, int N, int FIELDS)
{
	if (F == N - 1)
		return (FIELDS - 1);

	return (F);
}

/*
# WHAT KIND OF COLUMN, FROM WHAT WAS READ WITHOUT RULES. *DIGITS: 1 = HALF
# OF ITS CELLS OR MORE ARE DIGITS WITH AT MOST ONE LETTER.
*/
static int
	COLUMN_KIND(
		struct S_READ *READS,
		int COUNT,
		int FIELDS,
		int COL,
		int *DIGITS_MOSTLY
	)
{
	int	FILLED;
	int	NUMBERS;
	int	CODES;
	int	PAIRS;
	int	WORDY;
	int	DIGIT_FIRST;
	int	I;

	FILLED = 0;
	NUMBERS = 0;
	CODES = 0;
	PAIRS = 0;
	WORDY = 0;
	DIGIT_FIRST = 0;

	for (I = 0; I < COUNT; ++I)
	{
		struct S_CELL	C;
		const char		*P;
		int				DIGITS;
		int				LETTERS;
		int				F;

		if (!READS[I].FIELDS)
			continue ;

		for (F = 0; F < READS[I].FIELDS; ++F)
			if (COLUMN_OF(F, READS[I].FIELDS, FIELDS) == COL)
				break ;

		if (F == READS[I].FIELDS)
			continue ;

		FIELD_CELL(&READS[I], I, F, -1, &C);

		if (C.LINE < 0)
			continue ;

		++FILLED;
		DIGITS = 0;
		LETTERS = 0;

		for (P = C.TEXT; *P; ++P)
		{
			DIGITS += *P >= '0' && *P <= '9';
			LETTERS += (*P >= 'A' && *P <= 'Z') || (*P >= 'a' && *P <= 'z');
			LETTERS += (unsigned char)*P >= 0XC0;
		}

		/* "12 TOP": DIGITS FIRST, THEN LETTERS */
		if (C.TEXT[0] >= '0' && C.TEXT[0] <= '9' && LETTERS >= 2)
			++PAIRS;

		DIGIT_FIRST += C.TEXT[0] >= '0' && C.TEXT[0] <= '9';

		if (DIGITS && LETTERS <= 1)
			++NUMBERS;

		if (DIGITS >= 2)
			++CODES;

		if (LETTERS >= 2 && !DIGITS)
			++WORDY;
	}

	*DIGITS_MOSTLY = FILLED && NUMBERS * 2 >= FILLED;

	if (!FILLED || WORDY * 2 >= FILLED)
		return (KIND_TEXT);

	if (PAIRS * 10 >= FILLED * 6)
		return (KIND_NUMBER_UNIT);

	/*
	# THE LAST COLUMN, MOSTLY STARTING WITH A NUMBER, WITH A UNIT IN 40% OF
	# ITS CELLS: THE UNIT OF THE OTHERS WAS READ BADLY ("12 707" IS "12 Top")
	*/
	if (
		COL > 0 &&
		COL == FIELDS - 1 &&
		PAIRS * 10 >= FILLED * 4 &&
		DIGIT_FIRST * 10 >= FILLED * 6
	)
		return (KIND_NUMBER_UNIT);

	if (NUMBERS * 10 >= FILLED * 9)
		return (KIND_NUMBER);

	if (CODES * 10 >= FILLED * 6)
		return (KIND_CODE);

	return (KIND_TEXT);
}

/*
# THE KINDS OF THE FIELDS OF ONE LINE. IN A COLUMN OF CODES, A CODE THAT WAS
# READ WITH AT MOST ONE LETTER IS READ AS DIGITS ONLY ("1O098" IS "10098").
*/
static void
	LINE_KINDS(
		const struct S_READ *R,
		int LINE,
		int FIELDS,
		const int *COL_KIND,
		int *KINDS
	)
{
	int	F;

	for (F = 0; F < R->FIELDS; ++F)
	{
		struct S_CELL	C;
		const char		*P;
		int				LETTERS;
		int				DIGITS;

		KINDS[F] = COL_KIND[COLUMN_OF(F, R->FIELDS, FIELDS)];

		if (KINDS[F] != KIND_CODE)
			continue ;

		FIELD_CELL(R, LINE, F, -1, &C);
		LETTERS = 0;
		DIGITS = 0;

		for (P = C.TEXT; *P; ++P)
		{
			DIGITS += *P >= '0' && *P <= '9';
			LETTERS += (*P >= 'A' && *P <= 'Z') || (*P >= 'a' && *P <= 'z');
		}

		if (LETTERS <= 1 && DIGITS >= 3)
			KINDS[F] = KIND_NUMBER;
	}
}

/* LETTERS READ IN THE FIELDS OF A LINE THAT ARE CODES */
static int
	CODE_LETTERS(const struct S_READ *R, const int *KINDS)
{
	int	COUNT;
	int	I;

	COUNT = 0;

	for (I = 0; I < R->COUNT; ++I)
		if (R->TAG[I] >= 0 && KINDS[R->TAG[I] / 4] == KIND_CODE)
			COUNT += G__LETTER__[R->GLYPH[I].CLASS];

	return (COUNT);
}

/*
# A LIST LINE READ WITH THE RULES OF ITS COLUMNS. A CODE IN A COLUMN OF
# MOSTLY DIGITS IS READ AS DIGITS WHEN THAT COSTS LITTLE: AT MOST
# LETTER_COST FOR EVERY LETTER IT HAD ("1OL0S" IS "10105", "SLM861" STAYS).
*/
static void
	READ_TYPED(
		struct S_READ *R,
		int LINE,
		int FIELDS,
		const int *COL_KIND,
		const int *DIGITS_MOSTLY
	)
{
	struct S_GRAMMAR	G;
	int					KINDS[FIELDS_MAX];
	int					LETTERS;
	int					ANY;
	double				SCORE;
	int					F;

	LINE_KINDS(R, LINE, FIELDS, COL_KIND, KINDS);
	LIST_GRAMMAR(&G, R->FIELDS, KINDS);
	SCORE = READ_GRAMMAR(R, &G);
	LETTERS = CODE_LETTERS(R, KINDS);
	memcpy(R->KINDS, KINDS, sizeof(KINDS));
	R->SCORE = SCORE;
	ANY = 0;

	for (F = 0; F < R->FIELDS; ++F)
	{
		if (
			KINDS[F] == KIND_CODE &&
			DIGITS_MOSTLY[COLUMN_OF(F, R->FIELDS, FIELDS)]
		)
		{
			KINDS[F] = KIND_NUMBER;
			ANY = 1;
		}
	}

	if (!ANY || !LETTERS)
		return ;

	LIST_GRAMMAR(&G, R->FIELDS, KINDS);

	if (SCORE_GRAMMAR(R, &G) > SCORE - LETTER_COST * LETTERS)
	{
		R->SCORE = READ_GRAMMAR(R, &G);
		memcpy(R->KINDS, KINDS, sizeof(KINDS));
	}
}

/*
# A LINE WHOSE UNIT IS NONE OF THE UNITS OF THE LIST ("12 To?" READ AS
# "12707"), READ AGAIN WITH THEM, WHEN THAT COSTS LESS THAN STRICT_ROW.
# 1 = READ AGAIN.
*/
static int
	READ_KNOWN_UNIT(struct S_READ *R)
{
	struct S_GRAMMAR	G;
	int					KINDS[FIELDS_MAX];
	int					F;

	memcpy(KINDS, R->KINDS, sizeof(KINDS));

	for (F = 0; F < R->FIELDS; ++F)
		if (KINDS[F] == KIND_NUMBER_UNIT)
			KINDS[F] = KIND_NUMBER_KNOWN_UNIT;

	LIST_GRAMMAR(&G, R->FIELDS, KINDS);

	if (SCORE_GRAMMAR(R, &G) <= R->SCORE - STRICT_ROW)
		return (0);

	R->SCORE = READ_GRAMMAR(R, &G);
	memcpy(R->KINDS, KINDS, sizeof(KINDS));
	return (1);
}

/* A TABLE CELL (NULL WHEN EMPTY) */
static char
	*CELL_COPY(const struct S_CELL *C)
{
	char	*COPY;

	if (C->LINE < 0 || !C->TEXT[0])
		return (NULL);

	COPY = (char *)malloc(strlen(C->TEXT) + 1);

	if (COPY)
		strcpy(COPY, C->TEXT);

	return (COPY);
}

/* 1 = TEXT IS ONE OF THE UNITS (CASE AND SPACES LEFT OUT) */
static int
	IS_UNIT(const struct S_UNITS *U, const char *TEXT)
{
	char	A[CELL_MAX];
	int		K;

	BARE(TEXT, A, sizeof(A));

	for (K = 0; K < U->COUNT; ++K)
	{
		char	B[CELL_MAX];

		BARE(U->WORD[K], B, sizeof(B));

		if (A[0] && strcmp(A, B) == 0)
			return (1);
	}

	return (0);
}

/* THE LETTERS AND DIGITS OF A LINE */
static int
	ALNUM_GLYPHS(const struct S_READ *R)
{
	int	ALNUM;
	int	K;

	ALNUM = 0;

	for (K = 0; K < R->COUNT; ++K)
		ALNUM += G__ALNUM__[R->GLYPH[K].CLASS];

	return (ALNUM);
}

/* 1 = LINE I HAS ONE LETTER OR DIGIT AT MOST, AND LIST LINES ABOVE AND BELOW */
static int
	STRAY_LINE(const struct S_READ *READS, int COUNT, int I)
{
	int	ABOVE;
	int	BELOW;
	int	K;

	if (ALNUM_GLYPHS(&READS[I]) > 1)
		return (0);

	ABOVE = 0;
	BELOW = 0;

	for (K = 0; K < I; ++K)
		ABOVE |= READS[K].FIELDS != 0;

	for (K = I + 1; K < COUNT; ++K)
		BELOW |= READS[K].FIELDS != 0;

	return (ABOVE && BELOW);
}

/* A DATE: 1 OR 2 DIGITS, A MARK, 1 OR 2 DIGITS, A MARK, 2 OR 4 DIGITS */
static void
	DATE_GRAMMAR(struct S_GRAMMAR *G)
{
	int	NODES[11];
	int	K;

	G->NODES = 0;
	G->ARCS = 0;

	for (K = 0; K < 11; ++K)
		NODES[K] = NODE(G, PART_MAIN);

	/* 0 SPACES, 1 2 DAY, 3 MARK, 4 5 MONTH, 6 MARK, 7 8 9 10 YEAR */
	ARC(G, NODES[0], NODES[0], G__SPACE_ONLY__);
	ARC(G, NODES[0], NODES[1], G__DIGIT__);
	ARC(G, NODES[1], NODES[2], G__DIGIT__);
	ARC(G, NODES[1], NODES[3], G__DATE_MARK__);
	ARC(G, NODES[2], NODES[3], G__DATE_MARK__);
	ARC(G, NODES[3], NODES[4], G__DIGIT__);
	ARC(G, NODES[4], NODES[5], G__DIGIT__);
	ARC(G, NODES[4], NODES[6], G__DATE_MARK__);
	ARC(G, NODES[5], NODES[6], G__DATE_MARK__);
	ARC(G, NODES[6], NODES[7], G__DIGIT__);
	ARC(G, NODES[7], NODES[8], G__DIGIT__);
	ARC(G, NODES[8], NODES[9], G__DIGIT__);
	ARC(G, NODES[9], NODES[10], G__DIGIT__);
	G->FINAL[NODES[8]] = 1;
	G->FINAL[NODES[10]] = 1;
}

/* A TIME OF DAY: 1 OR 2 DIGITS (2 ONLY: TWO_HOURS), ":" OR ".", 2 DIGITS */
static void
	TIME_GRAMMAR(struct S_GRAMMAR *G, int TWO_HOURS)
{
	int	NODES[6];
	int	K;

	G->NODES = 0;
	G->ARCS = 0;

	for (K = 0; K < 6; ++K)
		NODES[K] = NODE(G, PART_MAIN);

	/* 0 SPACES, 1 2 HOURS, 3 MARK, 4 5 MINUTES */
	ARC(G, NODES[0], NODES[0], G__SPACE_ONLY__);
	ARC(G, NODES[0], NODES[1], G__DIGIT__);
	ARC(G, NODES[1], NODES[2], G__DIGIT__);
	ARC(G, NODES[2], NODES[3], G__TIME_MARK__);

	if (!TWO_HOURS)
		ARC(G, NODES[1], NODES[3], G__TIME_MARK__);

	ARC(G, NODES[3], NODES[4], G__DIGIT__);
	ARC(G, NODES[4], NODES[5], G__DIGIT__);
	ARC(G, NODES[5], NODES[5], G__SPACE_ONLY__);
	G->FINAL[NODES[5]] = 1;
}

/* DIGITS ONLY (SPACES AROUND THEM) */
static void
	DIGITS_GRAMMAR(struct S_GRAMMAR *G)
{
	int	NODES[3];
	int	K;

	G->NODES = 0;
	G->ARCS = 0;

	for (K = 0; K < 3; ++K)
		NODES[K] = NODE(G, PART_MAIN);

	ARC(G, NODES[0], NODES[0], G__SPACE_ONLY__);
	ARC(G, NODES[0], NODES[1], G__DIGIT__);
	ARC(G, NODES[1], NODES[1], G__DIGIT__);
	ARC(G, NODES[1], NODES[2], G__SPACE_ONLY__);
	ARC(G, NODES[2], NODES[2], G__SPACE_ONLY__);
	G->FINAL[NODES[1]] = 1;
	G->FINAL[NODES[2]] = 1;
}

/*
# A SHORT CODE OF LENGTH GLYPHS (2 TO 4): A LETTER, THEN DIGITS ONLY
# (PAIRED, "A3") OR LETTERS AND DIGITS ("KN", "T1")
*/
static void
	CODE_GRAMMAR(struct S_GRAMMAR *G, int LENGTH, int PAIRED)
{
	int	NODES[6];
	int	K;

	G->NODES = 0;
	G->ARCS = 0;

	for (K = 0; K <= LENGTH; ++K)
		NODES[K] = NODE(G, PART_MAIN);

	ARC(G, NODES[0], NODES[0], G__SPACE_ONLY__);
	ARC(G, NODES[0], NODES[1], G__LETTER__);

	for (K = 1; K < LENGTH; ++K)
	{
		if (PAIRED)
			ARC(G, NODES[K], NODES[K + 1], G__DIGIT__);
		else
			ARC(G, NODES[K], NODES[K + 1], G__ALNUM__);
	}

	ARC(G, NODES[LENGTH], NODES[LENGTH], G__SPACE_ONLY__);
	G->FINAL[NODES[LENGTH]] = 1;
}

/*
# HOW MUCH WORSE A CELL FITS A GRAMMAR THAN THE BEST WAY TO READ IT (LOG OF
# HOW LIKELY), AND ITS TEXT THAT WAY (NO SPACES). 1E30 = IT DOES NOT FIT.
*/
static double
	GRAMMAR_COST(
		const struct S_READ *R,
		const struct S_CELL *C,
		const struct S_GRAMMAR *G,
		char *TEXT,
		size_t SIZE
	)
{
	struct S_GLYPH	GLYPHS[CELL_MAX];
	double			BEST;
	double			SCORE;
	int				N;
	int				T;

	BEST = 0.0;

	for (T = C->FROM; T < C->TO; ++T)
	{
		const float	*ROW = R->LOGP + (size_t)T * G__CLASSES__;
		float		MAX;
		int			K;

		MAX = ROW[0];

		for (K = 1; K < G__CLASSES__; ++K)
			if (ROW[K] > MAX)
				MAX = ROW[K];

		BEST += MAX;
	}

	SCORE = CTC_GRAMMAR(
		R->LOGP,
		G__CLASSES__,
		C->FROM,
		C->TO,
		G,
		GLYPHS,
		NULL,
		CELL_MAX,
		&N
	);

	if (SCORE < -1E29 || N < 1)
		return (1E30);

	GLYPH_TEXT(GLYPHS, 0, N - 1, TEXT, SIZE);
	NO_SPACES(TEXT);
	return (BEST - SCORE);
}

/*
# A TITLE THAT IS A DATE IS READ AS ONE ("23,10.2024" IS "23.10.2024"),
# WHEN THAT COSTS AT MOST DATE_COST MORE THAN ANY OTHER WAY TO READ IT
*/
static void
	READ_DATE(struct S_READ *R)
{
	struct S_GRAMMAR	G;
	char				TEXT[CELL_MAX];
	int					DIGITS;
	int					I;

	if (R->DROP || R->FIELDS || !R->COUNT)
		return ;

	GLYPH_TEXT(R->GLYPH, 0, R->COUNT - 1, TEXT, sizeof(TEXT));
	DIGITS = 0;

	for (I = 0; TEXT[I]; ++I)
		DIGITS += TEXT[I] >= '0' && TEXT[I] <= '9';

	if (DIGITS < 4)
		return ;

	DATE_GRAMMAR(&G);

	if (R->BEST - SCORE_GRAMMAR(R, &G) <= DATE_COST)
		READ_GRAMMAR(R, &G);
}

/* THE CELLS OF ONE LIST LINE (OR A TITLE: ALL OF IT IN THE FIRST COLUMN) */
static void
	LIST_ROW(
		const struct S_READ *R,
		int LINE,
		int FIELDS,
		const int *COL_KIND,
		const int *OUT_COL,
		struct S_CELL *ROW
	)
{
	int	F;

	if (!R->FIELDS)
	{
		MAKE_CELL(R, LINE, 0, R->COUNT - 1, &ROW[0]);
		return ;
	}

	for (F = 0; F < R->FIELDS; ++F)
	{
		const int		COL = COLUMN_OF(F, R->FIELDS, FIELDS);
		struct S_CELL	*C = &ROW[OUT_COL[COL]];

		if (COL_KIND[COL] == KIND_NUMBER_UNIT)
		{
			FIELD_CELL(R, LINE, F, PART_MAIN, C);
			FIELD_CELL(R, LINE, F, PART_UNIT, C + 1);
		}
		else
			FIELD_CELL(R, LINE, F, -1, C);

		if (COL_KIND[COL] != KIND_TEXT)
			NO_SPACES(C->TEXT);

		/* CODES ARE WRITTEN IN CAPITALS ("SLM861") */
		if (COL_KIND[COL] == KIND_CODE)
		{
			char	UP[CELL_MAX];

			CASE_VARIANT(C->TEXT, 2, UP, sizeof(UP));
			strcpy(C->TEXT, UP);
		}
	}
}

/*
# THE LINE OF THE TOTAL UNDER THE LAST LIST LINE (LAST): OF THE LINES WITH
# A DIGIT, OR THAT READ AS A NUMBER AND A UNIT OF THE LIST FOR AT MOST
# TOTAL_FIT ("1ta iaı" IS "170 Top"), THE LONGEST (NOT A BIT OF THE RULE
# ABOVE IT). -1 = NONE.
*/
static int
	TOTAL_LINE(struct S_READ *READS, int COUNT, int LAST, struct S_UNITS *U)
{
	int	BEST;
	int	I;

	BEST = -1;

	for (I = LAST + 1; I < COUNT; ++I)
	{
		struct S_READ	*R = &READS[I];
		char			TEXT[CELL_MAX];
		int				FITS;

		if (R->DROP || !R->COUNT)
			continue ;

		GLYPH_TEXT(R->GLYPH, 0, R->COUNT - 1, TEXT, sizeof(TEXT));
		FITS = strpbrk(TEXT, "0123456789") != NULL;

		if (!FITS && U->COUNT)
		{
			struct S_GRAMMAR	ONLY;

			TOTAL_GRAMMAR(&ONLY, U, 1);
			FITS = R->BEST - SCORE_GRAMMAR(R, &ONLY) <= TOTAL_FIT;
		}

		if (FITS && (BEST < 0 || R->STEPS > READS[BEST].STEPS))
			BEST = I;
	}

	return (BEST);
}

/*
# THE TOTAL (SEE TOTAL_LINE), READ AS A NUMBER AND ONE OF THE UNITS OF THE
# LIST. ITS CELLS GO INTO ROW.
*/
static void
	READ_TOTAL(
		struct S_READ *READS,
		int COUNT,
		struct S_UNITS *U,
		int NUMBER_COL,
		int UNIT_COL,
		struct S_CELL *CELLS,
		int COLS
	)
{
	struct S_GRAMMAR	G;
	struct S_READ		*R;
	struct S_CELL		*ROW;
	double				SCORE;
	int					LAST;
	int					I;
	int					K;

	LAST = -1;

	for (I = 0; I < COUNT; ++I)
		if (READS[I].FIELDS)
			LAST = I;

	I = TOTAL_LINE(READS, COUNT, LAST, U);

	if (I < 0)
		return ;

	R = &READS[I];
	ROW = &CELLS[I * COLS];

	/*
	# A NUMBER AND A UNIT OF THE LIST, WHEN THAT COSTS NOT MUCH MORE THAN ANY
	# OTHER WAY TO READ IT ("17a Ta?" IS "170 Top")
	*/
	TOTAL_GRAMMAR(&G, U, 0);
	SCORE = SCORE_GRAMMAR(R, &G);

	if (U->COUNT)
	{
		struct S_GRAMMAR	ONLY;

		TOTAL_GRAMMAR(&ONLY, U, 1);

		if (SCORE_GRAMMAR(R, &ONLY) > SCORE - STRICT_TOTAL)
			G = ONLY;
	}

	if (READ_GRAMMAR(R, &G) < -1E29)
		return ;

	R->TOTAL = 1;

	for (K = 0; K < COLS; ++K)
		ROW[K].LINE = -1;

	FIELD_CELL(R, I, 0, PART_PREFIX, &ROW[0]);
	FIELD_CELL(R, I, 0, PART_MAIN, &ROW[NUMBER_COL]);
	NO_SPACES(ROW[NUMBER_COL].TEXT);

	if (UNIT_COL < 0)
		return ;

	FIELD_CELL(R, I, 0, PART_UNIT, &ROW[UNIT_COL]);

	/* ONE OF THE UNITS OF THE LIST: SPELLED AS IN THE LIST */
	for (K = 0; K < U->COUNT; ++K)
	{
		char	A[CELL_MAX];
		char	B[CELL_MAX];

		BARE(ROW[UNIT_COL].TEXT, A, sizeof(A));
		BARE(U->WORD[K], B, sizeof(B));

		if (strcmp(A, B) == 0)
			snprintf(ROW[UNIT_COL].TEXT, CELL_MAX, "%s", U->WORD[K]);
	}
}

/* THE CELLS OF ONE TABLE COLUMN, IN THE LIST LINES (AND THE TOTAL) */
static int
	COLUMN_CELLS(
		struct S_READ *READS,
		int COUNT,
		struct S_CELL *CELLS,
		int COLS,
		int COL,
		int WITH_TOTAL,
		struct S_CELL **DEST
	)
{
	int	N;
	int	I;

	N = 0;

	for (I = 0; I < COUNT; ++I)
		if (READS[I].FIELDS || (WITH_TOTAL && READS[I].TOTAL))
			DEST[N++] = &CELLS[I * COLS + COL];

	return (N);
}

/* 1 = MORE LETTERS THAN DIGITS */
static int
	MOSTLY_LETTERS(const char *TEXT)
{
	int	LETTERS;
	int	DIGITS;

	LETTERS = 0;
	DIGITS = 0;

	for (; *TEXT; ++TEXT)
	{
		const unsigned char	C = (unsigned char)*TEXT;

		DIGITS += C >= '0' && C <= '9';
		LETTERS += (C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z');
	}

	return (LETTERS > DIGITS);
}

/* HOW MANY LIST LINES HAVE THIS TEXT IN COLUMN C */
static int
	TIMES_IN_COLUMN(
		const struct S_READ *READS,
		int COUNT,
		const struct S_CELL *CELLS,
		int COLS,
		int C,
		const char *TEXT
	)
{
	int	TIMES;
	int	I;

	TIMES = 0;

	for (I = 0; I < COUNT; ++I)
	{
		const struct S_CELL	*X = &CELLS[I * COLS + C];

		if (READS[I].FIELDS && X->LINE >= 0 && strcmp(X->TEXT, TEXT) == 0)
			++TIMES;
	}

	return (TIMES);
}

/*
# THE CELLS TO THE TABLE, EVERY ONE WITH ITS "NOT SURE" MARK. 1 = DONE.
# AGREE[C]: A CELL OF COLUMN C WHOSE TEXT COMES THIS MANY TIMES IN ITS
# COLUMN IS SURE (THE SAME WORD OR CODE AGAIN AND AGAIN); 0 = NEVER.
# DIGIT_CODES[C]: 1 = COLUMN C HOLDS CODES THAT ARE MOSTLY DIGITS.
*/
static int
	LIST_OUT(
		struct S_READ *READS,
		int COUNT,
		const struct S_CELL *CELLS,
		int COLS,
		const unsigned char *NUMERIC,
		const int *AGREE,
		const unsigned char *DIGIT_CODES,
		struct S_TABLE *TABLE
	)
{
	int	I;

	TABLE->ROWS = 0;
	TABLE->COLS = COLS;
	TABLE->CELL = (char **)calloc((size_t)COUNT * COLS, sizeof(char *));
	TABLE->CHECK = (unsigned char *)calloc((size_t)COUNT * COLS, 1);

	for (I = 0; I < COUNT && TABLE->CELL && TABLE->CHECK; ++I)
	{
		const int	IN_LIST = READS[I].FIELDS || READS[I].TOTAL;
		int			ANY;
		int			C;

		ANY = 0;

		for (C = 0; C < COLS; ++C)
		{
			const struct S_CELL	*X = &CELLS[I * COLS + C];
			char				*COPY;

			COPY = CELL_COPY(X);
			TABLE->CELL[TABLE->ROWS * COLS + C] = COPY;
			ANY |= COPY != NULL;

			/* A WORD: HOW SURE ITS CHOICE WAS. ANY OTHER CELL: ITS INK */
			if (COPY && X->LINE >= 0 && IN_LIST && !NUMERIC[C])
				TABLE->CHECK[TABLE->ROWS * COLS + C] = (unsigned char)X->CHECK;
			else if (COPY && X->LINE >= 0)
				TABLE->CHECK[TABLE->ROWS * COLS + C] = (unsigned char)DOUBT(
					&READS[X->LINE],
					X,
					NUMERIC[C] && IN_LIST
				);

			if (
				COPY &&
				READS[I].FIELDS &&
				AGREE[C] &&
				TIMES_IN_COLUMN(READS, COUNT, CELLS, COLS, C, X->TEXT) >=
				AGREE[C]
			)
				TABLE->CHECK[TABLE->ROWS * COLS + C] = 0;

			/* A CODE TAKEN FROM ITS NEIGHBOURS AGAINST ITS INK */
			if (COPY && READS[I].FIELDS && NUMERIC[C] && X->CHECK)
				TABLE->CHECK[TABLE->ROWS * COLS + C] = 1;

			/* MORE LETTERS THAN DIGITS IN NUMBERED CODES ("1ALOS") */
			if (
				COPY &&
				READS[I].FIELDS &&
				DIGIT_CODES[C] &&
				MOSTLY_LETTERS(X->TEXT)
			)
				TABLE->CHECK[TABLE->ROWS * COLS + C] = 1;
		}

		TABLE->ROWS += ANY;
	}

	return (TABLE->CELL && TABLE->CHECK && TABLE->ROWS > 0);
}

/* A LIST PAGE TO A TABLE. 1 = DONE */
static int
	LIST_TABLE(
		struct S_READ *READS,
		int COUNT,
		int FIELDS,
		struct S_TABLE *TABLE
	)
{
	struct S_GRAMMAR	G;
	struct S_UNITS		*UNITS;
	struct S_CELL		*CELLS;
	struct S_CELL		**COLUMN;
	int					COL_KIND[FIELDS_MAX];
	int					DIGITS[FIELDS_MAX];
	int					OUT_COL[FIELDS_MAX + 1]; /* FIRST TABLE COLUMN */
	unsigned char		NUMERIC[COLS_MAX]; /* 1 = SPACES DO NOT COUNT */
	int					AGREE[COLS_MAX]; /* SEE LIST_OUT */
	unsigned char		WORDY[COLS_MAX]; /* SEE CORRECT_PAGE */
	unsigned char		DIGIT_CODES[COLS_MAX]; /* SEE LIST_OUT */
	int					NUMBER_COL;
	int					UNIT_COL;
	int					COLS;
	int					OK;
	int					N;
	int					I;
	int					C;

	MARK_LIST_LINES(READS, COUNT, FIELDS);

	/* FIRST WITHOUT RULES, TO SEE WHAT THE COLUMNS HOLD */
	for (I = 0; I < COUNT; ++I)
	{
		int	KINDS[FIELDS_MAX];

		if (!READS[I].FIELDS)
			continue ;

		memset(KINDS, 0, sizeof(KINDS));
		LIST_GRAMMAR(&G, READS[I].FIELDS, KINDS);
		READ_GRAMMAR(&READS[I], &G);
	}

	COLS = 0;
	NUMBER_COL = -1;
	UNIT_COL = -1;
	memset(NUMERIC, 0, sizeof(NUMERIC));

	for (C = 0; C < FIELDS; ++C)
	{
		COL_KIND[C] = COLUMN_KIND(READS, COUNT, FIELDS, C, &DIGITS[C]);
		OUT_COL[C] = COLS;
		NUMERIC[COLS] = COL_KIND[C] != KIND_TEXT;

		if (COL_KIND[C] == KIND_NUMBER && C > 0)
			NUMBER_COL = COLS;

		if (COL_KIND[C] == KIND_NUMBER_UNIT)
		{
			NUMBER_COL = COLS;
			UNIT_COL = COLS + 1;
			++COLS;
		}

		++COLS;
	}

	OUT_COL[FIELDS] = COLS;

	/* THEN AGAIN WITH THE RULES OF EVERY COLUMN */
	for (I = 0; I < COUNT; ++I)
		if (READS[I].FIELDS)
			READ_TYPED(&READS[I], I, FIELDS, COL_KIND, DIGITS);

	CELLS = (struct S_CELL *)calloc((size_t)COUNT * COLS, sizeof(*CELLS));
	COLUMN = (struct S_CELL **)calloc((size_t)COUNT, sizeof(*COLUMN));
	UNITS = (struct S_UNITS *)calloc(1, sizeof(*UNITS));

	if (!CELLS || !COLUMN || !UNITS)
	{
		free(CELLS);
		free(COLUMN);
		free(UNITS);
		return (0);
	}

	for (I = 0; I < COUNT * COLS; ++I)
		CELLS[I].LINE = -1;

	/*
	# A LINE OF ONE LETTER OR DIGIT BETWEEN LIST LINES IS A PIECE OF ANOTHER
	# LINE (A DASH AND THE CURL OF A "?" THAT WENT ASTRAY): NO LINE
	*/
	for (I = 1; I + 1 < COUNT; ++I)
		if (!READS[I].FIELDS && STRAY_LINE(READS, COUNT, I))
			READS[I].DROP = 1;

	/* THE TITLES ABOVE THE LIST: A DATE IS READ AS A DATE */
	for (I = 0; I < COUNT && !READS[I].FIELDS; ++I)
		READ_DATE(&READS[I]);

	for (I = 0; I < COUNT; ++I)
		if (!READS[I].DROP)
			LIST_ROW(&READS[I], I, FIELDS, COL_KIND, OUT_COL, &CELLS[I * COLS]);

	/* THE UNITS OF THE LIST; A LINE WITH ANOTHER "UNIT" IS READ AGAIN */
	if (UNIT_COL >= 0)
	{
		N = COLUMN_CELLS(READS, COUNT, CELLS, COLS, UNIT_COL, 0, COLUMN);
		CORRECT_COLUMN(READS, COLUMN, N);
		MAKE_UNITS(COLUMN, N, UNITS);
		G__UNITS__ = UNITS;

		for (I = 0; I < COUNT && UNITS->COUNT; ++I)
		{
			struct S_CELL	*ROW = &CELLS[I * COLS];

			if (!READS[I].FIELDS || IS_UNIT(UNITS, ROW[UNIT_COL].TEXT))
				continue ;

			if (!READ_KNOWN_UNIT(&READS[I]))
				continue ;

			for (C = 0; C < COLS; ++C)
				ROW[C].LINE = -1;

			LIST_ROW(&READS[I], I, FIELDS, COL_KIND, OUT_COL, ROW);
		}
	}

	/* THE WORDS OF EVERY WORD COLUMN AGAINST EACH OTHER AND THE KNOWN WORDS */
	for (C = 0; C < FIELDS; ++C)
	{
		if (COL_KIND[C] == KIND_TEXT)
		{
			N = COLUMN_CELLS(READS, COUNT, CELLS, COLS, OUT_COL[C], 0, COLUMN);
			CORRECT_COLUMN(READS, COLUMN, N);
		}

		if (COL_KIND[C] == KIND_NUMBER_UNIT)
		{
			N = OUT_COL[C] + 1;
			N = COLUMN_CELLS(READS, COUNT, CELLS, COLS, N, 0, COLUMN);
			CORRECT_COLUMN(READS, COLUMN, N);
		}

		/* THE CODES OF THE FIRST COLUMN AGAINST EACH OTHER */
		if (C == 0 && (COL_KIND[C] == KIND_CODE || COL_KIND[C] == KIND_NUMBER))
		{
			N = COLUMN_CELLS(READS, COUNT, CELLS, COLS, OUT_COL[C], 0, COLUMN);
			CORRECT_CODES(READS, COLUMN, N);
		}
	}

	/* WHOLE AMOUNTS: NO "." OR "," IN THEM */
	for (C = 0; C < FIELDS; ++C)
	{
		if (
			C > 0 &&
			(COL_KIND[C] == KIND_NUMBER || COL_KIND[C] == KIND_NUMBER_UNIT)
		)
		{
			N = COLUMN_CELLS(READS, COUNT, CELLS, COLS, OUT_COL[C], 0, COLUMN);
			WHOLE_AMOUNTS(READS, COLUMN, N);
		}
	}

	/* THE WORDS OF ALL WORD COLUMNS TOGETHER */
	memset(WORDY, 0, sizeof(WORDY));

	for (C = 0; C < FIELDS; ++C)
		if (COL_KIND[C] == KIND_TEXT)
			WORDY[OUT_COL[C]] = 1;

	CORRECT_PAGE(READS, COUNT, CELLS, COLS, WORDY);

	/* THE TOTAL, WITH THE UNITS OF THE LIST, AND THE SUM OF THE AMOUNTS */
	if (NUMBER_COL >= 0)
	{
		if (UNIT_COL >= 0)
		{
			N = COLUMN_CELLS(READS, COUNT, CELLS, COLS, UNIT_COL, 0, COLUMN);
			MAKE_UNITS(COLUMN, N, UNITS);
		}

		G__UNITS__ = NULL;
		READ_TOTAL(READS, COUNT, UNITS, NUMBER_COL, UNIT_COL, CELLS, COLS);

		for (I = 0; I < COUNT; ++I)
		{
			if (!READS[I].TOTAL)
				continue ;

			N = COLUMN_CELLS(READS, I, CELLS, COLS, NUMBER_COL, 0, COLUMN);
			FIX_SUM(READS, COLUMN, N, &CELLS[I * COLS + NUMBER_COL]);
		}
	}

	/* UNDER THE LIST: A LINE OF ONE OR TWO LETTERS IS A BIT OF THE RULE */
	for (I = COUNT - 1; I >= 0 && !READS[I].FIELDS; --I)
	{
		if (READS[I].TOTAL || ALNUM_GLYPHS(&READS[I]) > 2)
			continue ;

		for (C = 0; C < COLS; ++C)
			CELLS[I * COLS + C].LINE = -1;
	}

	/* A WORD 3 TIMES IN ITS COLUMN, OR A CODE TWICE, IS SURE */
	memset(AGREE, 0, sizeof(AGREE));

	for (C = 0; C < FIELDS; ++C)
	{
		if (COL_KIND[C] == KIND_TEXT)
			AGREE[OUT_COL[C]] = 3;

		if (COL_KIND[C] == KIND_NUMBER_UNIT)
			AGREE[OUT_COL[C] + 1] = 3;

		if (C == 0 && COL_KIND[C] == KIND_CODE)
			AGREE[OUT_COL[C]] = 2;
	}

	memset(DIGIT_CODES, 0, sizeof(DIGIT_CODES));

	for (C = 0; C < FIELDS; ++C)
		if (COL_KIND[C] == KIND_CODE && DIGITS[C])
			DIGIT_CODES[OUT_COL[C]] = 1;

	OK = LIST_OUT(
		READS,
		COUNT,
		CELLS,
		COLS,
		NUMERIC,
		AGREE,
		DIGIT_CODES,
		TABLE
	);
	free(CELLS);
	free(COLUMN);
	free(UNITS);
	return (OK);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          LISTS                           |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          TABLES                          |  \\//  */
/*   \/   #==========================================================#   \/   */
/*
# A WORD THAT LOOKS LIKE A DATE IS READ AS ONE ("12,05.2026" IS
# "12.05.2026") WHEN THAT COSTS AT MOST DATE_COST MORE THAN THE WORD AS IT
# WAS READ. 1 = IT IS A DATE NOW.
*/
static int
	CORRECT_DATE(const struct S_READ *R, struct S_CELL *C)
{
	struct S_GRAMMAR	G;
	struct S_GLYPH		GLYPHS[CELL_MAX];
	const char			*P;
	double				BEST;
	double				SCORE;
	int					DIGITS;
	int					N;
	int					T;

	DIGITS = 0;

	for (P = C->TEXT; *P; ++P)
		DIGITS += *P >= '0' && *P <= '9';

	if (DIGITS < 4 || !strpbrk(C->TEXT, ".,/-"))
		return (0);

	BEST = 0.0;

	for (T = C->FROM; T < C->TO; ++T)
	{
		const float	*ROW = R->LOGP + (size_t)T * G__CLASSES__;
		float		MAX;
		int			K;

		MAX = ROW[0];

		for (K = 1; K < G__CLASSES__; ++K)
			if (ROW[K] > MAX)
				MAX = ROW[K];

		BEST += MAX;
	}

	DATE_GRAMMAR(&G);
	SCORE = CTC_GRAMMAR(
		R->LOGP,
		G__CLASSES__,
		C->FROM,
		C->TO,
		&G,
		GLYPHS,
		NULL,
		CELL_MAX,
		&N
	);

	if (SCORE < -1E29 || BEST - SCORE > DATE_COST || N < 1)
		return (0);

	GLYPH_TEXT(GLYPHS, 0, N - 1, C->TEXT, sizeof(C->TEXT));
	return (1);
}

/* A WORD THAT IS MOSTLY DIGITS IS READ AGAIN WITH DIGITS ONLY */
static void
	CORRECT_NUMBER(const struct S_READ *R, struct S_CELL *C)
{
	struct S_GLYPH	G[CELL_MAX];
	const char		*P;
	int				DIGITS;
	int				OTHERS;
	int				N;

	DIGITS = 0;
	OTHERS = 0;

	for (P = C->TEXT; *P; ++P)
	{
		if (*P >= '0' && *P <= '9')
			++DIGITS;
		else if (*P != '.' && *P != ',')
			++OTHERS;
	}

	if (!DIGITS || DIGITS < 2 * OTHERS || !OTHERS || IS_SIZE(C->TEXT))
		return ;

	N = CTC_GREEDY(
		R->LOGP,
		G__CLASSES__,
		C->FROM,
		C->TO,
		G__NUMBER__,
		G,
		CELL_MAX
	);
	GLYPH_TEXT(G, 0, N - 1, C->TEXT, sizeof(C->TEXT));
}

/* 1 = WORD IS ONE OF THE WORDS OF TEXT (SPLIT BY SPACES) */
static int
	HAS_WORD(const char *TEXT, const char *WORD)
{
	const size_t	LEN = strlen(WORD);
	const char		*P;

	for (P = TEXT; LEN && (P = strstr(P, WORD)) != NULL; ++P)
		if ((P == TEXT || P[-1] == ' ') && (P[LEN] == ' ' || !P[LEN]))
			return (1);

	return (0);
}

/*
# THE TABLE MAKER DOES NOT SAY WHICH WORD WENT WHERE: A CELL IS MARKED
# WHEN IT HOLDS A WORD THAT THE READER WAS NOT SURE OF.
*/
static void
	MARK_DOUBTS(
		const struct S_READ *READS,
		const struct S_CELL *WORDS,
		int COUNT,
		struct S_TABLE *TABLE
	)
{
	const int	CELLS = TABLE->ROWS * TABLE->COLS;
	int			I;

	/* THE TABLE MAKER MAY HAVE MARKED THE CELLS IT CORRECTED ALREADY */
	if (!TABLE->CHECK)
		TABLE->CHECK = (unsigned char *)calloc((size_t)CELLS + 1, 1);

	for (I = 0; I < COUNT && TABLE->CHECK; ++I)
	{
		const struct S_CELL	*W = &WORDS[I];
		int					K;

		if (W->LINE < 0 || !DOUBT(&READS[W->LINE], W, IS_NUMBER_TEXT(W->TEXT)))
			continue ;

		for (K = 0; K < CELLS; ++K)
			if (TABLE->CELL[K] && HAS_WORD(TABLE->CELL[K], W->TEXT))
				TABLE->CHECK[K] = 1;
	}
}

/* 1 = A DIGIT, OR A DOT OR A COMMA (A PART OF A NUMBER) */
static int
	IS_NUMBER_CLASS(int K)
{
	return (G__NUMBER__[K]);
}

/*
# 1 = A DRAWN COLUMN LINE (A WALL OF A TABLE) OR A WIDE GAP LIES BETWEEN
# GLYPH A AND GLYPH B OF A LINE: THEY ARE IN TWO CELLS
*/
static int
	WALL_BETWEEN(const struct S_READ *R, int A, int B)
{
	const int	FROM = R->GLYPH[A].LAST;
	const int	TO = R->GLYPH[B].FIRST;
	int			I;

	if (TO - FROM >= CELL_GAP)
		return (1);

	for (I = 0; R->LINE && I < R->LINE->CUTS; ++I)
	{
		const int	STEP = R->LINE->CUT_AT[I] / 4;

		if (STEP >= FROM && STEP <= TO)
			return (1);
	}

	return (0);
}

/*
# THE LAST GLYPH OF THE WORD THAT STARTS AT GLYPH FIRST. A SHORT SPACE
# BETWEEN TWO PARTS OF A NUMBER DOES NOT END IT ("2744 55" IS ONE NUMBER,
# READ AGAIN LATER AS "2744,55" OR "274455"). A DRAWN COLUMN LINE OR A WIDE
# GAP ENDS IT EVEN WITHOUT A SPACE.
*/
static int
	WORD_END(const struct S_READ *R, int FIRST)
{
	int	END;

	END = FIRST;

	for (;;)
	{
		while (
			END + 1 < R->COUNT &&
			R->GLYPH[END + 1].CLASS != G__SPACE__ &&
			!WALL_BETWEEN(R, END, END + 1)
		)
			++END;

		if (
			END + 2 >= R->COUNT ||
			R->GLYPH[END + 1].CLASS != G__SPACE__ ||
			!IS_NUMBER_CLASS(R->GLYPH[END].CLASS) ||
			!IS_NUMBER_CLASS(R->GLYPH[END + 2].CLASS) ||
			R->GLYPH[END + 2].FIRST - R->GLYPH[END].LAST > NUMBER_GAP ||
			WALL_BETWEEN(R, END, END + 2)
		)
			return (END);

		END += 2;
	}
}

/* 1 = A DRAWN COLUMN LINE OF THE LINE STANDS BETWEEN STEPS FROM AND TO */
static int
	CUT_BETWEEN(const struct S_LINE *LINE, float FROM, float TO)
{
	int	I;

	for (I = 0; I < LINE->CUTS; ++I)
	{
		const float	STEP = (float)LINE->CUT_AT[I] / 4.0F;

		if (STEP >= FROM && STEP <= TO)
			return (1);
	}

	return (0);
}

/* 1 = A LETTER OR A DIGIT (OR A BYTE OF ONE THAT IS NOT A TO Z) */
static int
	IS_ALNUM_BYTE(char C)
{
	return (
		(C >= '0' && C <= '9') ||
		(C >= 'A' && C <= 'Z') ||
		(C >= 'a' && C <= 'z') ||
		(unsigned char)C >= 0X80
	);
}

/* 1 = C IS SPACE OR A BIT OF A DRAWN LINE READ AS A MARK */
static int
	IS_JUNK(char C)
{
	return (C && strchr(" -_.,'\"`", C) != NULL);
}

/*
# 1 = A WORD OF A LINE THAT CROSSES DRAWN COLUMN LINES IS ONLY MARKS ("'",
# ".", "_"): A BIT OF A DRAWN LINE, NOT TEXT (A "-" ALONE STAYS)
*/
static int
	BIT_OF_LINE(const struct S_READ *READS, const struct S_WORD *W)
{
	const char	*P;

	if (!READS[W->LINE].LINE->CUTS || strcmp(W->TEXT, "-") == 0)
		return (0);

	for (P = W->TEXT; *P && IS_JUNK(*P); ++P)
	{}

	return (!*P);
}

/*
# WORD B (AND ITS SPAN) JOINS WORD A WHEN THEY ARE IN ONE CELL OF A DRAWN
# TABLE: ONE LINE, THAT CROSSES DRAWN COLUMN LINES, BUT NONE BETWEEN THEM.
# 1 = JOINED.
*/
static int
	JOIN_WORD(
		const struct S_READ *READS,
		struct S_WORD *A,
		struct S_CELL *SPAN_A,
		const struct S_WORD *B,
		const struct S_CELL *SPAN_B
	)
{
	const struct S_LINE	*L = READS[B->LINE].LINE;
	const size_t		USED = strlen(A->TEXT);
	const size_t		MORE = strlen(B->TEXT);

	if (
		A->LINE != B->LINE ||
		!L->CUTS ||
		CUT_BETWEEN(L, A->X + A->W, B->X) ||
		USED + 1 + MORE >= sizeof(A->TEXT) ||
		USED + 1 + MORE >= sizeof(SPAN_A->TEXT)
	)
		return (0);

	A->TEXT[USED] = ' ';
	strcpy(A->TEXT + USED + 1, B->TEXT);
	strcpy(SPAN_A->TEXT, A->TEXT);
	A->W = B->X + B->W - A->X;
	SPAN_A->TO = SPAN_B->TO;
	return (1);
}

/*
# THE WORDS OF ONE CELL OF A DRAWN TABLE BECOME ONE WORD: NEXT TO EACH OTHER
# IN A LINE THAT CROSSES DRAWN COLUMN LINES, WITH NO SUCH LINE BETWEEN THEM.
# PLACES ARE STILL IN STEPS OF THE LINE. SPANS (THE SAME WORDS AS CELLS OF
# THE READER) ARE JOINED THE SAME WAY. BITS OF LINES GO (BIT_OF_LINE).
# RETURNS THE NEW COUNT.
*/
static int
	JOIN_CELL_WORDS(
		const struct S_READ *READS,
		struct S_WORD *WORDS,
		struct S_CELL *SPANS,
		int COUNT
	)
{
	int	N;
	int	I;

	N = 0;

	for (I = 0; I < COUNT; ++I)
	{
		if (BIT_OF_LINE(READS, &WORDS[I]))
			continue ;

		if (
			!N ||
			!JOIN_WORD(
				READS,
				&WORDS[N - 1],
				&SPANS[N - 1],
				&WORDS[I],
				&SPANS[I]
			)
		)
		{
			SPANS[N] = SPANS[I];
			WORDS[N++] = WORDS[I];
		}
	}

	return (N);
}

/*
# THE LENGTH (BYTES) OF THE SMALL LETTER AT TEXT (A TO Z, OR A TURKISH ONE),
# 0 = NOT A SMALL LETTER
*/
static int
	SMALL_LETTER(const char *TEXT)
{
	static const char *const	TURKISH[] = {"ç", "ğ", "ı", "ö", "ş", "ü"};
	int							I;

	if (*TEXT >= 'a' && *TEXT <= 'z')
		return (1);

	for (I = 0; I < 6; ++I)
		if (strncmp(TEXT, TURKISH[I], 2) == 0)
			return (2);

	return (0);
}

/* 1 = TEXT FROM ITS START TO END IS SMALL LETTERS ONLY (ONE OR MORE) */
static int
	ALL_SMALL(const char *TEXT, const char *END)
{
	if (TEXT >= END)
		return (0);

	while (TEXT < END)
	{
		const int	LEN = SMALL_LETTER(TEXT);

		if (!LEN)
			return (0);

		TEXT += LEN;
	}

	return (1);
}

/* 1 = THE TEXT FROM ITS START TO END ENDS IN A SMALL LETTER */
static int
	ENDS_SMALL(const char *TEXT, const char *END)
{
	if (END - TEXT >= 1 && ALL_SMALL(END - 1, END))
		return (1);

	return (END - TEXT >= 2 && ALL_SMALL(END - 2, END));
}

/*
# THE FIRST LETTER OF A WORD (FROM WORD TO END) READ AS "I" BEFORE SMALL
# LETTERS, AFTER A WORD OF SMALL LETTERS: "l" WHEN THAT MAKES A KNOWN WORD,
# "I" WHEN "I" DOES, ELSE "l" FOR A SHORT WORD ("Ien" IS "len") AND "I"
# FOR A LONGER ONE (A NAME: "Interlok").
*/
static char
	SMALL_OR_CAPITAL(const char *WORD, const char *END)
{
	char	OTHER[CELL_MAX];
	size_t	LEN;

	LEN = (size_t)(END - WORD);

	if (LEN >= sizeof(OTHER))
		return ('I');

	memcpy(OTHER, WORD, LEN);
	OTHER[LEN] = '\0';

	if (IS_KNOWN(OTHER))
		return ('I');

	OTHER[0] = 'l';

	if (IS_KNOWN(OTHER) || LEN <= 3)
		return ('l');

	return ('I');
}

/*
# AN "I" THAT STARTS A WORD OF SMALL LETTERS RIGHT AFTER A WORD THAT ENDS IN
# A SMALL LETTER MAY BE AN "l": THE TWO LOOK THE SAME IN HANDWRITING, AND A
# CAPITAL IS RARE THERE ("tam Ien" IS "tam len", SMALL_OR_CAPITAL).
# SMALL_BEFORE: THE WORD BEFORE TEXT (IN ITS LINE) ENDS IN A SMALL LETTER.
*/
static void
	SMALL_L(char *TEXT, int SMALL_BEFORE)
{
	char	*P;

	P = TEXT;

	while (*P)
	{
		char	*END;

		END = P;

		while (*END && *END != ' ')
			++END;

		if (SMALL_BEFORE && P[0] == 'I' && ALL_SMALL(P + 1, END))
			P[0] = SMALL_OR_CAPITAL(P, END);

		SMALL_BEFORE = ENDS_SMALL(P, END);

		P = END;

		while (*P == ' ')
			++P;
	}
}

/*
# BITS OF A DRAWN LINE READ AT THE ENDS OF A CELL GO ("AB-" IS "AB"): MARKS
# AFTER A LETTER OR DIGIT, AND BEFORE ONE (BUT NOT "-": A MINUS)
*/
static void
	TRIM_JUNK(char *TEXT)
{
	size_t	LEN;
	size_t	END;
	size_t	FIRST;

	LEN = strlen(TEXT);
	END = LEN;

	while (END > 0 && IS_JUNK(TEXT[END - 1]))
		--END;

	if (END > 0 && END < LEN && IS_ALNUM_BYTE(TEXT[END - 1]))
	{
		TEXT[END] = '\0';
		LEN = END;
	}

	FIRST = 0;

	while (FIRST < LEN && TEXT[FIRST] != '-' && IS_JUNK(TEXT[FIRST]))
		++FIRST;

	if (FIRST > 0 && FIRST < LEN && IS_ALNUM_BYTE(TEXT[FIRST]))
		memmove(TEXT, TEXT + FIRST, LEN - FIRST + 1);
}

/* THE COLUMN OF A WORD (PIXELS OF THE PAGE) BETWEEN THE EDGES OF A TABLE */
static int
	EDGE_COLUMN(const struct S_WORD *W, const float *EDGES, int EDGE_COUNT)
{
	const float	MIDDLE = W->X + W->W / 2.0F;
	int			COL;

	COL = 0;

	while (COL < EDGE_COUNT && EDGES[COL] < MIDDLE)
		++COL;

	return (COL);
}

static int
	IS_DIGIT_CHAR(char C)
{
	return (C >= '0' && C <= '9');
}

/* 1 = TEXT IS A TIME OF DAY: "H:MM" OR "HH:MM", HOURS 0-23, MINUTES 0-59 */
static int
	IS_TIME(const char *TEXT)
{
	const size_t	LEN = strlen(TEXT);
	const char		*M = TEXT + LEN - 3;
	int				HOURS;

	if (LEN < 4 || LEN > 5 || (*M != ':' && *M != '.'))
		return (0);

	if (
		!IS_DIGIT_CHAR(TEXT[0]) ||
		!IS_DIGIT_CHAR(M[-1]) ||
		!IS_DIGIT_CHAR(M[1]) ||
		!IS_DIGIT_CHAR(M[2]) ||
		M[1] > '5'
	)
		return (0);

	HOURS = TEXT[0] - '0';

	if (LEN == 5)
		HOURS = HOURS * 10 + TEXT[1] - '0';

	return (HOURS <= 23);
}

/*
# A CELL OF A COLUMN OF TIMES READ AS A TIME, WRITTEN WITH ":": 5 GLYPHS
# READ AS DIGITS WITH SOMETHING ELSE IN THE MIDDLE ("11830": THE ":" WAS
# WRITTEN AS TWO SMALL RINGS), 3 OR 4 DIGITS (THE MARK WAS NOT SEEN), OR
# ELSE THE CELL READ WITH THE TIME GRAMMAR (AT MOST TIME_FIT WORSE). 1 =
# TEXT IS THE TIME.
*/
static int
	READ_TIME(
		const struct S_READ *R,
		const struct S_CELL *SPAN,
		const char *SEEN,
		const struct S_GRAMMAR *TIME,
		char *TEXT
	)
{
	char	*M;

	if (
		strlen(SEEN) == 5 &&
		IS_DIGIT_CHAR(SEEN[0]) &&
		IS_DIGIT_CHAR(SEEN[1]) &&
		IS_DIGIT_CHAR(SEEN[3]) &&
		IS_DIGIT_CHAR(SEEN[4])
	)
		strcpy(TEXT, SEEN);
	else if ((strlen(SEEN) == 3 || strlen(SEEN) == 4) && ALL_DIGITS(SEEN))
	{
		/* THE MARK WAS NOT SEEN: "0730" IS "07:30" */
		const size_t	HOURS = strlen(SEEN) - 2;

		memcpy(TEXT, SEEN, HOURS);
		TEXT[HOURS] = ':';
		strcpy(TEXT + HOURS + 1, SEEN + HOURS);
	}
	else if (GRAMMAR_COST(R, SPAN, TIME, TEXT, CELL_MAX) > TIME_FIT)
		return (0);

	M = TEXT + strlen(TEXT) - 3;

	if (M < TEXT)
		return (0);

	*M = ':';
	return (IS_TIME(TEXT));
}

/* 1 = A SHORT CODE: 1 TO 4 LETTERS OR DIGITS (A TO Z), NO SPACE */
static int
	IS_SHORT_CODE(const char *TEXT)
{
	const size_t	LEN = strlen(TEXT);
	size_t			I;

	if (!LEN || LEN > 4)
		return (0);

	for (I = 0; I < LEN; ++I)
		if (
			!IS_DIGIT_CHAR(TEXT[I]) &&
			!(TEXT[I] >= 'A' && TEXT[I] <= 'Z') &&
			!(TEXT[I] >= 'a' && TEXT[I] <= 'z')
		)
			return (0);

	return (1);
}

/* 1 = A LETTER, THEN A DIGIT ("A3") */
static int
	IS_LETTER_DIGIT(const char *TEXT)
{
	return (
		strlen(TEXT) == 2 &&
		!IS_DIGIT_CHAR(TEXT[0]) &&
		IS_SHORT_CODE(TEXT) &&
		IS_DIGIT_CHAR(TEXT[1])
	);
}

/*
# 1 = MOST CELLS OF A COLUMN OF TIMES (BUT THE TITLE) READ AS TIMES WITH TWO
# DIGITS FOR THE HOURS ("08:30"): A ZERO WAS MISSED WHERE ONE HAS ONE
*/
static int
	TWO_HOURS(
		const struct S_READ *READS,
		const struct S_WORD *WORDS,
		const struct S_CELL *SPANS,
		const int *KEYS,
		int FIRST,
		int LAST,
		int COL,
		const float *EDGES,
		int EDGE_COUNT
	)
{
	struct S_GRAMMAR	TIME;
	int					LONG;
	int					SHORT;
	int					I;

	TIME_GRAMMAR(&TIME, 0);
	LONG = 0;
	SHORT = 0;

	for (I = FIRST; I < LAST; ++I)
	{
		char	TEXT[CELL_MAX];

		if (
			KEYS[I] == KEYS[FIRST] ||
			EDGE_COLUMN(&WORDS[I], EDGES, EDGE_COUNT) != COL ||
			!READ_TIME(
				&READS[SPANS[I].LINE],
				&SPANS[I],
				WORDS[I].TEXT,
				&TIME,
				TEXT
			)
		)
			continue ;

		LONG += strlen(TEXT) == 5;
		SHORT += strlen(TEXT) == 4;
	}

	return (LONG > SHORT);
}

/*
# THE KIND OF A COLUMN OF A DRAWN TABLE, FROM ITS CELLS BUT THE TITLE (WORDS
# FIRST TO LAST, ROWS KEYS): KIND_TIME WHEN TIME_FIT OR LESS MAKES TIMES OF
# HALF OF THEM (2 OR MORE), KIND_NUMBER WHEN HALF ARE DIGITS, KIND_CODE WHEN
# ALL ARE SHORT CODES AND HALF HAVE A LETTER, ELSE KIND_TEXT (OR LESS THAN
# 2 CELLS). *SHAPE OF A COLUMN OF CODES: SHAPE_PAIRED WHEN 60% ARE A LETTER
# THEN A DIGIT, SHAPE_LEAD WHEN 60% START WITH A LETTER; OF A COLUMN OF
# TIMES: SHAPE_TWO_HOURS (TWO_HOURS).
*/
static int
	DRAWN_KIND(
		const struct S_READ *READS,
		const struct S_WORD *WORDS,
		const struct S_CELL *SPANS,
		const int *KEYS,
		int FIRST,
		int LAST,
		int COL,
		const float *EDGES,
		int EDGE_COUNT,
		int *SHAPE
	)
{
	struct S_GRAMMAR	TIME;
	int					COUNT[5];
	int					PAIRS;
	int					LEAD;
	int					I;

	TIME_GRAMMAR(&TIME, 0);
	memset(COUNT, 0, sizeof(COUNT));
	PAIRS = 0;
	LEAD = 0;
	*SHAPE = 0;

	for (I = FIRST; I < LAST; ++I)
	{
		const char	*TEXT = WORDS[I].TEXT;
		char		READ[CELL_MAX];

		if (
			KEYS[I] == KEYS[FIRST] ||
			EDGE_COLUMN(&WORDS[I], EDGES, EDGE_COUNT) != COL
		)
			continue ;

		++COUNT[0];
		COUNT[1] += ALL_DIGITS(TEXT);
		COUNT[2] += IS_SHORT_CODE(TEXT);
		COUNT[3] += IS_SHORT_CODE(TEXT) && !ALL_DIGITS(TEXT);
		PAIRS += IS_LETTER_DIGIT(TEXT);
		LEAD += IS_SHORT_CODE(TEXT) && !IS_DIGIT_CHAR(TEXT[0]);

		if (
			GRAMMAR_COST(
				&READS[SPANS[I].LINE],
				&SPANS[I],
				&TIME,
				READ,
				sizeof(READ)
			) <= TIME_FIT
		)
			++COUNT[4];
	}

	if (COUNT[0] < 2)
		return (KIND_TEXT);

	if (COUNT[4] >= 2 && COUNT[4] * 2 >= COUNT[0])
	{
		if (
			TWO_HOURS(
				READS,
				WORDS,
				SPANS,
				KEYS,
				FIRST,
				LAST,
				COL,
				EDGES,
				EDGE_COUNT
			)
		)
			*SHAPE |= SHAPE_TWO_HOURS;

		return (KIND_TIME);
	}

	if (COUNT[1] * 2 >= COUNT[0])
		return (KIND_NUMBER);

	if (COUNT[2] < COUNT[0] || COUNT[3] * 2 < COUNT[0])
		return (KIND_TEXT);

	if (PAIRS >= 2 && PAIRS * 10 >= COUNT[0] * 6)
		*SHAPE |= SHAPE_PAIRED;

	if (LEAD >= 2 && LEAD * 10 >= COUNT[0] * 6)
		*SHAPE |= SHAPE_LEAD;

	return (KIND_CODE);
}

/*
# ONE CELL OF A COLUMN OF A KIND (DRAWN_KIND) READ AGAIN INTO TEXT: A TIME,
# DIGITS (AT MOST DIGIT_FIT WORSE FOR EVERY GLYPH: "S" IS "5"), OR A CODE IN
# CAPITALS, OF THE SHAPE OF ITS COLUMN WHEN IT IS NOT ("46" IS "V6" WHEN
# MOST ARE A LETTER AND A DIGIT; AT MOST DIGIT_FIT WORSE FOR EVERY GLYPH).
# 1 = TEXT IS THE NEW CELL.
*/
static int
	READ_KIND(
		const struct S_READ *R,
		const struct S_CELL *SPAN,
		const char *SEEN,
		int KIND,
		int SHAPE,
		char *TEXT
	)
{
	struct S_GRAMMAR	G;
	int					LENGTH;

	if (KIND == KIND_TIME)
	{
		TIME_GRAMMAR(&G, 0);

		if (!READ_TIME(R, SPAN, SEEN, &G, TEXT))
			return (0);

		/* "8:30" IN A COLUMN OF "08:30": READ WITH TWO DIGITS IF IT FITS */
		if (SHAPE & SHAPE_TWO_HOURS && strlen(TEXT) == 4)
		{
			char	LONG[CELL_MAX];

			TIME_GRAMMAR(&G, 1);

			if (GRAMMAR_COST(R, SPAN, &G, LONG, sizeof(LONG)) <= TIME_FIT)
			{
				LONG[2] = ':';

				if (IS_TIME(LONG))
					strcpy(TEXT, LONG);
			}
		}

		return (1);
	}

	if (KIND == KIND_NUMBER && IS_SIZE(SEEN))
		return (0);

	if (KIND == KIND_NUMBER && !ALL_DIGITS(SEEN))
	{
		DIGITS_GRAMMAR(&G);
		return (
			GRAMMAR_COST(R, SPAN, &G, TEXT, CELL_MAX) <=
			DIGIT_FIT * (double)strlen(TEXT)
		);
	}

	if (KIND != KIND_CODE)
		return (0);

	CASE_VARIANT(SEEN, 2, TEXT, CELL_MAX);
	LENGTH = (int)strlen(SEEN);

	if (
		LENGTH >= 2 &&
		(
			(SHAPE & SHAPE_PAIRED && LENGTH == 2 && !IS_LETTER_DIGIT(SEEN)) ||
			(SHAPE & SHAPE_LEAD && IS_DIGIT_CHAR(SEEN[0]))
		)
	)
	{
		char	CODE[CELL_MAX];

		CODE_GRAMMAR(&G, LENGTH, SHAPE & SHAPE_PAIRED && LENGTH == 2);

		if (
			GRAMMAR_COST(R, SPAN, &G, CODE, sizeof(CODE)) <=
			DIGIT_FIT * (double)LENGTH
		)
			CASE_VARIANT(CODE, 2, TEXT, CELL_MAX);
	}

	return (1);
}

/*
# THE COLUMNS OF A DRAWN TABLE (WORDS FIRST TO LAST IN ORDER, ROW KEYS[I];
# THE FIRST ROW IS ITS TITLES): BITS OF LINES AT THE ENDS OF CELLS GO, AND
# A COLUMN OF TIMES, DIGITS OR CODES HAS ITS CELLS READ AS SUCH (READ_KIND).
# TIMES ARE WRITTEN WITH ":" (A SHEET TAKES "11:30" AS A TIME, "11.30" AS
# A NUMBER). FIXED[I] = 1 FOR A WORD THAT CHANGED.
*/
static void
	FIX_DRAWN_COLUMNS(
		const struct S_READ *READS,
		struct S_WORD *WORDS,
		const struct S_CELL *SPANS,
		const int *KEYS,
		int FIRST,
		int LAST,
		const float *EDGES,
		int EDGE_COUNT,
		unsigned char *FIXED
	)
{
	int	COL;

	/* BITS OF LINES: AT THE ENDS OF A CELL, OR ALL OF A WORD (BUT "-") */
	for (COL = FIRST; COL < LAST; ++COL)
	{
		const char	*P;

		TRIM_JUNK(WORDS[COL].TEXT);

		for (P = WORDS[COL].TEXT; *P && IS_JUNK(*P); ++P)
		{}

		if (!*P && strcmp(WORDS[COL].TEXT, "-") != 0)
			WORDS[COL].TEXT[0] = '\0';
	}

	for (COL = 0; COL <= EDGE_COUNT; ++COL)
	{
		int	SHAPE;
		int	KIND;
		int	I;

		KIND = DRAWN_KIND(
			READS,
			WORDS,
			SPANS,
			KEYS,
			FIRST,
			LAST,
			COL,
			EDGES,
			EDGE_COUNT,
			&SHAPE
		);

		for (I = FIRST; I < LAST && KIND != KIND_TEXT; ++I)
		{
			char	TEXT[CELL_MAX];

			if (
				KEYS[I] == KEYS[FIRST] ||
				EDGE_COLUMN(&WORDS[I], EDGES, EDGE_COUNT) != COL ||
				!READ_KIND(
					&READS[SPANS[I].LINE],
					&SPANS[I],
					WORDS[I].TEXT,
					KIND,
					SHAPE,
					TEXT
				) ||
				strcmp(TEXT, WORDS[I].TEXT) == 0
			)
				continue ;

			snprintf(WORDS[I].TEXT, sizeof(WORDS[I].TEXT), "%s", TEXT);
			FIXED[I] = 1;
		}
	}
}

/* THE CELLS OF A PART OF THE PAGE THAT HOLD A WORD THAT CHANGED ARE MARKED */
static void
	MARK_FIXED(
		struct S_TABLE *TABLE,
		const struct S_WORD *WORDS,
		const unsigned char *FIXED,
		int FIRST,
		int LAST
	)
{
	int	I;

	for (I = FIRST; I < LAST && TABLE->CHECK; ++I)
	{
		int	K;

		if (!FIXED[I])
			continue ;

		for (K = 0; K < TABLE->ROWS * TABLE->COLS; ++K)
			if (TABLE->CELL[K] && HAS_WORD(TABLE->CELL[K], WORDS[I].TEXT))
				TABLE->CHECK[K] = 1;
	}
}

/*
# 1 = LINE B STARTS A NEW PART OF THE PAGE AFTER LINE A: NOT IN THE SAME
# DRAWN TABLE, OR (OUTSIDE TABLES) A GAP OF BLOCK_GAP LINES OR MORE, OR BOTH
# CROSS DRAWN COLUMN LINES BUT NONE OF THEM ARE IN THE SAME PLACE. HEIGHT
# IS THE USUAL HEIGHT OF A LINE.
*/
static int
	NEW_BLOCK(const struct S_LINE *A, const struct S_LINE *B, float HEIGHT)
{
	int	I;

	/* IN A DRAWN TABLE: THE TABLE IS THE PART, HOWEVER TALL ITS ROWS */
	if (A->TABLE >= 0 || B->TABLE >= 0)
		return (A->TABLE != B->TABLE);

	if ((float)(B->Y0 - A->Y1) > BLOCK_GAP * HEIGHT)
		return (1);

	if (!A->CUTS || !B->CUTS)
		return (0);

	for (I = 0; I < A->CUTS; ++I)
	{
		const float	XA = (float)A->LEFT + (float)A->CUT_AT[I] * A->SCALE;
		int			J;

		for (J = 0; J < B->CUTS; ++J)
		{
			const float	XB = (float)B->LEFT + (float)B->CUT_AT[J] * B->SCALE;

			if (fabsf(XA - XB) < HEIGHT)
				return (0);
		}
	}

	return (1);
}

static int
	CMP_HEIGHT(const void *A, const void *B)
{
	const float	FA = *(const float *)A;
	const float	FB = *(const float *)B;

	return ((FA > FB) - (FA < FB));
}

/* THE USUAL HEIGHT OF THE LINES THAT HAVE WORDS (PIXELS OF THE LINES) */
static float
	USUAL_LINE(const struct S_READ *READS, int COUNT)
{
	float	*HEIGHTS;
	float	USUAL;
	int		N;
	int		I;

	HEIGHTS = (float *)malloc(((size_t)COUNT + 1) * sizeof(float));

	if (!HEIGHTS)
		return (1.0F);

	N = 0;

	for (I = 0; I < COUNT; ++I)
		if (!READS[I].DROP && READS[I].COUNT)
			HEIGHTS[N++] = (float)(READS[I].LINE->Y1 - READS[I].LINE->Y0);

	USUAL = 1.0F;
	qsort(HEIGHTS, (size_t)N, sizeof(float), CMP_HEIGHT);

	if (N)
		USUAL = HEIGHTS[N / 2];

	free(HEIGHTS);
	return (USUAL);
}

/*
# THE COLUMN EDGES OF A PART OF THE PAGE THAT IS A DRAWN TABLE (WORDS FROM
# FIRST TO LAST, IN PIXELS OF THE PAGE): THE PLACES WHERE ITS LINES CROSS
# DRAWN COLUMN LINES, PLACES LESS THAN HEIGHT APART AS ONE, CROSSED BY A
# THIRD OF THOSE LINES OR MORE. RETURNS HOW MANY (AT MOST EDGES_MAX), 0 =
# NOT A DRAWN TABLE (ONE LINE, OR LESS THAN HALF OF ITS LINES CROSS ANY).
*/
static int
	WALL_EDGES(
		const struct S_READ *READS,
		const struct S_WORD *WORDS,
		int FIRST,
		int LAST,
		float SCALE,
		float HEIGHT,
		float *EDGES
	)
{
	float	PLACES[EDGES_MAX * 4];
	int		LINES;
	int		CROSSING;
	int		COUNT;
	int		N;
	int		I;

	LINES = 0;
	CROSSING = 0;
	COUNT = 0;

	for (I = FIRST; I < LAST; ++I)
	{
		const struct S_LINE	*L = READS[WORDS[I].LINE].LINE;
		int					K;

		if (I > FIRST && WORDS[I].LINE == WORDS[I - 1].LINE)
			continue ;

		++LINES;
		CROSSING += L->CUTS > 0;

		for (K = 0; K < L->CUTS && COUNT < EDGES_MAX * 4; ++K)
			PLACES[COUNT++] =
				SCALE * ((float)L->LEFT + (float)L->CUT_AT[K] * L->SCALE);
	}

	if (LINES < 2 || CROSSING * 2 < LINES)
		return (0);

	qsort(PLACES, (size_t)COUNT, sizeof(float), CMP_HEIGHT);
	N = 0;
	I = 0;

	while (I < COUNT && N < EDGES_MAX)
	{
		float	SUM;
		int		J;

		SUM = PLACES[I];
		J = I + 1;

		while (J < COUNT && PLACES[J] - PLACES[J - 1] < HEIGHT)
			SUM += PLACES[J++];

		if ((J - I) * 3 >= CROSSING)
			EDGES[N++] = SUM / (float)(J - I);

		I = J;
	}

	return (N);
}

/* THE MIDDLE OF A WORD ON THE PAGE OF ITS LINES (PIXELS OF THE LINES) */
static void
	WORD_MIDDLE(
		const struct S_READ *READS,
		const struct S_WORD *W,
		float SCALE,
		float *X,
		float *Y
	)
{
	const struct S_LINE	*L = READS[W->LINE].LINE;

	*X = (W->X + W->W / 2.0F) / SCALE;
	*Y = (float)(L->Y0 + L->Y1) / 2.0F;

	if (L->X1 > L->X0)
		*Y += (L->Y_RIGHT - L->Y_LEFT) * (*X - (float)L->X0) /
			(float)(L->X1 - L->X0);
}

/*
# THE LINES BETWEEN THE ROWS OF A DRAWN TABLE WHOSE WORDS ARE FIRST TO LAST:
# THE PIECES OF LYING LINES AROUND THEM (PAGE_RULES) PUT LEVEL WITH THE
# SLANT OF THE TABLE (*SLANT, FROM THE LONG PIECES), THOSE LESS THAN HALF
# A LINE APART AS ONE. *LEFT: THE X THEY ARE LEVEL AT. RETURNS HOW MANY
# ROW LINES (AT MOST ROW_LINES_MAX, IN ROW_Y FROM THE TOP).
*/
static int
	ROW_LINES(
		const struct S_READ *READS,
		const struct S_WORD *WORDS,
		int FIRST,
		int LAST,
		float SCALE,
		float HEIGHT,
		float *SLANT,
		float *LEFT,
		float *ROW_Y
	)
{
	const float	*E;
	const int	RULES = PAGE_RULES(&E);
	float		PLACES[ROW_LINES_MAX * 4];
	float		BOX[4];
	float		WEIGHT;
	float		SUM;
	int			COUNT;
	int			N;
	int			I;

	BOX[0] = 1E30F;
	BOX[1] = 1E30F;
	BOX[2] = -1E30F;
	BOX[3] = -1E30F;

	for (I = FIRST; I < LAST; ++I)
	{
		float	X;
		float	Y;

		WORD_MIDDLE(READS, &WORDS[I], SCALE, &X, &Y);
		BOX[0] = fminf(BOX[0], WORDS[I].X / SCALE);
		BOX[2] = fmaxf(BOX[2], (WORDS[I].X + WORDS[I].W) / SCALE);
		BOX[1] = fminf(BOX[1], Y - 1.5F * HEIGHT);
		BOX[3] = fmaxf(BOX[3], Y + 1.5F * HEIGHT);
	}

	*LEFT = BOX[0];
	WEIGHT = 0.0F;
	SUM = 0.0F;

	for (I = 0; I < RULES; ++I)
	{
		const float	*R = &E[I * 4];
		const float	LONG = R[2] - R[0];

		if (
			LONG <= 0.0F ||
			R[2] < BOX[0] ||
			R[0] > BOX[2] ||
			(R[1] + R[3]) / 2.0F < BOX[1] ||
			(R[1] + R[3]) / 2.0F > BOX[3]
		)
			continue ;

		SUM += R[3] - R[1];
		WEIGHT += LONG;
	}

	*SLANT = 0.0F;

	if (WEIGHT > 0.0F)
		*SLANT = SUM / WEIGHT;

	COUNT = 0;

	for (I = 0; I < RULES && COUNT < ROW_LINES_MAX * 4; ++I)
	{
		const float	*R = &E[I * 4];
		const float	MX = (R[0] + R[2]) / 2.0F;
		const float	MY = (R[1] + R[3]) / 2.0F;

		if (
			R[2] <= R[0] ||
			R[2] < BOX[0] ||
			R[0] > BOX[2] ||
			MY < BOX[1] ||
			MY > BOX[3]
		)
			continue ;

		PLACES[COUNT++] = MY - *SLANT * (MX - *LEFT);
	}

	qsort(PLACES, (size_t)COUNT, sizeof(float), CMP_HEIGHT);
	N = 0;
	I = 0;

	while (I < COUNT && N < ROW_LINES_MAX)
	{
		float	TOTAL;
		int		J;

		TOTAL = PLACES[I];
		J = I + 1;

		while (J < COUNT && PLACES[J] - PLACES[J - 1] < 0.5F * HEIGHT)
			TOTAL += PLACES[J++];

		ROW_Y[N++] = TOTAL / (float)(J - I);
		I = J;
	}

	return (N);
}

/*
# THE ROWS OF THE WORDS OF A DRAWN TABLE (WORDS FIRST TO LAST): HOW MANY
# LINES BETWEEN ITS ROWS (ROW_LINES) ARE ABOVE THE MIDDLE OF A WORD. A CELL
# WRITTEN HIGHER OR LOWER THAN THE OTHERS OF ITS ROW MAKES AN OCR LINE OF
# ITS OWN, BUT IT IS IN THE SAME ROW. WITHOUT SUCH LINES (ALL IN ONE ROW)
# EVERY OCR LINE IS A ROW. HEIGHT: THE USUAL HEIGHT OF A LINE.
*/
static void
	ROW_KEYS(
		const struct S_READ *READS,
		const struct S_WORD *WORDS,
		int FIRST,
		int LAST,
		float SCALE,
		float HEIGHT,
		int *KEYS
	)
{
	float	ROW_Y[ROW_LINES_MAX];
	float	SLANT;
	float	LEFT;
	int		ROWS;
	int		SPLIT;
	int		I;

	ROWS = ROW_LINES(
		READS,
		WORDS,
		FIRST,
		LAST,
		SCALE,
		HEIGHT,
		&SLANT,
		&LEFT,
		ROW_Y
	);
	SPLIT = 0;

	for (I = FIRST; I < LAST; ++I)
	{
		float	X;
		float	Y;
		int		K;

		WORD_MIDDLE(READS, &WORDS[I], SCALE, &X, &Y);
		Y -= SLANT * (X - LEFT);
		KEYS[I] = 0;

		for (K = 0; K < ROWS; ++K)
			KEYS[I] += ROW_Y[K] < Y;

		SPLIT |= KEYS[I] != KEYS[FIRST];
	}

	for (I = FIRST; I < LAST && !SPLIT; ++I)
		KEYS[I] = WORDS[I].LINE;
}

/* 1 = WORD A COMES AFTER WORD B IN A DRAWN TABLE: ROW, COLUMN, LINE, X */
static int
	WORD_AFTER(
		const struct S_WORD *A,
		int KEY_A,
		const struct S_WORD *B,
		int KEY_B,
		const float *EDGES,
		int EDGE_COUNT
	)
{
	const int	COL_A = EDGE_COLUMN(A, EDGES, EDGE_COUNT);
	const int	COL_B = EDGE_COLUMN(B, EDGES, EDGE_COUNT);

	if (KEY_A != KEY_B)
		return (KEY_A > KEY_B);

	if (COL_A != COL_B)
		return (COL_A > COL_B);

	if (A->LINE != B->LINE)
		return (A->LINE > B->LINE);

	return (A->X > B->X);
}

/*
# THE WORDS OF A DRAWN TABLE (FIRST TO LAST) IN ORDER: ROW BY ROW, CELL BY
# CELL. THEIR SPANS, MARKS AND ROWS MOVE WITH THEM.
*/
static void
	SORT_CELLS(
		struct S_WORD *WORDS,
		struct S_CELL *SPANS,
		unsigned char *FIXED,
		int *KEYS,
		int FIRST,
		int LAST,
		const float *EDGES,
		int EDGE_COUNT
	)
{
	int	I;

	for (I = FIRST + 1; I < LAST; ++I)
	{
		const struct S_WORD		W = WORDS[I];
		const struct S_CELL		C = SPANS[I];
		const unsigned char		F = FIXED[I];
		const int				K = KEYS[I];
		int						J;

		J = I;

		while (
			J > FIRST &&
			WORD_AFTER(&WORDS[J - 1], KEYS[J - 1], &W, K, EDGES, EDGE_COUNT)
		)
		{
			WORDS[J] = WORDS[J - 1];
			SPANS[J] = SPANS[J - 1];
			FIXED[J] = FIXED[J - 1];
			KEYS[J] = KEYS[J - 1];
			--J;
		}

		WORDS[J] = W;
		SPANS[J] = C;
		FIXED[J] = F;
		KEYS[J] = K;
	}
}

/* 1 = A DIGIT IS IN ONE OF THE WORDS FIRST TO LAST */
static int
	LINE_HAS_DIGITS(const struct S_WORD *WORDS, int FIRST, int LAST)
{
	int	I;

	for (I = FIRST; I < LAST; ++I)
		if (strpbrk(WORDS[I].TEXT, "0123456789"))
			return (1);

	return (0);
}

/*
# ANY OTHER PAGE: THE WORDS WITH THEIR PLACES GO TO THE TABLE MAKER, ONE
# PART OF THE PAGE AT A TIME (TWO TABLES ONE UNDER THE OTHER, THEN A TEXT):
# THE PARTS COME ONE UNDER THE OTHER, AN EMPTY ROW BETWEEN THEM. A PART
# THAT IS ONE LINE OF WORDS ONLY (A NOTE) IS ONE CELL.
*/
static int
	WORD_TABLE(
		struct S_READ *READS,
		int COUNT,
		float SCALE,
		struct S_TABLE *TABLE
	)
{
	struct S_WORD	*WORDS;
	struct S_CELL	*CELLS;
	struct S_CELL	*SPANS;
	struct S_CELL	**LIST;
	unsigned char	*FIXED;
	int				*KEYS;
	struct S_VOCAB	V;
	float			HEIGHT;
	int				TOTAL;
	int				N;
	int				WORD_COUNT;
	int				FIRST_WORD;
	int				FIRST_CELL;
	int				I;
	int				OK;

	TOTAL = 0;

	for (I = 0; I < COUNT; ++I)
		TOTAL += READS[I].COUNT + 1;

	WORDS = (struct S_WORD *)calloc((size_t)TOTAL + 1, sizeof(*WORDS));
	CELLS = (struct S_CELL *)calloc((size_t)TOTAL + 1, sizeof(*CELLS));
	SPANS = (struct S_CELL *)calloc((size_t)TOTAL + 1, sizeof(*SPANS));
	LIST = (struct S_CELL **)calloc((size_t)TOTAL + 1, sizeof(*LIST));
	FIXED = (unsigned char *)calloc((size_t)TOTAL + 1, 1);
	KEYS = (int *)calloc((size_t)TOTAL + 1, sizeof(int));

	if (!WORDS || !CELLS || !SPANS || !LIST || !FIXED || !KEYS)
	{
		free(WORDS);
		free(CELLS);
		free(SPANS);
		free(LIST);
		free(FIXED);
		free(KEYS);
		return (0);
	}

	N = 0;

	for (I = 0; I < COUNT; ++I)
	{
		const struct S_READ	*R = &READS[I];
		int					G;

		G = 0;

		while (G < R->COUNT && !R->DROP)
		{
			int	END;

			while (G < R->COUNT && R->GLYPH[G].CLASS == G__SPACE__)
				++G;

			if (G >= R->COUNT)
				break ;

			END = WORD_END(R, G);
			MAKE_CELL(R, I, G, END, &CELLS[N]);
			WORDS[N].LINE = I;
			WORDS[N].X = (float)R->GLYPH[G].FIRST;
			WORDS[N].W = (float)(R->GLYPH[END].LAST - R->GLYPH[G].FIRST + 1);
			LIST[N] = &CELLS[N];
			++N;
			G = END + 1;
		}
	}

	/* THE WORDS AGAINST EACH OTHER AND THE KNOWN WORDS, THE NUMBERS ALONE */
	if (MAKE_VOCAB(&V, LIST, N))
	{
		for (I = 0; I < N; ++I)
		{
			const struct S_READ	*R = &READS[CELLS[I].LINE];

			if (!CORRECT_DATE(R, &CELLS[I]))
				CORRECT_NUMBER(R, &CELLS[I]);

			if (!strpbrk(CELLS[I].TEXT, "0123456789"))
				CORRECT_WORD(R, &CELLS[I], &V);
		}

		FREE_VOCAB(&V);
	}

	for (I = 0; I < N; ++I)
		snprintf(WORDS[I].TEXT, sizeof(WORDS[I].TEXT), "%s", CELLS[I].TEXT);

	memcpy(SPANS, CELLS, (size_t)N * sizeof(*SPANS));
	WORD_COUNT = JOIN_CELL_WORDS(READS, WORDS, SPANS, N);

	for (I = 0; I < WORD_COUNT; ++I)
	{
		int	SMALL_BEFORE;

		SMALL_BEFORE = 0;

		if (I > 0 && WORDS[I - 1].LINE == WORDS[I].LINE)
			SMALL_BEFORE = ENDS_SMALL(
				WORDS[I - 1].TEXT,
				WORDS[I - 1].TEXT + strlen(WORDS[I - 1].TEXT)
			);

		SMALL_L(WORDS[I].TEXT, SMALL_BEFORE);
	}

	/* STEPS OF THE LINE TO PIXELS OF THE PAGE (EVERY STEP IS 4 COLUMNS) */
	for (I = 0; I < WORD_COUNT; ++I)
	{
		const struct S_LINE	*L = READS[WORDS[I].LINE].LINE;
		const float			F = SCALE * 4.0F * L->SCALE;

		WORDS[I].X = SCALE * (float)L->LEFT + WORDS[I].X * F;
		WORDS[I].W *= F;
		WORDS[I].Y = SCALE * (float)L->Y0;
		WORDS[I].H = SCALE * (float)(L->Y1 - L->Y0);
	}

	/* THE PARTS OF THE PAGE: RUNS OF LINES (THEIR WORDS AND CELLS) */
	HEIGHT = USUAL_LINE(READS, COUNT);
	memset(TABLE, 0, sizeof(*TABLE));
	FIRST_WORD = 0;
	FIRST_CELL = 0;
	OK = 1;

	while (FIRST_WORD < WORD_COUNT && OK)
	{
		struct S_TABLE	PART;
		float			EDGES[EDGES_MAX];
		int				EDGE_COUNT;
		int				LAST_WORD;
		int				LAST_CELL;
		int				BUILT;

		LAST_WORD = FIRST_WORD + 1;

		while (
			LAST_WORD < WORD_COUNT &&
			(
				WORDS[LAST_WORD].LINE == WORDS[LAST_WORD - 1].LINE ||
				!NEW_BLOCK(
					READS[WORDS[LAST_WORD - 1].LINE].LINE,
					READS[WORDS[LAST_WORD].LINE].LINE,
					HEIGHT
				)
			)
		)
			++LAST_WORD;

		LAST_CELL = FIRST_CELL;

		while (
			LAST_CELL < N &&
			CELLS[LAST_CELL].LINE <= WORDS[LAST_WORD - 1].LINE
		)
			++LAST_CELL;

		EDGE_COUNT = WALL_EDGES(
			READS,
			WORDS,
			FIRST_WORD,
			LAST_WORD,
			SCALE,
			SCALE * HEIGHT,
			EDGES
		);

		if (
			!EDGE_COUNT &&
			WORDS[FIRST_WORD].LINE == WORDS[LAST_WORD - 1].LINE &&
			!LINE_HAS_DIGITS(WORDS, FIRST_WORD, LAST_WORD)
		)
			BUILT = LAYOUT_GRID(
				WORDS + FIRST_WORD,
				NULL,
				LAST_WORD - FIRST_WORD,
				EDGES,
				0,
				&PART
			);
		else if (EDGE_COUNT)
		{
			ROW_KEYS(
				READS,
				WORDS,
				FIRST_WORD,
				LAST_WORD,
				SCALE,
				HEIGHT,
				KEYS
			);
			SORT_CELLS(
				WORDS,
				SPANS,
				FIXED,
				KEYS,
				FIRST_WORD,
				LAST_WORD,
				EDGES,
				EDGE_COUNT
			);
			FIX_DRAWN_COLUMNS(
				READS,
				WORDS,
				SPANS,
				KEYS,
				FIRST_WORD,
				LAST_WORD,
				EDGES,
				EDGE_COUNT,
				FIXED
			);
			BUILT = LAYOUT_GRID(
				WORDS + FIRST_WORD,
				KEYS + FIRST_WORD,
				LAST_WORD - FIRST_WORD,
				EDGES,
				EDGE_COUNT,
				&PART
			);
		}
		else
			BUILT = LAYOUT_BUILD(
				WORDS + FIRST_WORD,
				LAST_WORD - FIRST_WORD,
				&PART
			);

		if (BUILT)
		{
			MARK_DOUBTS(
				READS,
				CELLS + FIRST_CELL,
				LAST_CELL - FIRST_CELL,
				&PART
			);
			MARK_FIXED(&PART, WORDS, FIXED, FIRST_WORD, LAST_WORD);
			OK = LAYOUT_APPEND(TABLE, &PART);

			if (!OK)
				LAYOUT_FREE(&PART);
		}

		FIRST_WORD = LAST_WORD;
		FIRST_CELL = LAST_CELL;
	}

	if (!OK || !TABLE->ROWS)
	{
		LAYOUT_FREE(TABLE);
		OK = 0;
	}

	free(WORDS);
	free(CELLS);
	free(SPANS);
	free(LIST);
	free(FIXED);
	free(KEYS);
	return (OK);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          TABLES                          |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                        PUBLIC API                        |  \\//  */
/*   \/   #==========================================================#   \/   */
/*
# THE LINES OF THE PAGE, A BIG PHOTO OR BIG LETTERS MADE SMALLER FIRST, VERY
# SMALL LETTERS MADE TWICE AS BIG. *SCALE: PICTURE PIXELS PER LINE PIXEL.
*/
static int
	FIND_LINES(
		const unsigned char *GRAY,
		int WIDTH,
		int HEIGHT,
		struct S_LINE **LINES,
		float *SCALE
	)
{
	unsigned char	*SMALL;
	int				COUNT;
	int				F;

	F = 1;

	while (WIDTH / F > PAGE_SIDE_MAX || HEIGHT / F > PAGE_SIDE_MAX)
		++F;

	*SCALE = (float)F;
	SMALL = NULL;

	if (F > 1)
	{
		int	NW;
		int	NH;

		SMALL = SHRINK(GRAY, WIDTH, HEIGHT, F, &NW, &NH);

		if (!SMALL)
			return (0);

		GRAY = SMALL;
		WIDTH = NW;
		HEIGHT = NH;
	}

	COUNT = PAGE_LINES(GRAY, WIDTH, HEIGHT, LINES);

	if (COUNT && PAGE_LETTER_HEIGHT() > LETTER_MAX)
	{
		const int		LETTER = PAGE_LETTER_HEIGHT();
		const int		SHRINK_BY = (LETTER + LETTER_GOAL / 2) / LETTER_GOAL;
		unsigned char	*SMALLER;
		int				NW;
		int				NH;

		SMALLER = SHRINK(GRAY, WIDTH, HEIGHT, SHRINK_BY, &NW, &NH);

		if (SMALLER)
		{
			PAGE_FREE(*LINES, COUNT);
			COUNT = PAGE_LINES(SMALLER, NW, NH, LINES);
			free(SMALLER);
			*SCALE *= (float)SHRINK_BY;
		}
	}
	else if (
		COUNT &&
		PAGE_LETTER_HEIGHT() < LETTER_LEAST &&
		WIDTH * 2 <= PAGE_SIDE_MAX &&
		HEIGHT * 2 <= PAGE_SIDE_MAX
	)
	{
		unsigned char	*BIGGER;

		BIGGER = GROW(GRAY, WIDTH, HEIGHT);

		if (BIGGER)
		{
			PAGE_FREE(*LINES, COUNT);
			COUNT = PAGE_LINES(BIGGER, WIDTH * 2, HEIGHT * 2, LINES);
			free(BIGGER);
			*SCALE /= 2.0F;
		}
	}

	free(SMALL);
	return (COUNT);
}

struct S_HAND
	*HAND_OPEN(const unsigned char *GRAY, int WIDTH, int HEIGHT)
{
	struct S_HAND	*H;
	int				I;

	if (!G__CLASSES__ || G__SPACE__ < 0 || G__DASH__ < 0)
		return (NULL);

	H = (struct S_HAND *)calloc(1, sizeof(*H));

	if (!H)
		return (NULL);

	H->COUNT = FIND_LINES(GRAY, WIDTH, HEIGHT, &H->LINES, &H->SCALE);

	if (H->COUNT)
		H->READS = (struct S_READ *)calloc((size_t)H->COUNT, sizeof(*H->READS));

	if (!H->READS)
	{
		HAND_CLOSE(H);
		return (NULL);
	}

	for (I = 0; I < H->COUNT; ++I)
		H->READS[I].LINE = &H->LINES[I];

	return (H);
}

/* THE LINES THAT WERE NOT READ YET (ALL OF THEM, OR A FEW SPREAD OUT) */
static void
	READ_PAGE(struct S_HAND *H, int MOST)
{
	int	*WHICH;
	int	COUNT;
	int	I;

	WHICH = (int *)malloc((size_t)H->COUNT * sizeof(int));

	if (!WHICH)
		return ;

	COUNT = 0;

	if (MOST > H->COUNT)
		MOST = H->COUNT;

	for (I = 0; I < MOST; ++I)
	{
		const int	K = (int)(((long)I * H->COUNT + H->COUNT / 2) / MOST);

		if (!H->READS[K].DONE)
			WHICH[COUNT++] = K;
	}

	READ_LINES(H->READS, WHICH, COUNT);
	free(WHICH);
}

/*
# CLASSES OF A TEXT FROM ANOTHER READER: CHARACTERS THIS READER DOES NOT
# KNOW ARE LEFT OUT. RETURNS THEIR NUMBER.
*/
static int
	KNOWN_CLASSES(const char *TEXT, int *DEST, int MOST)
{
	int	COUNT;

	COUNT = 0;

	while (*TEXT && COUNT < MOST)
	{
		int	LEN;
		int	K;

		K = CLASS_OF(TEXT, &LEN);

		if (K)
			DEST[COUNT++] = K;

		TEXT += LEN;
	}

	return (COUNT);
}

double
	HAND_JUDGE(struct S_HAND *H, const struct S_WORD *WORDS, int COUNT)
{
	double	COST;
	int		CHARS;
	int		MISSED;
	int		SEEN;
	int		I;

	READ_PAGE(H, JUDGE_LINES);
	COST = 0.0;
	CHARS = 0;
	MISSED = 0;
	SEEN = 0;

	for (I = 0; I < H->COUNT; ++I)
	{
		const struct S_READ	*R = &H->READS[I];
		const struct S_LINE	*L = R->LINE;
		char				TEXT[1024];
		int					LABELS[512];
		size_t				LEN;
		double				LEFT;
		int					N;

		if (!R->DONE || !R->LOGP || !R->COUNT)
			continue ;

		++SEEN;
		LEN = 0;
		TEXT[0] = '\0';
		LEFT = -1E9;

		/* THE WORDS ON THIS LINE, FROM THE LEFT (A FEW WORDS: NO SORTING) */
		for (;;)
		{
			double	NEXT;
			int		BEST;
			int		W;

			NEXT = 1E9;
			BEST = -1;

			for (W = 0; W < COUNT; ++W)
			{
				float	X;
				float	Y;

				/* THE MIDDLE OF THE WORD ON THE PAGE OF THE LINES */
				X = (WORDS[W].X + WORDS[W].W / 2.0F) / H->SCALE;
				Y = (WORDS[W].Y + WORDS[W].H / 2.0F) / H->SCALE;
				PAGE_TURN(&X, &Y);

				if (
					Y < (float)L->Y0 ||
					Y > (float)L->Y1 ||
					X < (float)L->X0 ||
					X > (float)L->X1 ||
					X <= LEFT ||
					X >= NEXT
				)
					continue ;

				NEXT = X;
				BEST = W;
			}

			if (BEST < 0 || LEN + strlen(WORDS[BEST].TEXT) + 2 > sizeof(TEXT))
				break ;

			if (LEN)
				TEXT[LEN++] = ' ';

			strcpy(TEXT + LEN, WORDS[BEST].TEXT);
			LEN += strlen(WORDS[BEST].TEXT);
			LEFT = NEXT;
		}

		N = KNOWN_CLASSES(TEXT, LABELS, 512);

		/* THE OCR SAW NOTHING, OR REACH LESS THAN THE READER, ON THIS LINE */
		if (N * 2 < R->COUNT)
		{
			++MISSED;
			continue ;
		}

		/* (A LINE THE TEXT CAN NOT FIT AT ALL IS VERY BAD, NOT ALL THAT IS) */
		COST += fmin(
			R->BEST - CTC_SCORE(R->LOGP, G__CLASSES__, 0, R->STEPS, LABELS, N),
			JUDGE_WORST * N
		);
		CHARS += N;
	}

	if (!SEEN)
		return (-1.0);

	if (MISSED * 3 > SEEN || !CHARS)
		return (1E9);

	return (COST / CHARS);
}

int
	HAND_TABLE(struct S_HAND *H, struct S_TABLE *TABLE)
{
	int	FIELDS;
	int	I;

	memset(TABLE, 0, sizeof(*TABLE));
	READ_PAGE(H, H->COUNT);

	/* A LINE WITH NO LETTER AND NO DIGIT ("---", "=") IS NO LINE */
	for (I = 0; I < H->COUNT; ++I)
	{
		struct S_READ	*R = &H->READS[I];
		int				G;

		R->DROP = 1;

		for (G = 0; G < R->COUNT; ++G)
			if (G__ALNUM__[R->GLYPH[G].CLASS])
				R->DROP = 0;

		if (!R->LOGP)
			R->DROP = 1;
	}

	FIELDS = LIST_FIELDS(H->READS, H->COUNT);

	if (FIELDS >= 2)
		return (LIST_TABLE(H->READS, H->COUNT, FIELDS, TABLE));

	return (WORD_TABLE(H->READS, H->COUNT, H->SCALE, TABLE));
}

void
	HAND_CLOSE(struct S_HAND *H)
{
	int	I;

	if (!H)
		return ;

	for (I = 0; H->READS && I < H->COUNT; ++I)
		free(H->READS[I].LOGP);

	free(H->READS);

	if (H->LINES)
		PAGE_FREE(H->LINES, H->COUNT);

	free(H);
}

int
	HAND_READ(
		const unsigned char *GRAY,
		int WIDTH,
		int HEIGHT,
		struct S_TABLE *TABLE
	)
{
	struct S_HAND	*H;
	int				OK;

	memset(TABLE, 0, sizeof(*TABLE));
	H = HAND_OPEN(GRAY, WIDTH, HEIGHT);

	if (!H)
		return (0);

	OK = HAND_TABLE(H, TABLE);
	HAND_CLOSE(H);
	return (OK);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                        PUBLIC API                        |  //\\  */
/* '""""' #==========================================================# '""""' */
