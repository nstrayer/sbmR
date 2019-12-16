// !preview r2d3 data=sim_basic_block_network(3, 20), options = list(color_col = 'group', shape_col = 'type')
svg.attr("viewBox", [0, 0, width, height]);

// Get color and shape column names from options
const {color_col, shape_col} = options;

// Clean up data into the node and link format that d3 needs
const links = HTMLWidgets.dataframeToD3(data.edges)
  .map(function(edge){return({source: edge.from, target: edge.to})});
const nodes = HTMLWidgets.dataframeToD3(data.nodes);


// If we're missing values for our aesthetic mapping columns,  fill them in with constant values.
if(!nodes[0][color_col]){
  nodes.forEach(function(node){node[color_col] = color_col});
}
if(!nodes[0][shape_col]){
  nodes.forEach(function(node){node[shape_col] = shape_col});
}

// Color encodes the node's group
const Color = d3.scaleOrdinal(d3.schemeCategory10, unique(nodes.map(d => d[color_col])));
const color_node = node => Color(node[color_col]);

// Shape encodes the node's type
const Shape = d3.scaleOrdinal()
  .range(d3.symbols)
  .domain(unique(nodes.map(d => d[shape_col])));
const draw_shape = node => d3.symbol().size(150)
  .type(Shape(node[shape_col]))();


const padding = 15;
const X = d3.scaleLinear()
  .range([padding, width-padding]);

const Y = d3.scaleLinear()
  .range([padding, height-padding]);

// Setup the simulation
const simulation = d3.forceSimulation(nodes)
      .force("link", d3.forceLink(links).id(d => d.id))
      .force("charge", d3.forceManyBody())
      .force("center", d3.forceCenter(width / 2, height / 2));

let not_being_dragged = true;

// Setup the node and link visual components
const link = svg.append("g")
  .attr("stroke", "#999")
  .attr("stroke-opacity", 0.6)
  .selectAll("line")
  .data(links)
  .enter().append("line")
  .attr("stroke-width", 1);

const node = svg.append("g")
  .attr("stroke", "#fff")
  .selectAll("path.node")
  .data(nodes)
  .enter().append('path')
  .classed('node', true)
  .attr('fill', color_node)
  .attr('d', draw_shape)
  .call(drag(simulation));

// Kickoff simulation
simulation.on("tick", () => {

 if(not_being_dragged){
   X.domain(d3.extent(nodes.map(d => d.x)));
   Y.domain(d3.extent(nodes.map(d => d.y)));
 }


  link
    .attr("x1", d => X(d.source.x))
    .attr("x2", d => X(d.target.x))
    .attr("y1", d => Y(d.source.y))
    .attr("y2", d => Y(d.target.y));

  node.attr('transform', d => `translate(${X(d.x)}, ${Y(d.y)})`);
});


function drag(simulation){
  function dragstarted(d) {
    if (!d3.event.active) simulation.alphaTarget(0.3).restart();
    d.fx = d.x;
    d.fy = d.y;
    not_being_dragged = false;
  }

  function dragged(d) {
    d.fx = d3.event.x;
    d.fy = d3.event.y;
  }

  function dragended(d) {
    if (!d3.event.active) simulation.alphaTarget(0);
    d.fx = null;
    d.fy = null;
    not_being_dragged = true;
  }

  return d3.drag()
      .on("start", dragstarted)
      .on("drag", dragged)
      .on("end", dragended);
}


function unique (vals){
  return [...new Set(vals)];
}

