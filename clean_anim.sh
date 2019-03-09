#!/bin/bash

dst_mod='./data/animations/'

for i in $dst_mod/*.anim; do
  rm $i
done
