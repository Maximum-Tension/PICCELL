/******************************************************************************\
# C - ctc                                        #       Maximum Tension       #
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

/* **************************** [v] INCLUDES [v] **************************** */
#include <math.h>
/******* """"""""
 * double	exp(double);
 * double	log1p(double);
 */
#include <stdlib.h>
/******* """"""""""
 * void	free(void *);
 * void	*malloc(size_t);
 */
#include <string.h>
/******* """"""""""
 * typedef	size_t;
 */
#include "ctc.h"
/******* """""""
 * #define CTC_NODES_MAX
 * struct S_ARC;
 * struct S_GLYPH;
 * struct S_GRAMMAR;
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define NEVER -1E30

/* LOG(EXP(A) + EXP(B)) WITHOUT LEAVING THE RANGE OF A DOUBLE */
static double
	LOG_ADD(double A, double B)
{
	if (A < B)
	{
		const double	T = A;

		A = B;
		B = T;
	}

	if (B <= NEVER / 2)
		return (A);

	return (A + log1p(exp(B - A)));
}

int
	CTC_GREEDY(
		const float *LOGP,
		int CLASSES,
		int FROM,
		int TO,
		const unsigned char *ALLOWED,
		struct S_GLYPH *GLYPHS,
		int MOST
	)
{
	int	COUNT;
	int	PREV;
	int	T;

	COUNT = 0;
	PREV = 0;

	for (T = FROM; T < TO; ++T)
	{
		const float	*ROW = LOGP + (size_t)T * CLASSES;
		int			BEST;
		int			K;

		BEST = 0;

		for (K = 1; K < CLASSES; ++K)
			if ((!ALLOWED || ALLOWED[K]) && ROW[K] > ROW[BEST])
				BEST = K;

		if (BEST && BEST == PREV && COUNT)
			GLYPHS[COUNT - 1].LAST = T;
		else if (BEST && COUNT < MOST)
		{
			GLYPHS[COUNT].CLASS = BEST;
			GLYPHS[COUNT].FIRST = T;
			GLYPHS[COUNT].LAST = T;
			++COUNT;
		}

		PREV = BEST;
	}

	return (COUNT);
}

/*
# THE FORWARD SUMS OF CTC: THE LABELS WITH A "NOTHING" BETWEEN AND AROUND
# THEM (2 * COUNT + 1 STATES). A STATE GOES ON TO ITSELF, TO THE NEXT ONE, OR
# OVER A "NOTHING" TO THE NEXT LABEL WHEN THAT LABEL IS A DIFFERENT ONE.
*/
double
	CTC_SCORE(
		const float *LOGP,
		int CLASSES,
		int FROM,
		int TO,
		const int *LABELS,
		int COUNT
	)
{
	const int	STATES = 2 * COUNT + 1;
	double		*A;
	double		*B;
	double		RESULT;
	int			T;
	int			S;

	if (TO <= FROM && COUNT)
		return (NEVER);

	if (TO <= FROM)
		return (0.0);

	A = (double *)malloc((size_t)STATES * sizeof(double));
	B = (double *)malloc((size_t)STATES * sizeof(double));

	if (!A || !B)
	{
		free(A);
		free(B);
		return (NEVER);
	}

	for (S = 0; S < STATES; ++S)
		A[S] = NEVER;

	A[0] = LOGP[(size_t)FROM * CLASSES];

	if (COUNT)
		A[1] = LOGP[(size_t)FROM * CLASSES + LABELS[0]];

	for (T = FROM + 1; T < TO; ++T)
	{
		const float	*ROW = LOGP + (size_t)T * CLASSES;
		double		*SWAP;

		for (S = 0; S < STATES; ++S)
		{
			double	V;
			int		K;

			V = A[S];

			if (S >= 1)
				V = LOG_ADD(V, A[S - 1]);

			if (S >= 2 && S % 2 && LABELS[S / 2] != LABELS[S / 2 - 1])
				V = LOG_ADD(V, A[S - 2]);

			K = 0;

			if (S % 2)
				K = LABELS[S / 2];

			B[S] = V + ROW[K];
		}

		SWAP = A;
		A = B;
		B = SWAP;
	}

	RESULT = A[STATES - 1];

	if (COUNT)
		RESULT = LOG_ADD(RESULT, A[STATES - 2]);

	free(A);
	free(B);
	return (RESULT);
}

