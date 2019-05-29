from matplotlib import pyplot as plt
import pandas as pd
import seaborn as sns
import numpy as np
import pylab as pl
import itertools
sns.set(color_codes = True)

rel_dir = './data/measurements/'
figure_directory = './data/figures/'

trajectories = ['segment', 'pentagon', 'M']
mirrors = ['', '_mirror']
m_to_mirror_dict = {'':'', '_m':'_mirror'}

cm_to_inches = 2.54
figure_size = (40/cm_to_inches, 50/cm_to_inches)
trajektorijų_kilmininkas = {'pentagon':'penkiakampio', 'M':'M', 'segment':'atkarpos'}

show_figures = False
write_figures = True

controllers = ['main', '16', '91', '127_stiff']

follow_writer = pd.ExcelWriter('data/figures/trajektoriju_sekimo_matavimai.xlsx')

for c in controllers:
    column_names = ['mean line dist', 'mean spline dist', 'mean line angle', 'mean spline angle']
    trajectory_deviation_table = pd.DataFrame(np.NaN, index=trajectories, columns=column_names)

    for m in ['', '_m']:
        for t in trajectories:
            short_c_name = c+m
            long_c_name = c+m_to_mirror_dict[m]
            follow_data = pd.read_csv(rel_dir+short_c_name+'_follow_'+t+'.csv', skipinitialspace=True)

            mean_dist_from_line = np.mean(follow_data['distance_from_segment'])
            mean_dist_from_spline = np.mean(follow_data['distance_from_spline'])
            abs_angle_from_line = np.abs(follow_data['signed_angle_from_segment'])
            abs_angle_from_spline = np.abs(follow_data['signed_angle_from_spline'])
            mean_angle_from_line = np.mean(abs_angle_from_line)
            mean_angle_from_spline = np.mean(abs_angle_from_spline)

            trajectory_deviation_table[column_names[0]][t] = mean_dist_from_line
            trajectory_deviation_table[column_names[1]][t] = mean_dist_from_spline
            trajectory_deviation_table[column_names[2]][t] = mean_angle_from_line
            trajectory_deviation_table[column_names[3]][t] = mean_angle_from_spline


            plt.figure(figsize=figure_size)
            plt.suptitle('Valdiklio \"' + short_c_name + '\" ' + trajektorijų_kilmininkas[t] + ' trajektorijos sekimo paklaidos');
            plt.subplot(2, 2, 1)
            plt.title("Atstumas iki atkarpos")
            plt.plot(follow_data['t'], follow_data['distance_from_segment'], label='momentinis atstumas')
            plt.plot(follow_data['t'], np.repeat(mean_dist_from_line, follow_data.index.size), label='vidutinis atstumas')
            plt.xlabel("Laikas (s)")
            plt.ylabel("Atstumas (m)")
            plt.legend(loc=1)
            plt.subplot(2, 2, 2)
            plt.title("Atstumas iki Catmull-Rom kreivės")
            plt.plot(follow_data['t'], follow_data['distance_from_spline'], label='momentinis atstumas')
            plt.plot(follow_data['t'], np.repeat(mean_dist_from_spline, follow_data.index.size), label='vidutinis atsumas')
            plt.xlabel("Laikas (s)")
            plt.ylabel("Atstumas (m)")
            plt.legend(loc=1)
            plt.subplot(2, 2, 3)
            plt.title("Absoliutus kampas su atkarpa")
            #plt.plot(follow_data['t'], follow_data['signed_angle_from_segment'])
            plt.plot(follow_data['t'], abs_angle_from_line, label='momentinis kampas')
            plt.plot(follow_data['t'], np.repeat(mean_angle_from_line, follow_data.index.size), label='vidutinis kampas')
            plt.xlabel("Laikas (s)")
            plt.ylabel("Nuokrypio kampas ("+u'\N{DEGREE SIGN}'+")")
            plt.legend(loc=1)
            plt.subplot(2, 2, 4)
            plt.title("Absoliutus kampas su Catmull-Rom kreivės liestine")
            #plt.plot(follow_data['t'], follow_data['signed_angle_from_spline'])
            plt.plot(follow_data['t'], abs_angle_from_spline, label='momentinis kampas')
            plt.plot(follow_data['t'], np.repeat(mean_angle_from_spline, follow_data.index.size), label='vidutinis kampas')
            plt.ylabel("Nuokrypio kampas ("+u'\N{DEGREE SIGN}'+")")
            plt.legend(loc=1)
            if write_figures:
                plt.savefig(figure_directory+short_c_name+'_nuokrypis.png');
            if show_figures:
                plt.show()
            plt.close('all')
            trajectory_deviation_table.round(3).to_excel(follow_writer, short_c_name)
