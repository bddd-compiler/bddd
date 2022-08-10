#!/bin/sh

for filename in $(ls ./testSource/functional/*.c); do
  ./build/compiler -S -o ${filename%.c}.s $filename -O2
  ret=$?
  if [ $ret -ne 0 ]; then
    echo "GG in $filename"
    exit 0
  fi
done
echo "NICE"
exit 0