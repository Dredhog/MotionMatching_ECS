#!/bin/bash

build='builder/builder'
src_dir='data/animations/'
dst_dir='data/animations/'
actor_dir='data/built/'
ybot_dir='data/animations/ybot_retargeted/fbx/'
sampling_frequency='--sample_rate 30'

for i in $dst_dir/*.anim $actor_dir*.actor; do
  rm $i
done

$build 'data/models_actors/xbot.fbx' 'data/built/xbot' '--actor' '--root_bone' 'mixamorig:Hips' '--scale' '0.01'

for i in $src_dir*.fbx; do
	if [[ $i == *"xbot"* ]]; then
		echo "building " $i
		$build $i $dst_dir`basename $i .fbx` '--animation' '--target_actor' 'data/built/xbot.actor' $sampling_frequency
	fi
done
$build 'data/models_actors/ybot.fbx' 'data/built/ybot' '--actor' '--root_bone' 'mixamorig:Hips' '--scale' '0.01'

for i in $src_dir*.fbx; do
	if [[ $i == *"ybot"* ]]; then
		echo "building " $i
		$build $i $dst_dir`basename $i .fbx` '--animation' '--target_actor' 'data/built/ybot.actor' $sampling_frequency
	fi
done

for i in $ybot_dir*.fbx; do
	echo "building " $i
	$build $i $dst_dir`basename $i .fbx` '--animation' '--target_actor' 'data/built/ybot.actor' $sampling_frequency
done

$build 'data/animations/69_01.bvh' 'data/built/69' '--actor' '--root_bone' 'Hips' '--scale' '0.056444'

for i in $src_dir*.bvh; do
	if [[ $i == *"69"*".bvh" ]]; then
		echo "building " $i
		$build $i $dst_dir`basename $i .bvh` '--animation' '--target_actor' 'data/built/69.actor'
	fi
done
