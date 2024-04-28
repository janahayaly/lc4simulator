# testing script for CIS 2400 Project 1 Part 1
# make sure this script is in your main working directory 
# (with your .c files etc) and run:
# ./test_p1.sh
# if you get permission denied, run
# chmod +x test_p1.sh
# and try again


# find .obj test cases and .txt solutions
# and get their names in two arrays
ls p1_test_cases | grep .obj > /tmp/obj 
ls p1_test_cases | grep .txt > /tmp/txt

declare -a obj_files
declare -a txt_files

let "i=0"
while read obj
do
    obj_files[i]=$obj
    let "i=i+1"
done < /tmp/obj 

let "i=0"
while read txt
do 
    txt_files[i]=$txt 
    let "i=i+1"
done < /tmp/txt 

# function for printing result
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'
BOLD='\033[1m'
NORMAL='\033[0m'

function result() {
    if [ $1 = "Success" ]; then
        printf "${GREEN}${BOLD}Success${NC}${NORMAL}\n"
    fi
    if [ $1 = "Failure" ]; then 
        printf "${RED}${BOLD}Failure${NC}${NORMAL}\n"
    fi
}

# function to check outut of diff command
function diff_to_success() {
    if [ "$1" = "" ]; then
        printf "Success"
    else                                                                           
        printf "Failure"
    fi  
}

# run tests
let "tests=i"
let "obj=0"
let "txt=0"
echo "--------------------------------------------"
while [ $txt -lt $tests ];
do 
    # skip os.obj (it isn't a test)
    if [ ${obj_files[obj]} = "os.obj" ]; then
        let "obj=obj+1"
    fi
    # echo the test we're running
    echo ${obj_files[obj]}
    # see if the test passes without os.obj
    ./trace out.txt p1_test_cases/${obj_files[obj]} > /dev/null
    diffres=$(diff out.txt p1_test_cases/${txt_files[txt]})
    success=$(diff_to_success $diffres)
    # if it fails the first time, try with os.obj
    if [ $success = "Failure" ]; then 
        echo "Failed without os.obj, retrying using os.obj"
        ./trace out.txt p1_test_cases/${obj_files[obj]} p1_test_cases/os.obj > /dev/null
        diffres=$(diff out.txt p1_test_cases/${txt_files[txt]})
        success=$(diff_to_success $diffres)
    fi 
    # echo the test result and go to the next test
    result $success
    let "obj=obj+1"
    let "txt=txt+1"
    echo "--------------------------------------------"
    rm out.txt
done 

# clean up
rm /tmp/obj
rm /tmp/txt
