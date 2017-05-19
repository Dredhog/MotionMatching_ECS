#!/bin/bash

dir='data/materials/'

dirfound=0
newmat=""

for file in $dir*.mtl; do
  while read line;
  do
    if [ "$(echo $line | grep "newmtl")" ]
    then
      newmat=$dir$(echo $line | cut -d ' ' -f 2)".mat"
      if [ -e $newmat ]
      then
        dirfound=0
        echo "File "$newmat" already exists!"
      else
        touch $newmat
        dirfound=1
      fi
    fi
    if [ $dirfound -eq 1 ]
    then
      if [ "$line" != "\n" ]
      then
        if [ -n "$line" ]
        then
          echo $line >> $newmat
        fi
      fi
    fi
  done < $file
done
