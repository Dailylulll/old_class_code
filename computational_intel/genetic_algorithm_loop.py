import math
import sys
import pickle

import numpy as np
import pandas as pd
import random
import matplotlib.pyplot
import matplotlib.pyplot as plt
from tqdm import tqdm
from operator import itemgetter, attrgetter
import pylab as pl
from IPython import display
import time
from matplotlib.patches import Ellipse
import threading
from multiprocessing import Process, Lock, Queue
import psutil

def initialization_of_population(ga):
    init_func = get_init_func(ga)
    return init_func(ga)


def get_init_func(ga):
    func = ga["init_func"]
    if func == "uniform_rand_init":
        return uniform_rand_init
    else:
        return grid_init

# np.random.uniform does not like negative arguments :(
def uniform_rand_init(ga):
    population = []
    for _1 in range(ga["size"]):
        chromosome = np.random.uniform(0, ga["pos_domain"], ga["dimensions"])
        for i in range(ga["dimensions"]):
            # chromosome = ((( np.random.rand(ga["d"]) - 0.5 ) * 2.0 ) * 5.0 )
            flip = np.random.randint(2)
            if  flip == 1:
                chromosome[i] = chromosome[i] * -1
        #population.append(np.random.uniform(ga["neg_domain"], ga["pos_domain"], ga["dimensions"]))
        population.append(chromosome)
        del flip
    return population


def grid_init(ga):
    population = []
    dimension_grid = []
    for dim in range(0, ga["dimensions"]):  # get each dimension seperated into a even grid
        dim_intervals = []
        abs_domain = ga["pos_domain"] + abs(ga["neg_domain"])
        interval = abs_domain / (ga["size"] - 1)  # check the spread, this fixes the lefthand bias i think
        current_grid_spot = -ga["neg_domain"]
        for _ in range(ga["size"]):
            dim_intervals.append(current_grid_spot)
            current_grid_spot += interval
        np.random.shuffle(dim_intervals)
        dimension_grid.append(dim_intervals)
    for _ in range(ga["size"]):
        chromo = []
        for dim in dimension_grid:
            chromo.append(dim.pop(0))
        population.append(chromo.copy())
    del dimension_grid
    del dim_intervals
    del abs_domain
    del interval
    del chromo
    del current_grid_spot
    return population

def fitness_score(population, ga):
    fit_func = get_fit_func(ga)
    scores = []
    for chromosome in population:
        score = fit_func(chromosome)
        scores.append(1.0 / (score + 0.0000000001))
    scores, pop = np.array(scores), np.array(population.copy())
    inds = np.argsort(scores)
    scores = list(scores[inds][::-1])
    pop = list(pop[inds, :][::-1])
    del population
    del score
    del inds
    return scores, pop

## Optimization funcs
# Rasting func
def fit_r(chromo):
    a = 10
    array = []
    for i in range(len(chromo)):
        calc = chromo[i] ** 2 - a * math.cos(2 * math.pi * chromo[i])
        array.append(calc)
    val = a * len(chromo) + np.sum(array)
    del array
    del calc
    return val

def fit_ra(x, y):
    a = 10
    return a * 2 + (x**2-a*np.cos(2*math.pi*x) + (y**2-a*np.cos(2*math.pi*y)))


# Sphere
def fit_s(chromo):
    sum = 0
    for i in range(len(chromo)):
        sum += chromo[i] ** 2
    return sum

'''
# Himmelblau
def fit_h(chromo):
    even = lambda x, y: (x ** 2 + y - 11) ** 2
    odd = lambda x, y: (x + y ** 2 - 7) ** 2
    ev = 0
    ov = 0
    sum = 0
    for i in range(len(chromo)):
        if i == 0 or i == 1:
            if i == 0:
                ev = chromo[0]
                continue
            if i == 1:
                ov = chromo[1]
                sum += even(ev, ov) + odd(ev, ov)
        else:
            ev = chromo[i - 1]
            ov = chromo[i]
            if i % 2:
                sum += even(ev, ov)
            else:
                sum += odd(ev, ov)
    return sum
'''
def fit_h(c):
    sum = 0
    for i in range(len(c)-1):
        sum += (c[i] ** 2 + c[i+1] - 11) ** 2 + (c[i] + c[i+1] ** 2 - 7) ** 2
    return sum


