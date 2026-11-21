/******************************************************************************\
# H - export                                     #       Maximum Tension       #
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
# A TABLE TO A FILE: EXCEL (.XLSX), CSV, PDF OR TAB SEPARATED TEXT. NO
# LIBRARIES: THE XLSX IS A ZIP OF A FEW XML FILES, THE PDF IS WRITTEN BY HAND
# WITH THE HELVETICA EVERY PDF READER HAS. NUMBERS AND DATES BECOME REAL
# NUMBERS AND DATES IN THE XLSX ("1.234,56", "12.05.2026", "15%"); A CODE
# WITH A LEADING ZERO ("0039") STAYS TEXT.
*/

#ifndef EXPORT_H
#define EXPORT_H

/* **************************** [v] INCLUDES [v] **************************** */
#include <stdio.h>
/******* """""""""
 * typedef	FILE;
 */
#include "layout.h"
/******* """"""""""
 * struct S_TABLE;
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define EXPORT_TSV 0 /* TAB BETWEEN CELLS (WHAT THE EXCEL ADD-IN READS) */
#define EXPORT_CSV 1
#define EXPORT_XLSX 2
#define EXPORT_PDF 3

/* THE KIND OF A FILE NAME (UTF-8 OR ASCII), BY ITS EXTENSION; TSV IF UNKNOWN */
extern int	EXPORT_KIND(const char *NAME);
/*
# TURKISH: 1 = "1.234,56" (A COMMA BEFORE THE DECIMALS) AND ";" BETWEEN THE
# CELLS OF A CSV, AS TURKISH EXCEL WANTS; 0 = "1,234.56" AND ",". 1 = WRITTEN.
*/
extern int	EXPORT_WRITE(
	const struct S_TABLE *TABLE,
	int KIND,
	int TURKISH,
	FILE *OUTPUT
);
/* THE NUMBER IN A CELL, AS EXCEL WOULD TAKE IT. 1 = IT IS ONE */
extern int	EXPORT_NUMBER(const char *TEXT, int TURKISH, double *VALUE);

#endif /* !EXPORT_H */
