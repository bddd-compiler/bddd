#!/bin/sh

for filename in $(ls ./testSource/functional/*.sy); do
  ./build/bddd -S -o ${filename%.sy}.s $filename -O2
  ret=$?
  if [ $ret -ne 0 ]; then
    echo "GG in $filename"
    exit 0
  fi
done
echo "NICE"
exit 0
