/* A TSV FILE TO XLSX, CSV, PDF: test_export IN.TSV OUT_BASE [en] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "export.h"
#include "layout.h"

int
	main(int ARGC, char **ARGV)
{
	static const char *const	EXT[4] = {"tsv", "csv", "xlsx", "pdf"};
	struct S_TABLE				T;
	char						LINE[4096];
	char						NAME[512];
	FILE						*F;
	int							K;

	if (ARGC < 3)
		return (1);

	F = fopen(ARGV[1], "rb");

	if (!F)
		return (1);

	memset(&T, 0, sizeof(T));
	T.COLS = 1;

	while (fgets(LINE, sizeof(LINE), F))
	{
		char	*P;
		int		C;

		LINE[strcspn(LINE, "\r\n")] = '\0';

		for (C = 1, P = LINE; *P; ++P)
			C += *P == '\t';

		if (C > T.COLS)
			T.COLS = C;

		++T.ROWS;
	}

	T.CELL = calloc((size_t)T.ROWS * T.COLS, sizeof(char *));
	rewind(F);
	T.ROWS = 0;

	while (fgets(LINE, sizeof(LINE), F))
	{
		char	*P;
		int		C;

		LINE[strcspn(LINE, "\r\n")] = '\0';
		P = LINE;

		for (C = 0; P; ++C)
		{
			char	*TAB = strchr(P, '\t');

			if (TAB)
				*TAB = '\0';

			if (*P)
				T.CELL[T.ROWS * T.COLS + C] = strdup(P);

			if (TAB)
				P = TAB + 1;
			else
				P = NULL;
		}

		++T.ROWS;
	}

	fclose(F);

	for (K = 0; K < 4; ++K)
	{
		snprintf(NAME, sizeof(NAME), "%s.%s", ARGV[2], EXT[K]);
		F = fopen(NAME, "wb");

		if (!F || !EXPORT_WRITE(&T, EXPORT_KIND(NAME), ARGC < 4, F))
			printf("FAILED %s\n", NAME);

		if (F)
			fclose(F);
	}

	LAYOUT_FREE(&T);
	return (0);
}
