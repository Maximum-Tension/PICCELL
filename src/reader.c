/******************************************************************************\
# C - reader                                     #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/19 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/19 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

/*
# THE NETWORK RUNS STEP BY STEP ON A STACK OF PICTURES (CHANNELS X ROWS X
# COLUMNS). A CONVOLUTION ADDS EVERY WEIGHT TIMES A WHOLE ROW AT ONCE, SO THE
# INNER LOOP WALKS MEMORY IN ORDER AND THE COMPILER CAN USE SSE. READER_RUN
# CHANGES NOTHING IN THE READER: SEVERAL THREADS MAY READ LINES AT THE SAME
# TIME.
*/

/* **************************** [v] INCLUDES [v] **************************** */
#include <math.h>
/******* """"""""
 * double	exp(double);
 * double	log(double);
 */
#include <stdlib.h>
/******* """"""""""
 * void	*calloc(size_t, size_t);
 * void	free(void *);
 * void	*malloc(size_t);
 */
#include <string.h>
/******* """"""""""
 * #define NULL
 * typedef	size_t;
 * int		memcmp(const void *, const void *, size_t);
 * void		*memcpy(void *, const void *, size_t);
 * void		*memset(void *, int, size_t);
 */
#include "page.h"
/******* """"""""
 * #define PAGE_HEIGHT
 */
#include "reader.h"
/******* """"""""""
 * #define READER_CHARS_MAX
 * struct S_READER;
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define STEP_CONV 1
#define STEP_POOL 2
#define STEP_KEEP 3
#define STEP_ADD 4
#define KEEP_SLOTS 4

struct	S_STEP
{
	int		TYPE; /* STEP_CONV, STEP_POOL, STEP_KEEP OR STEP_ADD */
	int		INPUTS;
	int		OUTPUTS;
	int		KH; /* KERNEL HEIGHT AND WIDTH; THE POOL SIZE FOR STEP_POOL */
	int		KW;
	int		PH; /* ZEROS ADDED ABOVE AND BELOW, LEFT AND RIGHT */
	int		PW;
	int		RELU;
	int		SLOT; /* STEP_KEEP AND STEP_ADD */
	float	*WEIGHT; /* OUTPUTS X INPUTS X KH X KW */
	float	*BIAS; /* OUTPUTS */
};

struct	S_READER
{
	int				CLASSES;
	int				HEIGHT;
	char			CHARS[READER_CHARS_MAX]; /* ALL CHARACTERS, UTF-8 */
	int				CHAR_AT[READER_CHARS_MAX]; /* WHERE CLASS K STARTS */
	char			ONE[READER_CHARS_MAX][8]; /* CLASS K ALONE, ENDING WITH 0 */
	int				COUNT;
	struct S_STEP	*STEPS;
};

struct	S_TENSOR
{
	int		C;
	int		H;
	int		W;
	float	*DATA;
};

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                         LOADING                          |  \\//  */
/*   \/   #==========================================================#   \/   */
struct	S_BYTES
{
	const unsigned char	*AT;
	const unsigned char	*END;
	int					OK;
};

static int
	READ_INT(struct S_BYTES *B)
{
	int	V;

	if (B->END - B->AT < 4)
	{
		B->OK = 0;
		return (0);
	}

	V = (int)(
		(unsigned int)B->AT[0] |
		((unsigned int)B->AT[1] << 8) |
		((unsigned int)B->AT[2] << 16) |
		((unsigned int)B->AT[3] << 24)
	);
	B->AT += 4;
	return (V);
}

static float
	READ_FLOAT(struct S_BYTES *B)
{
	float			V;
	unsigned int	U;

	U = (unsigned int)READ_INT(B);
	memcpy(&V, &U, 4);
	return (V);
}

