FILE="lognormal_0.1 lognormal_0.25 lognormal_0.5"
OFILE="lognormal.txt"

TFILE=temp.tmp
n=0

for i in {1..1000}; do
	echo $i | awk '{printf("%.4f\n", $1/1000)}' >> $TFILE
done

for FI in $FILE; do
	start=(`head -n 1 $FI.txt` + 0 )
	cat $FI.txt | awk '{printf("%.9f\n",$1/1000)}' | sort -n  > $n.tmp
	n=$(($n + 1))
done

echo "percent S=1 S=10 S=50" > $OFILE 
paste $TFILE 0.tmp 1.tmp 2.tmp >> $OFILE
rm *.tmp
