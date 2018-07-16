import sys
import pprint
from gexf import Gexf


# test helloworld.gexf
gexf = Gexf("Home With Kids", "A Demo Graph")
graph = gexf.addGraph("directed", "static", "A Demo Graph")

atr1 = graph.addNodeAttribute('ID', type='string')
atr2 = graph.addNodeAttribute('adult', type='boolean', defaultValue='true')

tmp = graph.addNode("0", "刘梅")
tmp.addAttribute(atr1, "123456")
tmp.addAttribute(atr2, 'true')

tmp = graph.addNode("1", "夏东海")
tmp.addAttribute(atr1, "534523")
tmp.addAttribute(atr2, 'true')

tmp = graph.addNode("2", "夏雪")
tmp.addAttribute(atr1, "2312311")
tmp.addAttribute(atr2, 'false')

tmp = graph.addNode("3", "刘星")
tmp.addAttribute(atr1, "1231231")
tmp.addAttribute(atr2, 'false')

tmp = graph.addNode("4", "夏雨")
tmp.addAttribute(atr1, "2314889")
tmp.addAttribute(atr2, 'false')

tmp = graph.addNode("5", "夏祥")
tmp.addAttribute(atr1, "786767")
tmp.addAttribute(atr2, 'true')

tmp = graph.addNode("6", "范晓英")
tmp.addAttribute(atr1, "786767")
tmp.addAttribute(atr2, 'true')


graph.addEdge("0", "0", "1", label='妻子', weight='1')
graph.addEdge("1", "1", "0", label='丈夫', weight='1')
graph.addEdge("2", "0", "2", label='继母', weight='1')
graph.addEdge("3", "2", "3", label='姐姐', weight='1')
graph.addEdge("4", "3", "4", label='哥哥', weight='1')
graph.addEdge("5", "0", "3", label='母亲', weight='1')
graph.addEdge("6", "0", "4", label='继母', weight='1')
graph.addEdge("7", "6", "1", label='母亲', weight='1')
graph.addEdge("8", "5", "1", label='父亲', weight='1')


output_file = open("home.gexf", "wb")
gexf.write(output_file)
