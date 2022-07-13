#!/bin/sh

for filename in $(ls ./testSource/functional/*.sy); do
  ./build/bddd $filename
  ret=$?
  if [ $ret -ne 0 ]; then
    echo "GG in $filename"
    exit 0
  fi
done
echo "NICE"
exit 0

