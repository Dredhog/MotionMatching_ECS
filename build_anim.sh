#!/bin/bash

build='builder/builder'
src_dir='data/animations/'
dst_dir='data/animations/'

for i in $dst_dir/*.anim $dst_dir/*.actor; do
  rm $i
done

for i in $src_dir*.bvh; do
  $build $i $dst_dir`basename $i .bvh` '--animation' '--root_bone' 'Hips' '--scale' '0.056444'
done
