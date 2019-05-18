#!/bin/bash

build='builder/builder'
src_dir='data/animations/'
dst_dir='data/animations/'
actor_dir='data/built/'
ybot_dir='data/animations/ybot_retargeted/fbx/'
sampling_frequency='--sampling_frequency 120'

$build 'data/animations/69_01.bvh' 'data/built/69' '--actor' '--root_bone' 'Hips' '--scale' '0.056444'

for i in $src_dir*.bvh; do
	if [[ $i == *"69"*".bvh" ]]; then
		echo "building " $i
		$build $i $dst_dir`basename $i .bvh` '--animation' '--target_actor' 'data/built/69.actor' $sampling_frequency
	fi
done

$build 'data/animations/127_01.bvh' 'data/built/127' '--actor' '--root_bone' 'Hips' '--scale' '0.056444'

for i in $src_dir*.bvh; do
	if [[ $i == *"127"*".bvh" ]]; then
		echo "building " $i
		$build $i $dst_dir`basename $i .bvh` '--animation' '--target_actor' 'data/built/127.actor' $sampling_frequency
	fi
done

printf "\n"
$build 'data/animations/91_01.bvh' 'data/built/91' '--actor' '--root_bone' 'Hips' '--scale' '0.056444'
printf "\n"

$build 'data/animations/91_01.bvh' 'data/built/01_91' '--actor' '--root_bone' 'Hips' '--scale' '0.056444' $sampling_frequency
printf "\n"

for i in $src_dir*.bvh; do
	if [[ $i == *"91"*".bvh" ]]; then
		echo "building " $i
  	$build $i $dst_dir`basename $i .bvh` '--animation' '--target_actor' 'data/built/91.actor' $sampling_frequency
	fi
done
