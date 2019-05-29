#!/bin/bash

build='builder/builder'
src_dir='data/animations/'
dst_dir='data/animations/'
actor_dir='data/built/'
ybot_dir='data/animations/ybot_retargeted/fbx/'
sampling_frequency='--sampling_frequency 120'

$build 'data/animations/small_step.fbx' 'data/built/balvonas' '--actor' '--root_bone' 'Root' '--scale' '1'

for i in $src_dir*.fbx; do
	if [[ $i == *".fbx" ]]; then
		echo "building " $i
		$build $i $dst_dir`basename $i .bvh` '--animation' '--target_actor' 'data/built/balvonas.actor' $sampling_frequency
	fi
done
