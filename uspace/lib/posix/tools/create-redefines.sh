#!/bin/sh

set -e

while read symbolname; do
	echo "--redefine-sym=$1$symbolname=$2$symbolname"
done
