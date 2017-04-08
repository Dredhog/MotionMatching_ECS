#!/bin/bash

src_dir='data/models_actors/'
dst_dir='data/built/'

for i in $dst_dir/*.model $dst_dir/*.actor; do
  rm $i
done
