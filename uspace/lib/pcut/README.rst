PCUT: Plain C Unit Testing mini-framework
=========================================

PCUT is a very simple framework for unit testing of C code.
Unlike many other frameworks where you need to specify manually which
functions belong to a particular test, PCUT provides several smart
macros that hides this and lets you focus on the most important
part of testing only: that is, coding the test cases.

This mini-framework is definitely not complete but it offers the basic
functionality needed for writing unit tests.
This includes the possibility to group tests into test suites, optionally
having set-up and tear-down functions.
There are several assert macros for evaluating the results, their highlight
is very detailed information about the problem.

The output of the test can come in two forms: either as an XML output suited
for later processing or in the form of Test-Anything-Protocol.
PCUT is able to capture standard output and display it together with test
results.
And by running each test in a separate process, the whole framework is pretty
safe against unexpected crashes, such as null pointer dereference.

More details can be found on PCUT wiki on GitHub.

					https://github.com/vhotspur/pcut/wiki


Main goal - simple to use
-------------------------

Let's illustrate how PCUT aims to be simple when creating unit tests for
function ``intmin`` that ought to return smaller of its two arguments.::

	int intmin(int a, int b) {
		return a > b ? b : a;
	}
	
Individual test-cases for such method could cover following cases: getting
minimal of

- two same numbers
- negative and positive number
- two negative numbers
- two positive numbers
- "corner case" numbers, such as minimal and maximal represented number

In PCUT, that would be translated to the following code:::

	#include <pcut/test.h>
	#include <limits.h>
	/* Other include to have declaration of intmin */
	
	PCUT_INIT

	PCUT_TEST_SUITE(intmin_tests);

	PCUT_TEST(same_number) {
		PCUT_ASSERT_INT_EQUALS(719, intmin(719, 719) );
		PCUT_ASSERT_INT_EQUALS(-4589, intmin(-4589, -4589) );
	}
	
	PCUT_TEST(positive_and_negative) {
		PCUT_ASSERT_INT_EQUALS(-5, intmin(-5, 71) );
		PCUT_ASSERT_INT_EQUALS(-17, intmin(423, -17) );
	}
	
	PCUT_TEST(same_sign) {
		PCUT_ASSERT_INT_EQUALS(22, intmin(129, 22) );
		PCUT_ASSERT_INT_EQUALS(-37, intmin(-37, -1) );
	}
	
	PCUT_TEST(corner_cases) {
		PCUT_ASSERT_INT_EQUALS(INT_MIN, intmin(INT_MIN, -1234) );
		PCUT_ASSERT_INT_EQUALS(9876, intmin(9876, INT_MAX) );
	}

	PCUT_MAIN()

And that's all.
You do not need to manually specify which tests to run etc., 
everything is done magically via the ``PCUT_INIT``, ``PCUT_MAIN`` and
``PCUT_TEST`` macros.
All you need to do is to compile this code and link it with ``libpcut``.
Result of the linking would be an executable that runs the tests and
reports the results.


Examples
--------

More examples, in the form of self-tests, are available in the ``tests/``
subdirectory.


Building and installing
-----------------------

On Unix systems, running ``make`` and ``make install`` shall do the job.

More details can be found on https://github.com/vhotspur/pcut/wiki/Building.