/*
# THE BEST WAY THROUGH THE LINE THAT THE GRAMMAR ALLOWS. V[N * CLASSES + L]:
# THE BEST SCORE OF BEING AT NODE N WITH CLASS L SEEN LAST (0 = A "NOTHING"
# OR THE START). AT EVERY STEP A STATE SEES A "NOTHING", SEES ITS CLASS AGAIN
# (NO NEW GLYPH), OR TAKES AN ARC AND SEES A NEW GLYPH; THE SAME CLASS TWICE
# IN A ROW NEEDS A "NOTHING" BETWEEN. SO AN ARC TAKES THE BEST SCORE OF ITS
# NODE, OR THE SECOND BEST WHEN THE BEST ONE SAW THE SAME CLASS.
*/
double
	CTC_GRAMMAR(
		const float *LOGP,
		int CLASSES,
		int FROM,
		int TO,
		const struct S_GRAMMAR *G,
		struct S_GLYPH *GLYPHS,
		int *TAGS,
		int MOST,
		int *COUNT
	)
{
	const int	STATES = G->NODES * CLASSES;
	const int	STEPS = TO - FROM;
	double		*V;
	double		*NV;
	int			*BACK;
	int			*PATH;
	double		BEST;
	int			STATE;
	int			T;
	int			S;

	*COUNT = 0;

	if (STEPS <= 0 || G->NODES <= 0)
		return (NEVER);

	V = (double *)malloc((size_t)STATES * sizeof(double));
	NV = (double *)malloc((size_t)STATES * sizeof(double));
	BACK = (int *)malloc((size_t)STEPS * STATES * sizeof(int));
	PATH = (int *)malloc((size_t)STEPS * sizeof(int));

	if (!V || !NV || !BACK || !PATH)
	{
		free(V);
		free(NV);
		free(BACK);
		free(PATH);
		return (NEVER);
	}

	for (S = 0; S < STATES; ++S)
		V[S] = NEVER;

	V[0] = 0.0; /* NODE 0, NOTHING SEEN YET */

	for (T = 0; T < STEPS; ++T)
	{
		const float	*ROW = LOGP + (size_t)(FROM + T) * CLASSES;
		int			*BP = BACK + (size_t)T * STATES;
		double		*SWAP;
		double		B1[CTC_NODES_MAX];
		double		B2[CTC_NODES_MAX];
		int			A1[CTC_NODES_MAX];
		int			A2[CTC_NODES_MAX];
		int			N;
		int			A;

		for (N = 0; N < G->NODES; ++N)
		{
			int	L;

			B1[N] = NEVER;
			B2[N] = NEVER;
			A1[N] = 0;
			A2[N] = 0;

			for (L = 0; L < CLASSES; ++L)
			{
				const double	X = V[N * CLASSES + L];

				if (X > B1[N])
				{
					B2[N] = B1[N];
					A2[N] = A1[N];
					B1[N] = X;
					A1[N] = L;
				}
				else if (X > B2[N])
				{
					B2[N] = X;
					A2[N] = L;
				}
			}
		}

		for (S = 0; S < STATES; ++S)
		{
			NV[S] = NEVER;
			BP[S] = 0;
		}

		/* A "NOTHING", OR THE SAME CLASS AGAIN */
		for (N = 0; N < G->NODES; ++N)
		{
			int	K;

			if (B1[N] <= NEVER / 2)
				continue ;

			NV[N * CLASSES] = B1[N] + ROW[0];
			BP[N * CLASSES] = N * CLASSES + A1[N];

			for (K = 1; K < CLASSES; ++K)
			{
				const double	X = V[N * CLASSES + K];

				if (X > NEVER / 2)
				{
					NV[N * CLASSES + K] = X + ROW[K];
					BP[N * CLASSES + K] = N * CLASSES + K;
				}
			}
		}

		/* A NEW GLYPH OVER AN ARC (THE BACK STEP IS STORED AS -(STATE + 1)) */
		for (A = 0; A < G->ARCS; ++A)
		{
			const struct S_ARC	*ARC = &G->ARC[A];
			int					K;

			if (B1[ARC->FROM] <= NEVER / 2)
				continue ;

			for (K = 1; K < CLASSES; ++K)
			{
				double	X;
				int		L;

				if (ARC->ALLOWED && !ARC->ALLOWED[K])
					continue ;

				X = B1[ARC->FROM];
				L = A1[ARC->FROM];

				if (L == K)
				{
					X = B2[ARC->FROM];
					L = A2[ARC->FROM];
				}

				if (X <= NEVER / 2)
					continue ;

				X += ROW[K] + ARC->COST;

				if (X > NV[ARC->TO * CLASSES + K])
				{
					NV[ARC->TO * CLASSES + K] = X;
					BP[ARC->TO * CLASSES + K] = -(ARC->FROM * CLASSES + L + 1);
				}
			}
		}

		SWAP = V;
		V = NV;
		NV = SWAP;
	}

	BEST = NEVER;
	STATE = -1;

	for (S = 0; S < STATES; ++S)
	{
		if (G->FINAL[S / CLASSES] && V[S] > BEST)
		{
			BEST = V[S];
			STATE = S;
		}
	}

	/* BACK FROM THE END: THE STATE AT EVERY STEP, THEN THE GLYPHS */
	for (T = STEPS - 1; T >= 0 && STATE >= 0; --T)
	{
		const int	BP = BACK[(size_t)T * STATES + STATE];

		PATH[T] = STATE;

		if (BP < 0)
		{
			PATH[T] = -(STATE + 1); /* A NEW GLYPH STARTS HERE */
			STATE = -BP - 1;
		}
		else
			STATE = BP;
	}

	for (T = 0; T < STEPS && BEST > NEVER / 2; ++T)
	{
		int	END;

		if (PATH[T] >= 0 || *COUNT >= MOST)
			continue ;

		STATE = -PATH[T] - 1;

		for (END = T; END + 1 < STEPS && PATH[END + 1] == STATE; ++END)
		{}

		GLYPHS[*COUNT].CLASS = STATE % CLASSES;
		GLYPHS[*COUNT].FIRST = FROM + T;
		GLYPHS[*COUNT].LAST = FROM + END;

		if (TAGS)
			TAGS[*COUNT] = G->TAG[STATE / CLASSES];

		++*COUNT;
	}

	free(V);
	free(NV);
	free(BACK);
	free(PATH);
	return (BEST);
}
