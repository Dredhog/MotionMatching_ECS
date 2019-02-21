#!/bin/bash

dst_mod='./data/built/'

for i in $dst_mod/*.model $dst_mod/*.actor; do
  rm $i
done
