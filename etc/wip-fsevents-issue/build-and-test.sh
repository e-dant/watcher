#! /usr/bin/env bash
(
  cd "$(dirname "$0")"
  ./build.sh
  for p in fsevents-issue-c fsevents-issue-cpp
  do
    n=0
    ok=0
    lim=1
    while [[ $ok -eq 0 && $n -lt $lim ]]
    do
      if ! echo "$*" | grep -qoE -- --skip-$p-prog
      then ./$p || ok=1
      else echo "(skipping $p)"
      fi
      n=$((n + 1))
    done
    if [ $ok -eq 0 ]
    then
      echo "$p: seems ok"
    else
      echo "$p: $n before crash"
    fi
  done
)

