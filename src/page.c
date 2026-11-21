/******************************************************************************\
# C - page                                       #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/19 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/20 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

/*
# 1. INK: EVERY PIXEL IS COMPARED WITH THE PAPER AROUND IT (THE BRIGHTEST
#    PIXEL NEARBY, THEN THE DARKEST OF THOSE), SO SHADOWS AND UNEVEN LIGHT
#    DO NOT MATTER. A GRAINY PHOTO IS SMOOTHED FIRST, A SOFT ONE SHARPENED,
#    PALE STROKES COUNT AS INK SOONER, AND A TURNED PAGE IS TURNED BACK.
# 2. THE LINES OF A TABLE DRAWN BY HAND ARE FOUND (THIN STROKES, LYING OR
#    STANDING, MUCH LONGER THAN A LETTER, EVEN WOBBLY OR PALE), REMEMBERED
#    (ROWS, COLUMNS, THE BOX OF EVERY TABLE) AND TAKEN OUT, AND SO ARE THE
#    CELLS CROSSED OUT BY SCRIBBLING. LONG STRAIGHT STROKES (UNDERLINES,
#    PRINTED BORDERS) ARE TAKEN OUT TOO.
# 3. PIECES: CONNECTED INK. THE DARK EDGE OF THE PHOTO AND DUST GO AWAY.
# 4. THE PAGE IS CUT IN STANDING STRIPS. IN EVERY STRIP THE ROWS WITH THE MOST
#    INK ARE THE MIDDLES OF THE LINES; A MIDDLE IN ONE STRIP GOES ON TO THE
#    CLOSEST MIDDLE IN THE NEXT ONE. THESE ARE THE PATHS OF THE LINES.
# 5. EVERY PIECE GOES TO THE PATH THAT PASSES CLOSEST TO ITS MIDDLE. THE
#    PIECES OF A LINE ARE MOVED UP OR DOWN SO THE LINE COMES OUT LEVEL.
*/

/* **************************** [v] INCLUDES [v] **************************** */
#include <math.h>
/******* """"""""
 * float	cosf(float);
 * float	expf(float);
 * float	fabsf(float);
 * float	floorf(float);
 * float	sinf(float);
 * float	tanf(float);
 */
#include <stdlib.h>
/******* """"""""""
 * void	*calloc(size_t, size_t);
 * void	free(void *);
 * void	*malloc(size_t);
 * void	qsort(void *, size_t, size_t, int (*)(const void *, const void *));
 */
#include <string.h>
/******* """"""""""
 * #define NULL
 * typedef	size_t;
 * void		*memcpy(void *, const void *, size_t);
 * void		*memmove(void *, const void *, size_t);
 * void		*memset(void *, int, size_t);
 */
#include "page.h"
/******* """"""""
 * #define PAGE_CUTS
 * #define PAGE_DASHES
 * #define PAGE_HEIGHT
 * struct S_LINE;
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define PAPER_WINDOW 15 /* THE BRIGHTEST PIXEL IN 15 X 15 IS THE PAPER */
#define INK_START 0.88F /* DARKER THAN 88% OF THE PAPER STARTS TO BE INK */
#define INK_RANGE 0.35F /* AND 35% DARKER STILL IS FULL INK */
#define NOT_PAPER 0.4F /* PAPER DARKER THAN 40% OF THE USUAL: NOT THE PAGE */
#define NOISE_LEAST 4.0F /* A NOISIER PHOTO IS SMOOTHED FIRST */
#define STROKE_LEAST 0.06F /* 6% DARKER THAN THE PAPER: MAYBE A STROKE */
#define STROKE_USUAL 0.4F /* STROKES PALER THAN THIS: INK COUNTS SOONER */
#define STROKE_SCALE_LEAST 0.5F /* BUT AT MOST TWICE AS SOON */
#define PIECE_LEAST 6 /* PIECES SMALLER THAN THIS ARE DUST */
#define STRIP_LETTERS 8 /* A STRIP IS ABOUT 8 LETTER HEIGHTS WIDE */
#define SOFT_EDGE 1.2F /* EDGES SOFTER THAN THIS: THE PAGE IS SHARPENED */
#define SHARP_AMOUNT 1.5F /* HOW MUCH (AN UNSHARP MASK OF 150%) */
#define PATH_SLOPE_MOST 0.25F /* A LINE LEANS 14 DEGREES AT MOST */
#define WALLS_MAX 256 /* THE MOST DRAWN COLUMN LINES OF A PAGE */
#define RULES_MAX 512 /* THE MOST DRAWN LYING LINES OF A PAGE */
#define TABLES_MAX 32 /* THE MOST DRAWN TABLES OF A PAGE */
#define DRAWN_PIECES 200000 /* THE MOST THIN STROKES LOOKED AT */
#define DRAWN_ALONE 3.0F /* A LYING LINE THAT MAY BE OF A TABLE: 3 LETTERS */
#define DRAWN_LONG 6.0F /* A DRAWN LINE ALONE: 6 LETTERS LONG OR MORE */
#define DRAWN_ONE_END 2.0F /* ONE END ON A LINE OF THE OTHER WAY: 2 */
#define DRAWN_BOTH_ENDS 1.2F /* BOTH ENDS ON SUCH LINES (A CELL): 1.2 */
#define BLOT_STROKES 3 /* A BLOT: A SQUARE OF 3 STROKE WIDTHS FITS IN IT */
#define BLOT_LETTERS 0.6F /* IT IS 0.6 LETTERS WIDE AND HIGH OR MORE */
#define BLOT_FILL 0.6F /* IT FILLS 60% OF ITS BOX */
#define BLOT_PIECE 0.5F /* AND IS HALF OF ITS PIECE OF INK OR MORE */
#define LETTER_TALLEST 1024 /* TALLER PIECES ARE NOT LETTERS */
#define RULE_ERASE 0.35F /* INK ABOVE THIS IS PART OF A STROKE */
#define RULE_FAINT 0.15F /* A PALE DRAWN LINE IS FOUND ABOVE THIS */
#define TALL_LETTERS 4 /* A PIECE TALLER THAN 4 LETTERS IS NOT TEXT */
#define TURN_MOST 8.0F /* A PAGE TURNED UP TO 8 DEGREES IS TURNED BACK */
#define FAN_LEAST 2.0F /* TOP AND BOTTOM LEAN THIS MUCH APART: A FAN */
#define TURN_LEAST 3.5F /* LESS IS LEFT AS IT IS: THE LINES FOLLOW IT */

struct	S_PIECE
{
	int		LABEL;
	int		X0;
	int		Y0;
	int		X1; /* ONE PAST THE LAST COLUMN */
	int		Y1;
	int		AREA;
	float	CX; /* THE MIDDLE OF THE INK, NOT OF THE BOX */
	float	CY;
	int		PATH; /* THE LINE IT WENT TO, -1 FOR NONE */
};

struct	S_PATH
{
	int		COUNT;
	float	*X; /* ONE POINT PER STRIP IT GOES THROUGH */
	float	*Y;
	int		OPEN; /* IT MAY STILL GO ON IN THE NEXT STRIP */
};

static int	G__LETTER__ = 10;
/* THE TURN OF THE LAST PAGE: RADIANS, AND THE MIDDLES BEFORE AND AFTER */
static float	G__TURN__ = 0.0F;
static float	G__FROM_X__ = 0.0F;
static float	G__FROM_Y__ = 0.0F;
static float	G__TO_X__ = 0.0F;
static float	G__TO_Y__ = 0.0F;
/* THIN STROKES OF ONE WAY (THIN_PIECES) */
struct	S_THIN
{
	int	*LABEL; /* THE PIECE OF EVERY PIXEL, FROM 1 (0 = NONE) */
	int	*BOX; /* X0, Y0, X1, Y1 OF EVERY PIECE */
	int	*END; /* ITS TWO END PIXELS: X, Y, X, Y */
};

/* THE DRAWN COLUMN LINES OF THE LAST PAGE: X, FIRST AND LAST PIXEL ROW */
static int		G__WALLS__ = 0;
static float	G__WALL_X__[WALLS_MAX];
static int		G__WALL_Y0__[WALLS_MAX];
static int		G__WALL_Y1__[WALLS_MAX];
/* THE DRAWN TABLES OF THE LAST PAGE: X0, Y0, X1, Y1 OF EVERY ONE */
static int		G__TABLES__ = 0;
static int		G__TABLE_BOX__[TABLES_MAX][4];
/* THE LYING LINES OF THE DRAWN TABLES OF THE LAST PAGE: THEIR TWO ENDS */
static int		G__RULES__ = 0;
static float	G__RULE_END__[RULES_MAX][4];
/* THE FAN OF THE LAST PAGE (FAN_INK): SLOPES AT TWO PIXEL ROWS */
static int		G__FAN__ = 0;
static float	G__FAN_TOP__ = 0.0F;
static float	G__FAN_TOP_Y__ = 0.0F;
static float	G__FAN_BOTTOM__ = 0.0F;
static float	G__FAN_BOTTOM_Y__ = 1.0F;
static float	G__FAN_X__ = 0.0F;

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                         HELPERS                          |  \\//  */
/*   \/   #==========================================================#   \/   */
static int
	CMP_INT(const void *A, const void *B)
{
	const int	IA = *(const int *)A;
	const int	IB = *(const int *)B;

	return ((IA > IB) - (IA < IB));
}

static int
	CMP_FLOAT(const void *A, const void *B)
{
	const float	FA = *(const float *)A;
	const float	FB = *(const float *)B;

	return ((FA > FB) - (FA < FB));
}

/* THE MIDDLE VALUE, LIKE NUMPY: THE MEAN OF THE TWO MIDDLE ONES WHEN EVEN */
static float
	MEDIAN(float *VALUES, int COUNT)
{
	if (COUNT <= 0)
		return (0.0F);

	qsort(VALUES, (size_t)COUNT, sizeof(float), CMP_FLOAT);

	if (COUNT % 2)
		return (VALUES[COUNT / 2]);

	return ((VALUES[COUNT / 2 - 1] + VALUES[COUNT / 2]) / 2.0F);
}

/* THE BRIGHTEST VALUE IN A SIZE X SIZE SQUARE AROUND EVERY PIXEL */
static void
	MAX_FILTER(float *A, float *TMP, int W, int H, int SIZE)
{
	const int	HALF = SIZE / 2;
	int			X;
	int			Y;

	for (Y = 0; Y < H; ++Y)
	{
		for (X = 0; X < W; ++X)
		{
			float	BEST;
			int		K;

			BEST = 0.0F;

			for (K = X - HALF; K <= X + HALF; ++K)
				if (K >= 0 && K < W && A[Y * W + K] > BEST)
					BEST = A[Y * W + K];

			TMP[Y * W + X] = BEST;
		}
	}

	for (X = 0; X < W; ++X)
	{
		for (Y = 0; Y < H; ++Y)
		{
			float	BEST;
			int		K;

			BEST = 0.0F;

			for (K = Y - HALF; K <= Y + HALF; ++K)
				if (K >= 0 && K < H && TMP[K * W + X] > BEST)
					BEST = TMP[K * W + X];

			A[Y * W + X] = BEST;
		}
	}
}

/* THE DARKEST VALUE IN A SIZE X SIZE SQUARE AROUND EVERY PIXEL */
static void
	MIN_FILTER(float *A, float *TMP, int W, int H, int SIZE)
{
	const int	HALF = SIZE / 2;
	int			X;
	int			Y;

	for (Y = 0; Y < H; ++Y)
	{
		for (X = 0; X < W; ++X)
		{
			float	BEST;
			int		K;

			BEST = 1E30F;

			for (K = X - HALF; K <= X + HALF; ++K)
				if (K >= 0 && K < W && A[Y * W + K] < BEST)
					BEST = A[Y * W + K];

			TMP[Y * W + X] = BEST;
		}
	}

	for (X = 0; X < W; ++X)
	{
		for (Y = 0; Y < H; ++Y)
		{
			float	BEST;
			int		K;

			BEST = 1E30F;

			for (K = Y - HALF; K <= Y + HALF; ++K)
				if (K >= 0 && K < H && TMP[K * W + X] < BEST)
					BEST = TMP[K * W + X];

			A[Y * W + X] = BEST;
		}
	}
}

/* THE MIDDLE OF 9 VALUES */
static float
	MIDDLE_OF_9(float *V)
{
	int	I;
	int	J;

	for (I = 0; I < 5; ++I)
	{
		int	LOW;

		LOW = I;

		for (J = I + 1; J < 9; ++J)
			if (V[J] < V[LOW])
				LOW = J;

		if (LOW != I)
		{
			const float	SWAP = V[I];

			V[I] = V[LOW];
			V[LOW] = SWAP;
		}
	}

	return (V[4]);
}

/*
# EVERY PIXEL BECOMES THE MIDDLE VALUE OF ITS 3 X 3 SQUARE (NOISE GOES).
# NULL = NO MEMORY.
*/
static unsigned char
	*MEDIAN_3(const unsigned char *GRAY, int W, int H)
{
	unsigned char	*TARGET;
	int				X;
	int				Y;

	TARGET = (unsigned char *)malloc((size_t)W * (size_t)H);

	if (!TARGET)
		return (NULL);

	for (Y = 0; Y < H; ++Y)
	{
		for (X = 0; X < W; ++X)
		{
			float	V[9];
			int		N;
			int		DY;

			N = 0;

			for (DY = -1; DY <= 1; ++DY)
			{
				int	DX;

				for (DX = -1; DX <= 1; ++DX)
				{
					int	XX;
					int	YY;

					XX = X + DX;
					YY = Y + DY;

					if (XX < 0)
						XX = 0;

					if (XX >= W)
						XX = W - 1;

					if (YY < 0)
						YY = 0;

					if (YY >= H)
						YY = H - 1;

					V[N++] = (float)GRAY[(size_t)YY * W + XX];
				}
			}

			TARGET[(size_t)Y * W + X] = (unsigned char)MIDDLE_OF_9(V);
		}
	}

	return (TARGET);
}

/*
# HOW NOISY THE PHOTO IS: THE USUAL DIFFERENCE BETWEEN A PIXEL AND THE
# MIDDLE OF ITS 3 X 3 SQUARE (EVERY 3RD ROW AND COLUMN), AS A STANDARD
# DEVIATION. 1 OR 2 FOR A CLEAN PHOTO.
*/
static float
	NOISE_OF(const unsigned char *GRAY, int W, int H)
{
	const size_t	ROOM = (size_t)(W / 3 + 1) * (size_t)(H / 3 + 1);
	float			*DIFFS;
	float			RESULT;
	int				COUNT;
	int				X;
	int				Y;

	DIFFS = (float *)malloc(ROOM * sizeof(float));

	if (!DIFFS)
		return (0.0F);

	COUNT = 0;

	for (Y = 1; Y < H - 1; Y += 3)
	{
		for (X = 1; X < W - 1; X += 3)
		{
			float	V[9];
			int		N;
			int		DY;

			N = 0;

			for (DY = -1; DY <= 1; ++DY)
			{
				const unsigned char	*ROW = GRAY + (size_t)(Y + DY) * W;

				V[N++] = (float)ROW[X - 1];
				V[N++] = (float)ROW[X];
				V[N++] = (float)ROW[X + 1];
			}

			DIFFS[COUNT++] = fabsf(
				(float)GRAY[(size_t)Y * W + X] - MIDDLE_OF_9(V)
			);
		}
	}

	RESULT = 0.0F;

	if (COUNT)
		RESULT = 1.4826F * MEDIAN(DIFFS, COUNT);

	free(DIFFS);
	return (RESULT);
}