/* ONE CONVOLUTION: INT8 WEIGHTS TIMES THEIR SCALE BECOME FLOATS. 1 = OK */
static int
	READ_CONV(struct S_BYTES *B, struct S_STEP *S)
{
	size_t	PER;
	size_t	I;
	int		O;
	float	*SCALE;

	S->INPUTS = READ_INT(B);
	S->OUTPUTS = READ_INT(B);
	S->KH = READ_INT(B);
	S->KW = READ_INT(B);
	S->PH = READ_INT(B);
	S->PW = READ_INT(B);
	S->RELU = READ_INT(B);

	if (
		!B->OK ||
		S->INPUTS <= 0 || S->INPUTS > 4096 ||
		S->OUTPUTS <= 0 || S->OUTPUTS > 4096 ||
		S->KH <= 0 || S->KH > 15 ||
		S->KW <= 0 || S->KW > 15
	)
		return (0);

	PER = (size_t)S->INPUTS * (size_t)S->KH * (size_t)S->KW;

	if ((size_t)(B->END - B->AT) < (size_t)S->OUTPUTS * (PER + 8))
		return (0);

	SCALE = (float *)malloc((size_t)S->OUTPUTS * sizeof(float));
	S->WEIGHT = (float *)malloc((size_t)S->OUTPUTS * PER * sizeof(float));
	S->BIAS = (float *)malloc((size_t)S->OUTPUTS * sizeof(float));

	if (!SCALE || !S->WEIGHT || !S->BIAS)
	{
		free(SCALE);
		return (0);
	}

	for (O = 0; O < S->OUTPUTS; ++O)
		SCALE[O] = READ_FLOAT(B);

	for (O = 0; O < S->OUTPUTS; ++O)
		for (I = 0; I < PER; ++I)
			S->WEIGHT[(size_t)O * PER + I] = SCALE[O] * (signed char)*B->AT++;

	for (O = 0; O < S->OUTPUTS; ++O)
		S->BIAS[O] = READ_FLOAT(B);

	free(SCALE);
	return (B->OK);
}

/* THE CHARACTER LIST: CLASS K (FROM 1) IS THE K-TH UTF-8 CHARACTER. 1 = OK */
static int
	READ_CHARS(struct S_BYTES *B, struct S_READER *R)
{
	const int	LEN = READ_INT(B);
	int			I;
	int			K;

	if (!B->OK || LEN <= 0 || LEN >= READER_CHARS_MAX || B->END - B->AT < LEN)
		return (0);

	memcpy(R->CHARS, B->AT, (size_t)LEN);
	R->CHARS[LEN] = '\0';
	B->AT += LEN;
	K = 1;
	I = 0;

	while (I < LEN && K < READER_CHARS_MAX)
	{
		int	N;

		N = 1;

		/* THE LENGTH OF A UTF-8 CHARACTER FROM ITS FIRST BYTE */
		if (((unsigned char)R->CHARS[I] & 0XE0) == 0XC0)
			N = 2;
		else if (((unsigned char)R->CHARS[I] & 0XF0) == 0XE0)
			N = 3;
		else if (((unsigned char)R->CHARS[I] & 0XF8) == 0XF0)
			N = 4;

		R->CHAR_AT[K] = I;
		memcpy(R->ONE[K], R->CHARS + I, (size_t)N);
		R->ONE[K][N] = '\0';
		I += N;
		++K;
	}

	return (K == R->CLASSES);
}

struct S_READER
	*READER_LOAD(const void *DATA, size_t SIZE)
{
	struct S_BYTES	B;
	struct S_READER	*R;
	int				I;

	B.AT = (const unsigned char *)DATA;
	B.END = B.AT + SIZE;
	B.OK = 1;

	if (SIZE < 16 || memcmp(DATA, "TRD1", 4) != 0)
		return (NULL);

	B.AT += 4;
	R = (struct S_READER *)calloc(1, sizeof(*R));

	if (!R)
		return (NULL);

	R->CLASSES = READ_INT(&B);
	R->HEIGHT = READ_INT(&B);

	if (
		R->CLASSES < 2 ||
		R->CLASSES >= READER_CHARS_MAX ||
		R->HEIGHT != PAGE_HEIGHT ||
		!READ_CHARS(&B, R)
	)
	{
		free(R);
		return (NULL);
	}

	R->COUNT = READ_INT(&B);

	if (!B.OK || R->COUNT <= 0 || R->COUNT > 256)
	{
		free(R);
		return (NULL);
	}

	R->STEPS = (struct S_STEP *)calloc((size_t)R->COUNT, sizeof(*R->STEPS));

	if (!R->STEPS)
	{
		free(R);
		return (NULL);
	}

	for (I = 0; I < R->COUNT && B.OK; ++I)
	{
		struct S_STEP	*S = &R->STEPS[I];

		S->TYPE = READ_INT(&B);

		if (S->TYPE == STEP_CONV)
			B.OK = READ_CONV(&B, S);
		else if (S->TYPE == STEP_POOL)
		{
			S->KH = READ_INT(&B);
			S->KW = READ_INT(&B);
			B.OK = B.OK && S->KH > 0 && S->KW > 0;
		}
		else if (S->TYPE == STEP_KEEP || S->TYPE == STEP_ADD)
		{
			S->SLOT = READ_INT(&B);
			B.OK = B.OK && S->SLOT >= 0 && S->SLOT < KEEP_SLOTS;
		}
		else
			B.OK = 0;
	}

	if (!B.OK)
	{
		READER_FREE(R);
		return (NULL);
	}

	return (R);
}

