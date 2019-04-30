Graph_javascript Compare

# Basic Intro

1. vis.js

  - :star: 6821
  - Github: <https://github.com/almende/vis>
  - Official : <http://visjs.org/>
  - Introduct: vis.js is a dynamic, browser-based visualization library
  - Commnet

  > 基于d3，对多节点的网络图有很强的表现力。支持JSON、DOT导入，API很强大<br>
  > 绘图自由度很高，能配合前端UI实现好看相衬的图<br>
  > _赞美之心_

  - Example

  <iframe width="600" height="450" src="http://visjs.org/examples/network/exampleApplications/nodeLegend.html" scrolling="yes">
  </iframe>

  - 一个不错的实现 <https://github.com/neo4j-contrib/neovis.js>

  ![](img/example-viz.png)

2. sigma.js

  - :star: 8007
  - Github: <https://github.com/jacomyal/sigma.js>
  - Official : <http://sigmajs.org>
  - Introduct: A JavaScript library dedicated to graph drawing
  - Comment

  > 以轻量著称，专注于展示多节点超大图，支持Gaphi、JSON导入<br>
  > 简洁高效的风格，默认是纯色系，扁平化展示效果，适合快速原型开发<br>
  > 与Neo4j风格差异很大，可能不太适合更深层次的用户交互

  - Example

    <iframe width="800" height="600" src="http://sigmajs.org/" scrolling="yes">
    </iframe>

  - 一个不错的实现 <https://dunnock.github.io/react-sigma/> ![](img/example-sigmajs.jpg)

3. echarts

  - :star: 28695
  - Github: <https://github.com/apache/incubator-echarts>
  - Official : <http://echarts.baidu.com/index.html>
  - Commnet

  > 百度产品，商业级，各方面都很完善，与 _Highchart_ 相当<br>
  > Network Graph只是特性之一，支持Gaphi，适合快速开发，UI配色也很不错<br>
  > vue-echarts :star: 2000+<br>
  > Google charts 貌似没有力导向图的模块，而且其js地址被墙了,不方便<br>
  > Microsoft charts for asp.net

  - Example

    <iframe width="600" height="450" src="http://echarts.baidu.com/examples/editor.html?c=graph-force" frameborder="0" scrolling="yes">
    </iframe>

4. Alchemy.js

  - :star: 423
  - Github: <https://github.com/GraphAlchemist/Alchemy>
  - Official : <http://graphalchemist.github.io/Alchemy/#/>
  - Example

    <iframe width="600" height="450" src="http://graphalchemist.github.io/Alchemy/#/examples" frameborder="0" scrolling="yes">
    </iframe>

5. cytoscape.js

  - :star: 4431
  - Github: <https://github.com/cytoscape/cytoscape.js>
  - Official : <http://js.cytoscape.org/>
  - Introduct: Graph theory / network library for analysis and visualisation
  - Comment

  > 支持广泛，专注于Network Graph

  > > Supports CommonJS/Node.js/Browserify/Webpack, AMD/Require.js, jQuery, npm, CDNJS, Bower, jspm, Meteor/Atmosphere, and plain JS/JavaScript<br>

  > 社区强大，完成度较高，给我的感觉是vis的Network Graph部分

  - Example

    <iframe width="600" height="450" src="http://js.cytoscape.org/demos/spread-layout/" frameborder="0" scrolling="yes">
    </iframe>

6. Highcharts

  - Official : <https://www.highcharts.com/>
  - Comment

    > 真正商业级产品,可以深度定制化,超级强大 (官网上没给力导向图的例子)

7. arbor.js

  - :star: 2476
  - Github: <https://github.com/samizdatco/arbor>
  - Official : <http://arborjs.org/>
  - Introduct: a graph visualization library using web workers and jQuery

  > UI和动画很有特点，在交互式上有独到之处 适合局部小图展示及交互<br>
  > github 更新停留在7年前

  <iframe width="600" height="450" src="http://arborjs.org/" frameborder="0" scrolling="yes">
  </iframe>

8. protovis

  - :star: 606
  - Github: <https://github.com/mbostock/protovis>
  - Official : <http://protovis.org/>
  - Comment

  > 早年比较有名，在stackoverflow上出现频率不低。现已停止维护

  - Example

    <iframe width="600" height="450" src="http://mbostock.github.io/protovis/ex/force.html" frameborder="0" scrolling="yes">
    </iframe>

9. Gojs

  - :star: 2076
  - Github: <https://github.com/NorthwoodsSoftware/GoJS>年前
  - Official : <https://gojs.net/latest/index.html>
  - Introduct: JavaScript diagramming library for interactive flowcharts, org charts, design tools, planning tools, visual languages.
  - Comment:
  - Example

  <iframe width="600" height="450" src="https://gojs.net/latest/extensions/NodeLabelDragging.html" scrolling="yes">
  </iframe>

10. popoto.js

  - :star: 93
  - Github: <https://github.com/Nhogs/popoto>
  - Official : <http://www.popotojs.com/>
  - Introduce： Visual query builder for Neo4j graph database年前
  - Comment:

  > 这个交互很有意思

  - Example

  <iframe width="600" height="450" src="http://www.popotojs.com/" scrolling="yes">
  </iframe>

