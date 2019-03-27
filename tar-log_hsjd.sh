#! /bin/bash                                                                

cd /opt/ssa/htyh/SSA_hsjd/log

DATE=`date +"%Y-%m-%d" -d "-24hour"`
ALL_DATE="ssa.log."$DATE
CMD="tar zcvf $ALL_DATE".tar.gz" $ALL_DATE"*" --remove-files"
#echo $CMD
$CMD
