import numpy as np
import string

delta = np.zeros((34,34), dtype = np.int)
edge_sum = 0
k = np.zeros((5,), dtype = np.int)
with open("../network/club.txt") as text:
    for line in text:
        vertices = line.strip().split()
        v_i = string.atoi(vertices[0])
        v_j = string.atoi(vertices[1])
        delta[v_i][v_j] = 1
        delta[v_j][v_i] = 1
        k[v_i] += 1
        k[v_j] += 1
        edge_sum += 1

c = []
with open('../graphx_answer.txt') as text:
    for line in text:
        vertices = line.strip().split()
        v = string.atoi(vertices[0])
        c[v] = string.atoi(vertices[1])

for i in range(34):
    for j in range(34):
        Q += (delta[i][j] - k[i] *k[j]/(2*edge_sum)) * (c[i] * c[j])

Q = Q / (2*edge_sum)
Q