def get_fit_func(ga):
    func = ga["fitness_func"]
    if func == 'r':
        return fit_r
    elif func == 'h':
        return fit_h
    else:
        return fit_s

def selection(scores, pop_after_fit, ga):
    population_nextgen = []
    selection_start = 0
    parents = 2
    if ga["crossover_func"] == "udx":
        parents = 3
    if ga["elitism"]:
        for _ in range(parents):
            population_nextgen.append(pop_after_fit[0].copy())
        selection_start = 1
    selection_func = get_select_func(ga)
    population_nextgen.extend(selection_func(scores, pop_after_fit, selection_start, parents))
    del pop_after_fit
    del scores
    return population_nextgen


def get_select_func(ga):
    func = ga["selection_func"]
    if func == "roulette":
        return roulette
    else:
        return tourney


def roulette(scores, pop_after_fit, selection_start, num_of_parents):
    population_nextgen = []
    max_val = np.sum(scores)
    for i in range(selection_start, len(scores)):
        for k in range(num_of_parents):
            pick = np.random.uniform(0, max_val)
            current = 0
            for j in range(len(scores)):
                current += scores[j]
                if current > pick:
                    break
            population_nextgen.append(pop_after_fit[j].copy())  # Does j work here?
    return population_nextgen


def tourney(scores, pop_after_fit, selection_start, num_of_parents):
    tourney_size = 3  # TODO: magic number here
    population_nextgen = []
    pop_size = len(scores)
    selection_size = len(pop_after_fit) * num_of_parents - selection_start * num_of_parents  # elitism adjustment
    for _1 in range(selection_size):
        tourney_bracket = []
        for _2 in range(tourney_size):
            index = random.randint(selection_start, pop_size - 1)
            contender = [index, scores[index]]
            tourney_bracket.append(contender)
        tourney_bracket.sort(key=itemgetter(1), reverse=True)
        winner = tourney_bracket.pop(0)
        population_nextgen.append(pop_after_fit[winner[0]])
    del tourney_bracket
    return population_nextgen

def crossover(pop_after_sel, ga):
    population_nextgen = []
    reproduction_count = ga["size"]
    crossover_rate = ga["crossover_rate"]
    offset = 0
    if ga["elitism"]:
        population_nextgen.append(pop_after_sel[0].copy())
        offset = 1
    cross_func = get_cross_func(ga)
    # TODO: If no elitism is it better to shuffle?
    population_nextgen.extend(cross_func(pop_after_sel, reproduction_count, offset, crossover_rate))
    del pop_after_sel
    return population_nextgen


def get_cross_func(ga):
    func = ga["crossover_func"]
    if func == "two_point":
        return two_point
    elif func == "floating_point":
        return floating_point
    elif func == "umdx":
        return umdx
    else:
        return two_point


'''
TODO: This is where crossover amount matters
must be a percentage of a length of the chromo, rounded up or down depending on size
pick a random stop in chromo, then slice that length, making sure it doesnt go past dimensions
'''

# here crossover is the length of the slice
def two_point(pop_after_sel, reproduction_count, offset, crossover_rate):
    population_nextgen = []
    chromo_len = len(pop_after_sel[0])
    slice_len = int(chromo_len * crossover_rate)
    while slice_len >= chromo_len:
        slice_len -= 1
    for i in range(offset, reproduction_count):
        child = pop_after_sel[2 * i + 0].copy()
        parent2 = pop_after_sel[2 * i + 1].copy()
        loc = np.random.randint(0, chromo_len)
        if len(child)==2 or slice_len == 0:
            child[loc] = parent2[loc]
        else:
            if loc + slice_len > chromo_len:
                child[loc-slice_len:loc] = parent2[loc-slice_len:loc]
            else:
                child[loc:loc + slice_len] = parent2[loc:loc + slice_len]
        population_nextgen.append(child.copy())
    del child
    del parent2
    del loc
    del chromo_len
    del slice_len
    return population_nextgen

