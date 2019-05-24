#!/bin/bash

build='builder/builder'
src_dir='data/models_actors/'
dst_dir='data/built/'

for i in $dst_dir/*.model $dst_dir/*.actor; do
  rm $i
done

for i in $src_dir*.obj; do
  $build $i $dst_dir`basename $i .obj` '--model' '--actor'
done

for i in $src_dir*.dae; do
	$build $i $dst_dir`basename $i .dae` '--model' '--actor'
done
