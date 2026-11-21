/******************************************************************************\
# C - layout                                     #       Maximum Tension       #
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
# TURNS OCR WORDS INTO A TABLE:
# 1. BORDER LINES THAT THE OCR READ AS "|" OR "___" ARE DROPPED.
# 2. A PHOTO IS OFTEN A LITTLE TURNED. THE ANGLE WHERE THE WORDS STACK INTO
#    THE SHARPEST ROWS IS THE TILT, AND EVERY WORD IS TURNED BACK BY IT.
# 3. WORDS OF ONE OCR LINE THAT STAND CLOSE TOGETHER BECOME ONE SEGMENT.
# 4. EVERY SEGMENT IS LINKED TO THE NEAREST ONE ON ITS RIGHT AT THE SAME
#    HEIGHT; LINKED SEGMENTS ARE ONE ROW. SO A ROW MAY BEND A LITTLE.
# 5. A PHOTO TAKEN FROM THE SIDE MAKES THE COLUMNS LEAN AND NARROW TOWARDS THE
#    FAR END. THE LEAN AND NARROWING THAT GIVE THE CLEANEST GAPS ARE UNDONE.
# 6. A COLUMN BORDER IS WHERE (ALMOST) NO ROW HAS A SEGMENT.
# 7. EVERY WORD GOES TO THE COLUMN UNDER ITS MIDDLE. A ROW WITH ONE SEGMENT
#    ONLY (A TITLE) STAYS TOGETHER IN THE COLUMN WHERE IT STARTS.
# 8. IN A COLUMN OF NUMBERS, "O" BECOMES "0" AND "l" BECOMES "1", AND THE
#    AMOUNTS ARE WRITTEN ONE WAY ("12.362.89" AMONG "14.345,22"). IN A
#    COLUMN OF WORDS, A MISREAD WORD BECOMES THE ONE IT MISSES ("Matkpp").
# A TABLE DRAWN BY HAND NEEDS NONE OF 2 TO 7: ITS ROWS AND COLUMNS ARE KNOWN
# (LAYOUT_GRID). TABLES ONE UNDER THE OTHER ARE JOINED (LAYOUT_APPEND).
*/

/* **************************** [v] INCLUDES [v] **************************** */
#include <math.h>
/******* """"""""
 * float	cosf(float);
 * float	fabsf(float);
 * float	sinf(float);
 */
#include <stdio.h>
/******* """""""""
 * typedef	FILE;
 * int		ferror(FILE *);
 * int		fputc(int, FILE *);
 * int		fputs(const char *, FILE *);
 */
#include <stdlib.h>
/******* """"""""""
 * void	*calloc(size_t, size_t);
 * void	free(void *);
 * void	*malloc(size_t);
 * void	qsort(void *, size_t, size_t, int *);
 * void	*realloc(void *, size_t);
 */
#include <string.h>
/******* """"""""""
 * #define NULL
 * typedef	size_t;
 * void		*memcpy(void *, const void *, size_t);
 * void		*memmove(void *, const void *, size_t);
 * void		*memset(void *, int, size_t);
 * char		*strcat(char *, const char *);
 * char		*strchr(const char *, int);
 * int		strcmp(const char *, const char *);
 * char		*strcpy(char *, const char *);
 * size_t	strlen(const char *);
 */
#include "piccell.h"
/******* """""""""
 * struct S_WORD;
 */
#include "layout.h"
/******* """"""""""
 * struct S_TABLE;
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define GAP_SPLIT 0.8F /* A GAP OVER 0.8 WORD HEIGHTS STARTS A NEW SEGMENT */
#define ROW_NEAR 0.6F /* MIDDLES CLOSER THAN 0.6 WORD HEIGHTS: SAME ROW */
#define MIN_GAP 0.25F /* A COLUMN BORDER IS AT LEAST 0.25 WORD HEIGHTS WIDE */
#define TILT_STEPS 80 /* TILTS FROM -8 TO 8 DEGREES ARE TRIED, 0.1 APART */
#define LEAN_STEPS 20 /* COLUMN LEANS FROM -0.08 TO 0.08 ARE TRIED */
#define NARROW_STEPS 30 /* NARROWINGS FROM -1.5E-4 TO 1.5E-4 PER PIXEL */
#define TALL_LINE 1.6F /* "I" TALLER THAN 1.6 WORD HEIGHTS IS A BORDER LINE */
#define NUMBER_SHARE 0.6F /* 60 PERCENT NUMBERS: A NUMBER COLUMN */
#define WORD_FIX_LONGEST 63 /* LONGER CELLS ARE NOT COMPARED (FIX_WORDS) */
#define ALIGN_NEAR 0.5F /* EDGES CLOSER THAN 0.5 WORD HEIGHTS ARE ALIGNED */
#define ALIGN_SHARE 0.6F /* A REAL COLUMN HAS 60 PERCENT OF ITS ROWS ALIGNED */
#define RADIANS(DEGREES) ((DEGREES) * 3.14159265F / 180.0F)

struct	S_PIECE /* ONE WORD, LATER WITHOUT THE TILT */
{
	int		WORD; /* INDEX IN THE WORD LIST */
	int		LINE;
	float	CX;
	float	CY;
	float	X0;
	float	X1;
};

struct	S_SEG /* WORDS OF ONE OCR LINE THAT STAND CLOSE TOGETHER */
{
	int		FIRST; /* FIRST WORD IN THE SORTED PIECES */
	int		COUNT;
	int		ROW;
	float	X0;
	float	X1;
	float	CY;
};

struct	S_KEY /* FOR SORTING BY ONE NUMBER */
{
	float	VALUE;
	int		INDEX;
};

struct	S_ITEM /* ONE WORD IN ITS CELL */
{
	int		ROW;
	int		COL;
	int		BEFORE; /* COLUMN OF THE WORD BEFORE IT IN ITS SEGMENT, OR -1 */
	int		ALONE; /* 1 IN A ROW WITH ONE SEGMENT ONLY (A TITLE) */
	float	X;
	float	X1;
	int		WORD;
};

struct	S_BAND /* ONE COLUMN, IN PIXELS FROM THE LEFT EDGE OF THE TEXT */
{
	int	START;
	int	END;
};

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                         HELPERS                          |  \\//  */
/*   \/   #==========================================================#   \/   */
static int
	CMP_INT(const void *A, const void *B)
{
	return (*(const int *)A - *(const int *)B);
}

static int
	CMP_FLOAT(const void *A, const void *B)
{
	const float	FA = *(const float *)A;
	const float	FB = *(const float *)B;

	return ((FA > FB) - (FA < FB));
}

static int
	CMP_KEY(const void *A, const void *B)
{
	const struct S_KEY	*KA = (const struct S_KEY *)A;
	const struct S_KEY	*KB = (const struct S_KEY *)B;

	return ((KA->VALUE > KB->VALUE) - (KA->VALUE < KB->VALUE));
}

/* BY OCR LINE, THEN FROM LEFT TO RIGHT */
static int
	CMP_PIECE(const void *A, const void *B)
{
	const struct S_PIECE	*PA = (const struct S_PIECE *)A;
	const struct S_PIECE	*PB = (const struct S_PIECE *)B;

	if (PA->LINE != PB->LINE)
		return ((PA->LINE > PB->LINE) - (PA->LINE < PB->LINE));

	return ((PA->X0 > PB->X0) - (PA->X0 < PB->X0));
}

/* BY ROW, THEN COLUMN, THEN FROM LEFT TO RIGHT */
static int
	CMP_ITEM(const void *A, const void *B)
{
	const struct S_ITEM	*IA = (const struct S_ITEM *)A;
	const struct S_ITEM	*IB = (const struct S_ITEM *)B;

	if (IA->ROW != IB->ROW)
		return (IA->ROW - IB->ROW);

	if (IA->COL != IB->COL)
		return (IA->COL - IB->COL);

	return ((IA->X > IB->X) - (IA->X < IB->X));
}

/* SORTS VALUES IN PLACE */
static float
	MEDIAN(float *VALUES, int COUNT)
{
	if (COUNT <= 0)
		return (0.0F);

	qsort(VALUES, (size_t)COUNT, sizeof(float), CMP_FLOAT);
	return (VALUES[COUNT / 2]);
}

/*
# A BORDER LINE READ AS TEXT: ONLY "|", "[", "]", "_" OR LONG DASHES, OR ONE
# "I", "l", "1", "!" THAT IS MUCH TALLER THAN THE TEXT AROUND IT.
*/
static int
	IS_BORDER(const struct S_WORD *WORD, float HEIGHT)
{
	const unsigned char	*P;
	int					DASHES;

	if (
		WORD->H > TALL_LINE * HEIGHT &&
		WORD->TEXT[1] == '\0' &&
		strchr("Il1!|", WORD->TEXT[0])
	)
		return (1);

	DASHES = 0;

	for (P = (const unsigned char *)WORD->TEXT; *P; ++P)
	{
		if (*P == '|' || *P == '[' || *P == ']' || *P == '_')
			continue ;

		if (*P == '-')
		{
			++DASHES;
			continue ;
		}

		/* "—", "–" AND "─" ARE E2 80 94, E2 80 93 AND E2 94 80 IN UTF-8 */
		if (P[0] == 0XE2 && (P[1] == 0X80 || P[1] == 0X94) && P[2])
		{
			P += 2;
			DASHES += 2;
			continue ;
		}

		return (0);
	}

	/* ONE "-" IS A REAL CELL VALUE (NOTHING), "---" IS A LINE */
	return (DASHES != 1);
}