follow_writer.save()
follow_writer.close()

controllers = ['main', '16', '91', '127_stiff']
column_names =  controllers + [i+'_m' for i in controllers]
plt.figure(figsize=figure_size)
#facing_change_time_table = pd.DataFrame(np.NaN, index=np.arange(2*len(controllers)), columns=['min reach time', 'mean time to reach', 'max reach time'])
facing_change_time_table = pd.DataFrame(np.NaN, index=column_names, columns=['min reach time', 'mean time to reach', 'max reach time'])
for i, c in enumerate(controllers):
    plt.subplot(len(controllers), 1, i+1)
    plt.title('Valdiklio \"'+c+'\" žiūrėjimo tiklso kampo pasiekimo laikas')
    for j, m in enumerate(['', '_m']):
        short_c_name = c+m
        facing_data = pd.read_csv(rel_dir+short_c_name+'_facing.csv', skipinitialspace=True)

        index = short_c_name #i*len(['','_m'])+j
        plt.scatter(facing_data['angle'], facing_data['turn_time'], label=['be veidrodinių animacijų', 'su veidrodinėmis animacijomis'][j])
        plt.xlabel("Testuojamas nuokrypio kampas ("+u'\N{DEGREE SIGN}'+")")
        plt.ylabel("Konvergavimas iki "+str(int(facing_data['angle_threshold'][0]))+" " +u'\N{DEGREE SIGN}'+" (s)")
        facing_change_time_table['min reach time'][index] = np.min(facing_data['turn_time'])
        facing_change_time_table['mean time to reach'][index] = np.mean(facing_data['turn_time'])
        facing_change_time_table['max reach time'][index] = np.max(facing_data['turn_time'])
    plt.legend(loc=1);
if write_figures:
    plt.savefig(figure_directory+c+'_kampas.png');
if show_figures:
    plt.show()
plt.close('all')
print(facing_change_time_table)
facing_writer = pd.ExcelWriter('data/figures/atsisukimo_laiko_matavimai.xlsx')
facing_change_time_table.round(3).to_excel(facing_writer)
facing_writer.save()
facing_writer.close()

def measure_foot_skate(foot_skate, min_h, max_h, foot_side):
    h_diff = max_h-min_h
    foot_skate['speed'] = np.sqrt(np.square(foot_skate[foot_side+'_vel_x'])+np.square(foot_skate[foot_side+'_vel_z']))
    foot_skate['position_differences'] = foot_skate['dt'] * foot_skate['speed']
    foot_skate['height_exponent'] = (foot_skate[foot_side+'_h'] - min_h)/h_diff
    foot_skate['clamped_height_exponent'] = np.clip(foot_skate['height_exponent'], 0, 1 )
    foot_skate['height_weights'] = 2-np.power(2,foot_skate['clamped_height_exponent'])
    #mean_pos_difference = np.sum(foot_skate['position_differences'])/foot_skate['t'].tail(1)
    #print(foot_skate["t"].tail(1))
    return float(np.sum(foot_skate['position_differences']*foot_skate['height_weights'])/foot_skate["t"].tail(1))

