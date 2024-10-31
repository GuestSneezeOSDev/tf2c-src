#!/bin/bash
# sappho.io

# colors
source ./ci/bash-helpers.sh

# go to our pre setup tf2c server
cd /home/gitlab-runner/tf2c-CITEST/ || exit 42

# This validates sdk2013
# if ! bash updatesdk.sh; then
#     echo "FAILED checking SDK2013 validity!"
#     exit 1
# fi

conlog_loc="./tf2classic/conlog.log"
servercfg_log="./tf2classic/cfg/server.cfg"

# null console log
true                                    > ${conlog_loc}


echo "changelevel pl_upward"            > ./tf2classic/cfg/itemtest.cfg
echo "echo \"hello from pl_upward\""    > ./tf2classic/cfg/pl_upward.cfg


# null servercfg before writing to it
true                                    >  ${servercfg_log}
echo "echo \"hello from server.cfg\""   >> ${servercfg_log}
echo "stats"                            >> ${servercfg_log}
echo "status"                           >> ${servercfg_log}
echo "meta version"                     >> ${servercfg_log}
echo "sm version"                       >> ${servercfg_log}


# run our server in a tmux window with con_logfile
# then check that logfile to make sure everything's in order
tmux kill-session -t tf2c-ci; tmux new-session -d -s tf2c-ci bash
tmux send-keys \
'./sdk2013/srcds_run -console -game ../tf2classic +hostname tf2c-ci-server +map itemtest +maxplayers 32 +hide_server 1 +ip 0.0.0.0 +status -norestart +con_logfile conlog.log' C-m

# wait 30 seconds before checking to give the server time to breathe
sleep 30

# spew it to output regardless
cat ./tf2classic/conlog.log

if ! pgrep -f "\+hostname tf2c-ci-server" &> /dev/null; then
    error "Server crashed somewhere"
    exit 1
elif ! grep "hello from server.cfg" ${conlog_loc} &> /dev/null; then
    error "Failed to print echo from server.cfg"
    exit 1
elif ! grep "hello from pl_upward" ${conlog_loc} &> /dev/null; then
    error "Failed somewhere before getting to pl_upward"
    exit 1
elif ! grep "Metamod:Source Version Information" ${conlog_loc} &> /dev/null; then
    error "Failed loading Metamod:Source"
    exit 1
elif ! grep "SourceMod Version Information" ${conlog_loc} &> /dev/null; then
    error "Failed loading SourceMod"
    exit 1
elif ! grep "Little Anti-Cheat" ${conlog_loc} &> /dev/null; then
    error "Failed loading LilAC"
    exit 1
else
    ok "Successfully booted server. Great job!"
    exit 0
fi

