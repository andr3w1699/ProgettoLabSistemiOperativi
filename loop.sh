#!/bin/bash

# Function to run test.sh and wait for its completion
run_test() {
    ./test.sh
    wait $!
}

# Loop 50 times
for i in {1..50}
do
    echo running test 
    # Run test.sh and wait for its completion
    run_test
    
    # Optional: Add a delay between each execution (e.g., 1 second)
    #sleep 1
done

