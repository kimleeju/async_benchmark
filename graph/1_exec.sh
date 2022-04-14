DIR="../rslt/poisson/"
FILE="poisson_1 poisson_10 poisson_50"
OFILE=$DIR/poisson.txt

TFILE=temp.tmp
n=0

for i in {1..1000}; do
	echo $i | awk '{printf("%.4f\n", $1/1000)}' >> $TFILE
done

for FI in $FILE; do
	start=(`head -n 1 $DIR/$FI.txt` + 0 )
	cat $DIR/$FI.txt | awk '{printf("%.6f\n", ($1-'"$start"')/1000000)}' > $n.tmp
	n=$(($n + 1))
done

echo "percent L=1 L=10 L=50" > $OFILE 
paste $TFILE 0.tmp 1.tmp 2.tmp >> $OFILE
rm *.tmp