void
	READER_FREE(struct S_READER *R)
{
	int	I;

	if (!R)
		return ;

	for (I = 0; I < R->COUNT; ++I)
	{
		free(R->STEPS[I].WEIGHT);
		free(R->STEPS[I].BIAS);
	}

	free(R->STEPS);
	free(R);
}

int
	READER_CLASSES(const struct S_READER *R)
{
	return (R->CLASSES);
}

const char
	*READER_CHAR(const struct S_READER *R, int K)
{
	if (K < 1 || K >= R->CLASSES)
		return ("");

	return (R->ONE[K]);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                         LOADING                          |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                         RUNNING                          |  \\//  */
/*   \/   #==========================================================#   \/   */
/* OUTPUT = CONVOLUTION OF INPUT (STRIDE 1). 1 = OK */
static int
	CONV(
		const struct S_STEP *S,
		const struct S_TENSOR *INPUT,
		struct S_TENSOR *OUTPUT
	)
{
	const int	OH = INPUT->H + 2 * S->PH - S->KH + 1;
	const int	OW = INPUT->W + 2 * S->PW - S->KW + 1;
	int			O;

	if (INPUT->C != S->INPUTS || OH <= 0 || OW <= 0)
		return (0);

	OUTPUT->C = S->OUTPUTS;
	OUTPUT->H = OH;
	OUTPUT->W = OW;
	OUTPUT->DATA = (float *)malloc(sizeof(float) * S->OUTPUTS * OH * OW);

	if (!OUTPUT->DATA)
		return (0);

	for (O = 0; O < S->OUTPUTS; ++O)
	{
		float		*DST = OUTPUT->DATA + (size_t)O * OH * OW;
		const float	*W = S->WEIGHT + (size_t)O * S->INPUTS * S->KH * S->KW;
		int			I;
		int			K;

		for (K = 0; K < OH * OW; ++K)
			DST[K] = S->BIAS[O];

		for (I = 0; I < S->INPUTS; ++I)
		{
			const float	*SRC = INPUT->DATA + (size_t)I * INPUT->H * INPUT->W;
			int			KY;

			for (KY = 0; KY < S->KH; ++KY)
			{
				int	KX;

				for (KX = 0; KX < S->KW; ++KX)
				{
					const float	WV = W[(I * S->KH + KY) * S->KW + KX];
					const int	SHIFT = KX - S->PW;
					int			X0;
					int			X1;
					int			Y;

					if (WV == 0.0F)
						continue ;

					/* COLUMNS X WHERE X + SHIFT IS INSIDE THE INPUT */
					X0 = -SHIFT;
					X1 = INPUT->W - SHIFT;

					if (X0 < 0)
						X0 = 0;

					if (X1 > OW)
						X1 = OW;

					for (Y = 0; Y < OH; ++Y)
					{
						const int	SY = Y + KY - S->PH;
						const float	*ROW;
						float		*OUT_ROW;
						int			X;

						if (SY < 0 || SY >= INPUT->H)
							continue ;

						ROW = SRC + (size_t)SY * INPUT->W + SHIFT;
						OUT_ROW = DST + (size_t)Y * OW;

						for (X = X0; X < X1; ++X)
							OUT_ROW[X] += WV * ROW[X];
					}
				}
			}
		}

		if (S->RELU)
			for (K = 0; K < OH * OW; ++K)
				if (DST[K] < 0.0F)
					DST[K] = 0.0F;
	}

	return (1);
}

/* OUTPUT = THE LARGEST VALUE OF EVERY KH X KW BLOCK OF INPUT. 1 = OK */
static int
	POOL(
		const struct S_STEP *S,
		const struct S_TENSOR *INPUT,
		struct S_TENSOR *OUTPUT
	)
{
	const int	OH = INPUT->H / S->KH;
	const int	OW = INPUT->W / S->KW;
	int			C;

	if (OH <= 0 || OW <= 0)
		return (0);

	OUTPUT->C = INPUT->C;
	OUTPUT->H = OH;
	OUTPUT->W = OW;
	OUTPUT->DATA = (float *)malloc(sizeof(float) * INPUT->C * OH * OW);

	if (!OUTPUT->DATA)
		return (0);

	for (C = 0; C < INPUT->C; ++C)
	{
		int	Y;

		for (Y = 0; Y < OH; ++Y)
		{
			int	X;

			for (X = 0; X < OW; ++X)
			{
				float	BEST;
				int		DY;

				BEST = -1E30F;

				for (DY = 0; DY < S->KH; ++DY)
				{
					const float	*ROW = INPUT->DATA + X * S->KW;
					int			DX;

					ROW += ((size_t)C * INPUT->H + Y * S->KH + DY) * INPUT->W;

					for (DX = 0; DX < S->KW; ++DX)
						if (ROW[DX] > BEST)
							BEST = ROW[DX];
				}

				OUTPUT->DATA[((size_t)C * OH + Y) * OW + X] = BEST;
			}
		}
	}

	return (1);
}

float
	*READER_RUN(const struct S_READER *R, const float *INK, int W, int *STEPS)
{
	struct S_TENSOR	CUR;
	struct S_TENSOR	KEPT[KEEP_SLOTS];
	float			*OUTPUT;
	int				PADDED;
	int				OK;
	int				I;

	*STEPS = 0;
	PADDED = (W + 3) / 4 * 4;
	memset(KEPT, 0, sizeof(KEPT));
	CUR.C = 1;
	CUR.H = R->HEIGHT;
	CUR.W = PADDED;
	CUR.DATA = (float *)calloc((size_t)R->HEIGHT * PADDED, sizeof(float));

	if (!CUR.DATA)
		return (NULL);

	for (I = 0; I < R->HEIGHT; ++I)
		memcpy(
			CUR.DATA + (size_t)I * PADDED,
			INK + (size_t)I * W,
			(size_t)W * sizeof(float)
		);

	OK = 1;

	for (I = 0; I < R->COUNT && OK; ++I)
	{
		const struct S_STEP	*S = &R->STEPS[I];
		struct S_TENSOR		NEXT;

		if (S->TYPE == STEP_KEEP)
		{
			const size_t	BYTES = sizeof(float) * CUR.C * CUR.H * CUR.W;

			free(KEPT[S->SLOT].DATA);
			KEPT[S->SLOT] = CUR;
			KEPT[S->SLOT].DATA = (float *)malloc(BYTES);
			OK = KEPT[S->SLOT].DATA != NULL;

			if (OK)
				memcpy(KEPT[S->SLOT].DATA, CUR.DATA, BYTES);

			continue ;
		}

		if (S->TYPE == STEP_ADD)
		{
			const struct S_TENSOR	*K = &KEPT[S->SLOT];
			size_t					N;

			OK = K->DATA && K->C == CUR.C && K->H == CUR.H && K->W == CUR.W;

			for (N = 0; OK && N < (size_t)CUR.C * CUR.H * CUR.W; ++N)
				CUR.DATA[N] += K->DATA[N];

			continue ;
		}

		if (S->TYPE == STEP_CONV)
			OK = CONV(S, &CUR, &NEXT);
		else
			OK = POOL(S, &CUR, &NEXT);

		free(CUR.DATA);
		CUR = NEXT;

		if (!OK)
			CUR.DATA = NULL;
	}

	for (I = 0; I < KEEP_SLOTS; ++I)
		free(KEPT[I].DATA);

	if (!OK || CUR.H != 1 || CUR.C != R->CLASSES)
	{
		free(CUR.DATA);
		return (NULL);
	}

	/* CLASSES X STEPS TO STEPS X CLASSES, AS LOG PROBABILITIES */
	OUTPUT = (float *)malloc((size_t)CUR.W * CUR.C * sizeof(float));

	if (OUTPUT)
	{
		int	T;

		for (T = 0; T < CUR.W; ++T)
		{
			float	*ROW = OUTPUT + (size_t)T * CUR.C;
			float	TOP;
			double	SUM;
			int		K;

			TOP = -1E30F;

			for (K = 0; K < CUR.C; ++K)
			{
				ROW[K] = CUR.DATA[(size_t)K * CUR.W + T];

				if (ROW[K] > TOP)
					TOP = ROW[K];
			}

			SUM = 0.0;

			for (K = 0; K < CUR.C; ++K)
				SUM += exp((double)(ROW[K] - TOP));

			for (K = 0; K < CUR.C; ++K)
				ROW[K] = ROW[K] - TOP - (float)log(SUM);
		}

		*STEPS = CUR.W;
	}

	free(CUR.DATA);
	return (OUTPUT);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                         RUNNING                          |  //\\  */
/* '""""' #==========================================================# '""""' */
