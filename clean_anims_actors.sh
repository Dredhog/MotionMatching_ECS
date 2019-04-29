#!/bin/bash

dst_anim_dir='data/animations/'
dst_actor_dir='data/built/'

for i in $dst_anim_dir/*.anim $dst_actor_dir/*.actor; do
  rm $i
done
