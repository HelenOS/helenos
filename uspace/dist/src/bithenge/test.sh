#!/bin/bash
shopt -s nullglob
set -e

BITHENGE=../../../app/bithenge/bithenge

if type valgrind >/dev/null 2>&1
then
	BITHENGE="valgrind -q --show-reachable=yes ${BITHENGE}"
else
	echo "Valgrind not found."
fi

for BH in *.bh
do
	for DAT in $(basename ${BH} .bh).dat $(basename ${BH} .bh).*.dat
	do
		OUT=$(basename ${DAT} .dat).out
		[ -e ${DAT} ] || continue
		[ -e ${OUT} ] || continue
		echo "Testing ${BH} on ${DAT}..."
		${BITHENGE} ${BH} ${DAT} 2>&1|diff ${OUT} -
	done
done

echo "Done!"
