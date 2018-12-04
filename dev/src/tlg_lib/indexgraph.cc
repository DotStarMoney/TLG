#include "tlg_lib/indexgraph.h"
/*
#include <memory>
#include <vector>
#include "stdint.h"

#include "glog/logging.h"
#include "indexstagegraph.h"

struct Content;
typedef std::vector<uint8_t> Data_t;
typedef tlg_lib::StageGraph<Data_t, Content> StageGraph_t;

struct Content {
  std::vector<StageGraph_t::EmbeddedIndex> embeds;
  std::vector<bool> has_embeds;
  Data_t content;
};

namespace {
Data_t IGDataToData_t(ig_Data* c) {
  return Data_t(reinterpret_cast<uint8_t*>(c->data),
                reinterpret_cast<uint8_t*>(c->data) + c->size);
}
}  // namespace

void ig_Init() { google::InitGoogleLogging("indexgraph"); }

ig_GraphBuilder ig_CreateGraphBuilder() {
  return new StageGraph_t::Builder(StageGraph_t::builder());
}

void ig_DeleteGraphBuilder(ig_GraphBuilder* graph_builder) {
  delete (*graph_builder);
  *graph_builder = nullptr;
}

void ig_AddBaseToBuilder(ig_GraphBuilder graph_builder, const char* key,
                         ig_Data* sc, ig_Data* c) {
  StageGraph_t::Builder* builder =
      reinterpret_cast<StageGraph_t::Builder*>(graph_builder);
  builder->push(key, IGDataToData_t(sc), {{}, {}, IGDataToData_t(c)});
}

ig_IndexGraph ig_Build(ig_GraphBuilder* graph_builder) {
  StageGraph_t* graph = new StageGraph_t(
      reinterpret_cast<StageGraph_t::Builder*>(graph_builder)->buildAndClear());
  *graph_builder = nullptr;
  return graph;
}

void ig_DeleteGraph(ig_IndexGraph* graph) {
  delete (*graph);
  *graph = nullptr;
}

uint32_t ig_GetVersionN(const ig_IndexGraph graph) {
  return reinterpret_cast<const StageGraph_t*>(graph)->versions();
}

ig_Index ig_CreateIndex(ig_IndexGraph graph, const char* key) {
  return new StageGraph_t::Index(
      reinterpret_cast<StageGraph_t*>(graph)->CreateIndex(key));
}

ig_Index ig_CloneIndex(ig_Index index) {
  return new StageGraph_t::Index(
      reinterpret_cast<StageGraph_t::Index*>(index)->Clone());
}

void ig_DeleteIndex(ig_Index* index) {
  delete (*index);
  *index = nullptr;
}

void ig_ConsumeIndex(ig_Index dest, ig_Index* src) {
  reinterpret_cast<StageGraph_t::Index*>(dest)->Replace(
      std::move(*reinterpret_cast<StageGraph_t::Index*>(src)));
  delete (*src);
  *src = nullptr;
}

uint32_t ig_Embed(ig_Index index, ig_Index* to_embed) {
  StageGraph_t::Index* dst = reinterpret_cast<StageGraph_t::Index*>(index);
  StageGraph_t::Index* src = reinterpret_cast<StageGraph_t::Index*>(to_embed);

  Content content_copy;
  dst->GetContent([&content_copy, dst](const Content& c) {
    content_copy.content = c.content;
    content_copy.has_embeds = c.has_embeds;
    for (const auto& ei : c.embeds) {
      content_copy.embeds.push_back(dst->ReEmbed(ei));
    }
  });

  uint32_t i;
  for (i = 0; i < content_copy.embeds.size(); ++i) {
    if (content_copy.has_embeds[i]) break;
  }
  if (i == content_copy.has_embeds.size()) {
    content_copy.embeds.push_back(dst->Embed(std::move(*src)));
    content_copy.has_embeds.push_back(true);
  } else {
    content_copy.embeds[i] = dst->Embed(std::move(*src));
    content_copy.has_embeds[i] = true;
  }

  dst->SetContent(std::move(content_copy));

  *to_embed = nullptr;
  return i;
}

void ig_Unembed(ig_Index index, uint32_t elem) {
  StageGraph_t::Index* dst = reinterpret_cast<StageGraph_t::Index*>(index);

  Content content_copy;
  dst->GetContent([&content_copy, dst](const Content& c) {
    content_copy.content = c.content;
    content_copy.has_embeds = c.has_embeds;
    for (const auto& ei : c.embeds) {
      content_copy.embeds.push_back(dst->ReEmbed(ei));
    }
  });


}

uint32_t ig_GetEmbedN(const ig_Index index) {}
ig_Index ig_IndexFromEmbedded(ig_Index index, uint32_t elem) {}

const char* ig_GetKey(const ig_Index index) {}

const ig_Data ig_GetSharedContent(const ig_Index index) {}
void ig_SetSharedContent(ig_Index index, ig_Data* sc) {}

const ig_Data ig_GetContent(const ig_Index index) {}
void ig_SetContent(ig_Index index, ig_Data* sc) {}

void ig_Go(ig_Index index, const char* key) {}
*/