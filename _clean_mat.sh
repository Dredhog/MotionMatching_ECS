#!/bin/bash

dst_mat='data/materials/'

for i in $dst_mat/*.mat; do
  rm $i
done
