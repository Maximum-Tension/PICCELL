/* HAND_JUDGE WITH MADE-UP OCR WORDS: ONE "WORD" PER LINE FROM A TEXT FILE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hand.h"
#include "layout.h"
#include "page.h"
#include "reader.h"
#include "piccell.h"

static unsigned char
	*READ_FILE(const char *PATH, size_t *SIZE)
{
	FILE			*F;
	unsigned char	*D;

	F = fopen(PATH, "rb");

	if (!F)
		return (NULL);

	fseek(F, 0, SEEK_END);
	*SIZE = (size_t)ftell(F);
	fseek(F, 0, SEEK_SET);
	D = malloc(*SIZE + 1);
	D[fread(D, 1, *SIZE, F)] = 0;
	fclose(F);
	return (D);
}

int
	main(int ARGC, char **ARGV)
{
	struct S_READER	*R;
	struct S_HAND	*H;
	struct S_LINE	*L;
	struct S_WORD	W[64];
	unsigned char	*BLOB;
	unsigned char	*PGM;
	char			*TEXT;
	char			*LINE;
	size_t			SIZE;
	int				WIDTH;
	int				HEIGHT;
	int				MAX;
	int				OFF;
	int				N;
	int				I;
	int				MAP[64];

	BLOB = READ_FILE(ARGV[1], &SIZE);
	R = READER_LOAD(BLOB, SIZE);
	PGM = READ_FILE(ARGV[2], &SIZE);
	TEXT = (char *)READ_FILE(ARGV[3], &SIZE);
	sscanf((char *)PGM, "P5 %d %d %d%n", &WIDTH, &HEIGHT, &MAX, &OFF);
	HAND_SET_READER(R);
	H = HAND_OPEN(PGM + OFF + 1, WIDTH, HEIGHT);
	N = PAGE_LINES(PGM + OFF + 1, WIDTH, HEIGHT, &L);

	/* LINE K OF THE TEXT GOES ON PAGE LINE MAP[K] (ARGV[4]: "0,1,...") */
	for (I = 0; I < 64; ++I)
		MAP[I] = I;

	if (ARGC > 4)
	{
		char	*P = ARGV[4];

		for (I = 0; *P && I < 64; ++I)
		{
			MAP[I] = (int)strtol(P, &P, 10);

			if (*P == ',')
				++P;
		}
	}

	I = 0;

	for (LINE = strtok(TEXT, "\r\n"); LINE && I < 64; LINE = strtok(NULL, "\r\n"))
	{
		const struct S_LINE	*X = &L[MAP[I] < N ? MAP[I] : N - 1];

		W[I].LINE = I;
		W[I].X = (float)X->X0;
		W[I].W = (float)(X->X1 - X->X0);
		W[I].Y = (float)X->Y0;
		W[I].H = (float)(X->Y1 - X->Y0);
		snprintf(W[I].TEXT, sizeof(W[I].TEXT), "%s", LINE);
		++I;
	}

	printf("judge %.3f\n", HAND_JUDGE(H, W, I));
	HAND_CLOSE(H);
	return (0);
}
