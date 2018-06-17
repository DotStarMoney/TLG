#include "tlg_lib/indexstagegraph.h"

#include <functional>
#include <memory>

#include "gtest/gtest.h"

using std::string;
using tlg_lib::StageGraph;

namespace {
template <typename C = string>
StageGraph<string, C> MakeSimpleGraph() {
  auto graph_builder = StageGraph<string, C>::builder();
  graph_builder.push("a", "shared_content_a", "Content_a")
      .push("b", "shared_content_b", "Content_b")
      .push("c", "shared_content_c", "Content_c");

  return graph_builder.buildAndClear();
}

template <typename C = string>
const C& GetContent(typename StageGraph<string, C>::Index* index) {
  const C* content = nullptr;
  index->GetContent([&content](const C& x) { content = &x; });
  return *content;
}
}  // namespace

TEST(IndexStageGraphTest, JumpBackwards) {
  auto graph = MakeSimpleGraph();

  auto index_a = graph.CreateIndex("a");
  auto index_b = index_a.Clone();

  // Update content at index_b
  index_b.SetContent("new");

  ASSERT_EQ(GetContent(&index_a), "Content_a");
  ASSERT_EQ(GetContent(&index_b), "new");

  // Make sure we still see version 0 in a stage that isn't a.
  index_b.Go("c");
  ASSERT_EQ(GetContent(&index_b), "Content_c");

  ASSERT_EQ(graph.versions(), 2);

  // Replace, or "jump" index b to index a. We'll lose a version and our view
  // will change.
  index_b.Replace(std::move(index_a));

  ASSERT_FALSE(index_a.valid());
  ASSERT_EQ(graph.versions(), 1);

  ASSERT_EQ(GetContent(&index_b), "Content_a");
}

TEST(IndexStageGraphTest, JumpForwards) {
  auto graph = MakeSimpleGraph();

  auto index_a = graph.CreateIndex("a");
  auto index_b = index_a.Clone();

  index_b.SetContent("new");

  ASSERT_EQ(GetContent(&index_a), "Content_a");
  ASSERT_EQ(GetContent(&index_b), "new");
  ASSERT_EQ(graph.versions(), 2);

  // Jump index a to index b. Since vers 0 is pinned by vers 1, we'll still
  // end up with 2 versions.
  index_a.Replace(std::move(index_b));

  ASSERT_FALSE(index_b.valid());
  ASSERT_EQ(graph.versions(), 2);

  ASSERT_EQ(GetContent(&index_a), "new");
}

TEST(IndexStageGraphTest, UpdateBaseVersMakesNewVers) {
  auto graph = MakeSimpleGraph();

  auto index_a = graph.CreateIndex("a");
  index_a.SetContent("broken pot");
  ASSERT_EQ(graph.versions(), 2);
}

struct ContentWithEmbedding {
  ContentWithEmbedding() {}
  // So can be used with MakeSimpleGraph
  ContentWithEmbedding(const char* x) : text(x) {}
  ContentWithEmbedding(
      const char* x,
      StageGraph<string, ContentWithEmbedding>::EmbeddedIndex&& embedded)
      : embedded(std::move(embedded)), text(x) {}
  ContentWithEmbedding(
      const char* x,
      StageGraph<string, ContentWithEmbedding>::EmbeddedIndex&& embedded,
      StageGraph<string, ContentWithEmbedding>::EmbeddedIndex&& extra_embedded)
      : embedded(std::move(embedded)),
        extra_embedded(std::move(extra_embedded)),
        text(x) {}

  ContentWithEmbedding(ContentWithEmbedding&& x)
      : embedded(std::move(x.embedded)),
        extra_embedded(std::move(x.extra_embedded)),
        text(std::move(x.text)) {}
  ContentWithEmbedding& operator=(ContentWithEmbedding&& x) {
    embedded = std::move(x.embedded);
    extra_embedded = std::move(x.extra_embedded);
    text = std::move(x.text);
    return *this;
  }

  StageGraph<string, ContentWithEmbedding>::EmbeddedIndex embedded;
  StageGraph<string, ContentWithEmbedding>::EmbeddedIndex extra_embedded;
  string text;
};