11. JSNetworkX

  - :star: 486
  - Github: <https://github.com/fkling/JSNetworkX>
  - Official : <http://jsnetworkx.org/>
  - Introduce: Build, process and analyze graphs in JavaScript (port of NetworkX)
  - Example

  <iframe width="600" height="450" src="http://jsnetworkx.org/examples.html" scrolling="yes">
  </iframe>

12. d3.js

  - :star: 77066
  - Github: <https://github.com/d3/d3>
  - Official : <https://d3js.org>
  - Introduce: D3.js is a JavaScript library for manipulating documents based on data. D3 helps you bring data to life using HTML, SVG, and CSS

  > 本质上可以看作是一个其他js都遵循的框架。<br>
  > 完全可定制化, Neo4j 就是基于d3开发的。<br>
  > _万物始祖_

  - Example

  <iframe width="600" height="450" src="https://bl.ocks.org/mbostock/4062045" scrolling="yes">
  </iframe>

  - 一个不错的实现 <https://github.com/eisman/neo4jd3> ![](img/example-neo4jd3.jpg)

13. cola.js

  - :star: 1161
  - Github: <https://github.com/tgdwyer/WebCola>
  - Official : <http://marvl.infotech.monash.edu/webcola/>
  - Introduce: Javascript constraint-based graph layout
  - Comment

  > 这是个力导图布局计算插件<br>
  > 需要配合渲染插件一起使用

  - Example

  <iframe width="600" height="450" src="http://ialab.it.monash.edu/webcola/examples/unconstrainedsmallworld.html" scrolling="yes">
  </iframe>

14. springy

  - :star: 1579
  - Github: <https://github.com/dhotson/springy/>
  - Official : <http://getspringy.com/>
  - Introduce: A force directed graph layout algorithm in JavaScript
  - Example

  <iframe width="600" height="450" src="http://getspringy.com/" scrolling="yes">
  </iframe>

15. Infovis

  - :star: 1443
  - Github: <https://github.com/philogb/jit>
  - Official : <http://thejit.org/>
  - Introduce: The JavaScript InfoVis Toolkit provides tools for creating Interactive Data Visualizations for the Web
  - Example

  <iframe width="600" height="450" src="http://philogb.github.io/jit/static/v20/Jit/Examples/ForceDirected/example1.html" scrolling="yes">
  </iframe>

16. ngraph + VivaGraphJS

  - :star: 768 + 2632
  - Github: <https://github.com/anvaka/VivaGraphJS>
  - Github: <https://github.com/anvaka/ngraph>
  - Official : NULL
  - Introduce: Graph drawing library for JavaScript
  - Commnet:

  > 将边点文件序列化，先离线计算布局，然后静态呈现大图。<br>
  > E.g. 5 million edges, 1 million nodes requires only 23 MB of space.<br>
  > ngraph是布局计算部分，用C++实现，还未实现在node.js上。<br>
  > VivaGraphJS是渲染部分。<br>
  > 适合快速布局和渲染超大图，速度上有相当优势

  - Example

  <iframe width="600" height="450" src="http://www.yasiv.com/graphs#Bai/rw496" scrolling="yes">
  </iframe>

17. dagre

  - :star: 1685
  - Github: <https://github.com/dagrejs/dagre>
  - Official : NULL
  - Introduct: Directed graph layout for JavaScript
  - Example

  <iframe width="600" height="450" src="  http://cs.brown.edu/people/jcmace/d3/graph.html?id=small.json" scrolling="yes">
  </iframe>

18. dracula

  - :star: 650
  - Github: <https://github.com/strathausen/dracula>
  - Official: <https://www.graphdracula.net/>
  - Introduct: JavaScript layout and representation of connected graphs
  - Example

  <iframe width="600" height="450" src="https://www.graphdracula.net/" scrolling="yes">
  </iframe>

## Compare_Table

name          | Github_stars | Speed | Support | Beauty | Easy_code | Import     | Lience
------------- | ------------ | ----- | ------- | ------ | --------- | ---------- | ----------
d3.js         | :star: 77066 |       | 5       |        |           | Gephi/JSON | BSD-3
echarts       | :star: 28695 | 3     | 3       | 3      | 5         | Gephi      | Apache-2.0
sigma.js      | :star: 8007  | 5     |         | 4      | 4         | Gephi/JSON | MIT
vis.js        | :star: 6821  | 4     | 4       | 3      | 3         | Gephi/DOT  | Apache-2.0
cytoscape.js  | :star: 4431  | 4     | 4       | 3      | 3         | JSON       | LGPL
------------- | ------------ | ----- | ------- | ------ | --------- | ---------- | ----------
VivaGraphJS   | :star: 2632  | 5     | 2       | 2      | 2         |            | BSD-3
arbor.js      | :star: 2476  | 2     | 1       | 5      | 3         |            | MIT
Highcharts    | Commercial   |       |         |        |           |            | ¥ 8088
FusionCharts  | Commercial   |       |         |        |           |            | $ 497