# cross over is a chance an individual gene to be crossed
def floating_point(pop_after_sel, reproduction_count, offset, crossover_rate):  # TODO
    population_nextgen = []
    # chromo_len = len(pop_after_sel[0])
    for i in range(offset, reproduction_count):
        child = pop_after_sel[2 * i + 0].copy()
        parent2 = pop_after_sel[2 * i + 1].copy()
        for j in range(len(child)):
            if random.random() < crossover_rate:
                if child[j] >= parent2[j]:
                    child[j] -= (child[j] - parent2[j]) * np.random.uniform(0,1)
                else:
                    child[j] += (parent2[j] - child[j]) * np.random.uniform(0,1)
        population_nextgen.append(child.copy())
    del child
    del parent2
    return population_nextgen

def umdx(pop_after_sel, reproduction_count, offset, crossover_rate):
    population_nextgen = []
    dimensions = len(pop_after_sel[0])
    for i in range(offset, reproduction_count):
        parent1 = np.array(pop_after_sel[3 * i + 0])  # TODO: Magic number here, number of parents
        parent2 = np.array(pop_after_sel[3 * i + 1])
        parent3 = np.array(pop_after_sel[3 * i + 2])
        mean = (parent1 + parent2) / 2
        diff_vec = parent1 - parent2
        distance = np.linalg.norm(mean - parent3)
        r1 = np.random.normal(0, .5)
        r2 = np.random.normal(0, 0.35 / (3 ** .5))
        normal_ortho = parent3 / abs(np.linalg.norm(parent3))
        child = mean + r1 * diff_vec + distance * r2 * normal_ortho
        # print(child)
        population_nextgen.append(child)
    return population_nextgen

def mutation(pop_after_cross, ga):
    population_nextgen = []
    start = 0
    if ga["elitism"]:
        population_nextgen.append(pop_after_cross[0].copy())
        start = 1
    mutation_func = get_mutation_func(ga)
    population_nextgen.extend(mutation_func(pop_after_cross, start, ga))
    del pop_after_cross
    return population_nextgen


def get_mutation_func(ga):
    func = ga["mutation_func"]
    if func == "random":
        return random_mutation
    elif func == "scalar":
        return scalar_mutation
    else:
        return gaussian_mutation


def random_mutation(pop_after_cross, start, ga):
    population_nextgen = []
    pop_mutation_rate = ga["pop_mutation_rate"]
    gene_mutation_rate = ga["gene_mutation_rate"]
    for i in range(start, ga["size"]):
        chromosome = pop_after_cross[i].copy()
        if random.random() < pop_mutation_rate:
            for j in range(len(chromosome)):
                if random.random() < gene_mutation_rate:
                    chromosome[j] = np.random.uniform(0, ga["pos_domain"])
                    if ga["neg_domain"] != 0:
                        if np.random.randint(2) == 1:
                            chromosome[j] *= -1
        population_nextgen.append(chromosome.copy())
    return population_nextgen

def scalar_mutation(pop_after_cross, start, ga):
    axis_flip = True  # TODO: Magic number here
    population_nextgen = []
    pop_mutation_rate = ga["pop_mutation_rate"]
    gene_mutation_rate = ga["gene_mutation_rate"]
    for i in range(start, ga["size"]):
        chromosome = pop_after_cross[i].copy()
        if random.random() < pop_mutation_rate:
            for j in range(len(chromosome)):
                if random.random() < gene_mutation_rate:
                    scalar = np.random.uniform(.5, 1.5)
                    if ga["neg_domain"] != 0:
                        if np.random.randint(4) == 1 and axis_flip: # TODO: Magic number her
                            scalar *= -1
                    chromosome[j] *= scalar
                    if chromosome[j] >=0 and chromosome[j] > ga["pos_domain"]:
                        chromosome[j] = ga["pos_domain"]
                    if chromosome[j] < 0 and abs(chromosome[j]) > ga["pos_domain"]:
                        chromosome[j] = ga["pos_domain"] * -1
        population_nextgen.append(chromosome.copy())
    return population_nextgen

