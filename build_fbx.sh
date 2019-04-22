#!/bin/bash

build='builder/builder'
src_dir='data/animations/'
dst_dir='data/animations/'

$build 'data/models_actors/xbot.fbx' 'data/built/xbot' '--actor' '--root_bone' 'mixamorig:Hips' '--scale' '0.01'

for i in $src_dir*.fbx; do
  $build $i $dst_dir`basename $i .fbx` '--animation' '--root_bone' 'mixamorig:Hips' '--target_actor' 'data/built/xbot.actor' '--print_scene'
done
