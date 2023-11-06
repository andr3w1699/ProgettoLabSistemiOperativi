echo test benchmark
start=`date +%s.%N`
./farm -n 4 -d testdir -q 8  file*  &
pid=$! 
wait $pid
end=`date +%s.%N`
runtime=$( echo "$end - $start" | bc -l )
echo runtime : $runtime