min_h = 0.045 #np.min(foot_skate.l_h)
max_h = 0.06

controllers = ['main', '16', '91', '127_stiff']
controllers =  controllers + [i+'_m' for i in controllers]

#FOOT SKATE TESTING
foot_skate_table = pd.DataFrame(np.NaN, index=controllers, columns=['l_controller_skate', 'l_anim_skate', 'l_worse_frac', 'r_controller_skate', 'r_anim_skate', 'r_worse_frac'])
for c in controllers:
    anim_names = pd.read_csv(rel_dir+c+'.ctrl_anims', skipinitialspace=True, header=None)
    anim_names = anim_names[0]

    foot_side = 'l'
    for foot_side in ['l', 'r']:
        plt.figure(figsize=figure_size)
        plt.subplots_adjust(hspace=0.5)
        plt.suptitle('\"' + c + '\" valdiklio animacijų rinkinio pėdų slidinjimas')
        anim_skate_amounts = pd.DataFrame(np.NaN, columns=['foot_skate', 'count', 'total_time'],index=anim_names);
        #ANIMATION FOOT SKATE
        for ia, a in enumerate(anim_names):
            plt.subplot(len(anim_names), 1, ia+1)

            anim_skate_data = pd.read_csv(rel_dir+anim_names[ia].split('.')[0]+'_anim_foot_skate.csv', skipinitialspace=True)
            anim_skate_data = anim_skate_data[anim_skate_data['t'] > 0.03]
            a_skate_amount = measure_foot_skate(anim_skate_data, min_h, max_h, foot_side)
            print(a + " " + str(a_skate_amount) + 'm/s')
            anim_skate_amounts['foot_skate'][a] = a_skate_amount

            plt.title("Animacijos "+a+" pėdų slydimo kiekis = " + str(a_skate_amount))
            plt.xlabel('laikas (s)')
            plt.ylabel('Atstumas (m)')
            plt.plot(anim_skate_data['t'], anim_skate_data[foot_side+'_h'], label='kairės pėdos aukštis virš žemės')
            plt.plot(anim_skate_data['t'], anim_skate_data['height_weights'], label='greičio daugiklis')
            plt.legend(loc=1)
            #plt.plot(anim_skate_data['t'], anim_skate_data['speed'])
            #plt.plot(anim_skate_data['t'], anim_skate_data['position_differences'])
            #plt.plot(anim_skate_data['t'], anim_skate_data['height_exponent'])
            #plt.plot(anim_skate_data['t'], anim_skate_data['clamped_height_exponent'])

            print(a + " " + str(a_skate_amount) + 'm/s')
        if write_figures:
            plt.savefig(figure_directory+c+'_'+t+'_animacijų_kojų_slydimas.png');
        if show_figures:
            plt.show()
        plt.close('all')

        #CONTROLLER FOOT SKATE
        short_c_name = c
        c_skate_data = pd.read_csv(rel_dir+short_c_name+'_ctrl_skate.csv', skipinitialspace=True)

        c_skate_amount = measure_foot_skate(c_skate_data, min_h, max_h, foot_side)
        print(c + ' skate: ' + str(c_skate_amount) + 'm/s');

        plt.figure(figsize=figure_size)
        plt.subplots_adjust(hspace=0.5)

        plt.suptitle('Valdiklio \"' + short_c_name +'\" animacijų naudojimas')
        for ia, a in enumerate(anim_names):
            plt.subplot(len(anim_names), 1, ia+1)
            data = [c_skate_data[(c_skate_data['anim_index']==ia) & (c_skate_data['anim_is_mirrored']==0)]['anim_local_time'],
                    c_skate_data[(c_skate_data['anim_index']==ia) & (c_skate_data['anim_is_mirrored']==1)]['anim_local_time']]
            plt.axvspan(0, np.max(c_skate_data[c_skate_data['anim_index']==ia]['anim_local_time']), facecolor='green', alpha=0.4)
            plt.hist(data, bins=80, stacked=True)
            ax = plt.gca()
            ax.set_xlim([0, np.max(c_skate_data['anim_local_time'])])
            #ax.set_xlim([0, np.max(c_skate_data[c_skate_data['anim_index']==ia]['anim_local_time'])])
            plt.xlabel("Momentas animacijoje "+a+" (s)");
            plt.ylabel("Grojimų skaičius");
            legend_labels = ['naudota animacijos dalis','originalioji animacija']
            legend_labels = legend_labels + ['veidrodinė animacija'] 
            plt.legend(loc=1, labels=legend_labels)
        if write_figures:
            plt.savefig(figure_directory+c+'_'+t+'_naudojimo_histogramos.png');
        if show_figures:
            plt.show()
        plt.close('all')

        plt.figure(figsize=figure_size)
        plt.suptitle('Valdiklio \"' + short_c_name +'\" animacijų naudojimas')

        plt.subplot(2, 1, 1)
        est_percentage = lambda x : 100*(len(x) / len(c_skate_data))
        sns.barplot(x='anim_count', y='anim_count', orient='v', data=c_skate_data, estimator=est_percentage)
        plt.xlabel('Maišomų animacijų kiekis')
        plt.ylabel("Dalis viso testo laiko (%)")

        plt.subplot(2, 1, 2)
        map_name = lambda x: anim_names[x]

        c_skate_data['anim_names'] = c_skate_data['anim_index'].map(map_name)
        sns.barplot(x='anim_names', y='anim_names', data=c_skate_data, orient='v', estimator=est_percentage)
        plt.xlabel('Grojama animacija')
        plt.ylabel("Dalis viso testo laiko (%)")

        used_anim_names = c_skate_data['anim_names'].unique()
        print(c+' used anim names:\n')
        print(used_anim_names)

        print(c+' anim names:\n')
        print(anim_names)
        for ia, a in enumerate(used_anim_names):
            anim_entries = c_skate_data[c_skate_data['anim_names']==a]
            anim_skate_amounts['count'][a] = len(anim_entries)
            anim_skate_amounts['total_time'][a] = np.sum(anim_entries['dt'])
        total_time = np.sum(anim_skate_amounts['total_time'])
        anim_skate_amounts['fraction'] = anim_skate_amounts['total_time']/total_time
        anim_skate_amounts['weighted_anim_skate'] = anim_skate_amounts['fraction']*anim_skate_amounts['foot_skate']
        print(anim_skate_amounts)
        weighted_anim_skate_amount = np.sum(anim_skate_amounts.weighted_anim_skate)
        worse_by_fraction = str(c_skate_amount/weighted_anim_skate_amount - 1)
        print('\nweighted anim skate: ' + str(weighted_anim_skate_amount))
        print('controller    skate: ' + str(c_skate_amount))
        print('worse by: '+ worse_by_fraction)
        foot_skate_table[foot_side+'_controller_skate'][c] = c_skate_amount 
        foot_skate_table[foot_side+'_anim_skate'][c] = weighted_anim_skate_amount 
        foot_skate_table[foot_side+'_worse_frac'][c] = worse_by_fraction 

        if write_figures:
            plt.savefig(figure_directory+c+'_'+t+'_naudojimo_stulpelinės.png');
        if show_figures:
            plt.show()
        plt.close('all')

foot_skate_table['avg_controller_skate'] = (foot_skate_table['l_controller_skate'] + foot_skate_table['r_controller_skate'])/2
foot_skate_table['avg_anim_skate'] = (foot_skate_table['l_anim_skate'] + foot_skate_table['r_anim_skate'])/2
foot_skate_writer = pd.ExcelWriter('data/figures/slydimo_matavimai.xlsx')
foot_skate_table.round(3).to_excel(foot_skate_writer, 'foot_skate')
foot_skate_writer.save();
foot_skate_writer.close();
