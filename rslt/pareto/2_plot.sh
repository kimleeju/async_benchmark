operation="pareto"

for op in $operation;do
	datafile=""$op".txt"
	outfile=""$op".eps"
	
    	opt_list="`echo '-'``echo 'e'` `echo datafile=\'$datafile\'`"
    	opt_list="$opt_list `echo '-'``echo 'e'` `echo outfile=\'$outfile\'`"
    	opt_list="$opt_list `echo '-'``echo 'e'` `echo gtitle=\'$graphtitle\'`"

		first=`head -n 1 $datafile | awk '{print $2}'`
    	second=`head -n 1 $datafile | awk '{print $3}'`
    	third=`head -n 1 $datafile | awk '{print $4}'`
		opt_list="$opt_list `echo '-'``echo 'e'` `echo gfirst=\'$first\'`"
		opt_list="$opt_list `echo '-'``echo 'e'` `echo gsecond=\'$second\'`"
		opt_list="$opt_list `echo '-'``echo 'e'` `echo gthird=\'$third\'`"
		opt_list="$opt_list `echo '-'``echo 'e'` `echo gxmax=\100`"
		echo "gnuplot $opt_list perf.cfg" 
    	gnuplot $opt_list perf.cfg
    	open $outfile
#	done
done
