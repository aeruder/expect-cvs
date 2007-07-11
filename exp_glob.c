/* exp_glob.c - expect functions for doing glob

Based on Tcl's glob functions but modified to support anchors and to
return information about the possibility of future matches

Modifications by: Don Libes, NIST, 2/6/90

Design and implementation of this program was paid for by U.S. tax
dollars.  Therefore it is public domain.  However, the author and NIST
would appreciate credit if this program or parts of it are used.

*/

#include "expect_cf.h"
#include "tcl.h"
#include "exp_int.h"

/* Proper forward declaration of internal function */
static int
Exp_StringCaseMatch2 _ANSI_ARGS_((CONST Tcl_UniChar *string, /* String. */
				  CONST Tcl_UniChar *stop,   /* First char _after_ string */
				  CONST Tcl_UniChar *pattern,	 /* Pattern, which may contain
								  * special characters. */
				  int nocase));

/* The following functions implement expect's glob-style string matching */
/* Exp_StringMatch allow's implements the unanchored front (or conversely */
/* the '^') feature.  Exp_StringMatch2 does the rest of the work. */

int	/* returns # of CHARS that matched */
Exp_StringCaseMatch(string, strlen, pattern, nocase, offset)		/* INTL */
     Tcl_UniChar *string;
     Tcl_UniChar *pattern;
     int strlen;
int nocase;
     int *offset;	/* offset in chars from beginning of string where pattern matches */
{
    CONST Tcl_UniChar *s;
    CONST Tcl_UniChar *stop = string + strlen;
    int ssm, sm;	/* count of bytes matched or -1 */
	int caret = FALSE;
	int star = FALSE;

	*offset = 0;

	if (pattern[0] == '^') {
		caret = TRUE;
		pattern++;
	} else if (pattern[0] == '*') {
		star = TRUE;
	}

	/*
	 * test if pattern matches in initial position.
	 * This handles front-anchor and 1st iteration of non-front-anchor.
	 * Note that 1st iteration must be tried even if string is empty.
	 */

    sm = Exp_StringCaseMatch2(string,stop,pattern, nocase);
	if (sm >= 0) return(sm);

	if (caret) return -1;
	if (star) return -1;

	if (*string == '\0') return -1;

    s = string + 1;
    sm = 0;
#if 0
    if ((*pattern != '[') && (*pattern != '?')
	    && (*pattern != '\\') && (*pattern != '$')
	    && (*pattern != '*')) {
	while (*s && (s < stop) && *pattern != *s) {
	    s++;
	    sm++;
	}
    }
    if (sm) {
	printf("skipped %d chars of %d\n", sm, strlen); fflush(stdout);
    }
#endif
    for (;s < stop; s++) {
	ssm = Exp_StringCaseMatch2(s,stop,pattern, nocase);
	if (ssm != -1) {
			*offset = s-string;
	    return(ssm+sm);
		}
	}
	return -1;
}

/* Exp_StringCaseMatch2 --
 *
 * Like Tcl_StringCaseMatch except that
 * 1: returns number of characters matched, -1 if failed.
 *    (Can return 0 on patterns like "" or "$")
 * 2: does not require pattern to match to end of string
 * 3: Much of code is stolen from Tcl_StringMatch
 * 4: front-anchor is assumed (Tcl_StringMatch retries for non-front-anchor)
*/

