
import pandas as pd
import numpy as np
from sklearn.linear_model import LinearRegression
from typing import List

MPC_SHALLOW_MIN = 1
MPC_SHALLOW_MAX = 6
MPC_DEEP_MIN = 3
MPC_DEEP_MAX = 14
MPC_NB_TRY = 2

pair_list = [
    [(3, 1)],
    [(4, 2)],
    [(5, 1)],
    [(6, 2)],
    [(7, 3)],
    [(8, 4)],
    [(9, 3), (9, 5)],
    [(10, 4), (10, 6)],
    [(11, 3), (11, 5)],
    [(12, 4)],
    [(13, 5)],
    [(14, 6)],
]


class MPCPair:
    def __init__(self, deep):
        self.shallow = shallow
        self.slope = -1.0
        self.bias = -1.0
        self.sigma = -1.0


def calc_score_pair(score_df):
    score_list = [
        [[] for i in range(2)]for i in range(MPC_DEEP_MAX-MPC_DEEP_MIN+1)
    ]

    matchIdxList = score_df["matchIdx"].unique()
    for match in matchIdxList:
        match_df = score_df[score_df["matchIdx"] == match]
        for pairs in pair_list:
            # 最大２つ
            for i in range(len(pairs)):
                deep = pairs[i][0]
                shallow = pairs[i][1]

                d_score = match_df[match_df["depth"]
                                   == deep]["score"]
                s_score = match_df[match_df["depth"]
                                   == shallow]["score"]

                if len(d_score) > 0 and len(s_score) > 0:
                    score_list[deep-MPC_DEEP_MIN][i].append(
                        [s_score.values[0], d_score.values[0]]
                    )

    return score_list


def get_stats_list(df):
    score_list = calc_score_pair(df)
    for depth in range(MPC_DEEP_MAX-MPC_DEEP_MIN):
        for tries in range(2):
            print("a")


def write_mpc_info(pairs: List[MPCPair], outfile):
    with open(outfile, mode='w') as f:
        f.write("static const MPCPair mpcPairs{\n")
        for nb_empty in range(0, 60-MPC_DEEP_MIN):
            f.write("{\n")
            for pair in pairs:
                f.write("{"+pair.shallow+"}")

            f.write("}\n")
        f.write("}\n")


raw_csv_file = './resources/mpc_raw.csv'

raw_df = pd.read_csv(raw_csv_file, sep=',')
nb_empty_min = 10
nb_empty_max = raw_df['nbEmpty'].max()+1

for nb_empty in range(nb_empty_min, nb_empty_max):
    scores_df = raw_df[raw_df['nbEmpty'] == nb_empty]
    scores = calc_score_pair(scores_df)
    print("empty: "+str(nb_empty)+"finished")
