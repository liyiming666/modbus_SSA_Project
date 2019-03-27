#/bin/bash

killProgress() {
    PROCESS_NAME=$1
    PROCESS_COUNT=`ps -ef |grep $PROCESS_NAME |grep -v "grep" |wc -l`

    echo $PROCESS_COUNT
    if [ 0 != $PROCESS_COUNT ];then
        echo "kill " + $PROCESS_NAME + " process..."
        ps -efww | grep $PROCESS_NAME | grep -v grep | cut -c 9-15 | xargs kill -2
        echo "restart " + $PROCESS_NAME= + " process"
    fi
}

PROCESS_NAME="ssagent_hsjd"
killProgress $PROCESS_NAME      # call funtion to kill agent progress

export LD_LIBRARY_PATH=../cumulocity-sdk-c/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=../lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

#./bin/ssagent_hsjd
#./bin/srwatchdogd ./bin/ssagent_hsjd 100 > log/ssa.log 2>&1 &
./bin/srwatchdogd ./bin/ssagent_hsjd 100 > log/dtu.log 2>&1 &
