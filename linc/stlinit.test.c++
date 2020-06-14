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
  auto constexpr maxRelativeError = 0.0001F;
  try {
    {
      Stl const stl{getPath("test-models/empty.stl")};
      compare(stl.m_stats.number_of_facets, 0U);
      compare(stl.m_stats.type, Stl::Type::BINARY);
      check(stl.m_stats.size.isApprox(Vertex{0, 0, 0}, maxRelativeError));
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
    }
    {
      Stl const stl{getPath("test-models/3DBenchy.binary.stl")};
      compare(stl.m_stats.number_of_facets, 225706U);
      compare(stl.m_stats.type, Stl::Type::BINARY);
      check(stl.m_stats.size.isApprox(Vertex{60, 31, 48}, maxRelativeError));
      check(stl.m_stats.shortest_edge < 0.07401F and
            stl.m_stats.shortest_edge > 0.07399F);
      check(stl.m_initialized);
    }
    {
      Stl const stl{getPath("test-models/standing-triangle.ascii.stl")};
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
      Stl const stl{getPath("test-models/four-vertices.ascii.stl")};
      // Facet 0 has four vertices
      // We want the parser to just skip the fourth vertex
      // and keep parsing the rest of the facets as if nothing happened
      compare(stl.m_facets.size(), 4U);
      compare(stl.m_stats.number_of_facets, 4U);
      check(stl.m_facets[0].normal.isApprox(
          Normal{0.57735027F, 0.57735027F, 0.57735027F}, maxRelativeError));
      // The first three vertices of facet 0
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
      Stl const stl{getPath("test-models/four-vertices.ascii.stl")};
      compare();
    }
  } catch (...) {
    return 1;
  }

  return 0;
}
