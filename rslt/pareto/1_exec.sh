FILE="pareto_0.2 pareto_1 pareto_5"
OFILE="pareto.txt"

TFILE=temp.tmp
n=0

for i in {1..1000}; do
	echo $i | awk '{printf("%.4f\n", $1/1000)}' >> $TFILE
done

for FI in $FILE; do
	start=(`head -n 1 $FI.txt` + 0 )
	cat $FI.txt | awk '{printf("%.10f\n",$1/10000)}' | sort -n  > $n.tmp
	n=$(($n + 1))
done

echo "percent ALP=0.2 ALP=1 ALP=5" > $OFILE 
paste $TFILE 0.tmp 1.tmp 2.tmp >> $OFILE
rm *.tmp
