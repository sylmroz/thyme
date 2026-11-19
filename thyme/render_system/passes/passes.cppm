export module th.render_system.passes;

import th.render_system.render_graph;

export import :mypass;

export namespace th {
    void copyTextureToTexturePass(std::string name, RenderGraph& render_graph, RenderGraphResource from, RenderGraphResource to);
}
