#!/bin/bash

TIER=$1
shift
FILE=$1

[[ ! -z ${TIER} ]] || exit 1
[[ ! -z ${FILE} ]] || exit 1

run_command() {
	RET=$(scp ${FILE} root@${1}: 2> /dev/null)
}

pids=()

for HOST in $(smcc ls-hosts -r ${TIER})
do
	run_command ${HOST} &
	pids+=( $! )
done

for pid in "${pids[@]}"; do
	wait $pid
done
