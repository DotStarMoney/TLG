#ifndef TLG_LIB_INDEXGRAPH_H_
#define TLG_LIB_INDEXGRAPH_H_

#include <stdint.h>

typedef void* ig_GraphBuilder;
typedef void* ig_IndexGraph;
typedef void* ig_Index;

struct ig_Data {
  unsigned long size;
  void* data;
};

extern "C" {
void ig_Init();

ig_GraphBuilder ig_CreateGraphBuilder();

void ig_DeleteGraphBuilder(ig_GraphBuilder* graph_builder);

void ig_AddBaseToBuilder(ig_GraphBuilder graph_builder, const char* key,
                         ig_Data* sc, ig_Data* c);

ig_IndexGraph ig_Build(ig_GraphBuilder* graph_builder);

void ig_DeleteGraph(ig_IndexGraph* graph);

uint32_t ig_GetVersionN(const ig_IndexGraph graph);

ig_Index ig_CreateIndex(ig_IndexGraph graph);
ig_Index ig_CloneIndex(ig_Index index);
void ig_DeleteIndex(ig_Index* index);
void ig_ConsumeIndex(ig_Index dest, ig_Index* src);

uint32_t ig_Embed(ig_Index index, ig_Index* to_embed);
void ig_Unembed(ig_Index index, uint32_t elem);

uint32_t ig_GetEmbedN(const ig_Index index);
ig_Index ig_IndexFromEmbedded(ig_Index index, uint32_t elem);

const char* ig_GetKey(const ig_Index index);

const ig_Data ig_GetSharedContent(const ig_Index index);
void ig_SetSharedContent(ig_Index index, ig_Data* sc);

const ig_Data ig_GetContent(const ig_Index index);
void ig_SetContent(ig_Index index, ig_Data* sc);

void ig_Go(ig_Index index, const char* key);
}
#endif  // TLG_LIB_INDEXGRAPH_H_