// Test a sequence of jumps performed from embedded indexes
TEST(IndexStageGraphTest, JumpSeqWithEmbedding) {
  auto graph = MakeSimpleGraph<ContentWithEmbedding>();

  auto index_a = graph.CreateIndex("a");
  auto index_b = index_a.Clone();

  index_a.Go("b");

  index_a.SetContent("adding picture base");
  index_a.SetContent({"adding picture", index_a.Embed(std::move(index_b))});

  ASSERT_FALSE(index_b.valid());

  auto index_c = index_a.Clone();

  const auto& content = GetContent<ContentWithEmbedding>(&index_a);
  ASSERT_EQ(content.text, "adding picture");

  index_a.Replace(index_a.IndexFromEmbedded(content.embedded));
  ASSERT_EQ(graph.versions(), 2);

  index_a.SetContent("adding second picture base");
  index_a.SetContent(
      {"adding second picture", index_a.Embed(std::move(index_c))});

  const auto& content2 = GetContent<ContentWithEmbedding>(&index_a);
  ASSERT_EQ(content2.text, "adding second picture");
  ASSERT_EQ(graph.versions(), 3);

  index_a.Replace(index_a.IndexFromEmbedded(content2.embedded));
  ASSERT_EQ(graph.versions(), 2);
  ASSERT_EQ(index_a.GetStage(), "b");

  const auto& content3 = GetContent<ContentWithEmbedding>(&index_a);
  ASSERT_EQ(content3.text, "adding picture");

  index_a.Replace(index_a.IndexFromEmbedded(content3.embedded));
  ASSERT_EQ(graph.versions(), 1);
  ASSERT_EQ(index_a.GetStage(), "a");

  const auto& content4 = GetContent<ContentWithEmbedding>(&index_a);
  ASSERT_EQ(content4.text, "Content_a");

  index_a.Go("b");

  const auto& content5 = GetContent<ContentWithEmbedding>(&index_a);
  ASSERT_EQ(content5.text, "Content_b");
}

TEST(IndexStageGraphTest, UnembedWithCompact) {
  auto graph = MakeSimpleGraph<ContentWithEmbedding>();

  auto index_a = graph.CreateIndex("c");
  auto index_b = index_a.Clone();

  index_a.SetContent("make new vers a");
  index_b.SetContent("make new vers b");

  ASSERT_EQ(graph.versions(), 3);

  index_a.SetContent({"adding link", index_a.Embed(std::move(index_b))});

  // Make a temp index to manually trigger compaction.
  auto temp_index = index_a.Clone();
  temp_index.Release();

  // Since we embedded the version from index_b in index_a, a compaction should
  // leave 3 versions hanging around
  ASSERT_EQ(graph.versions(), 3);

  // Now unembed index_b from index_a and immediately release it. This should
  // clear a version.
  const auto& content = GetContent<ContentWithEmbedding>(&index_a);
  auto index_c(index_a.Unembed(content.embedded));
  index_c.Release();

  ASSERT_EQ(graph.versions(), 2);
}

TEST(IndexStageGraphTest, MultipleEmbeddings) {
  auto graph = MakeSimpleGraph<ContentWithEmbedding>();

  auto index_a = graph.CreateIndex("c");
  auto index_b0 = index_a.Clone();

  index_a.SetContent("make new vers a");
  index_b0.SetContent("make new vers b");

  auto index_b1 = index_b0.Clone();

  ASSERT_EQ(graph.versions(), 3);

  index_a.SetContent({"adding links", index_a.Embed(std::move(index_b0)),
                      index_a.Embed(std::move(index_b1))});

  const auto& content = GetContent<ContentWithEmbedding>(&index_a);
  auto temp_index(index_a.Unembed(content.embedded));
  temp_index.Release();

  // Should still be 3 versions because of extra embedding
  ASSERT_EQ(graph.versions(), 3);

  auto temp_index_extra = index_a.Unembed(content.extra_embedded);
  temp_index_extra.Release();

  // Both embeddings are gone, expect only 2 versions
  ASSERT_EQ(graph.versions(), 2);
}
