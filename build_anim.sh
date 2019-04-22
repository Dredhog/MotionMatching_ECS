#!/bin/bash

build='builder/builder'
src_dir='data/animations/'
dst_dir='data/animations/'
model_sr_dir='data/models_actors/'

for i in $dst_dir/*.anim $dst_dir/*.actor; do
  rm $i
done

$build 'data/animations/91_01.bvh' 'data/built/91' '--actor' '--root_bone' 'Hips' '--scale' '0.056444'

for i in $src_dir*.bvh; do
  $build $i $dst_dir`basename $i .bvh` '--animation' '--target_actor' 'data/built/91.actor' '--undersample_period' '12'
done

$build 'data/models_actors/xbot.fbx' 'data/built/xbot' '--actor' '--root_bone' 'mixamorig:Hips' '--scale' '0.01'

for i in $src_dir*.fbx; do
  $build $i $dst_dir`basename $i .fbx` '--animation' '--root_bone' 'mixamorig:Hips' '--target_actor' 'data/built/xbot.actor'
done
