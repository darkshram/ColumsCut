#! /bin/sh

EXIT=0

Title()
{
echo -e "\n\033[01m ############### $* \033[00;39m"
}


OkayMessage()
{
echo -e "  \033[01;32mOKAY\033[00;39m    $1"
}

FailMessage()
{
echo -e "  \033[01;31mFAIL\033[00;39m    $1"
EXIT=3    # Bash says 1 and 2 are reserved for specific errors
}


TestCut()
{
RESULT=`cat $2 | ./ccut $1`
EXPECTED="$3"

if [ "$RESULT" = "$EXPECTED" ]
then
	OkayMessage "$4 works"
else
	FailMessage "$4 FAILED [$RESULT]"
fi

}



Title "Testing 'ccut'"
TestCut "-f 3" tests/cut.1 field3 "Cut a single field"
TestCut "-f 2-" tests/cut.1 "field2	field3	field4	field5	field6	field7" "Cut from a field to end of line"
TestCut "-f -4" tests/cut.1 "field1	field2	field3	field4" "Cut from start of line to a field"
TestCut "-f 2-5" tests/cut.1 "field2	field3	field4	field5" "Cut range of fields"
TestCut "-f 5-2" tests/cut.1 "field5	field4	field3	field2" "Cut reverse range of fields"
TestCut "-f 2,5,7" tests/cut.1 "field2	field5	field7" "Cut multiple discontinuous fields"
TestCut "-f 2-4 --complement" tests/cut.1 "field1	field5	field6	field7" "Cut complementary fields"
TestCut "-f 3,1,6,5" tests/cut.1 "field3	field1	field6	field5" "Cut multiple fields, rearranging their order"
TestCut "-f 3,1,7,5" tests/cut.1 "field3	field1	field7	field5" "Cut multiple fields, rearranging their order, with final field included"
TestCut "-T , -f 1-4" tests/cut.1 "field1,field2,field3,field4" "Cut with different delimiter for output than input"
TestCut "-T , -e '\t;,' -f 1-4" tests/cut.2 "field1,field2,field3,field4" "Cut using multiple delimiters"
TestCut "-T , -e '\t' -d ';,' -f 1-4" tests/cut.2 "field1,field2,field3,field4" "Cut using multiple -d options"
TestCut "-T , -e '\t;,' -q -f 1-6" tests/cut.2 "field1,field2,field3,field4,field5\,field5.5,\"field6 with a comma , in it\"" "Cut honoring quotes"
TestCut "-Q -d ',' -f 3,6" tests/cut.5 "field3,field6" "Cut honoring quotes but stripping them from output"
TestCut "-d , -j -f 3" tests/cut.3 "field3" "Cut combining runs of the same delimiter"
TestCut "-d ,;: -j -f 6" tests/cut.3 "field6" "Cut combining runs of different delimiters"
TestCut "-D ,, -f 2" tests/cut.3 ",field3,field4" "Cut using a string as a delimiter rather than a single char"
TestCut "-Q -d , -f 2,3,20" tests/cut.5 "field2,field3," "Cut with a non-existent field at the end"
TestCut "-Q -d , -f 2,20,3" tests/cut.5 "field2,,field3" "Cut with a non-existent field in the middle"
TestCut "-Q -d , -f 20,2,3" tests/cut.5 ",field2,field3" "Cut with a non-existent field at the start"
TestCut "-Q -d , -f 2,3,10-15" tests/cut.5 "field2,field3,,,,,," "Cut with a range of non-existent fields"
TestCut "-c 80-95 --utf8" tests/utf8.txt "Congress‘ infras" "Cut UTF-8 input"
TestCut "-s -d '*' -f 1 " tests/multiline.txt "1. this should be included
3. this should be included" "suppress lines without delimiter"

eval `cat tests/cut.1 | ./ccut -f 7,5,2 -V arg1,arg2,arg3`
if [ "$arg1" = "field7" -a "$arg2" = "field5" -a "$arg3" = "field2" ]
then
	OkayMessage "Cut fields into variables using eval works"
else
	FailMessage "Cut fields into variables using eval FAILED"
fi 

exit $EXIT