# not finished
def gaussian_mutation(pop_after_cross, start, ga):
    population_nextgen = []
    pop_mutation_rate = ga["pop_mutation_rate"]
    gene_mutation_rate = ga["gene_mutation_rate"]
    std = (ga["pos_domain"] - ga["neg_domain"]) * 0.1
    for i in range(start, ga["size"]):
        chromosome = pop_after_cross[i].copy()
        if random.random() < pop_mutation_rate:
            for j in range(len(chromosome)):
                if random.random() < gene_mutation_rate:
                    r = np.random.normal(0, std)
                    if chromosome[j] + r > ga["pos_domain"]:
                        chromosome[j] -= r
                    elif chromosome[j] + r < ga["neg_domain"]:
                        chromosome[j] -= r
        population_nextgen.append(chromosome.copy())
    del std
    return population_nextgen


def build_ga(label=1, size=100, ngen=100, pop_mutation_rate=0.1, gene_mutation_rate=0.1, crossover_rate=0.1,
             dimensions=10,
             neg_domain=(-5.12), pos_domain=5.12, elitism=True, fitness_func='s', init_func="uniform_rand_init",
             selection_func="roulette",
             crossover_func="two_point", mutation_func="random", termination_func="iter", cycle=False):
    if not cycle:
        return dict(label=label, size=size, ngen=ngen, pop_mutation_rate=pop_mutation_rate,
                    gene_mutation_rate=gene_mutation_rate,
                    crossover_rate=crossover_rate, dimensions=dimensions, neg_domain=-neg_domain,
                    pos_domain=pos_domain, elitism=elitism, fitness_func=fitness_func, init_func=init_func,
                    selection_func=selection_func, crossover_func=crossover_func, mutation_func=mutation_func,
                    termination_func=termination_func)


def generations(ga):
    best_chromo = []
    best_score = []
    population_nextgen = initialization_of_population(ga)
    stats_min = np.zeros(ga["ngen"])
    stats_avg = np.zeros(ga["ngen"])
    stats_max = np.zeros(ga["ngen"])
    stored_points = []
    if ga["termination_func"] == "iter":
        for i in range(ga["ngen"]):
            # evaluate
            scores, pop_after_fit = fitness_score(population_nextgen, ga)
            # select
            pop_sel = selection(scores, pop_after_fit, ga)
            # crossover
            pop_after_cross = crossover(pop_sel, ga)
            # mutation
            population_nextgen = mutation(pop_after_cross, ga)
            # post mut eval for stats
            scores, pop_after_fit = fitness_score(population_nextgen, ga)
            stored_points.append(pop_after_fit)
            best_chromo.append(pop_after_fit[0].copy())
            best_score.append(scores[0].copy())
            stats_min[i] = np.min(scores)
            stats_max[i] = np.amax(scores)
            stats_avg[i] = np.mean(scores)
        del scores
        del pop_after_fit
        return i + 1, best_chromo, best_score, stats_min, stats_avg, stats_max, stored_points
    else:
        streak = 0
        for i in range(ga["ngen"]):
            # evaluate
            scores, pop_after_fit = fitness_score(population_nextgen, ga)
            # select
            pop_sel = selection(scores, pop_after_fit, ga)
            # crossover
            pop_after_cross = crossover(pop_sel, ga)
            # mutation
            population_nextgen = mutation(pop_after_cross, ga)
            # post mut eval for stats
            scores, pop_after_fit = fitness_score(population_nextgen, ga)
            stored_points.append(pop_after_fit)
            best_chromo.append(pop_after_fit[0].copy())
            best_score.append(scores[0].copy())
            stats_min[i] = np.min(scores)
            stats_max[i] = np.amax(scores)
            stats_avg[i] = np.mean(scores)
            if len(best_chromo) > 1:
                if np.array_equal(best_chromo[-1], best_chromo[-2]):
                    streak += 1
                    if streak == 10:  # TODO: Magic number here, arbitrary streak length
                        break
                else:
                    streak = 0
        del scores
        del pop_after_fit
        return i + 1, best_chromo, best_score, stats_min, stats_avg, stats_max, stored_points


