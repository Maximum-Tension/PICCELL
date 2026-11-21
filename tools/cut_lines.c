/* THE LINES OF PGM PAGES, CUT EXACTLY AS PICCELL CUTS THEM, FOR TRAINING */
/*
# CUT_LINES PAGE.PGM ...: WRITES PAGE.PGM.LINES FOR EVERY PAGE. INT32 COUNT,
# THEN FOR EVERY LINE INT32 W AND PAGE_HEIGHT * W BYTES (255 PAPER, 0 INK).
# HAND.C IS INCLUDED TO REACH FIND_LINES (THE PAGE MADE BIGGER OR SMALLER,
# SHARPENED, TURNED BACK, THEN CUT).
*/
#include <stdio.h>
#include <stdlib.h>
#include "hand.c"

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

	if (!D || fread(D, 1, *SIZE, F) != *SIZE)
	{
		fclose(F);
		free(D);
		return (NULL);
	}

	fclose(F);
	return (D);
}

static int
	CUT_ONE(const char *PATH)
{
	struct S_LINE	*LINES;
	unsigned char	*PGM;
	char			OUT_PATH[1024];
	FILE			*OUT;
	size_t			SIZE;
	float			SCALE;
	int				COUNT;
	int				W;
	int				H;
	int				MAX;
	int				OFF;
	int				I;

	PGM = READ_FILE(PATH, &SIZE);

	if (!PGM)
		return (0);

	if (sscanf((char *)PGM, "P5 %d %d %d%n", &W, &H, &MAX, &OFF) != 3)
	{
		free(PGM);
		return (0);
	}

	COUNT = FIND_LINES(PGM + OFF + 1, W, H, &LINES, &SCALE);
	snprintf(OUT_PATH, sizeof(OUT_PATH), "%s.lines", PATH);
	OUT = fopen(OUT_PATH, "wb");

	if (!OUT)
	{
		PAGE_FREE(LINES, COUNT);
		free(PGM);
		return (0);
	}

	fwrite(&COUNT, sizeof(int), 1, OUT);

	for (I = 0; I < COUNT; ++I)
	{
		const struct S_LINE	*L = &LINES[I];
		int					K;

		fwrite(&L->W, sizeof(int), 1, OUT);

		for (K = 0; K < L->W * PAGE_HEIGHT; ++K)
			fputc((int)(255.0F * (1.0F - L->INK[K]) + 0.5F), OUT);
	}

	fclose(OUT);
	PAGE_FREE(LINES, COUNT);
	free(PGM);
	return (1);
}

int
	main(int ARGC, char **ARGV)
{
	int	I;

	for (I = 1; I < ARGC; ++I)
		if (!CUT_ONE(ARGV[I]))
			fprintf(stderr, "%s: not read\n", ARGV[I]);

	return (0);
}
