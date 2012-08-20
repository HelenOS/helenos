#!/bin/bash
shopt -s nullglob
set -e

BITHENGE=../../../app/bithenge/bithenge

if type valgrind >/dev/null 2>&1
then
	BITHENGE="valgrind -q --show-reachable=yes --error-exitcode=64 ${BITHENGE}"
else
	echo "Valgrind not found."
fi

test_file() {
	echo "Testing $1 on $2..."
	${BITHENGE} $1 $2 2>&1|diff $3 -
}

for BH in *.bh
do
	for DAT in $(basename ${BH} .bh).dat $(basename ${BH} .bh).*.dat
	do
		OUT=$(basename ${DAT} .dat).out
		[ -e ${DAT} ] || continue
		[ -e ${OUT} ] || continue
		test_file ${BH} ${DAT} ${OUT}
	done
done

test_file trip.bh file:trip.dat trip.out
test_file repeat.bh hex:7f07020305070b0D11020004000800102040010101040009 repeat.out

echo "Done!"
