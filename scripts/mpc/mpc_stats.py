
import pandas as pd
from typing import List

MPC_SHALLOW_MIN = 1
MPC_SHALLOW_MAX = 6
MPC_DEEP_MIN = 3
MPC_DEEP_MAX = 14
MPC_NB_TRY = 2

pair_list = [
    (3, 1),
    (4, 2),
    (5, 1),
    (6, 2),
    (7, 3),
    (8, 4),
    (9, 3),
    (9, 5),
    (10, 4),
    (10, 6),
    (11, 3),
    (11, 5),
    (12, 4),
    (13, 5),
    (14, 6),
]


class MPCPair:
    def __init__(self, nb_empty, shallow, deep):
        self.nb_empty = nb_empty
        self.shallow = shallow
        self.deep = deep
        self.slope = -1.0
        self.bias = -1.0
        self.sigma = -1.0


def calc_stats(score_df, nb_empty):
    ret_list = []
    for pair in pair_list:
        deep = pair[0]
        shallow = pair[1]

        for


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
nb_empty_min = 53  # raw_df['nbEmpty'].min()
nb_empty_max = raw_df['nbEmpty'].max()+1

for nb_empty in range(nb_empty_min, nb_empty_max):
    scores_df = raw_df[raw_df['nbEmpty'] == nb_empty]
    scores_df.to_csv('./test' + str(nb_empty) + '.csv')
