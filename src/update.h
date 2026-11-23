/******************************************************************************\
# H - update                                     #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/20 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/23 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

/*
# THE GITHUB RELEASE AUTO UPDATER. IT ONLY RUNS WHEN THE WINDOW IS OPENED,
# BEFORE IT OPENS, NEVER WHILE IT IS OPEN:
# 1. PICCELL ASKS GITHUB FOR THE LATEST RELEASE OF MAXIMUM-TENSION/PICCELL.
#    IT UPDATES WHEN THE SHA-256 GITHUB GIVES FOR THAT RELEASE'S PICCELL.EXE
#    IS NOT THE SHA-256 OF THE RUNNING EXE, SO TWO RELEASES ON THE SAME DAY
#    WORK. NO UPDATE: THE WINDOW OPENS.
# 2. THE NEW EXE IS DOWNLOADED INTO %TMP% AND ITS SHA-256 IS CHECKED AGAINST
#    THE SAME DIGEST AGAIN. A WRONG OR MISSING DIGEST MEANS NO UPDATE.
# 3. THE RUNNING EXE MOVES ITSELF INTO %TMP% ("PICCELL-OLD-...EXE"), THE NEW
#    EXE TAKES ITS NAME AND IS STARTED WITH "-updated", AND THIS ONE CLOSES.
# 4. THE NEW ONE DELETES THE OLD ONE IN %TMP% (UPDATE_CLEAN) AND OPENS ITS
#    WINDOW, WHICH SAYS IT WAS UPDATED.
# WHEN IT TAKES LONGER THAN A MOMENT, A SMALL WINDOW SAYS WHAT HAPPENS;
# CLOSING IT SKIPS THE UPDATE THIS TIME. TWO SAFETY RULES: A RELEASE WITH AN
# OLDER DATE THAN THIS EXE'S VERSION IS NEVER INSTALLED, AND AN EXE NEXT TO
# "SRC\PICCELL.H" (A TEST BUILD IN THE SOURCE FOLDER) NEVER UPDATES ITSELF.
# NO NETWORK, A RATE LIMIT OR ANY ERROR: THE WINDOW JUST OPENS. COMMAND LINE
# RUNS (THE EXCEL ADD-IN) NEVER ASK GITHUB, SO THEY STAY FAST AND OFFLINE.
*/

#ifndef UPDATE_H
#define UPDATE_H

/* **************************** [v] INCLUDES [v] **************************** */
#include <stddef.h>
/******* """"""""""
 * typedef	size_t;
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define UPDATE_REPO L"Maximum-Tension/PICCELL"

#ifdef _WIN64
#define UPDATE_ASSET "PICCELL.exe"
#else /* !_WIN64 */
#define UPDATE_ASSET "PICCELL32.exe" /* THE 32-BIT BUILD STAYS 32-BIT */
#endif /* _WIN64 */

/* BEFORE THE WINDOW OPENS. 1 = THE NEW EXE RUNS NOW: CLOSE WITHOUT A WINDOW */
extern int	UPDATE_AT_START(void);
/*
# DELETES THE OLD EXE AN UPDATE LEFT IN %TMP%, IN THE BACKGROUND. UPDATED 1:
# IT JUST HAPPENED, SO IT TRIES FOR A FEW SECONDS (THE OLD ONE IS CLOSING).
*/
extern void	UPDATE_CLEAN(int UPDATED);
/* "-update-finish": AN OLDER PICCELL (20.26.11.23 OR OLDER) IS UPDATING */
extern void	UPDATE_FINISH(void);

#endif /* !UPDATE_H */