def make_table(ga_tables):
    columns = {}
    dimensions = [2, 10, 100, 1000]
    fitness_func = ['r', 's', 'h']
    for f in fitness_func:
        for d in dimensions:
            ga_list = list()
            stats = list()
            clabel = f"f:{f} d:{d}"
            for ga in ga_tables:
                if ga["fitness_func"] == f:
                    if ga["dimensions"] == d:
                        ga_list.append(ga)
            ga_list.sort(key=lambda x: x["label"])
            for ga in ga_list:
                stats.append(ga["score"])
            if len(stats) > 0:
                columns[clabel] = stats
    df = pd.DataFrame(columns)
    ckey = list(columns)[0]

    df.style \
        .format(thousands=".", decimal=",") \
        .format_index(str.upper, axis=1) \
        .relabel_index(range(1, len(columns[ckey]) + 1), axis=0)
    return df

def make_table_mut(ga_tables):
    columns = {}
    pop_mutation_rate = [.05, .25]
    gene_mutation_rate = [.05, .25]
    crossover_rate = [.25, .5]
    clabel = ""
    for pmr in pop_mutation_rate:
        for gmr in gene_mutation_rate:
            for cr in crossover_rate:
                stat_list = list()
                clabel = f"{str(pmr)} {str(gmr)} {str(cr)}"
                ga_list = list(ga_tables)
                ga_list.sort(key=lambda x: x["label"])
                for ga in ga_list:
                    if str(ga["pop_mutation_rate"]) == str(pmr):
                        if str(ga["gene_mutation_rate"]) == str(gmr):
                            if str(ga["crossover_rate"]) == str(cr):
                                stat_list.append(ga["score"])
                if len(stat_list) > 0:
                    columns[clabel] = stat_list
    df = pd.DataFrame(columns)
    df.style \
        .format(thousands=".", decimal=",") \
        .format_index(str.upper, axis=1) \
        .relabel_index(range(1, len(columns[clabel]) + 1), axis=0)
    # print("Ran on 2d r function")
    # print("pmr = pop mutation rate, gmr = gene mutation rate, cr = crossover rate")
    return df

def make_table_compar(ga_tables):
    columns = {}
    stats = ["min", "max", "avg", "score"]
    clabel = ""
    for stat in stats:
        stat_list = []
        clabel = f"{stat}"
        ga_list = list(ga_tables)
        ga_list.sort(key=lambda x: x["label"])
        for ga in ga_list:
            stat_list.append(ga[stat])
        if len(stats) > 0:
            columns[clabel] = stat_list
    df = pd.DataFrame(columns)
    df.style \
        .format(thousands=".", decimal=",") \
        .format_index(str.upper, axis=1) \
        .relabel_index(range(1,len(columns[clabel])+1), axis=0)
    # print("ran on 2d r function")
    return df

def make_legend(ga_legend):
    label_list = []
    base_gas = ""
    ga_legend.sort(key=lambda x: x["label"])
    print()
    constant = ga_legend[0]
    print(f'''GA LEGEND\n
CONSTANTS: gene_mutation_rate={constant["gene_mutation_rate"]}, pop_mutation_rate={constant["pop_mutation_rate"]},
crossover_rate{constant["crossover_rate"]}, elitism={constant["elitism"]}\n''')
    for ga in ga_legend:
        if ga["label"] not in label_list:
            label_list.append(ga["label"])
            base_gas += " " + str(ga)
        else:
            continue
        print(
            f'''Label={ga["label"]}, init_func={ga["init_func"]}, selection_func={ga["selection_func"]},crossover_func={ga["crossover_func"]}, mutation_func={ga["mutation_func"]}, termination_func={ga["termination_func"]}''')
        return base_gas


# Create a process pool, and a thread pool
# this computer has 4 cores, thats 4 processes
# Get the total ga count first, then evenly divide amongst the cores
# then thread out in each core, providing a progress bar on each process
# the only thing that needs to be returned is ga_legend
# put main copy on google drive or synchthing

def process_ga_list(ga_list, ga_legend, adv=False):
    print(f"Number of iterations: {len(ga_list)}")
    cross_list = Queue(len(ga_list))
    n_cpus = psutil.cpu_count()
    threads = math.ceil(len(ga_list) / n_cpus)
    process_list = list()
    p_master = list()
    ga_list_copy = ga_list.copy()
    for j in range(n_cpus):
        p_ga = list()
        for k in range(threads):
            if len(ga_list_copy) != 0:
                p_ga.append(ga_list_copy.pop(0))
        p_master.append(p_ga)
    for cpu in range(n_cpus):
        affinity = [cpu]
        process_list.append(Process(target=process_ga, args=[p_master[cpu], cross_list, affinity, adv]))
    for p in process_list:
        p.start()
    for _ in tqdm(range(len(ga_list))):
        val = cross_list.get()
        ga_legend.append(val)
    del cross_list
    for p in process_list:
        p.join()
        del p