/* GAUSSIAN SMOOTHING OF A LIST, MIRRORED AT THE ENDS (LIKE SCIPY) */
static void
	SMOOTH(const float *SOURCE, float *TARGET, int COUNT, float SIGMA)
{
	const int	R = (int)(4.0F * SIGMA + 0.5F);
	float		*KERNEL;
	float		SUM;
	int			I;
	int			K;

	KERNEL = (float *)malloc((size_t)(2 * R + 1) * sizeof(float));

	if (!KERNEL)
	{
		memcpy(TARGET, SOURCE, (size_t)COUNT * sizeof(float));
		return ;
	}

	SUM = 0.0F;

	for (K = -R; K <= R; ++K)
	{
		KERNEL[K + R] = expf(-0.5F * (float)(K * K) / (SIGMA * SIGMA));
		SUM += KERNEL[K + R];
	}

	for (K = 0; K <= 2 * R; ++K)
		KERNEL[K] /= SUM;

	for (I = 0; I < COUNT; ++I)
	{
		float	V;

		V = 0.0F;

		for (K = -R; K <= R; ++K)
		{
			int	AT;

			AT = I + K;

			/* D C B A | A B C D | D C B A */
			while (AT < 0 || AT >= COUNT)
			{
				if (AT < 0)
					AT = -AT - 1;

				if (AT >= COUNT)
					AT = 2 * COUNT - AT - 1;
			}

			V += KERNEL[K + R] * SOURCE[AT];
		}

		TARGET[I] = V;
	}

	free(KERNEL);
}

/*
# ONE DIRECTION OF A RESIZE WITH A TRIANGLE FILTER THAT GROWS WHEN THE PICTURE
# GETS SMALLER (THE WAY PILLOW RESIZES, SO THE READER SEES WHAT IT LEARNED).
*/
static void
	RESIZE_PASS(
		const float *SOURCE,
		int IN_COUNT,
		int SRC_STEP,
		float *TARGET,
		int OUT_COUNT,
		int DST_STEP,
		int LINES,
		int SRC_NEXT,
		int DST_NEXT
	)
{
	const float	SCALE = (float)IN_COUNT / (float)OUT_COUNT;
	float		FILTER_SCALE;
	int			O;

	FILTER_SCALE = SCALE;

	if (FILTER_SCALE < 1.0F)
		FILTER_SCALE = 1.0F;

	for (O = 0; O < OUT_COUNT; ++O)
	{
		const float	CENTER = ((float)O + 0.5F) * SCALE;
		float		WEIGHT[64];
		float		SUM;
		int			FIRST;
		int			LAST;
		int			K;
		int			L;

		FIRST = (int)(CENTER - FILTER_SCALE + 0.5F);
		LAST = (int)(CENTER + FILTER_SCALE + 0.5F);

		if (FIRST < 0)
			FIRST = 0;

		if (LAST > IN_COUNT)
			LAST = IN_COUNT;

		if (LAST - FIRST > 64)
			LAST = FIRST + 64;

		SUM = 0.0F;

		for (K = FIRST; K < LAST; ++K)
		{
			float	D;

			D = fabsf(((float)K - CENTER + 0.5F) / FILTER_SCALE);
			WEIGHT[K - FIRST] = 0.0F;

			if (D < 1.0F)
				WEIGHT[K - FIRST] = 1.0F - D;

			SUM += WEIGHT[K - FIRST];
		}

		for (L = 0; L < LINES; ++L)
		{
			const float	*SRC = SOURCE + (size_t)L * (size_t)SRC_NEXT;
			float		V;

			V = 0.0F;

			for (K = FIRST; K < LAST; ++K)
				V += WEIGHT[K - FIRST] * SRC[K * SRC_STEP];

			if (SUM > 0.0F)
				V /= SUM;

			TARGET[(size_t)L * DST_NEXT + (size_t)O * DST_STEP] = V;
		}
	}
}

