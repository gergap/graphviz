#include <string>

#include <catch2/catch.hpp>

#include "test_edge_node_overlap_utilities.h"
#include "test_utilities.h"

// FIXME: this test fails for the 'l' and 'r' arrow shape modifiers which do not
// handle the miter point correctly yet

TEST_CASE(
    "Edge node overlap for normal and inv arrow with all arrow shape modifiers",
    "[!shouldfail] An edge connected to a node shall touch that node and not "
    "overlap it too much, regardless of if the arrow shape is 'normal' or "
    "'inv' and which arrrow shape modifier is used") {

  std::string filename_base = "test_edge_node_overlap_all_edge_arrows";

  const std::string_view primitive_arrow_shape = GENERATE("normal", "inv");
  INFO("Edge primitive arrow shape: " << primitive_arrow_shape);

  const auto arrow_shape_modifier =
      GENERATE(from_range(all_arrow_shape_modifiers));
  INFO("Edge arrow shape modifier: " << arrow_shape_modifier);

  const auto arrow_shape =
      fmt::format("{}{}", arrow_shape_modifier, primitive_arrow_shape);
  INFO("Edge arrow shape: " << arrow_shape);
  filename_base += fmt::format("_arrow_{}", arrow_shape);

  const graph_options graph_options = {
      .node_shape = "polygon",
      .node_penwidth = 2,
      .dir = "both",
      .edge_penwidth = 2,
      .primitive_arrowhead_shape = primitive_arrow_shape,
      .primitive_arrowtail_shape = primitive_arrow_shape,
      .arrowhead_modifier = arrow_shape_modifier,
      .arrowtail_modifier = arrow_shape_modifier,
  };

  test_edge_node_overlap(graph_options, {}, {.filename_base = filename_base});
}