def process_ga(ga_list, ga_legend, affinity, adv):
    proc = psutil.Process()
    proc.cpu_affinity(affinity)
    score = get_score
    if adv:
        score = get_adv_score
    for ga in ga_list:
        score(ga, ga_legend)
    sys.exit()

def get_score(ga, ga_legend):
    scores = list()
    for i in range(10):
        gen, chromo, score, stats_min, stats_avg, stats_max, stored_points = generations(ga)
        scores.append(score[-1])
        del score
        del stats_min
        del stats_avg
        del stats_max
        del stored_points
        del chromo
        del gen
    # ga["score"] = f"{ga['label']} {np.average(scores):.2e} ({np.std(scores):.2e})"
    ga["score"] = f"{np.average(scores):.2e} ({np.std(scores):.2e})"
    del scores
    ga_legend.put(ga)

def get_adv_score(ga, ga_legend):
    scores = []
    stat_min = []
    stat_max = []
    stat_avg = []
    for i in range(100):
        gen, chromo, score, stats_min, stats_avg, stats_max, stored_points = generations(ga)
        scores.append(score[-1])
        stat_min.append(stats_min[gen-1])
        stat_max.append(stats_max[gen-1])
        stat_avg.append(stats_avg[gen-1])
        del score
        del stats_min
        del stats_avg
        del stats_max
        del stored_points
        del chromo
        del gen
    ga["score"] = f"{np.average(scores):.2e} ({np.std(scores):.2e})"
    ga["min"] = f"{np.average(stat_min):.2e} ({np.std(stat_min):.2e})"
    ga["max"] = f"{np.average(stat_max):.2e} ({np.std(stat_max):.2e})"
    ga["avg"] = f"{np.average(stat_avg):.2e} ({np.std(stat_avg):.2e})"
    ga_legend.put(ga)

