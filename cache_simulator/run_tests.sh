#!/bin/bash

arg_file="test_cases.txt"    
expected_output_dir="ref_outputs"  
generated_output_dir="generated_outputs"
binary="../CS21B029_assignment1/temp"

if [ -d "$generated_output_dir" ]; then
    rm -rf "$generated_output_dir"
fi

mkdir -p "$generated_output_dir"

case_number=0

while IFS= read -r args; do
    output_file="$generated_output_dir/output_$case_number.txt"
    echo "======================================================================="
    echo "Running: $binary $args"
    "$binary" $args > "$output_file"

    expected_output_file="$expected_output_dir/gcc.output$case_number.txt"

    if diff -iw "$output_file" "$expected_output_file" > /dev/null; then
        echo "Test case $case_number: Output matches."
    else
        echo "Test case $case_number: Output differs."
        echo "Differences:"
        diff -iw "$output_file" "$expected_output_file"
    fi
    ((case_number++))
    echo "======================================================================="
done < "$arg_file"
# rm -rf "$generated_output_dir"