/* A BORDER THE OCR GLUED TO A WORD ("Güneş_", "|Muğla") IS TAKEN OFF */
static void
	TRIM_BORDER(char *TEXT)
{
	size_t	LEN;
	size_t	SKIP;

	LEN = strlen(TEXT);

	while (LEN > 1 && (TEXT[LEN - 1] == '_' || TEXT[LEN - 1] == '|'))
		TEXT[--LEN] = '\0';

	SKIP = 0;

	while (SKIP + 1 < LEN && (TEXT[SKIP] == '_' || TEXT[SKIP] == '|'))
		++SKIP;

	if (SKIP)
		memmove(TEXT, TEXT + SKIP, LEN - SKIP + 1);
}

/* A LONE ",", ":", ";", "'" OR "`": A SPECK OF A BORDER, NOT A WORD */
static int
	IS_SPECK(const struct S_WORD *WORD)
{
	return (
		WORD->TEXT[0] &&
		!WORD->TEXT[1] &&
		strchr(",:;'`", WORD->TEXT[0]) != NULL
	);
}

/*
# HOW SHARP THE ROWS ARE WHEN THE WORDS ARE TURNED BY ANGLE: THE MIDDLES ARE
# COUNTED IN STRIPS OF A THIRD OF A WORD HEIGHT; FULL STRIPS AND EMPTY STRIPS
# (SUM OF SQUARES) MEAN SHARP ROWS.
*/
static double
	ROW_SHARPNESS(
		const struct S_PIECE *P,
		int COUNT,
		float HEIGHT,
		float ANGLE,
		int *STRIPS,
		int STRIP_COUNT
	)
{
	const float	C = cosf(ANGLE);
	const float	S = sinf(ANGLE);
	double		SUM;
	float		LOW;
	int			I;

	LOW = 1E30F;

	for (I = 0; I < COUNT; ++I)
	{
		const float	Y = P[I].CY * C - P[I].CX * S;

		if (Y < LOW)
			LOW = Y;
	}

	memset(STRIPS, 0, (size_t)STRIP_COUNT * sizeof(int));

	for (I = 0; I < COUNT; ++I)
	{
		const float	Y = P[I].CY * C - P[I].CX * S;
		const int	STRIP = (int)((Y - LOW) * 3.0F / HEIGHT);

		if (STRIP >= 0 && STRIP < STRIP_COUNT)
			STRIPS[STRIP] += 1;
	}

	SUM = 0.0;

	for (I = 0; I < STRIP_COUNT; ++I)
		SUM += (double)STRIPS[I] * STRIPS[I];

	return (SUM);
}

/* THE TILT IN RADIANS: FIRST IN STEPS OF 0.1 DEGREES, THEN OF 0.01 */
static float
	FIND_TILT(const struct S_PIECE *P, int COUNT, float HEIGHT)
{
	int		*STRIPS;
	double	BEST_SCORE;
	float	SPAN;
	float	BEST;
	float	CENTER;
	int		STRIP_COUNT;
	int		I;

	SPAN = 0.0F;

	for (I = 0; I < COUNT; ++I)
	{
		if (fabsf(P[I].CX) > SPAN)
			SPAN = fabsf(P[I].CX);

		if (fabsf(P[I].CY) > SPAN)
			SPAN = fabsf(P[I].CY);
	}

	STRIP_COUNT = (int)(SPAN * 3.0F * 3.0F / HEIGHT) + 8;
	STRIPS = (int *)malloc((size_t)STRIP_COUNT * sizeof(int));

	if (!STRIPS)
		return (0.0F);

	BEST = 0.0F;
	BEST_SCORE = -1.0;

	for (I = -TILT_STEPS; I <= TILT_STEPS; ++I)
	{
		const float		ANGLE = RADIANS((float)I * 0.1F);
		const double	SCORE = ROW_SHARPNESS(
			P,
			COUNT,
			HEIGHT,
			ANGLE,
			STRIPS,
			STRIP_COUNT
		);

		if (SCORE > BEST_SCORE)
		{
			BEST_SCORE = SCORE;
			BEST = ANGLE;
		}
	}

	CENTER = BEST;

	for (I = -10; I <= 10; ++I)
	{
		const float		ANGLE = CENTER + RADIANS((float)I * 0.01F);
		const double	SCORE = ROW_SHARPNESS(
			P,
			COUNT,
			HEIGHT,
			ANGLE,
			STRIPS,
			STRIP_COUNT
		);

		if (SCORE > BEST_SCORE)
		{
			BEST_SCORE = SCORE;
			BEST = ANGLE;
		}
	}

	free(STRIPS);
	return (BEST);
}

/* ROOT OF A UNION-FIND TREE, WITH PATH HALVING */
static int
	ROOT(int *PARENT, int I)
{
	while (PARENT[I] != I)
	{
		PARENT[I] = PARENT[PARENT[I]];
		I = PARENT[I];
	}

	return (I);
}

/* THE COLUMN UNDER X, OR THE NEAREST ONE WHEN X IS IN A GAP */
static int
	FIND_BAND(const struct S_BAND *BANDS, int COUNT, float X)
{
	float	BEST_DISTANCE;
	int		BEST;
	int		I;

	BEST = 0;
	BEST_DISTANCE = 1E30F;

	for (I = 0; I < COUNT; ++I)
	{
		float	DISTANCE;

		if (X >= (float)BANDS[I].START && X <= (float)BANDS[I].END + 1.0F)
			return (I);

		if (X < (float)BANDS[I].START)
			DISTANCE = (float)BANDS[I].START - X;
		else
			DISTANCE = X - (float)BANDS[I].END;

		if (DISTANCE < BEST_DISTANCE)
		{
			BEST_DISTANCE = DISTANCE;
			BEST = I;
		}
	}

	return (BEST);
}

/* 1 IF THE TEXT IS A NUMBER, MAYBE WITH "O" FOR "0" OR "l" FOR "1" IN IT */
static int
	NUMBER_LIKE(const char *TEXT)
{
	int	DIGITS;
	int	ODD;

	DIGITS = 0;
	ODD = 0;

	for (; *TEXT; ++TEXT)
	{
		if (*TEXT >= '0' && *TEXT <= '9')
			++DIGITS;
		else if (strchr("OoIl|", *TEXT))
			++ODD;
		else if (!strchr(".,-+%()/: ", *TEXT))
			return (0);
	}

	return (DIGITS > 0 && ODD <= DIGITS);
}

