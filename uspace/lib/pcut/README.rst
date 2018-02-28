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

More details can be found on PCUT wiki on GitHub:
https://github.com/vhotspur/pcut/wiki


Quick-start example
-------------------

The following code tests the standard ``atoi`` function::

	#include <pcut/pcut.h>
	#include <stdlib.h>

	PCUT_INIT

	PCUT_TEST(atoi_zero) {
	    PCUT_ASSERT_INT_EQUALS(0, atoi("0"));
	}

	PCUT_TEST(atoi_positive) {
	    PCUT_ASSERT_INT_EQUALS(42, atoi("42"));
	}

	PCUT_TEST(atoi_negative) {
	    PCUT_ASSERT_INT_EQUALS(-273, atoi("-273"));
	}

	PCUT_MAIN()

As you can see, there is no manual listing of tests that form the test
suite etc, only the tests and ``PCUT_INIT`` at the beginning and
``PCUT_MAIN`` at the end.

This code has to be linked with ``libpcut`` to get an executable that runs
the tests and reports the results.

More examples, in the form of self-tests, are available in the ``tests/``
subdirectory.
Other examples can be found on the Wiki.


Building and installing
-----------------------

PCUT uses CMake (http://www.cmake.org/).
On Unix systems, following commands build the library and execute the
built-in tests::

	mkdir build
	cd build
	cmake .. && make all test

More details can be found on https://github.com/vhotspur/pcut/wiki/Building.
