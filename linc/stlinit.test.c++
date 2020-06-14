#include <iostream>

#include <linc/test-framework.h++>

#include <linc/stl.h++>

static auto
getPath(std::string const &newFile,
        std::string const &thisFile = SourceLoc::current().file_name())
    -> std::string {
  return thisFile.substr(0, thisFile.find("stlinit.test.c++")) + newFile;
}

auto main() -> int {
  // 32-bit float gives 8 decimals precision...
  auto constexpr maxRelativeError = 0.0000001F;
  try {
    {
      Stl const stl{getPath("test-models/broken/empty.stl")};
      compare(stl.m_stats.number_of_facets, 0U);
      compare(stl.m_stats.size, Vertex{0, 0, 0});
      check(not stl.m_initialized);
    }
    {
      Stl const stl{getPath("test-models/broken/faceless.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 0U);
      compare(stl.m_stats.size, Vertex{0, 0, 0});
      // Although this is might be a valid ascii stl file in some philosophical
      // sense, I don't want to add logic to make m_initialized true after
      // trying to read it
      check(not stl.m_initialized);
    }
    {
      Stl const stl{getPath("test-models/wrong-header-length.binary.stl")};
      compare(stl.m_stats.number_of_facets, 0U);
      compare(stl.m_stats.size, Vertex{0, 0, 0});
      check(not stl.m_initialized);
    }
    {
      Stl const stl{getPath("test-models/broken/two-vertices.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 0U);
      compare(stl.m_stats.size, Vertex{0, 0, 0});
      check(not stl.m_initialized);
    }
    {
      Stl const stl{getPath("test-models/small-cube.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 12U);
      compare(stl.m_stats.type, Stl::Type::ASCII);
      check(stl.m_stats.size.isApprox(Vertex{10, 10, 10}, maxRelativeError));
      check(stl.m_initialized);
    }
    {
      Stl const stl{getPath("test-models/small-cube.binary.stl")};
      compare(stl.m_stats.number_of_facets, 12U);
      compare(stl.m_stats.type, Stl::Type::BINARY);
      check(stl.m_stats.size.isApprox(Vertex{10, 10, 10}, maxRelativeError));
      check(stl.m_initialized);
    }
    {
      Stl const stl{getPath("test-models/broken/3DBenchy.binary.stl")};
      compare(stl.m_stats.number_of_facets, 225706U);
      compare(stl.m_stats.type, Stl::Type::BINARY);
      check(stl.m_stats.size.isApprox(Vertex{60, 31, 48}, 0.0001F));
      check(stl.m_stats.shortest_edge < 0.07401F and
            stl.m_stats.shortest_edge > 0.07399F);
      check(stl.m_initialized);
    }
    {
      Stl const stl{getPath("test-models/broken/standing-triangle.ascii.stl")};
      // contents
      compare(stl.m_facets.size(), 1U);
      compare(stl.m_facets[0].normal, Normal{1, 0, 0});
      compare(stl.m_facets[0].vertices[0], Vertex{0, -5, 0});
      compare(stl.m_facets[0].vertices[1], Vertex{0, 5, 0});
      compare(stl.m_facets[0].vertices[2], Vertex{0, 0, 10});
      // stats
      compare(stl.m_stats.number_of_facets, 1U);
      compare(stl.m_stats.type, Stl::Type::ASCII);
      compare(stl.m_stats.max, Vertex{0, 5, 10});
      compare(stl.m_stats.min, Vertex{0, -5, 0});
      compare(stl.m_stats.size, Vertex{0, 10, 10});
      compare(stl.m_stats.bounding_diameter, stl.m_stats.size.norm());
      compare(stl.m_stats.shortest_edge, 10.0F);
      compare(stl.m_stats.volume, -1.0F); // default
      check(stl.m_stats.size.isApprox(Vertex{0, 10, 10}, maxRelativeError));
      // misc
      check(stl.m_initialized);
    }
    {
      Stl const stl{getPath("test-models/broken/four-vertices.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 4U);
      compare(stl.m_facets.size(), 4U);
      // Facet 0 has four vertices
      // We want the parser to just skip the fourth vertex
      // and keep parsing the rest of the facets as if nothing happened
      check(stl.m_facets[0].normal.isApprox(
          Normal{0.57735027F, 0.57735027F, 0.57735027F}, maxRelativeError));
      compare(stl.m_facets[0].vertices[0], Vertex{1, 0, 0});
      compare(stl.m_facets[0].vertices[1], Vertex{0, 1, 0});
      compare(stl.m_facets[0].vertices[2], Vertex{0, 0, 1});
      // Facet 1
      compare(stl.m_facets[1].normal, Normal{0, -1, 0});
      compare(stl.m_facets[1].vertices[0], Vertex{0, 0, 0});
      compare(stl.m_facets[1].vertices[1], Vertex{1, 0, 0});
      compare(stl.m_facets[1].vertices[2], Vertex{0, 0, 1});
      // Facet 2
      compare(stl.m_facets[2].normal, Normal{-1, 0, 0});
      compare(stl.m_facets[2].vertices[0], Vertex{0, 0, 0});
      compare(stl.m_facets[2].vertices[1], Vertex{0, 0, 1});
      compare(stl.m_facets[2].vertices[2], Vertex{0, 1, 0});
      // Facet 3
      compare(stl.m_facets[3].normal, Normal{0, 0, -1});
      compare(stl.m_facets[3].vertices[0], Vertex{0, 0, 0});
      compare(stl.m_facets[3].vertices[1], Vertex{0, 1, 0});
      compare(stl.m_facets[3].vertices[2], Vertex{1, 0, 0});
    }
    {
      Stl const stl{
          getPath("test-models/broken/incorrect-face-counter.binary.stl")};
      compare(stl.m_stats.number_of_facets, 4U);
      compare(stl.m_facets.size(), 4U);
      compare(stl.m_stats.header_num_facets, 66U);
      compare(stl.m_facets[3].vertices[2], Vertex{1, 0, 0});
    }
    {
      Stl const stl{getPath("test-models/broken/missing-endsolid.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 4U);
      compare(stl.m_facets.size(), 4U);
      compare(stl.m_facets[3].vertices[2], Vertex{1, 0, 0});
    }
    {
      Stl const stl{getPath("test-models/broken/missing-facet.ascii.stl")};
      // When there's a missing face we have a hole in the model.
      // The parser should not mind such things
      compare(stl.m_stats.number_of_facets, 3U);
    }
    {
      Stl const stl{getPath("test-models/broken/missing-normal.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 4U);
      compare(stl.m_facets.size(), 4U);
      // Facet 3 has no normal
      compare(stl.m_facets[3].normal, Normal{0, 0, 0});
      compare(stl.m_facets[3].vertices[0], Vertex{0, 0, 1});
      compare(stl.m_facets[3].vertices[1], Vertex{1, 0, 0});
      compare(stl.m_facets[3].vertices[2], Vertex{0, 1, 0});
    }
    {
      Stl const stl{
          getPath("test-models/broken/not-a-number-normal.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 4U);
      compare(stl.m_facets.size(), 4U);
      // Facet 1 has NaN in its normal
      compare(stl.m_facets[1].normal, Normal{0, 0, 0});
      compare(stl.m_facets[1].vertices[0], Vertex{0, 0, 0});
      compare(stl.m_facets[1].vertices[1], Vertex{1, 0, 0});
      compare(stl.m_facets[1].vertices[2], Vertex{0, 0, 1});
    }
    {
      // This model will have a hole since fourth vertex of first facet will be
      // ignored
      // Parser shouldn't care
      Stl const stl{getPath("test-models/broken/quad.ascii.stl")};
    }
    {
      Stl const stl{
          getPath("test-models/broken/solid-name-mismatch.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 4U);
      compare(stl.m_facets.size(), 4U);
      compare(stl.m_facets[3].vertices[2], Vertex{1, 0, 0});
    }
    {
      // There's something wrong in the header of this binary stl
      // For example, it starts with "solid"
      // I don't know what more is wrong in the header, but the parser shouldn't
      // care anyways. It should find a perfect, xyz-centered cube with sides of
      // length 100
      Stl const stl{getPath("test-models/broken/wrong-header.binary.stl")};
      compare(stl.m_stats.number_of_facets, 12U);
      compare(stl.m_stats.size, Vertex{100, 100, 100});
      compare(stl.m_stats.max, Vertex{50, 50, 50});
      compare(stl.m_stats.min, Vertex{-50, -50, -50});
      compare(stl.m_stats.shortest_edge, 100.0F);
    }
    {
      Stl const stl{getPath("test-models/broken/wrong-normal.ascii.stl")};
      // This stl has a normal that doesn't match its vertexes
      // The parser shouldn't care about this
      // It should just parse the numbers it finds
      compare(stl.m_stats.number_of_facets, 4U);
      compare(stl.m_facets.size(), 4U);
      // Facet 3 has a wrong normal
      compare(stl.m_facets[3].normal, Normal{0, 0, -1});
      compare(stl.m_facets[3].vertices[0], Vertex{0, 0, 1});
      compare(stl.m_facets[3].vertices[1], Vertex{1, 0, 0});
      compare(stl.m_facets[3].vertices[2], Vertex{0, 1, 0});
    }
    {
      Stl const stl{getPath("test-models/broken/zeroed-normals.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 4U);
      compare(stl.m_facets.size(), 4U);
      // Facet 0
      compare(stl.m_facets[0].normal, Normal{0, 0, 0});
      compare(stl.m_facets[0].vertices[0], Vertex{1, 0, 0});
      compare(stl.m_facets[0].vertices[1], Vertex{0, 1, 0});
      compare(stl.m_facets[0].vertices[2], Vertex{0, 0, 1});
      // Facet 1
      compare(stl.m_facets[1].normal, Normal{0, 0, 0});
      compare(stl.m_facets[1].vertices[0], Vertex{0, 0, 0});
      compare(stl.m_facets[1].vertices[1], Vertex{1, 0, 0});
      compare(stl.m_facets[1].vertices[2], Vertex{0, 0, 1});
      // Facet 2
      compare(stl.m_facets[2].normal, Normal{0, 0, 0});
      compare(stl.m_facets[2].vertices[0], Vertex{0, 0, 0});
      compare(stl.m_facets[2].vertices[1], Vertex{0, 0, 1});
      compare(stl.m_facets[2].vertices[2], Vertex{0, 1, 0});
      // Facet 3
      compare(stl.m_facets[3].normal, Normal{0, 0, 0});
      compare(stl.m_facets[3].vertices[0], Vertex{0, 0, 0});
      compare(stl.m_facets[3].vertices[1], Vertex{0, 1, 0});
      compare(stl.m_facets[3].vertices[2], Vertex{1, 0, 0});
    }
    {
      Stl const stl{
          getPath("test-models/broken/nameless-solid-without-space.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 4U);
      check(stl.m_initialized);
      compare(stl.m_facets[3].vertices[2], Vertex{1, 0, 0});
    }
    {
      Stl const stl{
          getPath("test-models/broken/nameless-solid-with-space.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 4U);
      check(stl.m_initialized);
      compare(stl.m_facets[3].vertices[2], Vertex{1, 0, 0});
    }
    {
      Stl const stl{getPath("test-models/multi-word-name.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 4U);
      check(stl.m_initialized);
      compare(stl.m_facets[3].vertices[2], Vertex{1, 0, 0});
    }
    {
      // These normals are not unit vectors as they should be but parser
      // shouldn't care
      Stl const stl{
          getPath("test-models/broken/non-normalized-normals.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 4U);
      compare(stl.m_facets.size(), 4U);
      check(stl.m_facets[0].normal.isApprox(
          Normal{5.7735027F, 5.7735027F, 5.7735027F}, maxRelativeError));
      compare(stl.m_facets[1].normal, Normal{0, -10, 0});
      compare(stl.m_facets[2].normal, Normal{-100, 0, 0});
      compare(stl.m_facets[3].normal, Normal{0, 0, -1000});
    }
    {
      // The parser should be able to handle a 20 m wide (among x, y, or z)
      // model, and crazy scaled normals
      Stl const stl{getPath("test-models/large-numbers.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 4U);
      check(stl.m_initialized);
      check(stl.m_facets[3].normal.isApprox(Vertex{0, 0, -9999999.9F},
                                            maxRelativeError));
      check(stl.m_facets[3].vertices[1].isApprox(Vertex{0, 10000.0F, 0},
                                                 maxRelativeError));
      check(stl.m_facets[3].vertices[2].isApprox(Vertex{10000.0F, 0, 0},
                                                 maxRelativeError));
    }
    {
      // Some ascii stl generators tend to produce text after "endloop" and
      // "endfacet". Just ignore it.
      Stl const stl{getPath(
          "test-models/broken/text-after-endloop-and-endfacet.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 4U);
      check(stl.m_initialized);
      compare(stl.m_facets[3].vertices[2], Vertex{1, 0, 0});
    }
    {
      // Same tetrahedron file, but broken up with solid/endsolid statements.
      // Just ignore them.
      Stl const stl{
          getPath("test-models/broken/many-solid-endsolid.ascii.stl")};
      compare(stl.m_stats.number_of_facets, 4U);
      check(stl.m_initialized);
      compare(stl.m_facets[3].vertices[2], Vertex{1, 0, 0});
    }
  } catch (...) {
    return 1;
  }

  return 0;
}
