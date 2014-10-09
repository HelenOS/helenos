# Re-generate with CMake

# abort
$(PCUT_TEST_PREFIX)abort$(PCUT_TEST_SUFFIX): tests/abort.o

# asserts
$(PCUT_TEST_PREFIX)asserts$(PCUT_TEST_SUFFIX): tests/asserts.o

# beforeafter
$(PCUT_TEST_PREFIX)beforeafter$(PCUT_TEST_SUFFIX): tests/beforeafter.o

# errno
$(PCUT_TEST_PREFIX)errno$(PCUT_TEST_SUFFIX): tests/errno.o

# inithook
$(PCUT_TEST_PREFIX)inithook$(PCUT_TEST_SUFFIX): tests/inithook.o

# manytests
$(PCUT_TEST_PREFIX)manytests$(PCUT_TEST_SUFFIX): tests/manytests.o

# multisuite
$(PCUT_TEST_PREFIX)multisuite$(PCUT_TEST_SUFFIX): tests/suite_all.o tests/suite1.o tests/suite2.o tests/tested.o

# preinithook
$(PCUT_TEST_PREFIX)preinithook$(PCUT_TEST_SUFFIX): tests/inithook.o

# printing
$(PCUT_TEST_PREFIX)printing$(PCUT_TEST_SUFFIX): tests/printing.o

# simple
$(PCUT_TEST_PREFIX)simple$(PCUT_TEST_SUFFIX): tests/simple.o tests/tested.o

# skip
$(PCUT_TEST_PREFIX)skip$(PCUT_TEST_SUFFIX): tests/skip.o

# suites
$(PCUT_TEST_PREFIX)suites$(PCUT_TEST_SUFFIX): tests/suites.o tests/tested.o

# teardownaborts
$(PCUT_TEST_PREFIX)teardownaborts$(PCUT_TEST_SUFFIX): tests/teardownaborts.o

# teardown
$(PCUT_TEST_PREFIX)teardown$(PCUT_TEST_SUFFIX): tests/teardown.o tests/tested.o

# testlist
$(PCUT_TEST_PREFIX)testlist$(PCUT_TEST_SUFFIX): tests/testlist.o

# timeout
$(PCUT_TEST_PREFIX)timeout$(PCUT_TEST_SUFFIX): tests/timeout.o

# xmlreport
$(PCUT_TEST_PREFIX)xmlreport$(PCUT_TEST_SUFFIX): tests/xmlreport.o tests/tested.o

