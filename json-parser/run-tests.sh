if [[ $# -ne 2 ]]
then
    echo "Usage: $0 TestExecutablePath TestDirectoryPath"
    exit 1
fi

exe_path=`realpath $1`
tests_dir=`realpath $2`

if [ ! -f $exe_path ]
then
    echo "$exe_path is not a file"
    exit 1
fi
if [ ! -d $tests_dir ]
then
    echo "$tests_dir is not a directory"
    exit 1
fi

cd $tests_dir
for file in `ls`
do
    echo "Running test for $file"
    starttime=$SECONDS
    
    status=`$exe_path $file 2>/dev/null`
    
    endtime=$SECONDS
    difftime=$(( endtime - starttime ))
    if [[ "$file" == fail*.json ]]
    then
        if [[ "$status" == FAILED* ]]
        then
            echo -e "$file\t$difftime s\t\033[0;32mExpected: FAILED\tActual: FAILED\t=> PASS\033[0m"
        else
            echo -e "$file\t$difftime s\t\033[0;31mExpected: FAILED\tActual: PASSED\t=> FAIL\033[0m"
        fi
    else
        if [[ "$status" == PASSED* ]]
        then
            echo -e "$file\t$difftime s\t\033[0;32mExpected: PASSED\tActual: PASSED\t=> PASS\033[0m"
        else
            echo -e "$file\t$difftime s\t\033[0;31mExpected: PASSED\tActual: FAILED\t=> FAIL\033[0m"
        fi
    fi
done
cd ..