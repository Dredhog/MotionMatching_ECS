#!/bin/bash

build='builder/builder'
src_dir='data/animations/'
dst_dir='data/animations/'
sampling_frequency='--sample_rate 15'

for i in $dst_dir/*.anim; do
  rm $i
done

$build 'data/models_actors/xbot.fbx' 'data/built/xbot' '--actor' '--root_bone' 'mixamorig:Hips' '--scale' '0.01'

for i in $src_dir*.fbx; do
	if [[ $i == *"xbot"* ]]; then
		echo "building " $i
		$build $i $dst_dir`basename $i .fbx` '--animation' '--root_bone' 'mixamorig:Hips' '--target_actor' 'data/built/xbot.actor' $sampling_frequency
	fi
done

for i in $src_dir*.dae; do
	if [[ $i == *"xbot"* ]]; then
		echo "building " $i
		$build $i $dst_dir`basename $i .dae` '--animation' '--root_bone' 'mixamorig:Hips' '--target_actor' 'data/built/xbot.actor'
	fi
done

$build 'data/models_actors/ybot.fbx' 'data/built/ybot' '--actor' '--root_bone' 'mixamorig:Hips' '--scale' '0.01'

for i in $src_dir*.fbx; do
	if [[ $i == *"ybot"* ]]; then
		echo "building " $i
		$build $i $dst_dir`basename $i .fbx` '--animation' '--root_bone' 'mixamorig:Hips' '--target_actor' 'data/built/ybot.actor' $sampling_frequency
	fi
done

for i in $src_dir*.dae; do
	if [[ $i == *"ybot"* ]]; then
		echo "building " $i
		$build $i $dst_dir`basename $i .dae` '--animation' '--root_bone' 'mixamorig:Hips' '--target_actor' 'data/built/ybot.actor'
	fi
done
