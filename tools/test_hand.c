/* HAND_READ ON A PGM PAGE: THE TABLE TO STDOUT */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "hand.h"
#include "layout.h"
#include "reader.h"

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
	D = malloc(*SIZE);

	if (!D || fread(D, 1, *SIZE, F) != *SIZE)
		return (NULL);

	fclose(F);
	return (D);
}

int
	main(int ARGC, char **ARGV)
{
	struct S_TABLE	TABLE;
	struct S_READER	*R;
	unsigned char	*BLOB;
	unsigned char	*PGM;
	size_t			SIZE;
	clock_t			T0;
	int				W;
	int				H;
	int				MAX;
	int				OFF;

	if (ARGC < 3)
		return (1);

	BLOB = READ_FILE(ARGV[1], &SIZE);
	R = READER_LOAD(BLOB, SIZE);
	PGM = READ_FILE(ARGV[2], &SIZE);

	if (!R || !PGM)
		return (1);

	sscanf((char *)PGM, "P5 %d %d %d%n", &W, &H, &MAX, &OFF);
	HAND_SET_READER(R);
	T0 = clock();

	if (!HAND_READ(PGM + OFF + 1, W, H, &TABLE))
	{
		printf("no table\n");
		return (1);
	}

	LAYOUT_WRITE(&TABLE, stdout);

	/* THE "NOT SURE" MARKS, ONE ROW PER LINE: 0 AND 1 */
	if (ARGC > 3 && TABLE.CHECK)
	{
		FILE	*F;
		int		I;

		F = fopen(ARGV[3], "wb");

		for (I = 0; F && I < TABLE.ROWS * TABLE.COLS; ++I)
		{
			const char	*END = (I + 1) % TABLE.COLS ? "\t" : "\n";

			fprintf(F, "%d%s", TABLE.CHECK[I], END);
		}

		if (F)
			fclose(F);
	}
	fprintf(stderr, "time %.2fs\n", (double)(clock() - T0) / CLOCKS_PER_SEC);
	LAYOUT_FREE(&TABLE);
	READER_FREE(R);
	return (0);
}