/* THE VALUE THAT THE MOST OTHER VALUES ARE NEAR (WITHIN TOLERANCE) */
static float
	MOST_COMMON(const float *VALUES, const int *HAS, int COUNT, float TOLERANCE)
{
	float	BEST;
	int		BEST_COUNT;
	int		I;

	BEST = 0.0F;
	BEST_COUNT = -1;

	for (I = 0; I < COUNT; ++I)
	{
		int	FOUND;
		int	J;

		if (!HAS[I])
			continue ;

		FOUND = 0;

		for (J = 0; J < COUNT; ++J)
			FOUND += HAS[J] && fabsf(VALUES[J] - VALUES[I]) <= TOLERANCE;

		if (FOUND > BEST_COUNT)
		{
			BEST_COUNT = FOUND;
			BEST = VALUES[I];
		}
	}

	return (BEST);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                         HELPERS                          |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          STEPS                           |  \\//  */
/*   \/   #==========================================================#   \/   */
/* STEPS 1 AND 2: WORDS TO PIECES, TILT TAKEN OUT. RETURNS THEIR NUMBER */
static int
	MAKE_PIECES(
		const struct S_WORD *WORDS,
		int COUNT,
		float HEIGHT,
		struct S_PIECE *P
	)
{
	float	ANGLE;
	float	C;
	float	S;
	int		KEPT;
	int		I;

	KEPT = 0;

	for (I = 0; I < COUNT; ++I)
	{
		if (
			!WORDS[I].TEXT[0] ||
			IS_BORDER(&WORDS[I], HEIGHT) ||
			IS_SPECK(&WORDS[I])
		)
			continue ;

		P[KEPT].WORD = I;
		P[KEPT].LINE = WORDS[I].LINE;
		P[KEPT].CX = WORDS[I].X + WORDS[I].W / 2.0F;
		P[KEPT].CY = WORDS[I].Y + WORDS[I].H / 2.0F;
		P[KEPT].X0 = WORDS[I].X;
		++KEPT;
	}

	ANGLE = FIND_TILT(P, KEPT, HEIGHT);
	C = cosf(ANGLE);
	S = sinf(ANGLE);

	for (I = 0; I < KEPT; ++I)
	{
		const float	X = P[I].CX * C + P[I].CY * S;
		const float	HALF = WORDS[P[I].WORD].W / 2.0F;

		P[I].CY = P[I].CY * C - P[I].CX * S;
		P[I].CX = X;
		P[I].X0 = X - HALF;
		P[I].X1 = X + HALF;
	}

	qsort(P, (size_t)KEPT, sizeof(*P), CMP_PIECE);
	return (KEPT);
}

/* STEP 3: CLOSE WORDS OF ONE OCR LINE TO SEGMENTS. RETURNS THEIR NUMBER */
static int
	MAKE_SEGMENTS(
		const struct S_PIECE *P,
		int COUNT,
		float HEIGHT,
		struct S_SEG *S
	)
{
	int	SEGS;
	int	I;

	SEGS = 0;

	for (I = 0; I < COUNT; ++I)
	{
		if (
			!SEGS ||
			P[I].LINE != P[I - 1].LINE ||
			P[I].X0 - S[SEGS - 1].X1 > GAP_SPLIT * HEIGHT
		)
		{
			S[SEGS].FIRST = I;
			S[SEGS].COUNT = 0;
			S[SEGS].X0 = P[I].X0;
			S[SEGS].X1 = P[I].X1;
			S[SEGS].CY = 0.0F;
			++SEGS;
		}

		S[SEGS - 1].COUNT += 1;
		S[SEGS - 1].CY += P[I].CY;

		if (P[I].X1 > S[SEGS - 1].X1)
			S[SEGS - 1].X1 = P[I].X1;
	}

	for (I = 0; I < SEGS; ++I)
		S[I].CY /= (float)S[I].COUNT;

	return (SEGS);
}

/*
# STEP 4: ROWS. EVERY SEGMENT IS LINKED TO THE NEAREST SEGMENT ON ITS RIGHT
# WHOSE MIDDLE IS LESS THAN ROW_NEAR WORD HEIGHTS HIGHER OR LOWER. LINKED
# SEGMENTS ARE ONE ROW; THE ROWS ARE NUMBERED FROM THE TOP. RETURNS THE NUMBER
# OF ROWS.
*/
static int
	MAKE_ROWS(struct S_SEG *S, int SEGS, float HEIGHT)
{
	int				*PARENT;
	int				*ROW_OF;
	float			*SUM;
	int				*IN_ROW;
	struct S_KEY	*KEYS;
	int				ROWS;

	PARENT = (int *)malloc((size_t)SEGS * sizeof(int));
	ROW_OF = (int *)malloc((size_t)SEGS * sizeof(int));
	SUM = (float *)calloc((size_t)SEGS, sizeof(float));
	IN_ROW = (int *)calloc((size_t)SEGS, sizeof(int));
	KEYS = (struct S_KEY *)malloc((size_t)SEGS * sizeof(*KEYS));
	ROWS = 0;

	if (PARENT && ROW_OF && SUM && IN_ROW && KEYS)
	{
		int	I;

		for (I = 0; I < SEGS; ++I)
			PARENT[I] = I;

		for (I = 0; I < SEGS; ++I)
		{
			float	BEST_GAP;
			int		BEST;
			int		J;

			BEST = -1;
			BEST_GAP = 1E30F;

			for (J = 0; J < SEGS; ++J)
			{
				const float	GAP = S[J].X0 - S[I].X1;

				if (
					J == I ||
					S[J].X0 <= S[I].X0 ||
					GAP < -0.5F * HEIGHT ||
					fabsf(S[J].CY - S[I].CY) > ROW_NEAR * HEIGHT
				)
					continue ;

				if (GAP < BEST_GAP)
				{
					BEST_GAP = GAP;
					BEST = J;
				}
			}

			if (BEST >= 0)
				PARENT[ROOT(PARENT, I)] = ROOT(PARENT, BEST);
		}

		for (I = 0; I < SEGS; ++I)
		{
			const int	TOP = ROOT(PARENT, I);

			SUM[TOP] += S[I].CY;
			IN_ROW[TOP] += 1;
		}

		/* ONE KEY PER ROW: ITS MIDDLE HEIGHT, SORTED FROM THE TOP */
		for (I = 0; I < SEGS; ++I)
		{
			if (IN_ROW[I])
			{
				KEYS[ROWS].VALUE = SUM[I] / (float)IN_ROW[I];
				KEYS[ROWS].INDEX = I;
				++ROWS;
			}
		}

		qsort(KEYS, (size_t)ROWS, sizeof(*KEYS), CMP_KEY);

		for (I = 0; I < ROWS; ++I)
			ROW_OF[KEYS[I].INDEX] = I;

		for (I = 0; I < SEGS; ++I)
			S[I].ROW = ROW_OF[ROOT(PARENT, I)];
	}

	free(PARENT);
	free(ROW_OF);
	free(SUM);
	free(IN_ROW);
	free(KEYS);
	return (ROWS);
}

/*
# HOW SHARP THE COLUMNS ARE WHEN THEY ARE STRAIGHTENED WITH LEAN AND NARROW:
# X MOVES BY LEAN * (Y - MIDDLE) AND IS STRETCHED BY 1 / (1 + NARROW * (Y -
# MIDDLE)). COVER[X] COUNTS THE ROWS WITH A WORD AT X (ROWS WITH TWO SEGMENTS
# OR MORE); WHEN THE ROWS LINE UP, THE COUNTS PILE UP AND THEIR SQUARES GROW.
*/
static double
	COLUMN_SHARPNESS(
		const struct S_SEG *S,
		int SEGS,
		const struct S_PIECE *P,
		const int *IN_ROW,
		float MIDDLE_X,
		float MIDDLE_Y,
		float LEAN,
		float NARROW,
		int *COVER,
		int *STAMP,
		int WIDTH,
		float LEFT
	)
{
	double	SUM;
	int		X;
	int		I;

	memset(COVER, 0, (size_t)WIDTH * sizeof(int));

	for (X = 0; X < WIDTH; ++X)
		STAMP[X] = -1;

	for (I = 0; I < SEGS; ++I)
	{
		const float	DY = S[I].CY - MIDDLE_Y;
		const float	SCALE = 1.0F / (1.0F + NARROW * DY);
		int			J;

		if (IN_ROW[S[I].ROW] < 2)
			continue ;

		for (J = S[I].FIRST; J < S[I].FIRST + S[I].COUNT; ++J)
		{
			int	FROM;
			int	TO;

			FROM = (int)(
				MIDDLE_X + (P[J].X0 - LEAN * DY - MIDDLE_X) * SCALE - LEFT
			);
			TO = (int)(
				MIDDLE_X + (P[J].X1 - LEAN * DY - MIDDLE_X) * SCALE - LEFT
			);

			if (FROM < 0)
				FROM = 0;

			if (TO >= WIDTH)
				TO = WIDTH - 1;

			for (X = FROM; X <= TO; ++X)
			{
				if (STAMP[X] != S[I].ROW)
				{
					STAMP[X] = S[I].ROW;
					COVER[X] += 1;
				}
			}
		}
	}

	SUM = 0.0;

	for (X = 0; X < WIDTH; ++X)
		SUM += (double)COVER[X] * COVER[X];

	return (SUM);
}

/*
# STEP 5: THE LEAN AND NARROWING THAT GIVE THE SHARPEST COLUMNS ARE UNDONE ON
# THE SEGMENTS AND ON THE WORDS.
*/
static void
	STRAIGHTEN_COLUMNS(
		struct S_SEG *S,
		int SEGS,
		struct S_PIECE *P,
		int ROWS
	)
{
	int		*IN_ROW;
	int		*COVER;
	int		*STAMP;
	float	MIDDLE_X;
	float	MIDDLE_Y;
	float	LEFT;
	float	RIGHT;
	int		WIDTH;
	int		I;

	if (SEGS <= 0)
		return ;

	LEFT = S[0].X0;
	RIGHT = S[0].X1;
	MIDDLE_Y = 0.0F;

	for (I = 0; I < SEGS; ++I)
	{
		if (S[I].X0 < LEFT)
			LEFT = S[I].X0;

		if (S[I].X1 > RIGHT)
			RIGHT = S[I].X1;

		MIDDLE_Y += S[I].CY;
	}

	MIDDLE_Y /= (float)SEGS;
	MIDDLE_X = (LEFT + RIGHT) / 2.0F;
	/* ROOM FOR THE STRAIGHTENED COLUMNS TO GROW ON BOTH SIDES */
	WIDTH = (int)(RIGHT - LEFT) * 2 + 2;
	LEFT -= (RIGHT - LEFT) / 2.0F;
	IN_ROW = (int *)calloc((size_t)ROWS, sizeof(int));
	COVER = (int *)malloc((size_t)WIDTH * sizeof(int));
	STAMP = (int *)malloc((size_t)WIDTH * sizeof(int));

	if (IN_ROW && COVER && STAMP)
	{
		double	BEST;
		float	BEST_LEAN;
		float	BEST_NARROW;

		for (I = 0; I < SEGS; ++I)
			IN_ROW[S[I].ROW] += 1;

		BEST = COLUMN_SHARPNESS(
			S,
			SEGS,
			P,
			IN_ROW,
			MIDDLE_X,
			MIDDLE_Y,
			0.0F,
			0.0F,
			COVER,
			STAMP,
			WIDTH,
			LEFT
		);
		BEST_LEAN = 0.0F;
		BEST_NARROW = 0.0F;

		for (I = -LEAN_STEPS; I <= LEAN_STEPS; ++I)
		{
			int	J;

			for (J = -NARROW_STEPS; J <= NARROW_STEPS; ++J)
			{
				const float		LEAN = (float)I * 0.004F;
				const float		NARROW = (float)J * 0.000005F;
				const double	SCORE = COLUMN_SHARPNESS(
					S,
					SEGS,
					P,
					IN_ROW,
					MIDDLE_X,
					MIDDLE_Y,
					LEAN,
					NARROW,
					COVER,
					STAMP,
					WIDTH,
					LEFT
				);

				if (SCORE > BEST)
				{
					BEST = SCORE;
					BEST_LEAN = LEAN;
					BEST_NARROW = NARROW;
				}
			}
		}

		for (I = 0; I < SEGS; ++I)
		{
			const float	DY = S[I].CY - MIDDLE_Y;
			const float	SCALE = 1.0F / (1.0F + BEST_NARROW * DY);
			const float	OFFSET = MIDDLE_X - (BEST_LEAN * DY + MIDDLE_X) * SCALE;
			int			J;

			S[I].X0 = S[I].X0 * SCALE + OFFSET;
			S[I].X1 = S[I].X1 * SCALE + OFFSET;

			for (J = S[I].FIRST; J < S[I].FIRST + S[I].COUNT; ++J)
			{
				P[J].X0 = P[J].X0 * SCALE + OFFSET;
				P[J].X1 = P[J].X1 * SCALE + OFFSET;
				P[J].CX = (P[J].X0 + P[J].X1) / 2.0F;
			}
		}
	}

	free(IN_ROW);
	free(COVER);
	free(STAMP);
}

/*
# STEP 6: THE COLUMNS. COVER[X] COUNTS THE ROWS THAT HAVE A WORD AT X. ONLY
# ROWS WITH TWO SEGMENTS OR MORE COUNT, SO A TITLE CAN NOT CLOSE A GAP. WHERE
# AT MOST ONE ROW IN TEN COVERS X, A COLUMN ENDS. WORDS, NOT SEGMENTS: TWO
# CELLS OFTEN STAND AS CLOSE AS TWO WORDS, BUT ONLY THE BORDER BETWEEN CELLS
# IS AT THE SAME PLACE IN EVERY ROW. RETURNS THE NUMBER OF COLUMNS; *LEFT GETS
# THE X WHERE THE BANDS START COUNTING.
*/
static int
	MAKE_BANDS(
		const struct S_SEG *S,
		int SEGS,
		const struct S_PIECE *P,
		int ROWS,
		float HEIGHT,
		struct S_BAND **BANDS,
		float *LEFT
	)
{
	struct S_BAND	*B;
	int				*COVER;
	int				*STAMP;
	int				*IN_ROW;
	float			RIGHT;
	int				WIDTH;
	int				MULTI;
	int				LIMIT;
	int				COUNT;
	int				X;
	int				I;

	if (SEGS <= 0)
		return (0);

	*LEFT = S[0].X0;
	RIGHT = S[0].X1;

	for (I = 1; I < SEGS; ++I)
	{
		if (S[I].X0 < *LEFT)
			*LEFT = S[I].X0;

		if (S[I].X1 > RIGHT)
			RIGHT = S[I].X1;
	}

	WIDTH = (int)(RIGHT - *LEFT) + 2;
	COVER = (int *)calloc((size_t)WIDTH, sizeof(int));
	STAMP = (int *)malloc((size_t)WIDTH * sizeof(int));
	IN_ROW = (int *)calloc((size_t)ROWS, sizeof(int));
	B = (struct S_BAND *)malloc(((size_t)WIDTH / 2 + 2) * sizeof(*B));

	if (!COVER || !STAMP || !IN_ROW || !B)
	{
		free(COVER);
		free(STAMP);
		free(IN_ROW);
		free(B);
		return (0);
	}

	for (I = 0; I < SEGS; ++I)
		IN_ROW[S[I].ROW] += 1;

	MULTI = 0;

	for (I = 0; I < ROWS; ++I)
		MULTI += IN_ROW[I] >= 2;

	for (X = 0; X < WIDTH; ++X)
		STAMP[X] = -1;

	for (I = 0; I < SEGS; ++I)
	{
		int	J;

		if (IN_ROW[S[I].ROW] < 2)
			continue ;

		for (J = S[I].FIRST; J < S[I].FIRST + S[I].COUNT; ++J)
		{
			const int	TO = (int)(P[J].X1 - *LEFT);

			for (X = (int)(P[J].X0 - *LEFT); X <= TO; ++X)
			{
				if (X >= 0 && X < WIDTH && STAMP[X] != S[I].ROW)
				{
					STAMP[X] = S[I].ROW;
					COVER[X] += 1;
				}
			}
		}
	}

	LIMIT = MULTI / 10;
	COUNT = 0;
	X = 0;

	while (X < WIDTH && MULTI)
	{
		while (X < WIDTH && COVER[X] <= LIMIT)
			++X;

		if (X >= WIDTH)
			break ;

		/* A GAP TOO NARROW FOR A BORDER: THE LAST COLUMN GOES ON */
		if (COUNT && X - B[COUNT - 1].END - 1 < (int)(MIN_GAP * HEIGHT))
			--COUNT;
		else
			B[COUNT].START = X;

		while (X < WIDTH && COVER[X] > LIMIT)
			++X;

		B[COUNT].END = X - 1;
		++COUNT;
	}

	if (!COUNT)
	{
		B[0].START = 0;
		B[0].END = WIDTH - 1;
		COUNT = 1;
	}

	free(COVER);
	free(STAMP);
	free(IN_ROW);
	*BANDS = B;
	return (COUNT);
}

/* A CELL THE TABLE MAKER CORRECTED: YELLOW IN THE WINDOW, TO BE LOOKED AT */
static void
	MARK_CELL(struct S_TABLE *TABLE, int R, int C)
{
	if (TABLE->CHECK)
		TABLE->CHECK[R * TABLE->COLS + C] = 1;
}

/*
# 1 = THE NUMBER ENDS IN A MARK AND TWO DIGITS (MARK: '.' OR ','), AND THE
# GROUPS BEFORE IT ARE GROUPS OF THREE AFTER THE FIRST ("12.362.89")
*/
static int
	ENDS_IN_CENTS(const char *TEXT, char MARK)
{
	const size_t	LEN = strlen(TEXT);
	size_t			I;
	int				RUN;

	if (
		LEN < 4 ||
		TEXT[LEN - 3] != MARK ||
		TEXT[LEN - 2] < '0' || TEXT[LEN - 2] > '9' ||
		TEXT[LEN - 1] < '0' || TEXT[LEN - 1] > '9'
	)
		return (0);

	/* BEFORE THE MARK: DIGITS, A GROUP MARK EVERY THREE DIGITS FROM THE END */
	RUN = 0;

	for (I = LEN - 3; I > 0; --I)
	{
		const char	C = TEXT[I - 1];

		if (C >= '0' && C <= '9')
			++RUN;
		else if ((C == '.' || C == ',') && RUN == 3)
			RUN = 0;
		else
			return (0);
	}

	return (RUN > 0 && RUN <= 3);
}

/*
# AMOUNTS OF ONE COLUMN ARE WRITTEN ONE WAY: WHEN MOST END IN ",DD"
# ("14.345,22"), ONE THAT ENDS IN ".DD" WAS READ WRONG ("12.362.89" IS
# "12.362,89"); THE SAME THE OTHER WAY ROUND ("1,234.56").
*/
static void
	SAME_CENTS(struct S_TABLE *TABLE, int C)
{
	int	COMMA;
	int	DOT;
	int	R;

	COMMA = 0;
	DOT = 0;

	for (R = 0; R < TABLE->ROWS; ++R)
	{
		const char	*CELL = TABLE->CELL[R * TABLE->COLS + C];

		if (!CELL)
			continue ;

		COMMA += ENDS_IN_CENTS(CELL, ',');
		DOT += ENDS_IN_CENTS(CELL, '.');
	}

	for (R = 0; R < TABLE->ROWS; ++R)
	{
		char	*CELL = TABLE->CELL[R * TABLE->COLS + C];

		if (!CELL)
			continue ;

		if (COMMA >= 3 && COMMA >= 3 * DOT && ENDS_IN_CENTS(CELL, '.'))
		{
			CELL[strlen(CELL) - 3] = ',';
			MARK_CELL(TABLE, R, C);
		}
		else if (DOT >= 3 && DOT >= 3 * COMMA && ENDS_IN_CENTS(CELL, ','))
		{
			CELL[strlen(CELL) - 3] = '.';
			MARK_CELL(TABLE, R, C);
		}
	}
}

/* HOW MANY CELLS OF COLUMN C HOLD EXACTLY TEXT */
static int
	TIMES_IN(const struct S_TABLE *TABLE, int C, const char *TEXT)
{
	int	TIMES;
	int	R;

	TIMES = 0;

	for (R = 0; R < TABLE->ROWS; ++R)
	{
		const char	*CELL = TABLE->CELL[R * TABLE->COLS + C];

		TIMES += CELL && strcmp(CELL, TEXT) == 0;
	}

	return (TIMES);
}

/* THE EDIT DISTANCE OF TWO SHORT TEXTS (BYTES), UP TO MOST + 1 */
static int
	TEXT_DISTANCE(const char *A, const char *B, int MOST)
{
	int			ROW[WORD_FIX_LONGEST + 1];
	const int	LA = (int)strlen(A);
	const int	LB = (int)strlen(B);
	int			I;
	int			J;

	if (LA > WORD_FIX_LONGEST || LB > WORD_FIX_LONGEST)
		return (MOST + 1);

	if (LA - LB > MOST || LB - LA > MOST)
		return (MOST + 1);

	for (J = 0; J <= LB; ++J)
		ROW[J] = J;

	for (I = 1; I <= LA; ++I)
	{
		int	DIAGONAL;

		DIAGONAL = ROW[0];
		ROW[0] = I;

		for (J = 1; J <= LB; ++J)
		{
			const int	UP = ROW[J];
			int			BEST;

			BEST = DIAGONAL + (A[I - 1] != B[J - 1]);

			if (UP + 1 < BEST)
				BEST = UP + 1;

			if (ROW[J - 1] + 1 < BEST)
				BEST = ROW[J - 1] + 1;

			ROW[J] = BEST;
			DIAGONAL = UP;
		}
	}

	return (ROW[LB]);
}

/* 1 = THE DIGITS OF A AND B, IN ORDER, ARE THE SAME ("M8" IS NOT "M6") */
static int
	SAME_DIGITS(const char *A, const char *B)
{
	for (;;)
	{
		while (*A && (*A < '0' || *A > '9'))
			++A;

		while (*B && (*B < '0' || *B > '9'))
			++B;

		if (*A != *B)
			return (0);

		if (!*A)
			return (1);

		++A;
		++B;
	}
}

/*
# IN A COLUMN OF WORDS, A TEXT THAT COMES ONCE AND IS AT MOST 2 LETTERS AWAY
# FROM ONE THAT COMES TWICE OR MORE, WITH THE SAME DIGITS, IS THAT ONE: THE
# OCR MISREAD IT ("Matkpp Ucu 6" AMONG "Matkap Ucu 6", "orum" AMONG
# "Çorum"). A DIGIT THAT DIFFERS ("Vida M6", "Vida M8") IS TWO THINGS.
*/
static void
	FIX_WORDS(struct S_TABLE *TABLE, int C)
{
	int	R;

	for (R = 0; R < TABLE->ROWS; ++R)
	{
		char		**CELL = &TABLE->CELL[R * TABLE->COLS + C];
		const char	*BEST;
		int			BEST_HITS;
		int			K;

		if (!*CELL || strlen(*CELL) < 4 || TIMES_IN(TABLE, C, *CELL) > 1)
			continue ;

		BEST = NULL;
		BEST_HITS = 1;

		for (K = 0; K < TABLE->ROWS; ++K)
		{
			const char	*OTHER = TABLE->CELL[K * TABLE->COLS + C];
			int			HITS;

			if (!OTHER || K == R || !SAME_DIGITS(*CELL, OTHER))
				continue ;

			if (TEXT_DISTANCE(*CELL, OTHER, 2) > 2)
				continue ;

			HITS = TIMES_IN(TABLE, C, OTHER);

			if (HITS > BEST_HITS)
			{
				BEST_HITS = HITS;
				BEST = OTHER;
			}
		}

		if (BEST && strlen(BEST) <= strlen(*CELL) + 2)
		{
			char	*COPY = (char *)malloc(strlen(BEST) + 1);

			if (!COPY)
				continue ;

			strcpy(COPY, BEST);
			free(*CELL);
			*CELL = COPY;
			MARK_CELL(TABLE, R, C);
		}
	}
}

/* STEP 8: "O" TO "0" AND "l" TO "1" IN COLUMNS THAT ARE MOSTLY NUMBERS */
static void
	FIX_NUMBERS(struct S_TABLE *TABLE)
{
	int	C;

	for (C = 0; C < TABLE->COLS; ++C)
	{
		int	FILLED;
		int	NUMBERS;
		int	R;

		FILLED = 0;
		NUMBERS = 0;

		for (R = 0; R < TABLE->ROWS; ++R)
		{
			const char	*CELL = TABLE->CELL[R * TABLE->COLS + C];

			FILLED += CELL != NULL;
			NUMBERS += CELL && NUMBER_LIKE(CELL);
		}

		/* A COLUMN OF WORDS: THE WORDS AGAINST EACH OTHER */
		if (FILLED >= 3 && (float)NUMBERS < NUMBER_SHARE * (float)FILLED)
			FIX_WORDS(TABLE, C);

		if (FILLED < 2 || (float)NUMBERS < NUMBER_SHARE * (float)FILLED)
			continue ;

		for (R = 0; R < TABLE->ROWS; ++R)
		{
			char	*P;

			P = TABLE->CELL[R * TABLE->COLS + C];

			if (!P || !NUMBER_LIKE(P))
				continue ;

			for (; *P; ++P)
			{
				if (*P == 'O' || *P == 'o')
				{
					*P = '0';
					MARK_CELL(TABLE, R, C);
				}

				if (*P == 'I' || *P == 'l' || *P == '|')
				{
					*P = '1';
					MARK_CELL(TABLE, R, C);
				}
			}
		}

		SAME_CENTS(TABLE, C);
	}
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          STEPS                           |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                         COLUMNS                          |  \\//  */
/*   \/   #==========================================================#   \/   */
/*
# THE LEFT AND RIGHT EDGE OF THE WORDS OF BAND IN EVERY ROW. HAS[R] IS 1 FOR
# A ROW WITH WORDS THERE. ROWS WITH ONE SEGMENT ONLY (TITLES) DO NOT COUNT.
*/
static void
	ROW_EDGES(
		const struct S_ITEM *ITEMS,
		int COUNT,
		int BAND,
		int ROWS,
		float *LEFT,
		float *RIGHT,
		int *HAS
	)
{
	int	I;

	memset(HAS, 0, (size_t)ROWS * sizeof(int));

	for (I = 0; I < COUNT; ++I)
	{
		const int	R = ITEMS[I].ROW;

		if (ITEMS[I].COL != BAND || ITEMS[I].ALONE)
			continue ;

		if (!HAS[R])
		{
			HAS[R] = 1;
			LEFT[R] = ITEMS[I].X;
			RIGHT[R] = ITEMS[I].X1;
		}

		if (ITEMS[I].X < LEFT[R])
			LEFT[R] = ITEMS[I].X;

		if (ITEMS[I].X1 > RIGHT[R])
			RIGHT[R] = ITEMS[I].X1;
	}
}

/*
# A REAL COLUMN IS ALIGNED: MOST OF ITS ROWS START AT THE SAME X (TEXT), END
# AT THE SAME X (NUMBERS) OR HAVE THE SAME MIDDLE (CENTERED). A BAND WHERE THE
# ROWS DO NONE OF THIS IS THE SECOND WORD OF LONGER CELLS ("BORU 20 MM") AND
# JOINS THE COLUMN ON ITS LEFT. SO DOES A BAND WHOSE WORDS ALL CONTINUE A
# SEGMENT FROM THE LEFT WHEN LESS THAN HALF OF THE ROWS HAVE IT: THE LAST WORD
# OF A NAME THAT COMES AGAIN AND AGAIN ("MATKAP UCU 6") IS ALIGNED, BUT A REAL
# COLUMN IS FILLED IN MOST ROWS. RETURNS THE NEW NUMBER OF COLUMNS; THE ITEMS
# GET THEIR NEW COLUMNS.
*/
static int
	JOIN_RAGGED_BANDS(
		struct S_ITEM *ITEMS,
		int COUNT,
		int BAND_COUNT,
		int ROWS,
		float HEIGHT
	)
{
	const float	TOLERANCE = ALIGN_NEAR * HEIGHT;
	float		*LEFT;
	float		*RIGHT;
	float		*MIDDLE;
	int			*HAS;
	int			*NEW;
	int			IN_TABLE;
	int			B;
	int			R;
	int			I;

	LEFT = (float *)malloc((size_t)ROWS * sizeof(float));
	RIGHT = (float *)malloc((size_t)ROWS * sizeof(float));
	MIDDLE = (float *)malloc((size_t)ROWS * sizeof(float));
	HAS = (int *)malloc((size_t)ROWS * sizeof(int));
	NEW = (int *)malloc((size_t)BAND_COUNT * sizeof(int));

	if (!LEFT || !RIGHT || !MIDDLE || !HAS || !NEW)
	{
		free(LEFT);
		free(RIGHT);
		free(MIDDLE);
		free(HAS);
		free(NEW);
		return (BAND_COUNT);
	}

	memset(HAS, 0, (size_t)ROWS * sizeof(int));

	for (I = 0; I < COUNT; ++I)
		HAS[ITEMS[I].ROW] |= !ITEMS[I].ALONE;

	IN_TABLE = 0;

	for (R = 0; R < ROWS; ++R)
		IN_TABLE += HAS[R];

	NEW[0] = 0;

	for (B = 1; B < BAND_COUNT; ++B)
	{
		float	BEST_LEFT;
		float	BEST_RIGHT;
		float	BEST_MIDDLE;
		int		IN_BAND;
		int		ALIGNED;
		int		CONTINUED;
		int		JOIN;

		ROW_EDGES(ITEMS, COUNT, B, ROWS, LEFT, RIGHT, HAS);
		CONTINUED = 1;

		for (I = 0; I < COUNT; ++I)
		{
			if (ITEMS[I].COL != B || ITEMS[I].ALONE)
				continue ;

			CONTINUED &= ITEMS[I].BEFORE >= 0 &&
				NEW[ITEMS[I].BEFORE] == NEW[B - 1];
		}

		IN_BAND = 0;

		for (R = 0; R < ROWS; ++R)
		{
			IN_BAND += HAS[R];
			MIDDLE[R] = (LEFT[R] + RIGHT[R]) / 2.0F;
		}

		BEST_LEFT = MOST_COMMON(LEFT, HAS, ROWS, TOLERANCE);
		BEST_RIGHT = MOST_COMMON(RIGHT, HAS, ROWS, TOLERANCE);
		BEST_MIDDLE = MOST_COMMON(MIDDLE, HAS, ROWS, TOLERANCE);
		ALIGNED = 0;

		for (R = 0; R < ROWS; ++R)
		{
			ALIGNED += HAS[R] && (
				fabsf(LEFT[R] - BEST_LEFT) <= TOLERANCE ||
				fabsf(RIGHT[R] - BEST_RIGHT) <= TOLERANCE ||
				fabsf(MIDDLE[R] - BEST_MIDDLE) <= TOLERANCE
			);
		}

		JOIN = IN_BAND > 0 && CONTINUED && 2 * IN_BAND < IN_TABLE;

		if (IN_BAND >= 3 && (float)ALIGNED < ALIGN_SHARE * (float)IN_BAND)
			JOIN = 1;

		if (JOIN)
			NEW[B] = NEW[B - 1];
		else
			NEW[B] = NEW[B - 1] + 1;
	}

	for (I = 0; I < COUNT; ++I)
		ITEMS[I].COL = NEW[ITEMS[I].COL];

	BAND_COUNT = NEW[BAND_COUNT - 1] + 1;
	free(LEFT);
	free(RIGHT);
	free(MIDDLE);
	free(HAS);
	free(NEW);
	return (BAND_COUNT);
}

/*
# A HEADER IS OFTEN ALIGNED THE OTHER WAY THAN ITS NUMBERS ("SIRA" ON THE
# LEFT, "1", "2", "3" ON THE RIGHT OF A WIDE COLUMN), SO IT GETS A BAND OF ITS
# OWN. TWO NEIGHBOR COLUMNS THAT NEVER HAVE TEXT IN THE SAME ROW, WHERE ONE OF
# THEM HAS TEXT IN ONE ROW IN EIGHT OR LESS, ARE ONE COLUMN. (DEBIT AND CREDIT
# COLUMNS ALSO NEVER SHARE A ROW, BUT BOTH ARE WELL FILLED.) RETURNS THE NEW
# NUMBER OF COLUMNS.
*/
static int
	JOIN_HEADER_BANDS(struct S_ITEM *ITEMS, int COUNT, int BAND_COUNT, int ROWS)
{
	char	*USED;
	int		*NEW;
	int		IN_TABLE;
	int		FEW;
	int		B;
	int		R;
	int		I;

	USED = (char *)calloc((size_t)ROWS * (size_t)BAND_COUNT, 1);
	NEW = (int *)malloc((size_t)BAND_COUNT * sizeof(int));

	if (!USED || !NEW)
	{
		free(USED);
		free(NEW);
		return (BAND_COUNT);
	}

	for (I = 0; I < COUNT; ++I)
		if (!ITEMS[I].ALONE)
			USED[ITEMS[I].ROW * BAND_COUNT + ITEMS[I].COL] = 1;

	IN_TABLE = 0;

	for (R = 0; R < ROWS; ++R)
	{
		for (B = 0; B < BAND_COUNT && !USED[R * BAND_COUNT + B]; ++B)
		{}

		IN_TABLE += B < BAND_COUNT;
	}

	FEW = 1;

	if (IN_TABLE / 8 > 1)
		FEW = IN_TABLE / 8;

	NEW[0] = 0;

	for (B = 1; B < BAND_COUNT; ++B)
	{
		int	LEFT_ROWS;
		int	RIGHT_ROWS;
		int	BOTH;

		LEFT_ROWS = 0;
		RIGHT_ROWS = 0;
		BOTH = 0;

		for (R = 0; R < ROWS; ++R)
		{
			LEFT_ROWS += USED[R * BAND_COUNT + B - 1];
			RIGHT_ROWS += USED[R * BAND_COUNT + B];
			BOTH += USED[R * BAND_COUNT + B - 1] && USED[R * BAND_COUNT + B];
		}

		NEW[B] = NEW[B - 1] + 1;

		if (
			!BOTH &&
			LEFT_ROWS &&
			RIGHT_ROWS &&
			(LEFT_ROWS <= FEW || RIGHT_ROWS <= FEW)
		)
		{
			NEW[B] = NEW[B - 1];

			/* THE JOINED COLUMN COUNTS AS THE LEFT ONE FOR THE NEXT TEST */
			for (R = 0; R < ROWS; ++R)
				USED[R * BAND_COUNT + B] |= USED[R * BAND_COUNT + B - 1];
		}
	}

	for (I = 0; I < COUNT; ++I)
		ITEMS[I].COL = NEW[ITEMS[I].COL];

	BAND_COUNT = NEW[BAND_COUNT - 1] + 1;
	free(USED);
	free(NEW);
	return (BAND_COUNT);
}

/*
# THE USUAL LEFT AND RIGHT EDGE OF EVERY COLUMN: WHERE MOST ROWS START AND
# WHERE MOST ROWS END IN IT.
*/
static void
	COLUMN_EDGES(
		const struct S_ITEM *ITEMS,
		int COUNT,
		int BAND_COUNT,
		int ROWS,
		float HEIGHT,
		float *LEFT_EDGE,
		float *RIGHT_EDGE
	)
{
	float	*LEFT;
	float	*RIGHT;
	int		*HAS;
	int		B;

	LEFT = (float *)malloc((size_t)ROWS * sizeof(float));
	RIGHT = (float *)malloc((size_t)ROWS * sizeof(float));
	HAS = (int *)malloc((size_t)ROWS * sizeof(int));

	for (B = 0; B < BAND_COUNT && LEFT && RIGHT && HAS; ++B)
	{
		ROW_EDGES(ITEMS, COUNT, B, ROWS, LEFT, RIGHT, HAS);
		LEFT_EDGE[B] = MOST_COMMON(LEFT, HAS, ROWS, ALIGN_NEAR * HEIGHT);
		RIGHT_EDGE[B] = MOST_COMMON(RIGHT, HAS, ROWS, ALIGN_NEAR * HEIGHT);
	}

	free(LEFT);
	free(RIGHT);
	free(HAS);
}

/*
# A SEGMENT THAT CROSSES A COLUMN BORDER IS CUT THERE ONLY WHEN THE CUT LOOKS
# LIKE TWO CELLS: THE WORD AFTER THE BORDER STARTS AT THE USUAL LEFT EDGE OF
# ITS COLUMN, OR THE WORD BEFORE IT ENDS AT THE USUAL RIGHT EDGE OF ITS OWN. A
# TITLE OR A NOTE THAT RUNS OVER SEVERAL COLUMNS STAYS IN ONE CELL.
*/
static void
	KEEP_SEGMENTS_TOGETHER(
		struct S_ITEM *ITEMS,
		const struct S_SEG *S,
		int SEGS,
		int BAND_COUNT,
		int ROWS,
		float HEIGHT
	)
{
	const float	TOLERANCE = ALIGN_NEAR * HEIGHT;
	const int	COUNT = S[SEGS - 1].FIRST + S[SEGS - 1].COUNT;
	float		*LEFT_EDGE;
	float		*RIGHT_EDGE;
	int			I;

	LEFT_EDGE = (float *)malloc((size_t)BAND_COUNT * sizeof(float));
	RIGHT_EDGE = (float *)malloc((size_t)BAND_COUNT * sizeof(float));

	if (!LEFT_EDGE || !RIGHT_EDGE)
	{
		free(LEFT_EDGE);
		free(RIGHT_EDGE);
		return ;
	}

	COLUMN_EDGES(
		ITEMS,
		COUNT,
		BAND_COUNT,
		ROWS,
		HEIGHT,
		LEFT_EDGE,
		RIGHT_EDGE
	);

	for (I = 0; I < SEGS; ++I)
	{
		int	J;

		for (J = S[I].FIRST + 1; J < S[I].FIRST + S[I].COUNT; ++J)
		{
			const int	A = ITEMS[J - 1].COL;
			const int	B = ITEMS[J].COL;

			if (
				A != B &&
				fabsf(ITEMS[J].X - LEFT_EDGE[B]) > TOLERANCE &&
				fabsf(ITEMS[J - 1].X1 - RIGHT_EDGE[A]) > TOLERANCE
			)
				ITEMS[J].COL = A;
		}
	}

	free(LEFT_EDGE);
	free(RIGHT_EDGE);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                         COLUMNS                          |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          CELLS                           |  \\//  */
/*   \/   #==========================================================#   \/   */
/* STEP 7: EVERY WORD INTO ITS CELL. 1 = DONE */
static int
	FILL_CELLS(
		const struct S_WORD *WORDS,
		const struct S_PIECE *P,
		const struct S_SEG *S,
		int SEGS,
		const struct S_BAND *BANDS,
		int BAND_COUNT,
		float LEFT,
		float HEIGHT,
		struct S_TABLE *TABLE
	)
{
	const int		COUNT = S[SEGS - 1].FIRST + S[SEGS - 1].COUNT;
	struct S_ITEM	*ITEMS;
	int				*IN_ROW;
	int				I;
	int				J;

	ITEMS = (struct S_ITEM *)malloc((size_t)COUNT * sizeof(*ITEMS) + 1);
	IN_ROW = (int *)calloc((size_t)TABLE->ROWS, sizeof(int));

	if (!ITEMS || !IN_ROW)
	{
		free(ITEMS);
		free(IN_ROW);
		return (0);
	}

	for (I = 0; I < SEGS; ++I)
		IN_ROW[S[I].ROW] += 1;

	for (I = 0; I < SEGS; ++I)
	{
		for (J = S[I].FIRST; J < S[I].FIRST + S[I].COUNT; ++J)
		{
			float	X;

			/* A TITLE STAYS WHERE IT STARTS, A WORD GOES BY ITS MIDDLE */
			if (IN_ROW[S[I].ROW] < 2)
				X = S[I].X0;
			else
				X = P[J].CX;

			ITEMS[J].ROW = S[I].ROW;
			ITEMS[J].COL = FIND_BAND(BANDS, BAND_COUNT, X - LEFT);

			if (J > S[I].FIRST)
				ITEMS[J].BEFORE = ITEMS[J - 1].COL;
			else
				ITEMS[J].BEFORE = -1;

			ITEMS[J].ALONE = IN_ROW[S[I].ROW] < 2;
			ITEMS[J].X = P[J].X0;
			ITEMS[J].X1 = P[J].X1;
			ITEMS[J].WORD = P[J].WORD;
		}
	}

	free(IN_ROW);
	BAND_COUNT = JOIN_RAGGED_BANDS(
		ITEMS,
		COUNT,
		BAND_COUNT,
		TABLE->ROWS,
		HEIGHT
	);
	KEEP_SEGMENTS_TOGETHER(
		ITEMS,
		S,
		SEGS,
		BAND_COUNT,
		TABLE->ROWS,
		HEIGHT
	);
	BAND_COUNT = JOIN_HEADER_BANDS(ITEMS, COUNT, BAND_COUNT, TABLE->ROWS);
	TABLE->COLS = BAND_COUNT;
	TABLE->CELL = (char **)calloc(
		(size_t)TABLE->ROWS * (size_t)BAND_COUNT,
		sizeof(char *)
	);

	if (!TABLE->CELL)
	{
		free(ITEMS);
		return (0);
	}

	qsort(ITEMS, (size_t)COUNT, sizeof(*ITEMS), CMP_ITEM);

	for (I = 0; I < COUNT; I = J)
	{
		size_t	SIZE;
		char	*CELL;
		int		K;

		SIZE = 1;

		for (
			J = I;
			J < COUNT &&
			ITEMS[J].ROW == ITEMS[I].ROW &&
			ITEMS[J].COL == ITEMS[I].COL;
			++J
		)
			SIZE += strlen(WORDS[ITEMS[J].WORD].TEXT) + 1;

		CELL = (char *)malloc(SIZE);

		if (!CELL)
		{
			free(ITEMS);
			return (0);
		}

		CELL[0] = '\0';

		for (K = I; K < J; ++K)
		{
			if (K > I)
				strcat(CELL, " ");

			strcat(CELL, WORDS[ITEMS[K].WORD].TEXT);
		}

		TRIM_BORDER(CELL);
		TABLE->CELL[ITEMS[I].ROW * BAND_COUNT + ITEMS[I].COL] = CELL;
	}

	free(ITEMS);
	return (1);
}

/* LETTERS AND DIGITS IN A TEXT (A UTF-8 LETTER COUNTS ONCE) */
static int
	ALNUM_COUNT(const char *TEXT)
{
	int	COUNT;

	COUNT = 0;

	for (; *TEXT; ++TEXT)
	{
		const unsigned char	C = (unsigned char)*TEXT;

		COUNT += (C >= '0' && C <= '9') || (C >= 'A' && C <= 'Z');
		COUNT += (C >= 'a' && C <= 'z') || C >= 0XC0;
	}

	return (COUNT);
}

/*
# IN A TABLE WHOSE ROWS USUALLY FILL 3 CELLS OR MORE, A ROW (NOT THE FIRST)
# OF ONE OR TWO CELLS WITH 2 LETTERS OR DIGITS AT MOST IS BITS OF BORDER THE
# OCR READ ("9-"): IT IS TAKEN OUT
*/
static void
	DROP_JUNK_ROWS(struct S_TABLE *TABLE)
{
	int	*FILLED;
	int	*SORTED;
	int	KEEP;
	int	R;
	int	C;

	if (TABLE->ROWS < 3 || TABLE->COLS < 3)
		return ;

	FILLED = (int *)calloc((size_t)TABLE->ROWS, sizeof(int));
	SORTED = (int *)calloc((size_t)TABLE->ROWS, sizeof(int));

	if (!FILLED || !SORTED)
	{
		free(FILLED);
		free(SORTED);
		return ;
	}

	for (R = 0; R < TABLE->ROWS; ++R)
		for (C = 0; C < TABLE->COLS; ++C)
			FILLED[R] += TABLE->CELL[R * TABLE->COLS + C] != NULL;

	memcpy(SORTED, FILLED, (size_t)TABLE->ROWS * sizeof(int));
	qsort(SORTED, (size_t)TABLE->ROWS, sizeof(int), CMP_INT);
	KEEP = 0;

	for (R = 0; R < TABLE->ROWS; ++R)
	{
		char	**ROW = &TABLE->CELL[R * TABLE->COLS];
		int		ALNUM;

		ALNUM = 0;

		for (C = 0; C < TABLE->COLS; ++C)
			if (ROW[C])
				ALNUM += ALNUM_COUNT(ROW[C]);

		if (
			R > 0 &&
			SORTED[TABLE->ROWS / 2] >= 3 &&
			FILLED[R] <= 2 &&
			ALNUM <= 2
		)
		{
			for (C = 0; C < TABLE->COLS; ++C)
				free(ROW[C]);

			continue ;
		}

		memmove(
			&TABLE->CELL[KEEP * TABLE->COLS],
			ROW,
			(size_t)TABLE->COLS * sizeof(char *)
		);
		++KEEP;
	}

	TABLE->ROWS = KEEP;
	free(FILLED);
	free(SORTED);
}

/* 1 = ONE CHARACTER THAT A BIT OF A STANDING BORDER IS READ AS ("l", "ı") */
static int
	IS_BAR(const char *TEXT)
{
	/* "ı" IS C4 B1 IN UTF-8 */
	if ((unsigned char)TEXT[0] == 0XC4 && (unsigned char)TEXT[1] == 0XB1)
		return (TEXT[2] == '\0');

	return (TEXT[0] && !TEXT[1] && strchr("|lIJj![]()", TEXT[0]) != NULL);
}

/* 1 = ONE CHARACTER (ONE BYTE, OR ONE UTF-8 LETTER OF TWO BYTES) */
static int
	ONE_CHARACTER(const char *TEXT)
{
	if ((unsigned char)TEXT[0] >= 0XC0)
		return (TEXT[1] && !TEXT[2]);

	return (TEXT[0] && !TEXT[1]);
}

/*
# A COLUMN OF ONLY SINGLE CHARACTERS, AT LEAST HALF OF THEM "l", "I", "J",
# "|"... IS A STANDING BORDER THE OCR READ ROW BY ROW: IT IS EMPTIED (AND THEN
# TAKEN OUT). A COLUMN OF "1"S OR OF "E" AND "K" STAYS.
*/
static void
	CLEAR_BAR_COLUMNS(struct S_TABLE *TABLE)
{
	int	R;
	int	C;

	for (C = 0; C < TABLE->COLS; ++C)
	{
		int	FILLED;
		int	BARS;
		int	OTHER;

		FILLED = 0;
		BARS = 0;
		OTHER = 0;

		for (R = 0; R < TABLE->ROWS; ++R)
		{
			const char	*TEXT = TABLE->CELL[R * TABLE->COLS + C];

			if (!TEXT)
				continue ;

			++FILLED;
			BARS += IS_BAR(TEXT);
			OTHER += !ONE_CHARACTER(TEXT);
		}

		if (!FILLED || OTHER || BARS * 2 < FILLED)
			continue ;

		for (R = 0; R < TABLE->ROWS; ++R)
		{
			free(TABLE->CELL[R * TABLE->COLS + C]);
			TABLE->CELL[R * TABLE->COLS + C] = NULL;
		}
	}
}

/* COLUMNS WITHOUT ANY TEXT ARE TAKEN OUT */
static void
	DROP_EMPTY_COLUMNS(struct S_TABLE *TABLE)
{
	int	KEEP;
	int	R;
	int	C;

	KEEP = 0;

	for (C = 0; C < TABLE->COLS; ++C)
	{
		int	USED;

		USED = 0;

		for (R = 0; R < TABLE->ROWS; ++R)
			USED |= TABLE->CELL[R * TABLE->COLS + C] != NULL;

		if (!USED)
			continue ;

		for (R = 0; R < TABLE->ROWS; ++R)
			TABLE->CELL[R * TABLE->COLS + KEEP] =
				TABLE->CELL[R * TABLE->COLS + C];

		++KEEP;
	}

	/* REPACK FROM COLS TO KEEP COLUMNS PER ROW */
	for (R = 0; R < TABLE->ROWS; ++R)
		for (C = 0; C < KEEP; ++C)
			TABLE->CELL[R * KEEP + C] = TABLE->CELL[R * TABLE->COLS + C];

	TABLE->COLS = KEEP;
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          CELLS                           |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                        PUBLIC API                        |  \\//  */
/*   \/   #==========================================================#   \/   */
int
	LAYOUT_BUILD(
		const struct S_WORD *WORDS,
		int COUNT,
		struct S_TABLE *TABLE
	)
{
	struct S_PIECE	*P;
	struct S_SEG	*S;
	struct S_BAND	*BANDS;
	float			*HEIGHTS;
	float			HEIGHT;
	int				PIECES;
	int				SEGS;
	int				OK;
	int				I;

	memset(TABLE, 0, sizeof(*TABLE));

	if (COUNT <= 0)
		return (0);

	P = (struct S_PIECE *)malloc((size_t)COUNT * sizeof(*P));
	S = (struct S_SEG *)malloc((size_t)COUNT * sizeof(*S));
	HEIGHTS = (float *)malloc((size_t)COUNT * sizeof(float));

	if (!P || !S || !HEIGHTS)
	{
		free(P);
		free(S);
		free(HEIGHTS);
		return (0);
	}

	for (I = 0; I < COUNT; ++I)
		HEIGHTS[I] = WORDS[I].H;

	HEIGHT = MEDIAN(HEIGHTS, COUNT);
	free(HEIGHTS);

	if (HEIGHT <= 1.0F)
		HEIGHT = 1.0F;

	OK = 0;
	BANDS = NULL;
	PIECES = MAKE_PIECES(WORDS, COUNT, HEIGHT, P);
	SEGS = 0;

	if (PIECES)
		SEGS = MAKE_SEGMENTS(P, PIECES, HEIGHT, S);

	if (SEGS)
		TABLE->ROWS = MAKE_ROWS(S, SEGS, HEIGHT);

	if (TABLE->ROWS)
	{
		float	LEFT;
		int		BAND_COUNT;

		STRAIGHTEN_COLUMNS(S, SEGS, P, TABLE->ROWS);
		BAND_COUNT = MAKE_BANDS(
			S,
			SEGS,
			P,
			TABLE->ROWS,
			HEIGHT,
			&BANDS,
			&LEFT
		);
		OK = BAND_COUNT && FILL_CELLS(
			WORDS,
			P,
			S,
			SEGS,
			BANDS,
			BAND_COUNT,
			LEFT,
			HEIGHT,
			TABLE
		);
	}

	free(BANDS);
	free(P);
	free(S);

	if (!OK)
	{
		LAYOUT_FREE(TABLE);
		return (0);
	}

	CLEAR_BAR_COLUMNS(TABLE);
	DROP_JUNK_ROWS(TABLE);
	DROP_EMPTY_COLUMNS(TABLE);
	TABLE->CHECK = (unsigned char *)calloc(
		(size_t)TABLE->ROWS * TABLE->COLS + 1,
		1
	);
	FIX_NUMBERS(TABLE);
	return (1);
}

/*
# 1 = WORD I STARTS A NEW ROW OF A GRID (LAYOUT_GRID): ITS ROW IS NOT THE ROW
# OF THE WORD WITH TEXT BEFORE IT. WORDS WITHOUT TEXT DO NOT COUNT.
*/
static int
	NEW_ROW(const struct S_WORD *WORDS, const int *ROWS, int I)
{
	int	J;

	if (!WORDS[I].TEXT[0])
		return (0);

	J = I - 1;

	while (J >= 0 && !WORDS[J].TEXT[0])
		--J;

	if (J < 0)
		return (0);

	if (ROWS)
		return (ROWS[I] != ROWS[J]);

	return (WORDS[I].LINE != WORDS[J].LINE);
}

int
	LAYOUT_GRID(
		const struct S_WORD *WORDS,
		const int *ROWS,
		int COUNT,
		const float *EDGES,
		int EDGE_COUNT,
		struct S_TABLE *TABLE
	)
{
	int	ROW;
	int	I;

	memset(TABLE, 0, sizeof(*TABLE));

	if (COUNT <= 0)
		return (0);

	TABLE->ROWS = 1;

	for (I = 0; I < COUNT; ++I)
		TABLE->ROWS += NEW_ROW(WORDS, ROWS, I);

	TABLE->COLS = EDGE_COUNT + 1;
	TABLE->CELL = (char **)calloc(
		(size_t)TABLE->ROWS * TABLE->COLS + 1,
		sizeof(char *)
	);

	if (!TABLE->CELL)
	{
		LAYOUT_FREE(TABLE);
		return (0);
	}

	ROW = 0;

	for (I = 0; I < COUNT; ++I)
	{
		const float	MIDDLE = WORDS[I].X + WORDS[I].W / 2.0F;
		char		**CELL;
		char		*JOINED;
		size_t		USED;
		int			COL;

		if (!WORDS[I].TEXT[0])
			continue ;

		if (NEW_ROW(WORDS, ROWS, I))
			++ROW;

		COL = 0;

		while (COL < EDGE_COUNT && EDGES[COL] < MIDDLE)
			++COL;

		CELL = &TABLE->CELL[ROW * TABLE->COLS + COL];
		USED = 0;

		if (*CELL)
			USED = strlen(*CELL) + 1;

		JOINED = (char *)realloc(*CELL, USED + strlen(WORDS[I].TEXT) + 1);

		if (!JOINED)
		{
			LAYOUT_FREE(TABLE);
			return (0);
		}

		if (USED)
			JOINED[USED - 1] = ' ';

		strcpy(JOINED + USED, WORDS[I].TEXT);
		*CELL = JOINED;
	}

	DROP_EMPTY_COLUMNS(TABLE);
	TABLE->CHECK = (unsigned char *)calloc(
		(size_t)TABLE->ROWS * TABLE->COLS + 1,
		1
	);
	FIX_NUMBERS(TABLE);
	return (1);
}

void
	LAYOUT_FREE(struct S_TABLE *TABLE)
{
	if (TABLE->CELL)
	{
		int	I;

		for (I = 0; I < TABLE->ROWS * TABLE->COLS; ++I)
			free(TABLE->CELL[I]);

		free(TABLE->CELL);
	}

	free(TABLE->CHECK);
	memset(TABLE, 0, sizeof(*TABLE));
}

int
	LAYOUT_APPEND(struct S_TABLE *TABLE, struct S_TABLE *MORE)
{
	int				GAP;
	int				ROWS;
	int				COLS;
	char			**CELL;
	unsigned char	*CHECK;
	int				R;

	if (!MORE->ROWS)
		return (1);

	if (!TABLE->ROWS)
	{
		LAYOUT_FREE(TABLE);
		*TABLE = *MORE;
		memset(MORE, 0, sizeof(*MORE));
		return (1);
	}

	GAP = 1;
	ROWS = TABLE->ROWS + GAP + MORE->ROWS;
	COLS = TABLE->COLS;

	if (MORE->COLS > COLS)
		COLS = MORE->COLS;

	CELL = (char **)calloc((size_t)ROWS * COLS + 1, sizeof(char *));
	CHECK = (unsigned char *)calloc((size_t)ROWS * COLS + 1, 1);

	if (!CELL || !CHECK)
	{
		free(CELL);
		free(CHECK);
		return (0);
	}

	for (R = 0; R < TABLE->ROWS + MORE->ROWS; ++R)
	{
		struct S_TABLE	*FROM = TABLE;
		int						ROW;
		int						TO;
		int						C;

		ROW = R;
		TO = R;

		if (R >= TABLE->ROWS)
		{
			FROM = MORE;
			ROW = R - TABLE->ROWS;
			TO = R + GAP;
		}

		for (C = 0; C < FROM->COLS; ++C)
		{
			const int	AT = ROW * FROM->COLS + C;

			CELL[TO * COLS + C] = FROM->CELL[AT];
			FROM->CELL[AT] = NULL;

			if (FROM->CHECK)
				CHECK[TO * COLS + C] = FROM->CHECK[AT];
		}
	}

	LAYOUT_FREE(TABLE);
	LAYOUT_FREE(MORE);
	TABLE->ROWS = ROWS;
	TABLE->COLS = COLS;
	TABLE->CELL = CELL;
	TABLE->CHECK = CHECK;
	return (1);
}

int
	LAYOUT_WRITE(const struct S_TABLE *TABLE, FILE *OUTPUT)
{
	int	R;

	for (R = 0; R < TABLE->ROWS; ++R)
	{
		int	C;

		for (C = 0; C < TABLE->COLS; ++C)
		{
			const char	*CELL = TABLE->CELL[R * TABLE->COLS + C];

			if (C)
				fputc('\t', OUTPUT);

			if (CELL)
				fputs(CELL, OUTPUT);
		}

		fputs("\r\n", OUTPUT);
	}

	return (!ferror(OUTPUT));
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                        PUBLIC API                        |  //\\  */
/* '""""' #==========================================================# '""""' */