def run(run="cycle", ga_list=None):
    print("run")
    dimensions = [2, 10, 100, 1000]
    init_func = ["uniform_rand", "grid"]
    selection_func = ["roulette", "tourney"]
    crossover_func = ["two_point", "floating_point"]
    termination_func = ["iter", "streak"]
    mutation_func = ["random", "gaussian"]
    fitness_func = [['r', -5.12, 5.12], ['s', -5.12, 5.12], ['h', -5, 5]]
    pop_mutation_rate = [.05, .25]
    gene_mutation_rate = [.05, .25]
    crossover_rate = [.25, .5]
    ga_legend = []
    ga = build_ga()
    ga_list_copy = list()
    label = 1

    if run == "cycle":
        for init in init_func:
            ga["init_func"] = init
            for sf in selection_func:
                ga["selection_func"] = sf
                for cf in crossover_func:
                    ga["crossover_func"] = cf
                    for mf in mutation_func:
                        ga["mutation_func"] = mf
                        for tf in termination_func:
                            ga["termination_func"] = tf
                            ga["label"] = label
                            for f in fitness_func:
                                ga["fitness_func"] = f[0]
                                ga["neg_domain"] = f[1]
                                ga["pos_domain"] = f[2]
                                for d in dimensions:
                                    ga["dimensions"] = d
                                    ga_copy = ga.copy()
                                    ga_list_copy.append(ga_copy)
                            label += 1
        process_ga_list(ga_list_copy, ga_legend)
        table = make_table(ga_legend)
        table.to_pickle('base_table.pickle')

    elif run == "map":
        for init in init_func:
            ga["init_func"] = init
            for sf in selection_func:
                ga["selection_func"] = sf
                for cf in crossover_func:
                    ga["crossover_func"] = cf
                    for mf in mutation_func:
                        ga["mutation_func"] = mf
                        for tf in termination_func:
                            ga["termination_func"] = tf
                            ga["label"] = label
                            f = fitness_func[0]
                            ga["fitness_func"] = f[0]
                            ga["neg_domain"] = f[1]
                            ga["pos_domain"] = f[2]
                            ga["dimensions"] = 2
                            ga_copy = ga.copy()
                            ga_list_copy.append(ga_copy)
                            label += 1
        with open('maps.pickle', 'wb') as f:
            pickle.dump(ga_list_copy, f)

    elif run == "compar":
        for init in init_func:
            ga["init_func"] = init
            for sf in selection_func:
                ga["selection_func"] = sf
                for cf in crossover_func:
                    ga["crossover_func"] = cf
                    for mf in mutation_func:
                        ga["mutation_func"] = mf
                        for tf in termination_func:
                            ga["termination_func"] = tf
                            ga["label"] = label
                            f = fitness_func[0]
                            ga["fitness_func"] = f[0]
                            ga["neg_domain"] = f[1]
                            ga["pos_domain"] = f[2]
                            ga["dimensions"] = 2
                            ga_copy = ga.copy()
                            ga_list_copy.append(ga_copy)
                            label += 1
        process_ga_list(ga_list_copy, ga_legend, adv=True)
        table = make_table_compar(ga_legend)
        table.to_pickle('compar_table.pickle')

    elif run == "2dgraph":
        gas = []
        for ga in ga_list:
            gas.append(eval(ga))
        it = iter(gas)
        for i in it:
            ga1 = i
            ga2 = next(it)
            gen1, chromo1, score1, stats_min1, stats_avg1, stats_max1, stored_points1 = generations(ga1)
            gen2, chromo2, score2, stats_min2, stats_avg2, stats_max2, stored_points2 = generations(ga2)
            plt.plot(stats_min1, 'r')
            plt.plot(stats_avg1, 'r')
            plt.plot(stats_max1, 'r')
            plt.plot(stats_min2, 'b')
            plt.plot(stats_avg2, 'b')
            plt.plot(stats_max2, 'b')
            x = range(0, 100)
            plt.fill_between(x, y1=stats_min1, y2=stats_max1, alpha=0.25, color='r')
            plt.fill_between(x, y1=stats_min2, y2=stats_max2, alpha=0.25, color='b')
            plt.ylabel('accuracy')
            plt.xlabel('generations')
            plt.title(f"GA {ga1['label']} red vs GA {ga2['label']} blue")
            plt.show()

    elif run == "3dgraph":  # limit to 2 dimensions
        gas = []  # get reprentations from each f
        for ga in ga_list:
            gas.append(eval(ga))
        for ga in gas:
            fig, ax = pl.subplots(nrows=1, ncols=1, figsize=(7, 5))
            gen, chromo, score, stats_min, stats_avg, stats_max, stored_points = generations(ga)
            for t in range(len(stored_points)):
                pl.clf()
                data = stored_points[t]
                for i in range(len(data)):
                    pl.scatter(data[i][0], data[i][1], edgecolor='b', alpha=0.3)
                if ga["fitness_func"] == 'h':
                    a = np.arange(-5, 5, 0.05)
                    b = np.arange(-5, 5, 0.05)
                    x, y = np.meshgrid(a, b)
                    z = fit_h([x, y])
                    pl.contour(x, y, z, levels=np.logspace(-9, 9, 50), cmap='jet', alpha=0.4)
                if ga["fitness_func"] == 'r':
                    a = np.arange(-5.12, 5.12, 0.05)
                    b = np.arange(-5.12, 5.12, 0.05)
                    x, y = np.meshgrid(a, b)
                    z = fit_ra(x, y)
                    pl.contour(x, y, z, levels=np.logspace(-9, 9, 50), cmap='jet', alpha=0.4)
                if ga["fitness_func"] == 's':
                    a = np.arange(-5.12, 5.12, 0.05)
                    b = np.arange(-5.12, 5.12, 0.05)
                    x, y = np.meshgrid(a, b)
                    z = fit_s([x, y])
                    pl.contour(x, y, z, levels=np.logspace(-9, 9, 50), cmap='jet', alpha=0.4)
                display.clear_output(wait=True)
                display.display(pl.gcf())
                plt.xlim(-5, 5)
                plt.ylim(-5, 5)
                time.sleep(0.01)

    elif run == "mut":
        for init in init_func:
            ga["init_func"] = init
            for sf in selection_func:
                ga["selection_func"] = sf
                for cf in crossover_func:
                    ga["crossover_func"] = cf
                    for mf in mutation_func:
                        ga["mutation_func"] = mf
                        for tf in termination_func:
                            f = fitness_func[0]
                            ga["termination_func"] = tf
                            ga["fitness_func"] = f[0]
                            ga["neg_domain"] = f[1]
                            ga["pos_domain"] = f[2]
                            ga["dimensions"] = 2
                            for cr in crossover_rate:
                                ga["crossover_rate"] = cr
                                for pmr in pop_mutation_rate:
                                    ga["pop_mutation_rate"] = pmr
                                    for gmr in gene_mutation_rate:
                                        ga["gene_mutation_rate"] = gmr
                                        ga_copy = ga.copy()
                                        ga_list_copy.append(ga_copy)
                            label += 1
        process_ga_list(ga_list_copy, ga_legend)
        table = make_table_mut(ga_legend)
        table.to_pickle('mut_table.pickle')

    else:
        for gAA in ga_list:
            ga = eval(gAA)
            ga_copy = ga.copy()
            ga_list_copy.append(ga_copy)
        process_ga_list(ga_list_copy, ga_legend)
        make_table(ga_legend)
        make_legend(ga_legend)


