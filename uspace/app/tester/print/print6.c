#include <stdio.h>
#include <unistd.h>
#include "../tester.h"

#include <str.h>




const char *test_print6(void)
{
	struct {
		double val;
		const char *fmt;
		const char *exp_str;
		const char *warn_str;
	} pat[] = {
		/* 
		 * Generic 
		 */
		{ 2.0,         "%g",       "2", 0 },
		{ 0,           "%g",       "0", 0 },
		{ 0.1,         "%g",     "0.1", 0 },
		{ 9e59,        "%g",   "9e+59", 0 },
		{ -9e-59,      "%g",  "-9e-59", 0 },
		{ 1e307,       "%g",  "1e+307", 0 },   
		{ 0.09999999999999999, "%g",  "9.999999999999999e-02", 0 },   
		{ 0.099999999999999999, "%g",  "0.1", 0 },   

		/* gcc and msvc convert "3.4567e-317" to different binary doubles. */
		{ 3.4567e-317, "%g",  "3.4567e-317", "3.456998e-317" },   
		{ 3.4567e-318, "%g",  "3.4567e-318", 0 },   
		{ 123456789012345.0, "%g",  "123456789012345", 0 },   
		{ -123456789012345.0, "%g",  "-123456789012345", 0 },   

		/* Special */
		{ 1e300 * 1e300, "%g",  "inf", 0 },   
		{ -1.0 /(1e300 * 1e300), "%g",  "-0", 0 },   

		{ 1234567.8901, "%g",  "1234567.8901", 0 },   
		{ 1234567.80012, "%g",  "1234567.80012", 0 },   
		{ 112e-32, "%g",  "1.12e-30", 0 },   
		{ 10.0e45, "%g",  "1e+46", 0 },   

		/* rounding w/ trailing zero removal */
		{ 0.01, "%10.6g",      "      0.01", 0 },   
		{ 9.495, "%10.2g",     "       9.5", 0 },   
		{ 9.495e30, "%10.2g",  "   9.5e+30", 0 },   
		{ 9.495e30, "%10g",  " 9.495e+30", 0 },   
		{ 9.495e30, "%10.6g",  " 9.495e+30", 0 },   

		/*
		 * Scientific 
		 */
		{ 1e05, "%e",  "1.000000e+05", 0 },   

		/* full padding */
		{ 1e-1, "%+010.3e",  "+1.000e-01", 0 },  /* __PRINTF_FLAG_SHOWPLUS | __PRINTF_FLAG_ZEROPADDED */
		{ 1e-1, "%+10.3e",  "+1.000e-01", 0 },   
		{ 1e-1, "%+-10.3e",  "+1.000e-01", 0 },  /* __PRINTF_FLAG_SHOWPLUS | __PRINTF_FLAG_LEFTALIGNED */

		/* padding */
		{ 1e-1, "%+010.2e",  "+01.00e-01", 0 },  /* __PRINTF_FLAG_SHOWPLUS | __PRINTF_FLAG_ZEROPADDED */
		{ 1e-1, "%+10.2e",  " +1.00e-01", 0 },   
		{ 1e-1, "%+-10.2e",  "+1.00e-01 ", 0 },  /* __PRINTF_FLAG_SHOWPLUS | __PRINTF_FLAG_LEFTALIGNED */

		{ 1e-1, "% 010.2e",  " 01.00e-01", 0 },  /* __PRINTF_FLAG_SPACESIGN | __PRINTF_FLAG_ZEROPADDED */
		{ 1e-1, "%010.2e",  "001.00e-01", 0 },   /* __PRINTF_FLAG_ZEROPADDED */
		{ 1e-1, "% 10.2e",  "  1.00e-01", 0 },   /* __PRINTF_FLAG_SPACESIGN */
		{ 1e-1, "%10.2e",  "  1.00e-01", 0 },   

		/* padding fractionals */
		{ 1.08e29, "%+010.3e",  "+1.080e+29", 0 },  /* __PRINTF_FLAG_SHOWPLUS | __PRINTF_FLAG_ZEROPADDED */
		{ 1.08e29, "%+10.3e",  "+1.080e+29", 0 },  
		{ 1.08e29, "%+011.2e",  "+001.08e+29", 0 }, /* __PRINTF_FLAG_SHOWPLUS | __PRINTF_FLAG_ZEROPADDED */
		{ 1.085e29, "%11.2e", "   1.09e+29", 0 },   

		/* rounding */
		{ 1.345e2, "%+10.2e",  " +1.35e+02", 0 }, 
		{ 9.995e2, "%+10.2e",  " +1.00e+03", 0 }, 
		{ -9.99499999e2, "%10.2e",  " -9.99e+02", 0 },   
		{ -9.99499999e2, "%10.0e",  "    -1e+03", 0 },   
		{ -9.99499999e2, "%#10.0e",  "   -1.e+03", 0 }, /* __PRINTF_FLAG_DECIMALPT */
		{ -1.2345006789e+231, "%#10.10e",   "-1.2345006789e+231", 0 },  /* __PRINTF_FLAG_DECIMALPT */
		{ -1.23450067995e+231, "%#10.10e",  "-1.2345006800e+231", 0 },  /* __PRINTF_FLAG_DECIMALPT */
			
		/* special */
		{ 1e300 * 1e300, "%10.5e",  "       inf", 0 },   
		{ -1.0 /(1e300 * 1e300), "%10.2e",  " -0.00e+00", 0 },   
		{ 1e300 * 1e300, "%10.5E",  "       INF", 0 },          /* __PRINTF_FLAG_BIGCHARS */
		{ -1.0 /(1e300 * 1e300), "%10.2E",  " -0.00E+00", 0 },  /* __PRINTF_FLAG_BIGCHARS */

		/*
		 * Fixed
		 */
		 
		/* padding */
		{ 1e-1, "% 010.3f",  " 00000.100", 0 },  /* __PRINTF_FLAG_SPACESIGN | __PRINTF_FLAG_ZEROPADDED */
		{ 1e-1, "% 0-10.3f",  " 0.100    ", 0 }, /* __PRINTF_FLAG_SPACESIGN | __PRINTF_FLAG_ZEROPADDED | __PRINTF_FLAG_LEFTALIGNED */
		{ 1e-1, "% 010.3f",  " 00000.100", 0 },  /* __PRINTF_FLAG_SPACESIGN | __PRINTF_FLAG_ZEROPADDED */
		{ 1e-1, "%10.3f",  "     0.100", 0 },   

		/* rounding */
		{ -0.0, "%10.0f",     "        -0", 0 },   
		{ -0.099, "%+10.3f",   "    -0.099", 0 }, 
		{ -0.0995, "%+10.3f",  "    -0.100", 0 }, 
		{ -0.0994, "%+10.3f",  "    -0.099", 0 }, 
		{ -99.995, "%+10.0f",  "      -100", 0 }, 
		{ 3.5, "%+10.30f",  "+3.500000000000000000000000000000", 0 }, 
		{ 3.5, "%+10.0f",      "        +4", 0 }, 
		{ 0.1, "%+10.6f",      " +0.100000", 0 }, 

		/* The compiler will go for closer 0.10..055 instead of 0.09..917 */
		{ 0.1, "%+10.20f",  "+0.10000000000000000550", 0 },  
		/* Next closest to 0.1 */
		{ 0.0999999999999999917, "%+10.20f",  "+0.09999999999999999170", 0 }, 
		{ 0.0999999999999999917, "%+10f",  " +0.100000", 0 },
		{ 0.0999999999999998945, "%10.20f",  "0.09999999999999989450", 0 },   
		
	};

	int patterns_len = (int)(sizeof(pat) / sizeof(pat[0]));
	int failed = 0;
	const int buf_size = 256;
	char buf[256 + 1] = { 0 };
	
	TPRINTF("Test printing of floating point numbers via printf(\"%%f\"):\n");
	
	for (int i = 0; i < patterns_len; ++i) {
		
		snprintf(buf, buf_size, pat[i].fmt, pat[i].val);
		
		if (0 == str_cmp(buf, pat[i].exp_str)) {
			TPRINTF("ok:  %s |%s| == |%s|\n", pat[i].fmt, buf, pat[i].exp_str);
		} else {
			if (pat[i].warn_str && 0 == str_cmp(buf, pat[i].warn_str)) {
				TPRINTF("warn: %s |%s| != |%s|\n", pat[i].fmt, buf, pat[i].exp_str);
			} else {
				++failed;
				TPRINTF("ERR: %s |%s| != |%s|\n", pat[i].fmt, buf, pat[i].exp_str);
			}
		}
	}
	
	if (failed) {
		return "Unexpectedly misprinted floating point numbers.";
	} else {
		return 0;
	}
}

