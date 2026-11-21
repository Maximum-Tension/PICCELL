#!/bin/bash
# EVERY TEST PAGE OF A FOLDER THROUGH THE C READER: EVAL_ALL.SH READER.BIN DIR
R=$1; D=$2; TAG=$(basename $R .bin)
for p in $D/*.pgm; do
	b=${p%.pgm}
	/tmp/test_hand $R $p $b.$TAG.chk > $b.$TAG.out 2>/dev/null
	L=$(python3 $(dirname $0)/eval_table.py $b.$TAG.out $b.txt | tail -1)
	echo "$(basename $b) $L"
done