/* W X H TO NEW_W X NEW_H: FIRST THE WIDTH, THEN THE HEIGHT. NULL = NO MEMORY */
static float
	*RESIZE(const float *SOURCE, int W, int H, int NEW_W, int NEW_H)
{
	float	*MIDDLE;
	float	*TARGET;

	MIDDLE = (float *)malloc((size_t)NEW_W * (size_t)H * sizeof(float));
	TARGET = (float *)malloc((size_t)NEW_W * (size_t)NEW_H * sizeof(float));

	if (!MIDDLE || !TARGET)
	{
		free(MIDDLE);
		free(TARGET);
		return (NULL);
	}

	RESIZE_PASS(SOURCE, W, 1, MIDDLE, NEW_W, 1, H, W, NEW_W);
	RESIZE_PASS(MIDDLE, H, NEW_W, TARGET, NEW_H, NEW_W, NEW_W, 1, 1);
	free(MIDDLE);
	return (TARGET);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                         HELPERS                          |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                           INK                            |  \\//  */
/*   \/   #==========================================================#   \/   */
/* THE USUAL BRIGHTNESS OF THE PAPER (THE MIDDLE OF EVERY 7TH PIXEL) */
static float
	USUAL_PAPER(const float *PAPER, size_t COUNT, float *TMP)
{
	size_t	N;
	size_t	I;

	N = 0;

	for (I = 0; I < COUNT; I += 7)
		TMP[N++] = PAPER[I];

	if (!N)
		return (0.0F);

	return (MEDIAN(TMP, (int)N));
}

/*
# HOW DARK THE STROKES OF THE PAGE ARE: OF THE PIXELS AT LEAST A LITTLE
# DARKER THAN THE PAPER, THE DARKEST QUARTER STARTS HERE. 0 = TOO FEW.
# TMP: ROOM FOR COUNT VALUES.
*/
static float
	STROKE_DARKNESS(const float *DARK, size_t COUNT, float *TMP)
{
	size_t	N;
	size_t	I;

	N = 0;

	for (I = 0; I < COUNT; ++I)
		if (DARK[I] >= STROKE_LEAST)
			TMP[N++] = DARK[I];

	if (N < 200)
		return (0.0F);

	qsort(TMP, N, sizeof(float), CMP_FLOAT);
	return (TMP[N * 3 / 4]);
}

/*
# STEP 1: 0 = PAPER, 1 = INK. NOISE: HOW NOISY THE PHOTO WAS (SEE NOISE_OF).
# NULL = NO MEMORY.
*/
static float
	*INK_MAP(const unsigned char *GRAY, int W, int H, float NOISE)
{
	const size_t	SIZE = (size_t)W * (size_t)H;
	float			*INK;
	float			*PAPER;
	float			*TMP;
	float			*G;
	float			START;
	float			RANGE;
	size_t			I;

	INK = (float *)malloc(SIZE * sizeof(float));
	PAPER = (float *)malloc(SIZE * sizeof(float));
	TMP = (float *)malloc(SIZE * sizeof(float));
	G = (float *)malloc(SIZE * sizeof(float));

	if (!INK || !PAPER || !TMP || !G)
	{
		free(INK);
		free(PAPER);
		free(TMP);
		free(G);
		return (NULL);
	}

	for (I = 0; I < SIZE; ++I)
		G[I] = (float)GRAY[I];

	/*
	# THE PAPER: THE BRIGHTEST PIXEL AROUND, THEN THE DARKEST OF THOSE, SO
	# THE EDGE OF A SHADOW STAYS WHERE IT IS (A BLUR WOULD SPREAD IT, AND
	# THE PAPER BY THE EDGE WOULD LOOK LIKE INK)
	*/
	memcpy(PAPER, G, SIZE * sizeof(float));
	MAX_FILTER(PAPER, TMP, W, H, PAPER_WINDOW);
	MIN_FILTER(PAPER, TMP, W, H, PAPER_WINDOW);

	/* FAR DARKER THAN THE USUAL PAPER: NOT THE PAGE (A TABLE, A LAPTOP) */
	{
		const float	USUAL = USUAL_PAPER(PAPER, SIZE, TMP);

		for (I = 0; I < SIZE; ++I)
		{
			float	N;

			N = G[I];

			if (PAPER[I] > 1.0F)
				N /= PAPER[I];

			if (N > 1.0F || PAPER[I] < NOT_PAPER * USUAL)
				N = 1.0F;

			INK[I] = 1.0F - N;
		}
	}

	/* PALE STROKES (A PENCIL, A WORN PEN) COUNT AS INK SOONER */
	START = 1.0F - INK_START;
	RANGE = INK_RANGE;

	{
		const float	CORE = STROKE_DARKNESS(INK, SIZE, TMP);

		if (CORE > 0.0F && CORE < STROKE_USUAL)
		{
			float	SCALE;

			SCALE = CORE / STROKE_USUAL;

			if (SCALE < STROKE_SCALE_LEAST)
				SCALE = STROKE_SCALE_LEAST;

			START *= SCALE;
			RANGE *= SCALE;
		}
	}

	/* BUT NOT IN A GRAINY PHOTO: ITS GRAIN WOULD BE INK */
	if (NOISE > NOISE_LEAST)
	{
		START = 1.0F - INK_START;
		RANGE = INK_RANGE;
	}

	for (I = 0; I < SIZE; ++I)
	{
		float	V;

		V = (INK[I] - START) / RANGE;

		if (V < 0.0F)
			V = 0.0F;

		if (V > 1.0F)
			V = 1.0F;

		INK[I] = V;
	}

	free(G);
	free(PAPER);
	free(TMP);
	return (INK);
}

/*
# HOW SHARP THE ROWS OF INK ARE WHEN THE PAGE IS TURNED BY ANGLE (RADIANS):
# EVERY INKED PIXEL (ONE IN 4) GOES TO THE ROW IT WOULD BE IN (2 PIXELS A
# ROW, AS EVERY OTHER PIXEL ROW IS LOOKED AT), AND THE SUM OF THE SQUARES
# OF THE ROWS IS BIGGEST WHEN THE LINES LIE FLAT. ONLY THE PIXEL ROWS FROM
# BAND[0] TO BAND[1] COUNT.
*/
static double
	ROW_SHARPNESS(
		const float *INK,
		int W,
		int H,
		const int *BAND,
		float ANGLE,
		float *ROWS,
		int ROW_COUNT
	)
{
	const float	C = cosf(ANGLE);
	const float	S = sinf(ANGLE);
	const float	MID = (float)ROW_COUNT / 2.0F;
	double		SUM;
	int			X;
	int			Y;

	memset(ROWS, 0, (size_t)ROW_COUNT * sizeof(float));

	for (Y = BAND[0] & ~1; Y < BAND[1]; Y += 2)
	{
		for (X = 0; X < W; X += 2)
		{
			const float	DX = (float)X - (float)W / 2.0F;
			const float	DY = (float)Y - (float)H / 2.0F;
			int			ROW;

			if (INK[Y * W + X] <= 0.35F)
				continue ;

			ROW = (int)floorf((DX * S + DY * C) / 2.0F + MID);

			if (ROW >= 0 && ROW < ROW_COUNT)
				ROWS[ROW] += 1.0F;
		}
	}

	SUM = 0.0;

	for (Y = 0; Y < ROW_COUNT; ++Y)
		SUM += (double)ROWS[Y] * (double)ROWS[Y];

	return (SUM);
}

/*
# THE TURN THAT MAKES THE LINES OF THE PIXEL ROWS BAND[0] TO BAND[1] FLAT:
# EVERY HALF DEGREE, THEN FINER
*/
static float
	FIND_TURN(const float *INK, int W, int H, const int *BAND)
{
	const float	DEGREE = 3.14159265F / 180.0F;
	const int	ROW_COUNT = W + H + 2;
	float		*ROWS;
	double		BEST_SHARPNESS;
	float		BEST;
	float		FROM;
	float		TO;
	float		STEP;
	int			ROUND;

	ROWS = (float *)malloc((size_t)ROW_COUNT * sizeof(float));

	if (!ROWS)
		return (0.0F);

	BEST = 0.0F;
	BEST_SHARPNESS = ROW_SHARPNESS(INK, W, H, BAND, 0.0F, ROWS, ROW_COUNT);
	FROM = -TURN_MOST;
	TO = TURN_MOST;
	STEP = 0.5F;

	for (ROUND = 0; ROUND < 2; ++ROUND)
	{
		float	A;

		for (A = FROM; A <= TO + 0.01F; A += STEP)
		{
			const double	SHARPNESS = ROW_SHARPNESS(
				INK,
				W,
				H,
				BAND,
				A * DEGREE,
				ROWS,
				ROW_COUNT
			);

			if (SHARPNESS > BEST_SHARPNESS)
			{
				BEST_SHARPNESS = SHARPNESS;
				BEST = A;
			}
		}

		/* THEN EVERY TENTH OF A DEGREE AROUND THE BEST HALF DEGREE */
		FROM = BEST - 0.4F;
		TO = BEST + 0.4F;
		STEP = 0.1F;
	}

	free(ROWS);
	return (BEST);
}

/* THE LEAN OF THE FANNED PAGE AT PIXEL ROW Y (A STRAIGHT LINE THROUGH TWO) */
static float
	FAN_SLOPE(float Y)
{
	float	T;

	T = (Y - G__FAN_TOP_Y__) / (G__FAN_BOTTOM_Y__ - G__FAN_TOP_Y__);

	if (T < -0.5F)
		T = -0.5F;

	if (T > 1.5F)
		T = 1.5F;

	return (G__FAN_TOP__ + (G__FAN_BOTTOM__ - G__FAN_TOP__) * T);
}

/*
# THE PIXEL ROWS WITH INK: THE TOP AND BOTTOM 40% OF THEM (BY INK) GO TO
# TOP[] AND BOTTOM[] (FROM, TO). 0 = TOO LITTLE INK.
*/
static int
	INK_BANDS(const float *INK, int W, int H, int *TOP, int *BOTTOM)
{
	double	*SUM;
	double	TOTAL;
	double	RUN;
	int		X;
	int		Y;

	SUM = (double *)calloc((size_t)H + 1, sizeof(double));

	if (!SUM)
		return (0);

	TOTAL = 0.0;

	for (Y = 0; Y < H; ++Y)
	{
		for (X = 0; X < W; X += 2)
			SUM[Y] += INK[(size_t)Y * W + X] > 0.35F;

		TOTAL += SUM[Y];
	}

	if (TOTAL < 500.0)
	{
		free(SUM);
		return (0);
	}

	RUN = 0.0;
	TOP[0] = -1;
	TOP[1] = 0;
	BOTTOM[0] = H;
	BOTTOM[1] = H;

	for (Y = 0; Y < H; ++Y)
	{
		RUN += SUM[Y];

		if (TOP[0] < 0 && RUN > 0.0)
			TOP[0] = Y;

		if (RUN <= 0.4 * TOTAL)
			TOP[1] = Y + 1;

		if (RUN < 0.6 * TOTAL)
			BOTTOM[0] = Y + 1;

		if (RUN < TOTAL)
			BOTTOM[1] = Y + 2;
	}

	free(SUM);

	if (BOTTOM[1] > H)
		BOTTOM[1] = H;

	return (TOP[1] > TOP[0] && BOTTOM[1] > BOTTOM[0]);
}

/*
# A PAGE PHOTOGRAPHED FROM THE SIDE: ITS LINES LEAN ONE WAY AT THE TOP AND
# ANOTHER AT THE BOTTOM (THE FAR SIDE IS SMALLER). EVERY PIXEL ROW IS MOVED
# UP OR DOWN ALONG ITS OWN LEAN, WHICH GOES FROM THE LEAN OF THE TOP BAND TO
# THE LEAN OF THE BOTTOM BAND (SLOPES, AT THE PIXEL ROWS TOP_Y, BOTTOM_Y).
# G__FAN_* REMEMBER IT FOR PAGE_TURN(). NULL = NO MEMORY.
*/
static float
	*FAN_INK(
		const float *INK,
		int W,
		int H,
		float TOP_SLOPE,
		float TOP_Y,
		float BOTTOM_SLOPE,
		float BOTTOM_Y
	)
{
	float	*TARGET;
	int		X;
	int		Y;

	TARGET = (float *)calloc((size_t)W * (size_t)H, sizeof(float));

	if (!TARGET)
		return (NULL);

	G__FAN__ = 1;
	G__FAN_TOP__ = TOP_SLOPE;
	G__FAN_TOP_Y__ = TOP_Y;
	G__FAN_BOTTOM__ = BOTTOM_SLOPE;
	G__FAN_BOTTOM_Y__ = BOTTOM_Y;
	G__FAN_X__ = (float)W / 2.0F;

	for (Y = 0; Y < H; ++Y)
	{
		const float	SLOPE = FAN_SLOPE((float)Y);

		for (X = 0; X < W; ++X)
		{
			const float	SY = (float)Y + ((float)X - G__FAN_X__) * SLOPE;
			const int	Y0 = (int)floorf(SY);
			const float	FY = SY - (float)Y0;

			if (Y0 < 0 || Y0 + 1 >= H)
				continue ;

			TARGET[(size_t)Y * W + X] = (
				INK[(size_t)Y0 * W + X] * (1.0F - FY) +
				INK[(size_t)(Y0 + 1) * W + X] * FY
			);
		}
	}

	return (TARGET);
}

/*
# A FANNED PAGE (SEE FAN_INK) IS STRAIGHTENED: WHEN THE TOP AND THE BOTTOM
# OF THE INK LEAN FAN_LEAST DEGREES APART OR MORE. *INK IS REPLACED. 1 =
# STRAIGHTENED.
*/
static int
	FAN_PAGE(float **INK, int W, int H)
{
	const float	DEGREE = 3.14159265F / 180.0F;
	float		*FANNED;
	int			TOP[2];
	int			BOTTOM[2];
	float		TOP_TURN;
	float		BOTTOM_TURN;

	if (!INK_BANDS(*INK, W, H, TOP, BOTTOM))
		return (0);

	TOP_TURN = FIND_TURN(*INK, W, H, TOP);
	BOTTOM_TURN = FIND_TURN(*INK, W, H, BOTTOM);

	if (fabsf(TOP_TURN - BOTTOM_TURN) < FAN_LEAST)
		return (0);

	FANNED = FAN_INK(
		*INK,
		W,
		H,
		-tanf(TOP_TURN * DEGREE),
		(float)(TOP[0] + TOP[1]) / 2.0F,
		-tanf(BOTTOM_TURN * DEGREE),
		(float)(BOTTOM[0] + BOTTOM[1]) / 2.0F
	);

	if (!FANNED)
	{
		G__FAN__ = 0;
		return (0);
	}

	free(*INK);
	*INK = FANNED;
	return (1);
}

/*
# THE INK MAP TURNED BY ANGLE DEGREES (THE LINES FLAT), ON A BIGGER PAGE SO
# NO CORNER IS LOST. G__TURN__ AND THE MIDDLES REMEMBER IT FOR PAGE_TURN().
# NULL = NO MEMORY.
*/
static float
	*TURN_INK(const float *INK, int W, int H, float ANGLE, int *NW, int *NH)
{
	const float	A = ANGLE * 3.14159265F / 180.0F;
	const float	C = cosf(A);
	const float	S = sinf(A);
	float		*TARGET;
	int			X;
	int			Y;

	*NW = (int)((float)W * C + (float)H * fabsf(S)) + 2;
	*NH = (int)((float)H * C + (float)W * fabsf(S)) + 2;
	TARGET = (float *)calloc((size_t)*NW * (size_t)*NH, sizeof(float));

	if (!TARGET)
		return (NULL);

	G__TURN__ = A;
	G__FROM_X__ = (float)W / 2.0F;
	G__FROM_Y__ = (float)H / 2.0F;
	G__TO_X__ = (float)*NW / 2.0F;
	G__TO_Y__ = (float)*NH / 2.0F;

	for (Y = 0; Y < *NH; ++Y)
	{
		for (X = 0; X < *NW; ++X)
		{
			const float	DX = (float)X - G__TO_X__;
			const float	DY = (float)Y - G__TO_Y__;
			const float	SX = DX * C + DY * S + G__FROM_X__;
			const float	SY = -DX * S + DY * C + G__FROM_Y__;
			const int	X0 = (int)floorf(SX);
			const int	Y0 = (int)floorf(SY);
			const float	FX = SX - (float)X0;
			const float	FY = SY - (float)Y0;

			if (X0 < 0 || Y0 < 0 || X0 + 1 >= W || Y0 + 1 >= H)
				continue ;

			TARGET[Y * *NW + X] = (
				INK[Y0 * W + X0] * (1.0F - FX) * (1.0F - FY) +
				INK[Y0 * W + X0 + 1] * FX * (1.0F - FY) +
				INK[(Y0 + 1) * W + X0] * (1.0F - FX) * FY +
				INK[(Y0 + 1) * W + X0 + 1] * FX * FY
			);
		}
	}

	return (TARGET);
}

/*
# HOW SOFT THE EDGES OF THE INK ARE: ALONG EVERY OTHER ROW, THE PIXELS
# BETWEEN PAPER (BELOW 0.1) AND INK (ABOVE 0.6) WHERE A STROKE BEGINS, ON
# AVERAGE. ABOUT 1 FOR A SHARP PHOTO, 2 OR MORE FOR A BLURRED ONE; -1 WHEN
# THERE ARE TOO FEW STROKES TO TELL.
*/
static float
	EDGE_WIDTH(const float *INK, int W, int H)
{
	double	SUM;
	int		EDGES;
	int		Y;

	SUM = 0.0;
	EDGES = 0;

	for (Y = 0; Y < H; Y += 2)
	{
		const float	*ROW = INK + (size_t)Y * W;
		int			X;

		X = 0;

		while (X < W - 1)
		{
			int	K;

			if (!(ROW[X] < 0.1F && ROW[X + 1] >= 0.1F))
			{
				++X;
				continue ;
			}

			for (K = X + 1; K < W && K - X < 10; ++K)
				if (ROW[K] > 0.6F || ROW[K] < 0.1F)
					break ;

			if (K < W && ROW[K] > 0.6F)
			{
				SUM += (double)(K - X - 1);
				++EDGES;
			}

			X = K;
		}
	}

	if (EDGES < 50)
		return (-1.0F);

	return ((float)(SUM / EDGES));
}

/*
# A SOFT PHOTO MADE SHARP (UNSHARP MASK): EVERY PIXEL MOVES AWAY FROM THE
# BLURRED PICTURE BY SHARP_AMOUNT TIMES ITS DIFFERENCE. THE READER KNOWS
# SHARP STROKES, AND READS BLURRED ONES BADLY. NULL = NO MEMORY.
*/
static unsigned char
	*SHARPEN(const unsigned char *GRAY, int W, int H, float SIGMA)
{
	const int		MOST = W > H ? W : H;
	unsigned char	*TARGET;
	float			*SOFT;
	float			*LINE;
	float			*OUT;
	int				X;
	int				Y;

	TARGET = (unsigned char *)malloc((size_t)W * (size_t)H);
	SOFT = (float *)malloc((size_t)W * (size_t)H * sizeof(float));
	LINE = (float *)malloc((size_t)MOST * sizeof(float));
	OUT = (float *)malloc((size_t)MOST * sizeof(float));

	if (!TARGET || !SOFT || !LINE || !OUT)
	{
		free(TARGET);
		free(SOFT);
		free(LINE);
		free(OUT);
		return (NULL);
	}

	for (Y = 0; Y < H; ++Y)
	{
		for (X = 0; X < W; ++X)
			LINE[X] = (float)GRAY[(size_t)Y * W + X];

		SMOOTH(LINE, SOFT + (size_t)Y * W, W, SIGMA);
	}

	for (X = 0; X < W; ++X)
	{
		for (Y = 0; Y < H; ++Y)
			LINE[Y] = SOFT[(size_t)Y * W + X];

		SMOOTH(LINE, OUT, H, SIGMA);

		for (Y = 0; Y < H; ++Y)
		{
			const float	G = (float)GRAY[(size_t)Y * W + X];
			float		V;

			V = G + SHARP_AMOUNT * (G - OUT[Y]) + 0.5F;

			if (V < 0.0F)
				V = 0.0F;

			if (V > 255.0F)
				V = 255.0F;

			TARGET[(size_t)Y * W + X] = (unsigned char)V;
		}
	}

	free(SOFT);
	free(LINE);
	free(OUT);
	return (TARGET);
}

/*
# THE PLACE IN A PAGE OF W COLUMNS OF THE PIXEL AT STEP INNER OF ROW OUTER
# (ACROSS = 1), OR AT STEP INNER OF COLUMN OUTER (ACROSS = 0)
*/
static size_t
	PLACE(int ACROSS, int W, int OUTER, int INNER)
{
	if (ACROSS)
		return ((size_t)OUTER * (size_t)W + (size_t)INNER);

	return ((size_t)INNER * (size_t)W + (size_t)OUTER);
}

/*
# THE LENGTH OF THE RUN OF STROKE PIXELS (SOLID) EVERY PIXEL IS IN: ALONG
# ITS ROW (ACROSS = 1) OR ALONG ITS COLUMN (ACROSS = 0)
*/
static void
	RUN_LENGTHS(
		const unsigned char *SOLID,
		int W,
		int H,
		int ACROSS,
		unsigned short *RUN
	)
{
	const int	OUTER = ACROSS ? H : W;
	const int	INNER = ACROSS ? W : H;
	int			A;

	for (A = 0; A < OUTER; ++A)
	{
		int	B;

		B = 0;

		while (B < INNER)
		{
			int	END;
			int	K;

			END = B;

			while (END < INNER && SOLID[PLACE(ACROSS, W, A, END)])
				++END;

			for (K = B; K < END; ++K)
			{
				const size_t	AT = PLACE(ACROSS, W, A, K);
				unsigned short	LENGTH;

				LENGTH = 65535;

				if (END - B < 65535)
					LENGTH = (unsigned short)(END - B);

				RUN[AT] = LENGTH;
			}

			B = END + 1;
		}
	}
}

/*
# THE PIECES (8 NEIGHBOURS) OF A MASK: EVERY PIXEL GETS ITS NUMBER (FROM 1)
# IN LABEL, AND BOX GETS X0, Y0, X1, Y1 OF EVERY PIECE (ROOM FOR MOST).
# RETURNS THE NUMBER OF PIECES (AT MOST MOST), -1 = NO MEMORY.
*/
static int
	MASK_PIECES(
		const unsigned char *MASK,
		int W,
		int H,
		int *LABEL,
		int *BOX,
		int MOST
	)
{
	int	*STACK;
	int	COUNT;
	int	I;

	STACK = (int *)malloc((size_t)W * (size_t)H * sizeof(int));

	if (!STACK)
		return (-1);

	memset(LABEL, 0, (size_t)W * (size_t)H * sizeof(int));
	COUNT = 0;

	for (I = 0; I < W * H; ++I)
	{
		int	TOP;
		int	*B;

		if (!MASK[I] || LABEL[I] || COUNT >= MOST)
			continue ;

		++COUNT;
		B = &BOX[(COUNT - 1) * 4];
		B[0] = W;
		B[1] = H;
		B[2] = -1;
		B[3] = -1;
		TOP = 0;
		STACK[TOP++] = I;
		LABEL[I] = COUNT;

		while (TOP)
		{
			const int	P = STACK[--TOP];
			const int	X = P % W;
			const int	Y = P / W;
			int			DY;

			if (X < B[0])
				B[0] = X;

			if (Y < B[1])
				B[1] = Y;

			if (X > B[2])
				B[2] = X;

			if (Y > B[3])
				B[3] = Y;

			for (DY = -1; DY <= 1; ++DY)
			{
				int	DX;

				for (DX = -1; DX <= 1; ++DX)
				{
					const int	XX = X + DX;
					const int	YY = Y + DY;

					if (XX < 0 || YY < 0 || XX >= W || YY >= H)
						continue ;

					if (MASK[YY * W + XX] && !LABEL[YY * W + XX])
					{
						LABEL[YY * W + XX] = COUNT;
						STACK[TOP++] = YY * W + XX;
					}
				}
			}
		}
	}

	free(STACK);
	return (COUNT);
}

/*
# THE USUAL WIDTH OF A STROKE: THE SHORTER RUN (ALONG ITS ROW OR COLUMN) OF
# EVERY 3RD STROKE PIXEL, THE MIDDLE ONE. 0 = NO STROKES.
*/
static int
	STROKE_WIDTH(
		const unsigned char *SOLID,
		const unsigned short *ALONG_ROW,
		const unsigned short *ALONG_COLUMN,
		int COUNT
	)
{
	int	HISTOGRAM[65];
	int	TOTAL;
	int	SEEN;
	int	I;

	memset(HISTOGRAM, 0, sizeof(HISTOGRAM));
	TOTAL = 0;

	for (I = 0; I < COUNT; I += 3)
	{
		int	SHORT;

		if (!SOLID[I])
			continue ;

		SHORT = ALONG_ROW[I];

		if (ALONG_COLUMN[I] < SHORT)
			SHORT = ALONG_COLUMN[I];

		if (SHORT > 64)
			SHORT = 64;

		++HISTOGRAM[SHORT];
		++TOTAL;
	}

	SEEN = 0;

	for (I = 0; I <= 64 && TOTAL; ++I)
	{
		SEEN += HISTOGRAM[I];

		if (SEEN * 2 >= TOTAL)
			return (I);
	}

	return (0);
}

/*
# THE USUAL HEIGHT OF A LETTER: THE MIDDLE HEIGHT OF THE PIECES OF INK THAT
# ARE 3 STROKE WIDTHS TALL OR MORE AND AT MOST 4 TIMES WIDER THAN TALL.
# 0 = TOO FEW PIECES (OR NO MEMORY).
*/
static int
	INK_LETTER(const unsigned char *SOLID, int W, int H, int STROKE)
{
	int	*LABEL;
	int	*BOX;
	int	HISTOGRAM[LETTER_TALLEST + 1];
	int	COUNT;
	int	TOTAL;
	int	SEEN;
	int	I;

	LABEL = (int *)malloc((size_t)W * (size_t)H * sizeof(int));
	BOX = (int *)malloc(DRAWN_PIECES * 4 * sizeof(int));
	COUNT = 0;

	if (LABEL && BOX)
		COUNT = MASK_PIECES(SOLID, W, H, LABEL, BOX, DRAWN_PIECES);

	memset(HISTOGRAM, 0, sizeof(HISTOGRAM));
	TOTAL = 0;

	for (I = 0; I < COUNT; ++I)
	{
		const int	*B = &BOX[I * 4];
		const int	TALL = B[3] - B[1] + 1;
		const int	WIDE = B[2] - B[0] + 1;

		if (TALL < 3 * STROKE || TALL > LETTER_TALLEST || WIDE > 4 * TALL)
			continue ;

		++HISTOGRAM[TALL];
		++TOTAL;
	}

	free(LABEL);
	free(BOX);
	SEEN = 0;

	for (I = 0; I <= LETTER_TALLEST && TOTAL >= 10; ++I)
	{
		SEEN += HISTOGRAM[I];

		if (SEEN * 2 >= TOTAL)
			return (I);
	}

	return (0);
}

/* A MASK MADE BIGGER BY R PIXELS EVERY WAY (A SQUARE) */
static void
	GROW_MASK(
		const unsigned char *MASK,
		unsigned char *OUT,
		int W,
		int H,
		int R
	)
{
	unsigned char	*ROWS;
	int				X;
	int				Y;

	ROWS = (unsigned char *)calloc((size_t)W * (size_t)H, 1);

	if (!ROWS)
	{
		memmove(OUT, MASK, (size_t)W * (size_t)H);
		return ;
	}

	for (Y = 0; Y < H; ++Y)
	{
		int	LAST;

		LAST = -(1 << 20);

		for (X = 0; X < W; ++X)
			if (MASK[(size_t)Y * W + X])
				LAST = X;
			else if (X - LAST <= R)
				ROWS[(size_t)Y * W + X] = 1;

		LAST = 1 << 20;

		for (X = W - 1; X >= 0; --X)
			if (MASK[(size_t)Y * W + X])
			{
				LAST = X;
				ROWS[(size_t)Y * W + X] = 1;
			}
			else if (LAST - X <= R)
				ROWS[(size_t)Y * W + X] = 1;
	}

	memset(OUT, 0, (size_t)W * (size_t)H);

	for (X = 0; X < W; ++X)
	{
		int	LAST;

		LAST = -(1 << 20);

		for (Y = 0; Y < H; ++Y)
			if (ROWS[(size_t)Y * W + X])
			{
				LAST = Y;
				OUT[(size_t)Y * W + X] = 1;
			}
			else if (Y - LAST <= R)
				OUT[(size_t)Y * W + X] = 1;

		LAST = 1 << 20;

		for (Y = H - 1; Y >= 0; --Y)
			if (ROWS[(size_t)Y * W + X])
				LAST = Y;
			else if (LAST - Y <= R)
				OUT[(size_t)Y * W + X] = 1;
	}

	free(ROWS);
}

/*
# THE DRAWN LINES ARE ERASED WITH THE PALE EDGE AROUND THEM (WITHIN 2
# PIXELS), AND SO ARE THE SPOTS WHERE A LYING AND A STANDING ONE CROSS:
# WITHIN CROSS PIXELS OF BOTH. (A WIDER SPOT TOOK THE FIRST LETTER OF EVERY
# CELL OF A PRINTED TABLE, WHICH SITS CLOSE TO ITS TOP LEFT CORNER.)
*/
static void
	ERASE_MARKED(
		float *INK,
		int W,
		int H,
		const unsigned char *LYING,
		const unsigned char *STANDING,
		int CROSS
	)
{
	const size_t	SIZE = (size_t)W * (size_t)H;
	unsigned char	*NEAR_LYING;
	unsigned char	*NEAR_STANDING;
	unsigned char	*EDGE;
	size_t			I;

	NEAR_LYING = (unsigned char *)malloc(SIZE);
	NEAR_STANDING = (unsigned char *)malloc(SIZE);
	EDGE = (unsigned char *)calloc(SIZE, 1);

	if (!NEAR_LYING || !NEAR_STANDING || !EDGE)
	{
		free(NEAR_LYING);
		free(NEAR_STANDING);
		free(EDGE);
		return ;
	}

	GROW_MASK(LYING, NEAR_LYING, W, H, CROSS);
	GROW_MASK(STANDING, NEAR_STANDING, W, H, CROSS);

	for (I = 0; I < SIZE; ++I)
		EDGE[I] = LYING[I] || STANDING[I];

	GROW_MASK(EDGE, EDGE, W, H, 2);

	for (I = 0; I < SIZE; ++I)
	{
		if (LYING[I] || STANDING[I])
			INK[I] = 0.0F;
		else if (EDGE[I] && INK[I] <= RULE_ERASE)
			INK[I] = 0.0F;
		else if (NEAR_LYING[I] && NEAR_STANDING[I])
			INK[I] = 0.0F;
	}

	free(NEAR_LYING);
	free(NEAR_STANDING);
	free(EDGE);
}

/*
# THE THIN STROKES OF ONE WAY (ACROSS = 1: LYING, 0: STANDING): PIECES OF
# STROKE PIXELS WHOSE RUN THE OTHER WAY (RUN) IS THIN AT MOST. EVERY PIXEL
# GETS ITS PIECE IN LABEL; BOX GETS X0, Y0, X1, Y1 OF EVERY PIECE AND END
# THE TWO END PIXELS (X, Y, X, Y): LEFT AND RIGHT, OR TOP AND BOTTOM.
# RETURNS THE NUMBER OF PIECES, -1 = NO MEMORY.
*/
static int
	THIN_PIECES(
		const unsigned char *SOLID,
		const unsigned short *RUN,
		int W,
		int H,
		int ACROSS,
		int THIN,
		struct S_THIN *T
	)
{
	unsigned char	*MASK;
	int				COUNT;
	int				I;

	MASK = (unsigned char *)calloc((size_t)W * (size_t)H, 1);

	if (!MASK)
		return (-1);

	for (I = 0; I < W * H; ++I)
		MASK[I] = SOLID[I] && RUN[I] <= THIN;

	COUNT = MASK_PIECES(MASK, W, H, T->LABEL, T->BOX, DRAWN_PIECES);
	free(MASK);

	for (I = 0; I < W * H && COUNT > 0; ++I)
	{
		const int	K = T->LABEL[I] - 1;
		const int	X = I % W;
		const int	Y = I / W;
		const int	*B = &T->BOX[K * 4];
		int			*E;

		if (K < 0)
			continue ;

		E = &T->END[K * 4];

		if ((ACROSS && X == B[0]) || (!ACROSS && Y == B[1]))
		{
			E[0] = X;
			E[1] = Y;
		}

		if ((ACROSS && X == B[2]) || (!ACROSS && Y == B[3]))
		{
			E[2] = X;
			E[3] = Y;
		}
	}

	return (COUNT);
}

/* HOW MANY OF THE TWO ENDS OF A THIN PIECE LIE IN A MASK (NEAR A LINE) */
static int
	ENDS_NEAR(const struct S_THIN *T, int K, const unsigned char *NEAR, int W)
{
	const int	*E = &T->END[K * 4];
	int			COUNT;

	COUNT = 0;

	if (NEAR[(size_t)E[1] * W + E[0]])
		++COUNT;

	if (NEAR[(size_t)E[3] * W + E[2]])
		++COUNT;

	return (COUNT);
}

/* 1 = A THIN PIECE REACHES THE EDGE OF THE PHOTO AT ONE OF ITS ENDS */
static int
	CUT_BY_EDGE(const int *B, int ACROSS, int W, int H)
{
	if (ACROSS)
		return (B[0] <= 2 || B[2] >= W - 3);

	return (B[1] <= 2 || B[3] >= H - 3);
}

/*
# 1 = A THIN PIECE IS WRITING, NOT A LINE: AT MORE THAN A THIRD OF ITS
# PLACES, OTHER INK (MASK) GOES ON RIGHT ABOVE OR BELOW IT (OR LEFT OR
# RIGHT OF A STANDING ONE): THE STEMS OF SMALL LETTERS JOINED UP AT THE
# BOTTOM ("eam"). A LINE OF A TABLE ONLY MEETS ITS OTHER LINES AND A FEW
# LETTERS THAT TOUCH IT.
*/
static int
	IS_WRITING(
		const struct S_THIN *T,
		int K,
		int ACROSS,
		int W,
		int H,
		const unsigned char *MASK
	)
{
	const int	*B = &T->BOX[K * 4];
	const int	FIRST = ACROSS ? B[0] : B[1];
	const int	LAST = ACROSS ? B[2] : B[3];
	const int	FROM = ACROSS ? B[1] : B[0];
	const int	TO = ACROSS ? B[3] : B[2];
	const int	SIDE = ACROSS ? H : W;
	int			GOING;
	int			A;

	GOING = 0;

	for (A = FIRST; A <= LAST; ++A)
	{
		int	LOW;
		int	HIGH;
		int	D;
		int	C;

		LOW = -1;
		HIGH = -1;

		for (C = FROM; C <= TO; ++C)
		{
			const int	X = ACROSS ? A : C;
			const int	Y = ACROSS ? C : A;

			if (T->LABEL[(size_t)Y * W + X] != K + 1)
				continue ;

			if (LOW < 0)
				LOW = C;

			HIGH = C;
		}

		for (D = 1; D <= 2 && LOW >= 0; ++D)
		{
			const int	BEFORE = LOW - D;
			const int	AFTER = HIGH + D;
			size_t		AT;

			if (BEFORE >= 0)
			{
				AT = PLACE(ACROSS, W, BEFORE, A);

				if (MASK[AT] && T->LABEL[AT] != K + 1)
				{
					++GOING;
					break ;
				}
			}

			if (AFTER < SIDE)
			{
				AT = PLACE(ACROSS, W, AFTER, A);

				if (MASK[AT] && T->LABEL[AT] != K + 1)
				{
					++GOING;
					break ;
				}
			}
		}
	}

	return (GOING * 3 > LAST - FIRST + 1);
}

/*
# WHICH THIN PIECES ARE LINES OF A DRAWN TABLE (KEEP[K] = 1): 4 TIMES LONGER
# THAN WIDE, AND LONG ENOUGH: ALONE LETTERS LONG, ONE LETTERS LONG WHEN ONE
# END TOUCHES A LINE OF THE OTHER WAY (NEAR), BOTH LETTERS LONG WHEN BOTH
# ENDS DO; NOT WRITING (IS_WRITING, WITH THE INK OF MASK), NOT AT THE EDGE
# OF THE PHOTO. RETURNS HOW MANY NEW ONES WERE FOUND.
*/
static int
	CHOOSE_DRAWN(
		const struct S_THIN *T,
		int COUNT,
		int ACROSS,
		const unsigned char *NEAR,
		const unsigned char *MASK,
		int W,
		int H,
		float LETTER,
		const float LONG[3],
		unsigned char *KEEP
	)
{
	int	FOUND;
	int	K;

	FOUND = 0;

	for (K = 0; K < COUNT; ++K)
	{
		const int	*B = &T->BOX[K * 4];
		const int	ALONG = ACROSS ? B[2] - B[0] + 1 : B[3] - B[1] + 1;
		const int	WIDE = ACROSS ? B[3] - B[1] + 1 : B[2] - B[0] + 1;
		int			ENDS;

		/*
		# THE EDGE OF THE PHOTO (A DARK BORDER ALONG IT) IS NO LINE OF A
		# TABLE. A LINE THAT ONLY ENDS THERE IS: THE PHOTO CUT THE TABLE.
		*/
		if (
			KEEP[K] ||
			WIDE * 4 > ALONG ||
			(ACROSS && (B[1] <= 1 || B[3] >= H - 2)) ||
			(!ACROSS && (B[0] <= 1 || B[2] >= W - 2))
		)
			continue ;

		ENDS = 0;

		if (NEAR)
			ENDS = ENDS_NEAR(T, K, NEAR, W);

		/* AN END THE PHOTO CUT OFF COUNTS AS AN END ON A LINE */
		if (NEAR && ENDS < 2 && CUT_BY_EDGE(B, ACROSS, W, H))
			++ENDS;

		if (
			(float)ALONG < LONG[ENDS] * LETTER ||
			IS_WRITING(T, K, ACROSS, W, H, MASK)
		)
			continue ;

		KEEP[K] = 1;
		++FOUND;
	}

	return (FOUND);
}

static int
	CMP_FLOATS(const void *A, const void *B)
{
	const float	FA = *(const float *)A;
	const float	FB = *(const float *)B;

	return ((FA > FB) - (FA < FB));
}

/*
# THE MIDDLE LINE OF A THIN PIECE: ALONG IT (COLUMNS OF A LYING ONE, ROWS
# OF A STANDING ONE), THE MIDDLE OF ITS PIXELS ACROSS, THEN THE MIDDLE OF
# THOSE WITHIN REACH ON BOTH SIDES. -1 WHERE THE PIECE HAS NO PIXELS NEAR.
# 0 = NO MEMORY.
*/
static int
	MIDDLE_LINE(
		const struct S_THIN *T,
		int K,
		int ACROSS,
		int W,
		int REACH,
		float *MIDDLE
	)
{
	const int	*B = &T->BOX[K * 4];
	const int	FIRST = ACROSS ? B[0] : B[1];
	const int	LENGTH = (ACROSS ? B[2] : B[3]) - FIRST + 1;
	const int	FROM = ACROSS ? B[1] : B[0];
	const int	SPAN = (ACROSS ? B[3] : B[2]) - FROM + 1;
	float		*LIST;
	float		*RAW;
	int			A;

	LIST = (float *)malloc(
		((size_t)SPAN + 2 * (size_t)REACH + 2) * sizeof(float)
	);
	RAW = (float *)malloc((size_t)LENGTH * sizeof(float));

	if (!LIST || !RAW)
	{
		free(LIST);
		free(RAW);
		return (0);
	}

	for (A = 0; A < LENGTH; ++A)
	{
		int	N;
		int	C;

		N = 0;

		for (C = 0; C < SPAN; ++C)
		{
			const int	X = ACROSS ? FIRST + A : FROM + C;
			const int	Y = ACROSS ? FROM + C : FIRST + A;

			if (T->LABEL[(size_t)Y * W + X] == K + 1)
				LIST[N++] = (float)(FROM + C);
		}

		RAW[A] = -1.0F;

		if (N)
			RAW[A] = LIST[N / 2];
	}

	for (A = 0; A < LENGTH; ++A)
	{
		int	N;
		int	D;

		N = 0;

		for (D = -REACH; D <= REACH; ++D)
			if (A + D >= 0 && A + D < LENGTH && RAW[A + D] >= 0.0F)
				LIST[N++] = RAW[A + D];

		MIDDLE[A] = -1.0F;

		if (!N)
			continue ;

		qsort(LIST, (size_t)N, sizeof(float), CMP_FLOATS);
		MIDDLE[A] = LIST[N / 2];
	}

	free(LIST);
	free(RAW);
	return (1);
}

/*
# THE PIXELS OF THE CHOSEN THIN PIECES GET 1 IN A MASK, BUT ONLY THOSE AT
# MOST HALF PIXELS FROM THE MIDDLE LINE OF THEIR PIECE: A STROKE OF A
# LETTER THAT GROWS OUT OF A LINE (THE FOOT OF AN "L" ON IT) STAYS.
*/
static void
	MARK_DRAWN(
		const struct S_THIN *T,
		int COUNT,
		const unsigned char *KEEP,
		int ACROSS,
		int W,
		int REACH,
		int HALF,
		unsigned char *MASK
	)
{
	int	K;

	for (K = 0; K < COUNT; ++K)
	{
		const int	*B = &T->BOX[K * 4];
		const int	FIRST = ACROSS ? B[0] : B[1];
		const int	LENGTH = (ACROSS ? B[2] : B[3]) - FIRST + 1;
		float		*MIDDLE;
		int			A;

		if (!KEEP[K])
			continue ;

		MIDDLE = (float *)malloc((size_t)LENGTH * sizeof(float));

		if (!MIDDLE || !MIDDLE_LINE(T, K, ACROSS, W, REACH, MIDDLE))
		{
			free(MIDDLE);
			continue ;
		}

		for (A = 0; A < LENGTH; ++A)
		{
			const int	FROM = ACROSS ? B[1] : B[0];
			const int	TO = ACROSS ? B[3] : B[2];
			int			C;

			for (C = FROM; C <= TO && MIDDLE[A] >= 0.0F; ++C)
			{
				const int		X = ACROSS ? FIRST + A : C;
				const int		Y = ACROSS ? C : FIRST + A;
				const size_t	AT = (size_t)Y * W + X;

				if (
					T->LABEL[AT] == K + 1 &&
					fabsf((float)C - MIDDLE[A]) <= (float)HALF
				)
					MASK[AT] = 1;
			}
		}

		free(MIDDLE);
	}
}

/* HOW MANY THIN PIECES WERE CHOSEN */
static int
	CHOSEN(const unsigned char *KEEP, int COUNT)
{
	int	N;
	int	K;

	N = 0;

	for (K = 0; K < COUNT; ++K)
		N += KEEP[K] != 0;

	return (N);
}

/*
# THE DRAWN TABLES: THE BOXES OF THE PIECES OF ALL THE LINES FOUND (LYING
# OR STANDING), LINES LESS THAN REACH APART AS ONE, THAT HOLD A STANDING
# LINE. 0 = NO MEMORY.
*/
static int
	KEEP_TABLES(
		const unsigned char *LYING,
		const unsigned char *STANDING,
		int W,
		int H,
		int REACH
	)
{
	const size_t	SIZE = (size_t)W * (size_t)H;
	unsigned char	*ALL;
	unsigned char	*HAS;
	int				*LABEL;
	int				*BOX;
	int				COUNT;
	size_t			I;
	int				K;

	ALL = (unsigned char *)calloc(SIZE, 1);
	HAS = (unsigned char *)calloc(DRAWN_PIECES, 1);
	LABEL = (int *)malloc(SIZE * sizeof(int));
	BOX = (int *)malloc(DRAWN_PIECES * 4 * sizeof(int));
	COUNT = -1;

	if (ALL && HAS && LABEL && BOX)
	{
		for (I = 0; I < SIZE; ++I)
			ALL[I] = LYING[I] || STANDING[I];

		GROW_MASK(ALL, ALL, W, H, REACH);
		COUNT = MASK_PIECES(ALL, W, H, LABEL, BOX, DRAWN_PIECES);
	}

	for (I = 0; I < SIZE && COUNT > 0; ++I)
		if (STANDING[I] && LABEL[I])
			HAS[LABEL[I] - 1] = 1;

	for (K = 0; K < COUNT && G__TABLES__ < TABLES_MAX; ++K)
	{
		if (!HAS[K])
			continue ;

		memcpy(G__TABLE_BOX__[G__TABLES__], &BOX[K * 4], 4 * sizeof(int));
		++G__TABLES__;
	}

	free(ALL);
	free(HAS);
	free(LABEL);
	free(BOX);
	return (COUNT >= 0);
}

/* THE DRAWN TABLE THAT HOLDS A POINT OF THE PAGE, -1 = NONE */
static int
	TABLE_AT(float X, float Y)
{
	int	I;

	for (I = 0; I < G__TABLES__; ++I)
	{
		const int	*B = G__TABLE_BOX__[I];

		if (
			X >= (float)B[0] &&
			X <= (float)B[2] &&
			Y >= (float)B[1] &&
			Y <= (float)B[3]
		)
			return (I);
	}

	return (-1);
}

/* THE CHOSEN LYING PIECES ARE REMEMBERED BY THEIR ENDS (LINES OF ROWS) */
static void
	KEEP_RULES(const struct S_THIN *T, int COUNT, const unsigned char *KEEP)
{
	int	K;

	for (K = 0; K < COUNT && G__RULES__ < RULES_MAX; ++K)
	{
		int	I;

		if (!KEEP[K])
			continue ;

		for (I = 0; I < 4; ++I)
			G__RULE_END__[G__RULES__][I] = (float)T->END[K * 4 + I];

		++G__RULES__;
	}
}

/* THE CHOSEN STANDING PIECES ARE REMEMBERED AS WALLS (LINES OF COLUMNS) */
static void
	KEEP_WALLS(const struct S_THIN *T, int COUNT, const unsigned char *KEEP)
{
	int	K;

	for (K = 0; K < COUNT && G__WALLS__ < WALLS_MAX; ++K)
	{
		const int	*B = &T->BOX[K * 4];

		if (!KEEP[K])
			continue ;

		G__WALL_X__[G__WALLS__] = (float)(B[0] + B[2]) / 2.0F;
		G__WALL_Y0__[G__WALLS__] = B[1];
		G__WALL_Y1__[G__WALLS__] = B[3];
		++G__WALLS__;
	}
}

/*
# STEP 2: THE LINES OF A TABLE DRAWN BY HAND ARE ERASED: THIN STROKES MUCH
# LONGER THAN A LETTER, LYING OR STANDING, EVEN WHEN THEY WOBBLE OR ARE
# PALE. LYING ONES 3 LETTERS LONG MAY BE OF A TABLE; A STANDING ONE IS A
# LINE OF IT WHEN ITS ENDS TOUCH THEM (A CELL), OR WHEN IT IS VERY LONG.
# THEN THE LYING ONES THAT ARE VERY LONG OR TOUCH A STANDING ONE ARE LINES
# TOO. A LETTER THAT TOUCHES A LINE STAYS (ITS STROKES ARE NOT THIN THAT
# WAY OR GROW OUT OF IT); THE SPOTS WHERE LINES CROSS GO TOO. THE STANDING
# ONES ARE REMEMBERED (WALLS). A PAGE WITH ONE STANDING LINE AT MOST HAS NO
# TABLE: NOTHING IS ERASED (AN UNDERLINE STAYS FOR THE STEPS AFTER).
*/
static void
	ERASE_DRAWN(float *INK, int W, int H)
{
	static const float	MAYBE[3] = {DRAWN_ALONE, DRAWN_ALONE, DRAWN_ALONE};
	static const float	SURE[3] = {DRAWN_LONG, DRAWN_ONE_END, DRAWN_BOTH_ENDS};
	const size_t		SIZE = (size_t)W * (size_t)H;
	unsigned char		*SOLID;
	unsigned char		*FAINT;
	unsigned char		*LYING;
	unsigned char		*STANDING;
	unsigned char		*NEAR;
	unsigned short		*ALONG_ROW;
	unsigned short		*ALONG_COLUMN;
	unsigned char		*KEEP_LYING;
	unsigned char		*KEEP_STANDING;
	struct S_THIN		ROWS;
	struct S_THIN		COLUMNS;
	int					STROKE;
	int					LETTER;
	size_t				I;

	G__WALLS__ = 0;
	G__RULES__ = 0;
	G__TABLES__ = 0;
	SOLID = (unsigned char *)malloc(SIZE);
	FAINT = (unsigned char *)malloc(SIZE);
	LYING = (unsigned char *)calloc(SIZE, 1);
	STANDING = (unsigned char *)calloc(SIZE, 1);
	NEAR = (unsigned char *)malloc(SIZE);
	ALONG_ROW = (unsigned short *)calloc(SIZE, 2);
	ALONG_COLUMN = (unsigned short *)calloc(SIZE, 2);
	KEEP_LYING = (unsigned char *)calloc(DRAWN_PIECES, 1);
	KEEP_STANDING = (unsigned char *)calloc(DRAWN_PIECES, 1);
	ROWS.LABEL = (int *)malloc(SIZE * sizeof(int));
	ROWS.BOX = (int *)malloc(DRAWN_PIECES * 4 * sizeof(int));
	ROWS.END = (int *)calloc(DRAWN_PIECES * 4, sizeof(int));
	COLUMNS.LABEL = (int *)malloc(SIZE * sizeof(int));
	COLUMNS.BOX = (int *)malloc(DRAWN_PIECES * 4 * sizeof(int));
	COLUMNS.END = (int *)calloc(DRAWN_PIECES * 4, sizeof(int));

	if (
		SOLID && FAINT && LYING && STANDING && NEAR && ALONG_ROW &&
		ALONG_COLUMN && KEEP_LYING && KEEP_STANDING && ROWS.LABEL &&
		ROWS.BOX && ROWS.END && COLUMNS.LABEL && COLUMNS.BOX && COLUMNS.END
	)
	{
		for (I = 0; I < SIZE; ++I)
		{
			SOLID[I] = INK[I] > RULE_ERASE;
			FAINT[I] = INK[I] > RULE_FAINT;
		}

		RUN_LENGTHS(FAINT, W, H, 1, ALONG_ROW);
		RUN_LENGTHS(FAINT, W, H, 0, ALONG_COLUMN);
		STROKE = STROKE_WIDTH(FAINT, ALONG_ROW, ALONG_COLUMN, W * H);
		LETTER = INK_LETTER(SOLID, W, H, STROKE);

		if (STROKE > 0 && LETTER > 0)
		{
			const int	THIN = 3 * STROKE + 1;
			const int	LYING_COUNT = THIN_PIECES(
				FAINT,
				ALONG_COLUMN,
				W,
				H,
				1,
				THIN,
				&ROWS
			);
			const int	STANDING_COUNT = THIN_PIECES(
				FAINT,
				ALONG_ROW,
				W,
				H,
				0,
				THIN,
				&COLUMNS
			);

			/* LYING LINES THAT MAY BE OF A TABLE, THEN ITS STANDING ONES */
			CHOOSE_DRAWN(
				&ROWS,
				LYING_COUNT,
				1,
				NULL,
				FAINT,
				W,
				H,
				(float)LETTER,
				MAYBE,
				KEEP_LYING
			);
			MARK_DRAWN(
				&ROWS,
				LYING_COUNT,
				KEEP_LYING,
				1,
				W,
				LETTER,
				STROKE + 1,
				LYING
			);
			GROW_MASK(LYING, NEAR, W, H, THIN + 2);
			CHOOSE_DRAWN(
				&COLUMNS,
				STANDING_COUNT,
				0,
				NEAR,
				FAINT,
				W,
				H,
				(float)LETTER,
				SURE,
				KEEP_STANDING
			);
			MARK_DRAWN(
				&COLUMNS,
				STANDING_COUNT,
				KEEP_STANDING,
				0,
				W,
				LETTER,
				STROKE + 1,
				STANDING
			);
			/* NO TABLE (AT MOST ONE STANDING LINE): NOTHING IS ERASED */
			if (CHOSEN(KEEP_STANDING, STANDING_COUNT) < 2)
				memset(KEEP_STANDING, 0, DRAWN_PIECES);

			/* THE LYING LINES: VERY LONG, OR MEETING THE STANDING ONES */
			GROW_MASK(STANDING, NEAR, W, H, THIN + 2);
			memset(KEEP_LYING, 0, DRAWN_PIECES);
			memset(LYING, 0, SIZE);
			CHOOSE_DRAWN(
				&ROWS,
				LYING_COUNT,
				1,
				NEAR,
				FAINT,
				W,
				H,
				(float)LETTER,
				SURE,
				KEEP_LYING
			);
			MARK_DRAWN(
				&ROWS,
				LYING_COUNT,
				KEEP_LYING,
				1,
				W,
				LETTER,
				STROKE + 1,
				LYING
			);

			if (CHOSEN(KEEP_STANDING, STANDING_COUNT) >= 2)
			{
				KEEP_WALLS(&COLUMNS, STANDING_COUNT, KEEP_STANDING);
				KEEP_RULES(&ROWS, LYING_COUNT, KEEP_LYING);
				KEEP_TABLES(LYING, STANDING, W, H, THIN);
				ERASE_MARKED(INK, W, H, LYING, STANDING, 2 * STROKE + 2);
			}
		}
	}

	free(SOLID);
	free(FAINT);
	free(LYING);
	free(STANDING);
	free(NEAR);
	free(ALONG_ROW);
	free(ALONG_COLUMN);
	free(KEEP_LYING);
	free(KEEP_STANDING);
	free(ROWS.LABEL);
	free(ROWS.BOX);
	free(ROWS.END);
	free(COLUMNS.LABEL);
	free(COLUMNS.BOX);
	free(COLUMNS.END);
}

/* THE SUM OF A MASK OVER EVERY BOX FROM (0, 0): (W + 1) * (H + 1) VALUES */
static int
	*MASK_SUMS(const unsigned char *MASK, int W, int H)
{
	int	*SUM;
	int	X;
	int	Y;

	SUM = (int *)calloc(((size_t)W + 1) * ((size_t)H + 1), sizeof(int));

	if (!SUM)
		return (NULL);

	for (Y = 0; Y < H; ++Y)
	{
		int	ROW;

		ROW = 0;

		for (X = 0; X < W; ++X)
		{
			ROW += MASK[(size_t)Y * W + X] != 0;
			SUM[(size_t)(Y + 1) * (W + 1) + X + 1] =
				SUM[(size_t)Y * (W + 1) + X + 1] + ROW;
		}
	}

	return (SUM);
}

/* THE SUM OF A MASK IN THE BOX X0 <= X < X1, Y0 <= Y < Y1 (MASK_SUMS) */
static int
	BOX_SUM(const int *SUM, int W, int X0, int Y0, int X1, int Y1)
{
	const size_t	STRIDE = (size_t)W + 1;

	return (
		SUM[Y1 * STRIDE + X1] - SUM[Y0 * STRIDE + X1] -
		SUM[Y1 * STRIDE + X0] + SUM[Y0 * STRIDE + X0]
	);
}

/*
# THE PARTS OF A MASK WHERE A SQUARE OF SIDE PIXELS FITS: EVERY PIXEL OF
# EVERY SUCH SQUARE GETS 1 IN THICK. 0 = NO MEMORY.
*/
static int
	THICK_PARTS(
		const unsigned char *MASK,
		int W,
		int H,
		int SIDE,
		unsigned char *THICK
	)
{
	unsigned char	*CORNER;
	int				*SUM;
	int				X;
	int				Y;

	SUM = MASK_SUMS(MASK, W, H);
	CORNER = (unsigned char *)calloc((size_t)W * (size_t)H, 1);

	if (!SUM || !CORNER)
	{
		free(SUM);
		free(CORNER);
		return (0);
	}

	for (Y = 0; Y + SIDE <= H; ++Y)
		for (X = 0; X + SIDE <= W; ++X)
			if (BOX_SUM(SUM, W, X, Y, X + SIDE, Y + SIDE) == SIDE * SIDE)
				CORNER[(size_t)Y * W + X] = 1;

	free(SUM);
	SUM = MASK_SUMS(CORNER, W, H);

	if (!SUM)
	{
		free(CORNER);
		return (0);
	}

	for (Y = 0; Y < H; ++Y)
	{
		for (X = 0; X < W; ++X)
		{
			int	X0;
			int	Y0;

			X0 = X - SIDE + 1;
			Y0 = Y - SIDE + 1;

			if (X0 < 0)
				X0 = 0;

			if (Y0 < 0)
				Y0 = 0;

			THICK[(size_t)Y * W + X] =
				BOX_SUM(SUM, W, X0, Y0, X + 1, Y + 1) > 0;
		}
	}

	free(SUM);
	free(CORNER);
	return (1);
}

/*
# 1 = THE MIDDLE OF BOX B (X0, Y0, X1, Y1) IS INSIDE A DRAWN TABLE: A WALL
# ON ITS LEFT AND ONE ON ITS RIGHT GO PAST IT
*/
static int
	IN_TABLE(const int *B)
{
	const float	X = (float)(B[0] + B[2]) / 2.0F;
	const int	Y = (B[1] + B[3]) / 2;
	int			LEFT;
	int			RIGHT;
	int			I;

	LEFT = 0;
	RIGHT = 0;

	for (I = 0; I < G__WALLS__; ++I)
	{
		if (Y < G__WALL_Y0__[I] || Y > G__WALL_Y1__[I])
			continue ;

		LEFT |= G__WALL_X__[I] < X;
		RIGHT |= G__WALL_X__[I] > X;
	}

	return (LEFT && RIGHT);
}

/*
# 1 = A THICK PART (BOX B, AREA THICK PIXELS) IS A BLOT: IN A CELL OF A
# DRAWN TABLE (A DIGIT WRITTEN OVER HARD IN A LIST STAYS), BLOT_LETTERS
# WIDE AND HIGH OR MORE (AT MOST 3 TIMES AS LONG AS WIDE), FILLING
# BLOT_FILL OF ITS BOX, AND MOST OF THE PIECE OF INK IT IS IN (PIECE
# PIXELS): A LETTER WRITTEN OVER HARD HAS THICK SPOTS, BUT THE REST OF ITS
# STROKES ARE THIN.
*/
static int
	IS_BLOT(const int *B, int AREA, int PIECE, int LETTER)
{
	const int	WIDE = B[2] - B[0] + 1;
	const int	TALL = B[3] - B[1] + 1;
	const float	LEAST = BLOT_LETTERS * (float)LETTER;

	return (
		IN_TABLE(B) &&
		(float)WIDE >= LEAST &&
		(float)TALL >= LEAST &&
		WIDE <= 3 * TALL &&
		TALL <= 3 * WIDE &&
		(float)AREA >= BLOT_FILL * (float)(WIDE * TALL) &&
		(float)AREA >= BLOT_PIECE * (float)PIECE
	);
}

/*
# THE BLOTS AMONG THE THICK PARTS: THICK PARTS LESS THAN A STROKE APART ARE
# ONE (GROWN); SOLID IS ALL THE INK. ONLY THE GROWN BLOTS (IS_BLOT) STAY IN
# GROWN. 0 = NO MEMORY.
*/
static int
	KEEP_BLOTS(
		const unsigned char *THICK,
		const unsigned char *SOLID,
		unsigned char *GROWN,
		int W,
		int H,
		int LETTER
	)
{
	const size_t	SIZE = (size_t)W * (size_t)H;
	int				*LABEL;
	int				*INK_LABEL;
	int				*BOX;
	int				*INK_BOX;
	int				*AREA;
	int				*INK_AREA;
	int				*PIECE;
	int				COUNT;
	int				INK_COUNT;
	size_t			I;
	int				K;

	LABEL = (int *)malloc(SIZE * sizeof(int));
	INK_LABEL = (int *)malloc(SIZE * sizeof(int));
	BOX = (int *)malloc(DRAWN_PIECES * 4 * sizeof(int));
	INK_BOX = (int *)malloc(DRAWN_PIECES * 4 * sizeof(int));
	AREA = (int *)calloc(DRAWN_PIECES, sizeof(int));
	INK_AREA = (int *)calloc(DRAWN_PIECES, sizeof(int));
	PIECE = (int *)calloc(DRAWN_PIECES, sizeof(int));
	COUNT = -1;
	INK_COUNT = -1;

	if (LABEL && INK_LABEL && BOX && INK_BOX && AREA && INK_AREA && PIECE)
	{
		COUNT = MASK_PIECES(GROWN, W, H, LABEL, BOX, DRAWN_PIECES);
		INK_COUNT = MASK_PIECES(SOLID, W, H, INK_LABEL, INK_BOX, DRAWN_PIECES);
	}

	if (COUNT >= 0 && INK_COUNT >= 0)
	{
		/* THE BOXES OF THE THICK PIXELS ONLY, AND THE PIECE OF INK OF EACH */
		for (K = 0; K < COUNT; ++K)
		{
			BOX[K * 4] = W;
			BOX[K * 4 + 1] = H;
			BOX[K * 4 + 2] = -1;
			BOX[K * 4 + 3] = -1;
		}

		for (I = 0; I < SIZE; ++I)
			if (INK_LABEL[I])
				++INK_AREA[INK_LABEL[I] - 1];

		for (I = 0; I < SIZE; ++I)
		{
			const int	X = (int)(I % (size_t)W);
			const int	Y = (int)(I / (size_t)W);
			int			*B;

			if (!THICK[I] || !LABEL[I])
				continue ;

			K = LABEL[I] - 1;
			B = &BOX[K * 4];
			++AREA[K];

			if (!PIECE[K] && INK_LABEL[I])
				PIECE[K] = INK_AREA[INK_LABEL[I] - 1];

			if (X < B[0])
				B[0] = X;

			if (Y < B[1])
				B[1] = Y;

			if (X > B[2])
				B[2] = X;

			if (Y > B[3])
				B[3] = Y;
		}

		for (K = 0; K < COUNT; ++K)
			if (!IS_BLOT(&BOX[K * 4], AREA[K], PIECE[K], LETTER))
				AREA[K] = 0;

		for (I = 0; I < SIZE; ++I)
			GROWN[I] = LABEL[I] && AREA[LABEL[I] - 1] > 0;
	}

	free(LABEL);
	free(INK_LABEL);
	free(BOX);
	free(INK_BOX);
	free(AREA);
	free(INK_AREA);
	free(PIECE);
	return (COUNT >= 0 && INK_COUNT >= 0);
}

/*
# STEP 2: A BLOT IN A DRAWN TABLE (A CELL CROSSED OUT BY SCRIBBLING OVER
# IT) IS NOT TEXT: A PART OF THE INK WHERE A SQUARE OF 3 STROKE WIDTHS FITS
# (A STROKE OF A LETTER IS THINNER), BIG AND FILLED (IS_BLOT). IT IS ERASED
# WITH A STROKE AROUND IT. ERASE_DRAWN FOUND THE WALLS OF THE TABLES.
*/
static void
	ERASE_BLOTS(float *INK, int W, int H)
{
	const size_t	SIZE = (size_t)W * (size_t)H;
	unsigned char	*SOLID;
	unsigned char	*THICK;
	unsigned char	*GROWN;
	unsigned short	*ALONG_ROW;
	unsigned short	*ALONG_COLUMN;
	size_t			I;

	SOLID = (unsigned char *)malloc(SIZE);
	THICK = (unsigned char *)calloc(SIZE, 1);
	GROWN = (unsigned char *)calloc(SIZE, 1);
	ALONG_ROW = (unsigned short *)calloc(SIZE, 2);
	ALONG_COLUMN = (unsigned short *)calloc(SIZE, 2);

	if (SOLID && THICK && GROWN && ALONG_ROW && ALONG_COLUMN)
	{
		int	STROKE;
		int	LETTER;

		for (I = 0; I < SIZE; ++I)
			SOLID[I] = INK[I] > RULE_ERASE;

		RUN_LENGTHS(SOLID, W, H, 1, ALONG_ROW);
		RUN_LENGTHS(SOLID, W, H, 0, ALONG_COLUMN);
		STROKE = STROKE_WIDTH(SOLID, ALONG_ROW, ALONG_COLUMN, W * H);
		LETTER = INK_LETTER(SOLID, W, H, STROKE);

		if (
			G__WALLS__ >= 2 &&
			STROKE > 0 &&
			LETTER > 0 &&
			THICK_PARTS(SOLID, W, H, BLOT_STROKES * STROKE, THICK)
		)
		{
			GROW_MASK(THICK, GROWN, W, H, STROKE);

			if (KEEP_BLOTS(THICK, SOLID, GROWN, W, H, LETTER))
				for (I = 0; I < SIZE; ++I)
					if (GROWN[I])
						INK[I] = 0.0F;
		}
	}

	free(SOLID);
	free(THICK);
	free(GROWN);
	free(ALONG_ROW);
	free(ALONG_COLUMN);
}

/* STEP 2: LONG LYING STROKES ARE NOT TEXT */
static void
	ERASE_RULES(float *INK, int W, int H)
{
	unsigned char	*SOLID;
	int				LIMIT;
	int				Y;

	SOLID = (unsigned char *)malloc((size_t)W * (size_t)H);

	if (!SOLID)
		return ;

	for (Y = 0; Y < W * H; ++Y)
		SOLID[Y] = INK[Y] > 0.35F;

	LIMIT = W / 6;

	if (LIMIT < 40)
		LIMIT = 40;

	for (Y = 0; Y < H; ++Y)
	{
		int	X;

		X = 0;

		while (X < W)
		{
			int	START;

			if (!SOLID[Y * W + X])
			{
				++X;
				continue ;
			}

			START = X;

			/* A GAP OF 1 OR 2 PIXELS (A PALE BIT OF A RULE) DOES NOT END IT */
			while (
				X < W &&
				(
					SOLID[Y * W + X] ||
					(X + 1 < W && SOLID[Y * W + X + 1]) ||
					(X + 2 < W && SOLID[Y * W + X + 2])
				)
			)
				++X;

			if (X - START >= LIMIT)
			{
				int	K;

				for (K = Y - 1; K <= Y + 1; ++K)
					if (K >= 0 && K < H)
						memset(INK + K * W + START, 0, (size_t)(X - START) * 4);
			}
		}
	}

	free(SOLID);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                           INK                            |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          PIECES                          |  \\//  */
/*   \/   #==========================================================#   \/   */
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

/*
# STEP 3: CONNECTED INK (EIGHT NEIGHBOURS). *LABELS GETS THE PIECE NUMBER OF
# EVERY PIXEL (0 = PAPER). RETURNS THE NUMBER OF PIECES KEPT, -1 = NO MEMORY.
*/
static int
	FIND_PIECES(
		const float *INK,
		int W,
		int H,
		int **LABELS,
		struct S_PIECE **PIECES
	)
{
	int				*LABEL;
	int				*PARENT;
	struct S_PIECE	*P;
	int				NEXT;
	int				COUNT;
	int				X;
	int				Y;
	int				I;

	LABEL = (int *)calloc((size_t)W * (size_t)H, sizeof(int));
	PARENT = (int *)malloc(((size_t)W * (size_t)H / 2 + 2) * sizeof(int));

	if (!LABEL || !PARENT)
	{
		free(LABEL);
		free(PARENT);
		return (-1);
	}

	NEXT = 1;
	PARENT[0] = 0;

	for (Y = 0; Y < H; ++Y)
	{
		for (X = 0; X < W; ++X)
		{
			int	BEST;
			int	K;

			if (INK[Y * W + X] <= 0.35F)
				continue ;

			/* THE NEIGHBOURS ALREADY SEEN: LEFT, UP LEFT, UP, UP RIGHT */
			BEST = 0;

			for (K = 0; K < 4; ++K)
			{
				static const int	DX[4] = {-1, -1, 0, 1};
				static const int	DY[4] = {0, -1, -1, -1};
				const int			NX = X + DX[K];
				const int			NY = Y + DY[K];
				int					L;

				if (NX < 0 || NX >= W || NY < 0)
					continue ;

				L = LABEL[NY * W + NX];

				if (!L)
					continue ;

				if (!BEST)
					BEST = L;
				else if (ROOT(PARENT, L) != ROOT(PARENT, BEST))
					PARENT[ROOT(PARENT, L)] = ROOT(PARENT, BEST);
			}

			if (!BEST)
			{
				if (NEXT >= W * H / 2 + 1)
					continue ;

				BEST = NEXT;
				PARENT[NEXT] = NEXT;
				++NEXT;
			}

			LABEL[Y * W + X] = BEST;
		}
	}

	/* EVERY TREE GETS ONE NUMBER, FROM 1 */
	{
		int	*NUMBER;

		NUMBER = (int *)calloc((size_t)NEXT, sizeof(int));
		P = (struct S_PIECE *)calloc((size_t)NEXT, sizeof(*P));

		if (!NUMBER || !P)
		{
			free(NUMBER);
			free(P);
			free(LABEL);
			free(PARENT);
			return (-1);
		}

		COUNT = 0;

		for (I = 1; I < NEXT; ++I)
		{
			const int	R = ROOT(PARENT, I);

			if (!NUMBER[R])
			{
				NUMBER[R] = ++COUNT;
				P[COUNT - 1].LABEL = COUNT;
				P[COUNT - 1].X0 = W;
				P[COUNT - 1].Y0 = H;
				P[COUNT - 1].PATH = -1;
			}

			NUMBER[I] = NUMBER[R];
		}

		for (I = 0; I < W * H; ++I)
		{
			struct S_PIECE	*Q;

			if (!LABEL[I])
				continue ;

			LABEL[I] = NUMBER[LABEL[I]];
			Q = &P[LABEL[I] - 1];
			X = I % W;
			Y = I / W;

			if (X < Q->X0)
				Q->X0 = X;

			if (Y < Q->Y0)
				Q->Y0 = Y;

			if (X + 1 > Q->X1)
				Q->X1 = X + 1;

			if (Y + 1 > Q->Y1)
				Q->Y1 = Y + 1;

			Q->AREA += 1;
			Q->CX += (float)X;
			Q->CY += (float)Y;
		}

		free(NUMBER);
	}

	free(PARENT);

	for (I = 0; I < COUNT; ++I)
	{
		P[I].CX /= (float)P[I].AREA;
		P[I].CY /= (float)P[I].AREA;
	}

	*LABELS = LABEL;
	*PIECES = P;
	return (COUNT);
}

/* DUST AND THE DARK EDGE OF THE PHOTO ARE NOT TEXT */
static int
	IS_TEXT_PIECE(const struct S_PIECE *P, int W, int H)
{
	const int	EDGE = P->X0 == 0 || P->X1 == W || P->Y0 == 0 || P->Y1 == H;

	if (P->AREA < PIECE_LEAST)
		return (0);

	if (EDGE && ((P->Y1 - P->Y0) * 4 > H || (P->X1 - P->X0) * 3 > W))
		return (0);

	return (1);
}

/* THE USUAL HEIGHT OF A PIECE (PIECES OF MORE THAN 20 PIXELS) */
static int
	LETTER_HEIGHT(const struct S_PIECE *P, int COUNT)
{
	int	*HEIGHTS;
	int	N;
	int	I;
	int	RESULT;

	HEIGHTS = (int *)malloc((size_t)(COUNT + 1) * sizeof(int));

	if (!HEIGHTS)
		return (10);

	N = 0;

	for (I = 0; I < COUNT; ++I)
		if (P[I].AREA > 20)
			HEIGHTS[N++] = P[I].Y1 - P[I].Y0;

	RESULT = 10;

	if (N)
	{
		qsort(HEIGHTS, (size_t)N, sizeof(int), CMP_INT);
		RESULT = HEIGHTS[N / 2];
	}

	free(HEIGHTS);

	if (RESULT < 1)
		RESULT = 1;

	return (RESULT);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          PIECES                          |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          PATHS                           |  \\//  */
/*   \/   #==========================================================#   \/   */
/* THE HEIGHT OF A PATH AT X; BEFORE AND AFTER ITS ENDS IT LEANS HALF AS MUCH */
static float
	PATH_Y(const struct S_PATH *P, float X)
{
	int	I;

	if (P->COUNT == 1)
		return (P->Y[0]);

	if (X < P->X[0])
		return (
			P->Y[0] +
			(P->Y[1] - P->Y[0]) * (X - P->X[0]) / (P->X[1] - P->X[0]) * 0.5F
		);

	if (X >= P->X[P->COUNT - 1])
	{
		const int	L = P->COUNT - 1;

		return (
			P->Y[L] +
			(P->Y[L] - P->Y[L - 1]) * (X - P->X[L]) / (P->X[L] - P->X[L - 1]) *
			0.5F
		);
	}

	for (I = 0; I + 1 < P->COUNT; ++I)
		if (X >= P->X[I] && X <= P->X[I + 1])
		{
			const float	RISE = P->Y[I + 1] - P->Y[I];

			return (P->Y[I] + RISE * (X - P->X[I]) / (P->X[I + 1] - P->X[I]));
		}

	return (P->Y[P->COUNT - 1]);
}

struct	S_LINK
{
	float	DY;
	int		PATH;
	int		PEAK;
};

static int
	CMP_LINK(const void *A, const void *B)
{
	const struct S_LINK	*LA = (const struct S_LINK *)A;
	const struct S_LINK	*LB = (const struct S_LINK *)B;

	return ((LA->DY > LB->DY) - (LA->DY < LB->DY));
}

/*
# THE MIDDLES OF THE LINES IN ONE STRIP: THE ROWS WITH THE MOST INK (AFTER
# SMOOTHING). TWO MIDDLES CLOSER THAN 0.7 LETTER HEIGHTS: THE STRONGER STAYS.
# RETURNS THEIR NUMBER.
*/
static int
	STRIP_PEAKS(
		const float *KEEP,
		int W,
		int H,
		int FROM,
		int TO,
		int LETTER,
		float *PROFILE,
		float *SMOOTHED,
		int *PEAKS
	)
{
	float	SIGMA;
	float	TOP;
	int		COUNT;
	int		Y;

	for (Y = 0; Y < H; ++Y)
	{
		float	SUM;
		int		X;

		SUM = 0.0F;

		for (X = FROM; X < TO; ++X)
			SUM += KEEP[Y * W + X];

		PROFILE[Y] = SUM;
	}

	SIGMA = (float)LETTER / 3.0F;

	if (SIGMA < 1.0F)
		SIGMA = 1.0F;

	SMOOTH(PROFILE, SMOOTHED, H, SIGMA);
	TOP = 0.0F;

	for (Y = 0; Y < H; ++Y)
		if (SMOOTHED[Y] > TOP)
			TOP = SMOOTHED[Y];

	COUNT = 0;

	for (Y = 1; Y + 1 < H; ++Y)
	{
		const float	V = SMOOTHED[Y];

		if (V < SMOOTHED[Y - 1] || V <= SMOOTHED[Y + 1] || V <= 0.12F * TOP)
			continue ;

		if (COUNT && (float)(Y - PEAKS[COUNT - 1]) < 0.7F * (float)LETTER)
		{
			if (V > SMOOTHED[PEAKS[COUNT - 1]])
				PEAKS[COUNT - 1] = Y;

			continue ;
		}

		PEAKS[COUNT++] = Y;
	}

	return (COUNT);
}

/*
# WHERE A PATH IS LIKELY TO BE AT X: GOING ON AS ITS LAST 3 STRIPS LEANED,
# OR AS THE PAGE LEANS (SLOPE) WHEN IT IS SHORTER. A PHOTO TAKEN FROM THE
# SIDE LEANS ITS LINES EACH ITS OWN WAY.
*/
static float
	PATH_NEXT(const struct S_PATH *P, float X, float SLOPE)
{
	const int	L = P->COUNT - 1;

	if (P->COUNT >= 4 && P->X[L] > P->X[L - 3])
		SLOPE = (P->Y[L] - P->Y[L - 3]) / (P->X[L] - P->X[L - 3]);

	if (SLOPE > PATH_SLOPE_MOST)
		SLOPE = PATH_SLOPE_MOST;

	if (SLOPE < -PATH_SLOPE_MOST)
		SLOPE = -PATH_SLOPE_MOST;

	return (P->Y[L] + SLOPE * (X - P->X[L]));
}

/*
# STEP 4: THE PATHS OF THE LINES. A PATH IN ONE STRIP GOES ON TO THE CLOSEST
# MIDDLE IN THE NEXT STRIP WHEN IT IS LESS THAN 0.45 LINE SPACINGS AWAY FROM
# WHERE IT WAS GOING (PATH_NEXT); THE CLOSEST PAIRS ARE JOINED FIRST.
# RETURNS THE NUMBER OF PATHS, -1 = NO MEMORY.
*/
static int
	MAKE_PATHS(
		const int *PEAKS,
		const int *PEAK_COUNT,
		int STRIPS,
		int MOST,
		const int *EDGES,
		float SPACING,
		float SLOPE,
		struct S_PATH **PATHS
	)
{
	struct S_PATH	*P;
	struct S_LINK	*LINKS;
	int				*TAKEN;
	int				TOTAL;
	int				COUNT;
	int				S;
	int				I;

	TOTAL = 0;

	for (S = 0; S < STRIPS; ++S)
		TOTAL += PEAK_COUNT[S];

	P = (struct S_PATH *)calloc((size_t)TOTAL + 1, sizeof(*P));
	LINKS = (struct S_LINK *)malloc(
		((size_t)TOTAL * (size_t)MOST + 1) * sizeof(*LINKS)
	);
	TAKEN = (int *)malloc(((size_t)MOST + 1) * sizeof(int));

	if (!P || !LINKS || !TAKEN)
	{
		free(P);
		free(LINKS);
		free(TAKEN);
		return (-1);
	}

	COUNT = 0;

	for (S = 0; S < STRIPS; ++S)
	{
		const float	CX = (float)(EDGES[S] + EDGES[S + 1]) / 2.0F;
		const int	*HERE = PEAKS + S * MOST;
		int			LINK_COUNT;
		int			*GOES_ON;

		LINK_COUNT = 0;

		for (I = 0; I < COUNT; ++I)
		{
			float	AHEAD;
			int		Q;

			if (!P[I].OPEN)
				continue ;

			AHEAD = PATH_NEXT(&P[I], CX, SLOPE);

			for (Q = 0; Q < PEAK_COUNT[S]; ++Q)
			{
				const float	DY = fabsf((float)HERE[Q] - AHEAD);

				if (DY < 0.45F * SPACING)
				{
					LINKS[LINK_COUNT].DY = DY;
					LINKS[LINK_COUNT].PATH = I;
					LINKS[LINK_COUNT].PEAK = Q;
					++LINK_COUNT;
				}
			}
		}

		qsort(LINKS, (size_t)LINK_COUNT, sizeof(*LINKS), CMP_LINK);
		memset(TAKEN, 0, ((size_t)MOST + 1) * sizeof(int));
		GOES_ON = (int *)calloc((size_t)COUNT + 1, sizeof(int));

		if (!GOES_ON)
			break ;

		for (I = 0; I < LINK_COUNT; ++I)
		{
			struct S_PATH	*Q = &P[LINKS[I].PATH];

			if (GOES_ON[LINKS[I].PATH] || TAKEN[LINKS[I].PEAK])
				continue ;

			GOES_ON[LINKS[I].PATH] = 1;
			TAKEN[LINKS[I].PEAK] = 1;
			Q->X[Q->COUNT] = CX;
			Q->Y[Q->COUNT] = (float)HERE[LINKS[I].PEAK];
			Q->COUNT += 1;
		}

		/* A PATH THAT DID NOT GO ON HAS ENDED */
		for (I = 0; I < COUNT; ++I)
			if (P[I].OPEN && !GOES_ON[I])
				P[I].OPEN = 0;

		free(GOES_ON);

		/* A MIDDLE THAT CONTINUES NO PATH STARTS A NEW ONE */
		for (I = 0; I < PEAK_COUNT[S]; ++I)
		{
			if (TAKEN[I])
				continue ;

			P[COUNT].X = (float *)malloc((size_t)STRIPS * sizeof(float));
			P[COUNT].Y = (float *)malloc((size_t)STRIPS * sizeof(float));

			if (!P[COUNT].X || !P[COUNT].Y)
			{
				free(P[COUNT].X);
				free(P[COUNT].Y);
				continue ;
			}

			P[COUNT].X[0] = CX;
			P[COUNT].Y[0] = (float)HERE[I];
			P[COUNT].COUNT = 1;
			P[COUNT].OPEN = 1;
			++COUNT;
		}
	}

	free(LINKS);
	free(TAKEN);
	*PATHS = P;
	return (COUNT);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          PATHS                           |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          LINES                           |  \\//  */
/*   \/   #==========================================================#   \/   */
/* THE POINTS OF PATH B GO INTO PATH A, FROM THE LEFT. 1 = DONE */
static int
	JOIN_POINTS(struct S_PATH *A, const struct S_PATH *B)
{
	float	*X;
	float	*Y;
	int		N;
	int		I;
	int		J;

	X = (float *)malloc((size_t)(A->COUNT + B->COUNT) * sizeof(float));
	Y = (float *)malloc((size_t)(A->COUNT + B->COUNT) * sizeof(float));

	if (!X || !Y)
	{
		free(X);
		free(Y);
		return (0);
	}

	N = 0;
	I = 0;
	J = 0;

	while (I < A->COUNT || J < B->COUNT)
	{
		float	PX;
		float	PY;

		if (J >= B->COUNT || (I < A->COUNT && A->X[I] <= B->X[J]))
		{
			PX = A->X[I];
			PY = A->Y[I++];
		}
		else
		{
			PX = B->X[J];
			PY = B->Y[J++];
		}

		/* TWO POINTS IN ONE STRIP: THE FIRST ONE STAYS */
		if (N && PX < X[N - 1] + 1.0F)
			continue ;

		X[N] = PX;
		Y[N++] = PY;
	}

	free(A->X);
	free(A->Y);
	A->X = X;
	A->Y = Y;
	A->COUNT = N;
	return (1);
}

/*
# ONE LINE CAN BECOME TWO PATHS WHEN A WIDE GAP BREAKS IT ("1346 -", THEN
# FAR TO THE RIGHT "Cream - vizon - 6 Top"). TWO PATHS WHOSE PIECES DO NOT
# OVERLAP FROM LEFT TO RIGHT, AND THAT MEET IN THEIR GAP (LESS THAN 0.4 OF
# THE LINE SPACING APART THERE), BECOME ONE: THE CLOSEST TWO FIRST.
*/
static void
	JOIN_PATHS(
		struct S_PIECE *P,
		int COUNT,
		struct S_PATH *PATHS,
		int PATH_COUNT,
		float SPACING,
		int LETTER
	)
{
	int	*LEFT_X;
	int	*RIGHT_X;
	int	I;

	LEFT_X = (int *)malloc((size_t)PATH_COUNT * sizeof(int));
	RIGHT_X = (int *)malloc((size_t)PATH_COUNT * sizeof(int));

	if (!LEFT_X || !RIGHT_X)
	{
		free(LEFT_X);
		free(RIGHT_X);
		return ;
	}

	for (;;)
	{
		float	BEST_D;
		int		BEST_A;
		int		BEST_B;
		int		A;
		int		B;

		for (A = 0; A < PATH_COUNT; ++A)
		{
			LEFT_X[A] = 1 << 30;
			RIGHT_X[A] = -1;
		}

		for (I = 0; I < COUNT; ++I)
		{
			const int	K = P[I].PATH;

			if (K < 0)
				continue ;

			if (P[I].X0 < LEFT_X[K])
				LEFT_X[K] = P[I].X0;

			if (P[I].X1 > RIGHT_X[K])
				RIGHT_X[K] = P[I].X1;
		}

		BEST_D = 0.4F * SPACING;
		BEST_A = -1;
		BEST_B = -1;

		/* A ON THE LEFT, B ON THE RIGHT */
		for (A = 0; A < PATH_COUNT; ++A)
		{
			for (B = 0; B < PATH_COUNT; ++B)
			{
				float	GAP_X;
				float	D;

				if (
					A == B ||
					RIGHT_X[A] < 0 ||
					RIGHT_X[B] < 0 ||
					RIGHT_X[A] > LEFT_X[B] + LETTER / 2 ||
					LEFT_X[A] >= LEFT_X[B]
				)
					continue ;

				GAP_X = (float)(RIGHT_X[A] + LEFT_X[B]) / 2.0F;
				D = fabsf(PATH_Y(&PATHS[A], GAP_X) - PATH_Y(&PATHS[B], GAP_X));

				if (D < BEST_D)
				{
					BEST_D = D;
					BEST_A = A;
					BEST_B = B;
				}
			}
		}

		if (BEST_A < 0 || !JOIN_POINTS(&PATHS[BEST_A], &PATHS[BEST_B]))
			break ;

		for (I = 0; I < COUNT; ++I)
			if (P[I].PATH == BEST_B)
				P[I].PATH = BEST_A;
	}

	free(LEFT_X);
	free(RIGHT_X);
}

/*
# ONLY LONG FLAT STROKES (AN UNDERLINE), OR NOTHING AS TALL AS HALF A LETTER
# (THE EDGE OF THE PAPER, DOTS): NOT TEXT
*/
static int
	ONLY_FLAT(const struct S_PIECE *P, int COUNT, int PATH, int LETTER)
{
	int	FLAT;
	int	TALLEST;
	int	I;

	FLAT = 1;
	TALLEST = 0;

	for (I = 0; I < COUNT; ++I)
	{
		const int	W = P[I].X1 - P[I].X0;
		const int	H = P[I].Y1 - P[I].Y0;

		if (P[I].PATH != PATH || P[I].AREA <= 10)
			continue ;

		if (H > TALLEST)
			TALLEST = H;

		if (!(W > 3 * H && (float)H < 0.8F * (float)LETTER))
			FLAT = 0;
	}

	return (FLAT || TALLEST * 2 < LETTER);
}

/*
# THE DASHES OF A LINE: A PIECE THAT IS FLAT (AT LEAST TWICE AS WIDE AS HIGH,
# LOWER THAN A THIRD OF A LETTER), AS WIDE AS A THIRD OF A LETTER OR MORE BUT
# NOT A RULE, IN THE MIDDLE OF THE LINE (NOT AN UNDERLINE OR AN ACCENT), AND
# WITH NOTHING ELSE ABOVE OR BELOW IT (NOT "=" OR THE BAR OF A "t"). THE
# READER MISSES A LONG THIN "-" BETWEEN WIDE GAPS; THIS DOES NOT.
*/
static void
	FIND_DASHES(
		const struct S_PIECE *P,
		int COUNT,
		const struct S_PATH *PATH,
		int NUMBER,
		int LETTER,
		int LEFT,
		float SCALE,
		struct S_LINE *LINE
	)
{
	int	I;

	LINE->DASHES = 0;

	for (I = 0; I < COUNT && LINE->DASHES < PAGE_DASHES; ++I)
	{
		const int	W = P[I].X1 - P[I].X0;
		const int	H = P[I].Y1 - P[I].Y0;
		int			J;

		if (P[I].PATH != NUMBER)
			continue ;

		if (
			W < 2 * H ||
			(float)H * 3.0F > (float)LETTER ||
			(float)W * 3.0F < (float)LETTER ||
			W > 4 * LETTER ||
			fabsf(P[I].CY - PATH_Y(PATH, P[I].CX)) > 0.45F * (float)LETTER
		)
			continue ;

		for (J = 0; J < COUNT; ++J)
		{
			int	OVER;

			if (J == I || P[J].PATH != NUMBER)
				continue ;

			OVER = P[I].X1;

			if (P[J].X1 < OVER)
				OVER = P[J].X1;

			if (P[J].X0 > P[I].X0)
				OVER -= P[J].X0;
			else
				OVER -= P[I].X0;

			if (OVER * 10 > W * 3)
				break ;
		}

		if (J < COUNT)
			continue ;

		LINE->DASH_FROM[LINE->DASHES] = (int)((float)(P[I].X0 - LEFT) / SCALE);
		LINE->DASH_TO[LINE->DASHES] = (int)((float)(P[I].X1 - LEFT) / SCALE);
		++LINE->DASHES;
	}
}

/*
# STEP 5: EVERY PIECE TO THE CLOSEST PATH, WITHIN 0.6 OF THE LINE SPACING.
# AGAIN = 1 AFTER PATHS WERE JOINED: ONLY THE PIECES WITHOUT A PATH, AND
# ONLY TO THE PATHS THAT HAVE PIECES (A "5" IN THE GAP BETWEEN TWO PATHS
# THAT BECAME ONE).
*/
static void
	PIECES_TO_PATHS(
		struct S_PIECE *P,
		int COUNT,
		const struct S_PATH *PATHS,
		int PATH_COUNT,
		float STRIP_W,
		float SPACING,
		int AGAIN
	)
{
	unsigned char	*USED;
	int				I;

	USED = (unsigned char *)calloc((size_t)PATH_COUNT + 1, 1);

	if (!USED)
		return ;

	for (I = 0; I < COUNT; ++I)
		if (P[I].PATH >= 0)
			USED[P[I].PATH] = 1;

	for (I = 0; I < COUNT; ++I)
	{
		float	BEST_D;
		int		BEST;
		int		K;

		if (AGAIN && P[I].PATH >= 0)
			continue ;

		BEST = -1;
		BEST_D = 1E30F;

		for (K = 0; K < PATH_COUNT; ++K)
		{
			const struct S_PATH	*Q = &PATHS[K];
			float				D;

			if (
				(AGAIN && !USED[K]) ||
				P[I].CX < Q->X[0] - STRIP_W ||
				P[I].CX > Q->X[Q->COUNT - 1] + STRIP_W
			)
				continue ;

			D = fabsf(P[I].CY - PATH_Y(Q, P[I].CX));

			if (D < BEST_D)
			{
				BEST_D = D;
				BEST = K;
			}
		}

		if (BEST >= 0 && BEST_D < 0.6F * SPACING)
			P[I].PATH = BEST;
	}

	free(USED);
}

/*
# THE DRAWN COLUMN LINES (WALLS) A LINE CROSSES: THEY STAND BETWEEN ITS
# FIRST AND LAST INK AND COVER HALF OF ITS HEIGHT OR MORE WHERE THEY ARE.
# THEIR COLUMNS OF INK[] GO TO CUT_AT.
*/
static void
	FIND_CUTS(const struct S_PATH *PATH, float BASE, struct S_LINE *LINE)
{
	const float	TALL = (float)(LINE->Y1 - LINE->Y0);
	int			I;

	LINE->CUTS = 0;

	for (I = 0; I < G__WALLS__ && LINE->CUTS < PAGE_CUTS; ++I)
	{
		const float	X = G__WALL_X__[I];
		float		SHIFT;
		float		OVER;
		float		TOP;
		float		BOTTOM;

		if (X <= (float)LINE->X0 || X >= (float)LINE->X1)
			continue ;

		SHIFT = PATH_Y(PATH, X) - BASE;
		TOP = (float)LINE->Y0 + SHIFT;
		BOTTOM = (float)LINE->Y1 + SHIFT;

		if ((float)G__WALL_Y0__[I] > TOP)
			TOP = (float)G__WALL_Y0__[I];

		if ((float)G__WALL_Y1__[I] < BOTTOM)
			BOTTOM = (float)G__WALL_Y1__[I];

		OVER = BOTTOM - TOP;

		if (OVER * 2.0F < TALL)
			continue ;

		LINE->CUT_AT[LINE->CUTS] = (int)((X - (float)LINE->LEFT) / LINE->SCALE);
		++LINE->CUTS;
	}
}

/*
# THE DRAWN TABLE A LINE (ITS PATH FROM X0 TO X1) IS IN: THE ONE THAT HOLDS
# MOST OF 9 POINTS ALONG IT (A SPECK FAR AWAY MAY MAKE THE LINE LONGER).
# -1 = NONE.
*/
static int
	LINE_TABLE(const struct S_PATH *PATH, int X0, int X1)
{
	int	HITS[TABLES_MAX];
	int	BEST;
	int	K;

	memset(HITS, 0, sizeof(HITS));

	for (K = 1; K <= 9; ++K)
	{
		const float	X = (float)X0 + (float)(X1 - X0) * (float)K / 10.0F;
		const int	T = TABLE_AT(X, PATH_Y(PATH, X));

		if (T >= 0)
			++HITS[T];
	}

	BEST = -1;

	for (K = 0; K < G__TABLES__; ++K)
		if (HITS[K] && (BEST < 0 || HITS[K] > HITS[BEST]))
			BEST = K;

	return (BEST);
}

/*
# STEP 5: THE PIECES OF ONE PATH, MOVED UP OR DOWN SO THE LINE IS LEVEL,
# THEN RESIZED TO PAGE_HEIGHT. 1 = DONE.
*/
static int
	CUT_LINE(
		const float *INK,
		const int *LABEL,
		int W,
		const struct S_PIECE *P,
		int COUNT,
		const struct S_PATH *PATH,
		int NUMBER,
		int LETTER,
		struct S_LINE *LINE
	)
{
	float	*CANVAS;
	float	BASE;
	int		X0;
	int		X1;
	int		TOP;
	int		BOTTOM;
	int		PAD;
	int		CW;
	int		CH;
	int		NEW_W;
	int		I;

	X0 = W;
	X1 = 0;
	TOP = 1 << 30;
	BOTTOM = -(1 << 30);

	for (I = 0; I < COUNT; ++I)
	{
		if (P[I].PATH != NUMBER)
			continue ;

		if (P[I].X0 < X0)
			X0 = P[I].X0;

		if (P[I].X1 > X1)
			X1 = P[I].X1;
	}

	BASE = PATH_Y(PATH, (float)X0);

	for (I = 0; I < COUNT; ++I)
	{
		int	SHIFT;

		if (P[I].PATH != NUMBER)
			continue ;

		SHIFT = (int)floorf(PATH_Y(PATH, P[I].CX) - BASE + 0.5F);

		if (P[I].Y0 - SHIFT < TOP)
			TOP = P[I].Y0 - SHIFT;

		if (P[I].Y1 - SHIFT > BOTTOM)
			BOTTOM = P[I].Y1 - SHIFT;
	}

	PAD = (int)(0.15F * (float)LETTER);

	if (PAD < 2)
		PAD = 2;

	CW = X1 - X0 + 2 * PAD;
	CH = BOTTOM - TOP + 2 * PAD;
	CANVAS = (float *)calloc((size_t)CW * (size_t)CH, sizeof(float));

	if (!CANVAS)
		return (0);

	for (I = 0; I < COUNT; ++I)
	{
		int	SHIFT;
		int	Y;

		if (P[I].PATH != NUMBER)
			continue ;

		SHIFT = (int)floorf(PATH_Y(PATH, P[I].CX) - BASE + 0.5F);

		for (Y = P[I].Y0; Y < P[I].Y1; ++Y)
		{
			const int	CY = Y - SHIFT - TOP + PAD;
			int			X;

			for (X = P[I].X0; X < P[I].X1; ++X)
			{
				float	*DST;

				if (LABEL[Y * W + X] != P[I].LABEL)
					continue ;

				DST = &CANVAS[CY * CW + (X - X0 + PAD)];

				if (INK[Y * W + X] > *DST)
					*DST = INK[Y * W + X];
			}
		}
	}

	NEW_W = (int)floorf((float)CW * (float)PAGE_HEIGHT / (float)CH + 0.5F);

	if (NEW_W < 8)
		NEW_W = 8;

	LINE->INK = RESIZE(CANVAS, CW, CH, NEW_W, PAGE_HEIGHT);
	free(CANVAS);

	if (!LINE->INK)
		return (0);

	LINE->W = NEW_W;
	LINE->X0 = X0;
	LINE->X1 = X1;
	LINE->Y0 = TOP;
	LINE->Y1 = BOTTOM;
	LINE->LEFT = X0 - PAD;
	LINE->SCALE = (float)CH / (float)PAGE_HEIGHT;
	FIND_DASHES(P, COUNT, PATH, NUMBER, LETTER, LINE->LEFT, LINE->SCALE, LINE);
	FIND_CUTS(PATH, BASE, LINE);
	LINE->Y_LEFT = PATH_Y(PATH, (float)X0);
	LINE->Y_RIGHT = PATH_Y(PATH, (float)X1);
	LINE->TABLE = LINE_TABLE(PATH, X0, X1);
	return (1);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          LINES                           |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                        PUBLIC API                        |  \\//  */
/*   \/   #==========================================================#   \/   */
struct	S_ORDER
{
	float	Y;
	int		PATH;
};

static int
	CMP_ORDER(const void *A, const void *B)
{
	const struct S_ORDER	*OA = (const struct S_ORDER *)A;
	const struct S_ORDER	*OB = (const struct S_ORDER *)B;

	return ((OA->Y > OB->Y) - (OA->Y < OB->Y));
}

int
	PAGE_LINES(
		const unsigned char *GRAY,
		int WIDTH,
		int HEIGHT,
		struct S_LINE **LINES
	)
{
	float			*INK;
	float			*KEEP;
	int				*LABEL;
	struct S_PIECE	*P;
	struct S_PATH	*PATHS;
	struct S_ORDER	*ORDER;
	struct S_LINE	*TARGET;
	float			*PROFILE;
	float			*SMOOTHED;
	float			*DIFFS;
	int				*PEAKS;
	int				*PEAK_COUNT;
	int				*EDGES;
	int				*AREA;
	unsigned char	*CLEAN; /* THE PHOTO WITHOUT ITS NOISE, OR NULL */
	float			SPACING;
	float			NOISE;
	float			LEAN; /* RADIANS */
	int				COUNT;
	int				LETTER;
	int				LEFT;
	int				RIGHT;
	int				STRIPS;
	int				MOST;
	int				PATH_COUNT;
	int				DIFF_COUNT;
	int				LINE_COUNT;
	int				I;
	int				S;

	*LINES = NULL;
	G__TURN__ = 0.0F;
	G__FAN__ = 0;
	CLEAN = NULL;
	NOISE = NOISE_OF(GRAY, WIDTH, HEIGHT);

	/* A NOISY PHOTO (A DARK ROOM, A CHEAP CAMERA) IS SMOOTHED FIRST */
	if (NOISE > NOISE_LEAST)
	{
		CLEAN = MEDIAN_3(GRAY, WIDTH, HEIGHT);

		if (CLEAN)
			GRAY = CLEAN;
	}

	INK = INK_MAP(GRAY, WIDTH, HEIGHT, NOISE);

	if (!INK)
	{
		free(CLEAN);
		return (0);
	}

	/* A SOFT PHOTO IS MADE SHARP, MORE THE SOFTER IT IS (NOT A GRAINY ONE) */
	{
		const float	EDGE = EDGE_WIDTH(INK, WIDTH, HEIGHT);

		if (EDGE > SOFT_EDGE && NOISE <= NOISE_LEAST)
		{
			unsigned char	*SHARP;
			float			SIGMA;

			SIGMA = EDGE - 0.5F;

			if (SIGMA < 1.0F)
				SIGMA = 1.0F;

			if (SIGMA > 2.0F)
				SIGMA = 2.0F;

			SHARP = SHARPEN(GRAY, WIDTH, HEIGHT, SIGMA);

			if (SHARP)
			{
				float	*SHARP_INK;

				SHARP_INK = INK_MAP(SHARP, WIDTH, HEIGHT, NOISE);
				free(SHARP);

				if (SHARP_INK)
				{
					free(INK);
					INK = SHARP_INK;
				}
			}
		}
	}

	/* FROM HERE ON ONLY THE INK MAP IS USED */
	free(CLEAN);
	CLEAN = NULL;

	/*
	# A TURNED PHOTO IS TURNED BACK FIRST: ITS LINES LIE FLAT. A LITTLE TURN
	# IS LEFT AS IT IS (THE LINES FOLLOW IT); LEAN REMEMBERS IT.
	*/
	{
		const int	ALL[2] = {0, HEIGHT};
		const float	TURN = FIND_TURN(INK, WIDTH, HEIGHT, ALL);

		LEAN = TURN * 3.14159265F / 180.0F;

		if (FAN_PAGE(&INK, WIDTH, HEIGHT))
			LEAN = 0.0F;
		else if (fabsf(TURN) >= TURN_LEAST)
		{
			float	*TURNED;
			int		NW;
			int		NH;

			TURNED = TURN_INK(INK, WIDTH, HEIGHT, TURN, &NW, &NH);

			if (TURNED)
			{
				free(INK);
				INK = TURNED;
				WIDTH = NW;
				HEIGHT = NH;
				LEAN = 0.0F;
			}
		}
	}

	ERASE_DRAWN(INK, WIDTH, HEIGHT);
	ERASE_RULES(INK, WIDTH, HEIGHT);
	ERASE_BLOTS(INK, WIDTH, HEIGHT);
	COUNT = FIND_PIECES(INK, WIDTH, HEIGHT, &LABEL, &P);

	if (COUNT <= 0)
	{
		free(INK);
		return (0);
	}

	/* ONLY TEXT PIECES STAY; KEEP[] IS THEIR INK */
	KEEP = (float *)calloc((size_t)WIDTH * (size_t)HEIGHT, sizeof(float));

	if (!KEEP)
	{
		free(INK);
		free(LABEL);
		free(P);
		return (0);
	}

	S = 0;

	for (I = 0; I < COUNT; ++I)
		if (IS_TEXT_PIECE(&P[I], WIDTH, HEIGHT))
			P[S++] = P[I];

	COUNT = S;
	LETTER = LETTER_HEIGHT(P, COUNT);
	G__LETTER__ = LETTER;
	S = 0;

	/*
	# A PIECE TALLER THAN TALL_LETTERS LETTERS IS NOT IN A LINE OF TEXT (THE
	# DARK EDGE OF A TURNED PHOTO, A RULE GOING DOWN): IT WOULD MAKE ITS
	# LINE AS TALL AS ITSELF
	*/
	for (I = 0; I < COUNT; ++I)
		if (P[I].Y1 - P[I].Y0 <= TALL_LETTERS * LETTER)
			P[S++] = P[I];

	COUNT = S;
	LEFT = WIDTH;
	RIGHT = 0;

	for (I = 0; I < COUNT; ++I)
	{
		int	Y;

		for (Y = P[I].Y0; Y < P[I].Y1; ++Y)
		{
			int	X;

			for (X = P[I].X0; X < P[I].X1; ++X)
				if (LABEL[Y * WIDTH + X] == P[I].LABEL)
					KEEP[Y * WIDTH + X] = INK[Y * WIDTH + X];
		}

		if (P[I].X0 < LEFT)
			LEFT = P[I].X0;

		if (P[I].X1 > RIGHT)
			RIGHT = P[I].X1;
	}

	if (RIGHT <= LEFT)
	{
		free(INK);
		free(KEEP);
		free(LABEL);
		free(P);
		return (0);
	}

	STRIPS = (int)((float)(RIGHT - LEFT) / (float)(STRIP_LETTERS * LETTER));

	if (STRIPS < 3)
		STRIPS = 3;

	MOST = HEIGHT / 2 + 1;
	EDGES = (int *)malloc((size_t)(STRIPS + 1) * sizeof(int));
	PEAKS = (int *)malloc((size_t)STRIPS * (size_t)MOST * sizeof(int));
	PEAK_COUNT = (int *)calloc((size_t)STRIPS, sizeof(int));
	PROFILE = (float *)malloc((size_t)HEIGHT * sizeof(float));
	SMOOTHED = (float *)malloc((size_t)HEIGHT * sizeof(float));
	DIFFS = (float *)malloc(
		((size_t)STRIPS * (size_t)MOST + 1) * sizeof(float)
	);
	PATH_COUNT = -1;
	LINE_COUNT = 0;
	TARGET = NULL;

	if (EDGES && PEAKS && PEAK_COUNT && PROFILE && SMOOTHED && DIFFS)
	{
		for (S = 0; S <= STRIPS; ++S)
			EDGES[S] = LEFT + (int)((double)(RIGHT - LEFT) * S / STRIPS);

		DIFF_COUNT = 0;

		for (S = 0; S < STRIPS; ++S)
		{
			int	*HERE = PEAKS + S * MOST;

			PEAK_COUNT[S] = STRIP_PEAKS(
				KEEP,
				WIDTH,
				HEIGHT,
				EDGES[S],
				EDGES[S + 1],
				LETTER,
				PROFILE,
				SMOOTHED,
				HERE
			);

			for (I = 1; I < PEAK_COUNT[S]; ++I)
				DIFFS[DIFF_COUNT++] = (float)(HERE[I] - HERE[I - 1]);
		}

		SPACING = 2.0F * (float)LETTER;

		if (DIFF_COUNT)
			SPACING = MEDIAN(DIFFS, DIFF_COUNT);

		PATH_COUNT = MAKE_PATHS(
			PEAKS,
			PEAK_COUNT,
			STRIPS,
			MOST,
			EDGES,
			SPACING,
			0.0F,
			&PATHS
		);
	}

	if (PATH_COUNT > 0)
	{
		const float	STRIP_W = (float)(EDGES[1] - EDGES[0]);

		PIECES_TO_PATHS(P, COUNT, PATHS, PATH_COUNT, STRIP_W, SPACING, 0);
		JOIN_PATHS(P, COUNT, PATHS, PATH_COUNT, SPACING, LETTER);
		PIECES_TO_PATHS(P, COUNT, PATHS, PATH_COUNT, STRIP_W, SPACING, 1);

		ORDER = (struct S_ORDER *)malloc((size_t)PATH_COUNT * sizeof(*ORDER));
		AREA = (int *)calloc((size_t)PATH_COUNT, sizeof(int));
		TARGET = (struct S_LINE *)calloc((size_t)PATH_COUNT, sizeof(*TARGET));

		if (ORDER && AREA && TARGET)
		{
			for (I = 0; I < COUNT; ++I)
				if (P[I].PATH >= 0)
					AREA[P[I].PATH] += P[I].AREA;

			/*
			# FROM THE TOP: THE MIDDLE OF EVERY LINE, WITH THE LEAN OF THE
			# PAGE (A SHORT LINE AT THE SIDE HAS NO LEAN OF ITS OWN)
			*/
			for (I = 0; I < PATH_COUNT; ++I)
			{
				const struct S_PATH	*Q = &PATHS[I];
				const float			XM = (Q->X[0] + Q->X[Q->COUNT - 1]) / 2.0F;
				const float			DX = XM - (float)WIDTH / 2.0F;
				const float			DY = PATH_Y(Q, XM) - (float)HEIGHT / 2.0F;

				ORDER[I].Y = DX * sinf(LEAN) + DY * cosf(LEAN);
				ORDER[I].PATH = I;
			}

			qsort(ORDER, (size_t)PATH_COUNT, sizeof(*ORDER), CMP_ORDER);

			for (I = 0; I < PATH_COUNT; ++I)
			{
				const int	K = ORDER[I].PATH;

				if (AREA[K] < 3 * LETTER || ONLY_FLAT(P, COUNT, K, LETTER))
					continue ;

				if (
					CUT_LINE(
						INK,
						LABEL,
						WIDTH,
						P,
						COUNT,
						&PATHS[K],
						K,
						LETTER,
						&TARGET[LINE_COUNT]
					)
				)
					++LINE_COUNT;
			}
		}

		free(ORDER);
		free(AREA);

		for (I = 0; I < PATH_COUNT; ++I)
		{
			free(PATHS[I].X);
			free(PATHS[I].Y);
		}

		free(PATHS);
	}

	free(EDGES);
	free(PEAKS);
	free(PEAK_COUNT);
	free(PROFILE);
	free(SMOOTHED);
	free(DIFFS);
	free(INK);
	free(KEEP);
	free(LABEL);
	free(P);

	if (!LINE_COUNT)
	{
		free(TARGET);
		return (0);
	}

	*LINES = TARGET;
	return (LINE_COUNT);
}

void
	PAGE_FREE(struct S_LINE *LINES, int COUNT)
{
	int	I;

	for (I = 0; I < COUNT; ++I)
		free(LINES[I].INK);

	free(LINES);
}

int
	PAGE_LETTER_HEIGHT(void)
{
	return (G__LETTER__);
}

void
	PAGE_TURN(float *X, float *Y)
{
	const float	C = cosf(G__TURN__);
	const float	S = sinf(G__TURN__);
	const float	DX = *X - G__FROM_X__;
	const float	DY = *Y - G__FROM_Y__;

	/* A FANNED PAGE: THE POINT MOVES UP OR DOWN ALONG ITS LEAN */
	if (G__FAN__)
	{
		*Y -= (*X - G__FAN_X__) * FAN_SLOPE(*Y);
		return ;
	}

	if (G__TURN__ == 0.0F)
		return ;

	*X = DX * C - DY * S + G__TO_X__;
	*Y = DX * S + DY * C + G__TO_Y__;
}

int
	PAGE_RULES(const float **ENDS)
{
	*ENDS = &G__RULE_END__[0][0];
	return (G__RULES__);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                        PUBLIC API                        |  //\\  */
/* '""""' #==========================================================# '""""' */