'''
if i == 1:
                    print(ga)
                    print(f"Generations ran {gen}")
                    plt.plot(stats_min, 'r')
                    plt.plot(stats_avg, 'b')
                    plt.plot(stats_max, 'g')
                    plt.ylabel('accuracy')
                    plt.xlabel('generations')
                    plt.show()
'''

if __name__ == "__main__":
    '''
    gas = ["{'label': 1, 'size': 100, 'ngen': 100, 'pop_mutation_rate': 0.1, 'gene_mutation_rate': 0.1, 'crossover_rate': 0.1, 'dimensions': 2, 'neg_domain': 5.12, 'pos_domain': 5.12, 'elitism': False, 'fitness_func': 'r', 'init_func': 'grid_init', 'selection_func': 'roulette', 'crossover_func': 'floating_point', 'mutation_func': 'gaussian', 'termination_func': 'streak'}"]
    '''
    gas = [
        "{'label': 1, 'size': 100, 'ngen': 100, 'pop_mutation_rate': 0.1, 'gene_mutation_rate': 0.1, 'crossover_rate': 0.1, 'dimensions': 10, 'neg_domain': 5.12, 'pos_domain': 5.12, 'elitism': False, 'fitness_func': 'h', 'init_func': 'uniform_rand_init', 'selection_func': 'roulette', 'crossover_func': 'two_point', 'mutation_func': 'gaussian', 'termination_func': 'streak'}",
        "{'label': 2, 'size': 100, 'ngen': 100, 'pop_mutation_rate': 0.1, 'gene_mutation_rate': 0.1, 'crossover_rate': 0.1, 'dimensions': 10, 'neg_domain': 5.12, 'pos_domain': 5.12, 'elitism': False, 'fitness_func': 'h', 'init_func': 'uniform_rand_init', 'selection_func': 'tourney', 'crossover_func': 'two_point', 'mutation_func': 'gaussian', 'termination_func': 'iter'}",
        "{'label': 1, 'size': 100, 'ngen': 100, 'pop_mutation_rate': 0.1, 'gene_mutation_rate': 0.1, 'crossover_rate': 0.1, 'dimensions': 10, 'neg_domain': 5.12, 'pos_domain': 5.12, 'elitism': False, 'fitness_func': 'h', 'init_func': 'grid_init', 'selection_func': 'roulette', 'crossover_func': 'floating_point', 'mutation_func': 'gaussian', 'termination_func': 'streak'}",
        "{'label': 2, 'size': 100, 'ngen': 100, 'pop_mutation_rate': 0.1, 'gene_mutation_rate': 0.1, 'crossover_rate': 0.1, 'dimensions': 10, 'neg_domain': 5.12, 'pos_domain': 5.12, 'elitism': False, 'fitness_func': 'h', 'init_func': 'uniform_rand_init', 'selection_func': 'tourney', 'crossover_func': 'floating_point', 'mutation_func': 'gaussian', 'termination_func': 'iter'}"]
    run(run="mut")