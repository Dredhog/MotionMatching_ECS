from matplotlib import pyplot as plt
import pandas as pd
import seaborn as sns
import numpy as np
sns.set(color_codes = True)

rel_dir = './data/measurements/'

for f in ['expressive_follow.csv','expressive_follow_mirror.csv']:
    follow_data = pd.read_csv(rel_dir+f, skipinitialspace=True)

    mean_dist_from_line = np.mean(follow_data['distance_from_segment'])
    mean_dist_from_spline = np.mean(follow_data['distance_from_spline'])
    mean_angle_from_line = np.mean(follow_data['signed_angle_from_segment'])
    mean_angle_from_spline = np.mean(follow_data['signed_angle_from_spline'])

    plt.figure()
    plt.suptitle(f);
    plt.subplot(2, 2, 1)
    plt.title("distance from segments")
    plt.plot(follow_data['t'], follow_data['distance_from_segment'])
    plt.plot(follow_data['t'], np.repeat(mean_dist_from_line, follow_data.index.size))
    plt.subplot(2, 2, 2)
    plt.title("distance from splines")
    plt.plot(follow_data['t'], follow_data['distance_from_spline'])
    plt.plot(follow_data['t'], np.repeat(mean_dist_from_spline, follow_data.index.size))
    plt.subplot(2, 2, 3)
    plt.title("angle away from segments")
    plt.plot(follow_data['t'], follow_data['signed_angle_from_segment'])
    plt.plot(follow_data['t'], np.repeat(mean_angle_from_line, follow_data.index.size))
    plt.subplot(2, 2, 4)
    plt.title("angle away from splines")
    plt.plot(follow_data['t'], follow_data['signed_angle_from_spline'])
    plt.plot(follow_data['t'], np.repeat(mean_angle_from_spline, follow_data.index.size))
    plt.show();

plt.figure()
for i, v in enumerate(['clean_facing.csv','clean_facing_mirror.csv', 'expressive_facing.csv', 'expressive_facing_mirror.csv', '91_clean_facing_mirror.csv', '69_front_and_back_facing_mirror.csv']):
    facing_data = pd.read_csv(rel_dir+v, skipinitialspace=True)

    plt.subplot(6, 1, i+1)
    #plt.title(f+ str(facing_data['angle_threshold'].iloc(0)))
    plt.title(v)
    plt.scatter(facing_data['angle'], facing_data['turn_time'])
plt.show()

#'clean_ctrl_skate_mirror.csv', 
for f in ['test_ctrl_skate_mirror.csv']:
    foot_skate = pd.read_csv(rel_dir+f, skipinitialspace=True)

    plt.figure()
    plt.suptitle(f)

    plt.subplot(3, 1, 1)
    plt.title("left foot velocity magnitude")
    plt.plot(foot_skate['t'], foot_skate['l_h'])
    plt.plot(foot_skate['t'], np.sqrt(np.square(foot_skate['l_vel_x'])+np.square(foot_skate['l_vel_z'])))

    plt.subplot(3, 1, 2)
    est_func = lambda  x : len(x) / len(foot_skate)
    sns.barplot(x='anim_count', y='anim_count', orient='v', data=foot_skate, estimator=est_func)

    plt.subplot(3, 1, 3)
    sns.barplot(x='anim_index', y='anim_index', orient='v', data=foot_skate, estimator=est_func)

    plt.show()