static int
Exp_StringCaseMatch2(string,stop,pattern,nocase)	/* INTL */
     register CONST Tcl_UniChar *string; /* String. */
     register CONST Tcl_UniChar *stop;   /* First char _after_ string */
     register CONST Tcl_UniChar *pattern;	 /* Pattern, which may contain
				 * special characters. */
    int nocase;
{
    Tcl_UniChar ch1, ch2, p;
    int match = 0;	/* # of bytes matched */
    CONST Tcl_UniChar *oldString;

    while (1) {
	/* If at end of pattern, success! */
	if (*pattern == 0) {
		return match;
	}

	/* If last pattern character is '$', verify that entire
	 * string has been matched.
	 */
	if ((*pattern == '$') && (pattern[1] == 0)) {
		if (*string == 0) return(match);
		else return(-1);		
	}

	/* Check for a "*" as the next pattern character.  It matches
	 * any substring.  We handle this by calling ourselves
	 * recursively for each postfix of string, until either we match
	 * or we reach the end of the string.
	 *
	 * ZZZ: Check against Tcl core, port optimizations found there over here.
	 */
	
	if (*pattern == '*') {
	    CONST Tcl_UniChar *tail;

	    /*
	     * Skip all successive *'s in the pattern
	     */
	    while (*(++pattern) == '*') {}
	    p = *pattern;

	    if (p == 0) {
		return((stop-string)+match); /* DEL */
	    }

	    if (nocase) {
		p = Tcl_UniCharToLower(p);
	    }

	    /* find LONGEST match */

	    /*
	     * NOTES
	     *
	     * The original code used 'strlen' to find the end of the
	     * string. With the recursion coming this was done over and
	     * over again, making this an O(n**2) operation overall. Now
	     * the pointer to the end is passed in from the caller, and
	     * even the topmost context now computes it from start and
	     * length instead of seaching.
	     *
	     * The conversion to unicode also allow us to step back via
	     * decrement, in linear time overall. The previously used
	     * Tcl_UtfPrev crawled to the previous character from the
	     * beginning of the string, another O(n**2) operation.
	     */

	    tail = stop - 1;
	    while (1) {
		int rc;

		/*
		 * Optimization for matching - cruise through the string
		 * quickly if the next char in the pattern isn't a special
		 * character.
		 *
		 * NOTE: We cruise backwards to keep the semantics of
		 * finding the LONGEST match.
		 *
		 * XXX JH: should this add '&& (p != '$')' ???
		 */
		if ((p != '[') && (p != '?') && (p != '\\')) {
		    if (nocase) {
			while ((tail >= string) && (p != *tail)
			       && (p != Tcl_UniCharToLower(*tail))) {
			    tail--;;
			}
		    } else {
			/*
			 * XXX JH: Should this be (tail > string)?
			 */
			while ((tail >= string) && (p != *tail)) { tail --; }
		    }
		}

		/* if we've backed up to beginning of string, give up */
		if (tail <= string) break;

		rc = Exp_StringCaseMatch2(tail, stop, pattern, nocase);
		if (rc != -1 ) {
		    return match + (tail - string) + rc;
		    /* match = # of bytes we've skipped before this */
		    /* (...) = # of bytes we've skipped due to "*" */
		    /* rc    = # of bytes we've matched after "*" */
		}

		/* if we've backed up to beginning of string, give up */
		if (tail == string) break;

		tail --;
		if (tail < string) tail = string;
	    }
	    return -1;					/* DEL */
	}
    
	/*
	 * after this point, all patterns must match at least one
	 * character, so check this
	 */

	if (*string == 0) return -1;

	/* Check for a "?" as the next pattern character.  It matches
	 * any single character.
	 */

	if (*pattern == '?') {
	    pattern++;
	    oldString = string;
	    string ++;
	    match ++; /* incr by # of matched chars */
	    continue;
	}

	/* Check for a "[" as the next pattern character.  It is
	 * followed by a list of characters that are acceptable, or by a
	 * range (two characters separated by "-").
	 */
	
	if (*pattern == '[') {
	    Tcl_UniChar ch, startChar, endChar;

	    pattern++;
	    oldString = string;
	    ch = *string++;

	    while (1) {
		if ((*pattern == ']') || (*pattern == '\0')) {
		    return -1;			/* was 0; DEL */
		}
		startChar = *pattern ++;
		if (nocase) {
		    startChar = Tcl_UniCharToLower(startChar);
		}
		if (*pattern == '-') {
		    pattern++;
		    if (*pattern == '\0') {
			return -1;		/* DEL */
		    }
		    endChar = *pattern ++;
		    if (nocase) {
			endChar = Tcl_UniCharToLower(endChar);
		    }
		    if (((startChar <= ch) && (ch <= endChar))
			    || ((endChar <= ch) && (ch <= startChar))) {
			/*
			 * Matches ranges of form [a-z] or [z-a].
			 */

			break;
		    }
		} else if (startChar == ch) {
		    break;
		}
	    }
	    while (*pattern != ']') {
		if (*pattern == '\0') {
		    pattern--;
		    break;
		}
		pattern++;
	    }
	    pattern++;
	    match += (string - oldString); /* incr by # matched chars */
	    continue;
	}
 
	/* If the next pattern character is backslash, strip it off so
	 * we do exact matching on the character that follows.
	 */
	
	if (*pattern == '\\') {
	    pattern += 1;
	    if (*pattern == 0) {
		return -1;
	    }
	}

	/* There's no special character.  Just make sure that the next
	 * characters of each string match.
	 */
	
	oldString = string;
	ch1 = *string ++;
	ch2 = *pattern ++;
	if (nocase) {
	    if (Tcl_UniCharToLower(ch1) != Tcl_UniCharToLower(ch2)) {
		return -1;
	    }
	} else if (ch1 != ch2) {
	    return -1;
	}
	match += (string - oldString);  /* incr by # matched chars */
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
