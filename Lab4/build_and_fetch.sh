#!/bin/bash

LOCAL_PROJECT_DIR="./"
REMOTE_PROJECT_DIR="/home/xingjunyang/os_lab/lab4"
REMOTE_USER="xingjunyang"
REMOTE_HOST="172.190.138.248"
COMPILE_LOG="compile.log"
IMG="a.img"

ssh -i /Users/xingjunyang/Documents/School/大二下/OS/实验/Lab3/key.pem $REMOTE_USER@$REMOTE_HOST "rm $REMOTE_PROJECT_DIR/$COMPILE_LOG"

rsync -e "ssh -i /Users/xingjunyang/Documents/School/大二下/OS/实验/Lab3/key.pem" -i /Users/xingjunyang/Documents/School/大二下/OS/实验/Lab3/key.pem -av --exclude '*.o' --exclude '.*' --exclude '.*/' --exclude '*.bin' $LOCAL_PROJECT_DIR $REMOTE_USER@$REMOTE_HOST:$REMOTE_PROJECT_DIR

ssh -i /Users/xingjunyang/Documents/School/大二下/OS/实验/Lab3/key.pem $REMOTE_USER@$REMOTE_HOST "cd $REMOTE_PROJECT_DIR && make image > $COMPILE_LOG 2>&1"

scp -i /Users/xingjunyang/Documents/School/大二下/OS/实验/Lab3/key.pem $REMOTE_USER@$REMOTE_HOST:$REMOTE_PROJECT_DIR/$COMPILE_LOG $LOCAL_PROJECT_DIR

scp -i /Users/xingjunyang/Documents/School/大二下/OS/实验/Lab3/key.pem $REMOTE_USER@$REMOTE_HOST:$REMOTE_PROJECT_DIR/$IMG $LOCAL_PROJECT_DIR

cat $COMPILE_LOG

