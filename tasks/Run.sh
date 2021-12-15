#!/bin/sh

while getopts n:j:q:f:c: flag
do
	case "${flag}" in
		n) namerep=${OPTARG};;  # name of the outputreport
		j) jitted=${OPTARG};;   # 0 -> compiled, 1 -> jitted
		q) query=${OPTARG};;    # query ind. from 1 to 8
		f) filesnum=${OPTARG};; # no. input files (currently 1 or 10)
		c) cores=${OPTARG};;    # no. of cores used (1 has no MT)
	esac
done

FILEN="/data/ssdext4/ikabadzh/ODB/Run2012B_SingleMu.root"

if [ "${filesnum}" -eq "10" ];
then
	FILEN="/data/ssdext4/ikabadzh/ODB/Run2012B_SingleMu*.root"
fi

if [ "${jitted}" -eq "0" ];
then
	g++ -O3 $(root-config --cflags --libs) compiled.cxx -o cmpl
	./cmpl ${cores} "${FILEN}" ${query}
	rm cmpl
else
	root -l -b -q "jitted.C(${cores},\"${FILEN}\",${query})"
fi

#perf script -i "../../OPENreportsD/${query}/${namerep}.data" | ~/FlameGraph/stackcollapse-perf.pl >  ~/perf_reps/out.perf-folded

#~/FlameGraph/flamegraph.pl ~/perf_reps/out.perf-folded > "reports/${query}/${namerep}.svg"

