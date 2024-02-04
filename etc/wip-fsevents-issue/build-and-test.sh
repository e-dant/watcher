#! /usr/bin/env bash
(
  cd "$(dirname "$0")"
  ./build.sh
  n=0
  ok=0
  while [ $ok -eq 0 ]
  do
    ./fsevents-issue
    ok=$?
    n=$((n + 1))
  done
  echo $n before crash
)

