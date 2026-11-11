#include "psbc_compile.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

static_assert(sizeof(PsbcDynamicVertexInput) == 32);

static std::vector<std::uint32_t> ReadSpirv(const char* path)
{
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    const auto size = input.tellg();
    if (!input || size < 20 || size % 4) throw std::runtime_error("Invalid SPIR-V file");
    std::vector<std::uint32_t> words(static_cast<std::size_t>(size) / 4);
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(words.data()), size)) throw std::runtime_error("Cannot read SPIR-V");
    return words;
}

int main(int argc, char** argv)
{
    if (argc != 3) throw std::invalid_argument("Expected vertex and geometry SPIR-V fixtures");
    const auto words = ReadSpirv(argv[1]);
    const auto geometry = ReadSpirv(argv[2]);
    PsbcCompileOptions options{};
    options.target = PSBC_TARGET_PS5;
    options.stage = PSBC_STAGE_VERTEX;
    options.entrypoint = "main";
    options.optimise = true;
    options.ngg = true;
    options.omit_implicit_primitive_id = true;
    options.primitive_type = 4;
    options.address32_hi = 2;
    options.rasterization_samples = 1;
    options.dynamic_vertex_input = true;
    PsbcShaderOutput output{};
    if (psbc_compile_shader(words.data(), words.size() * 4, &options, &output) != PSBC_RESULT_OK) throw std::runtime_error("Dynamic vertex compilation failed");
    const auto metadata = output.metadata;
    if (!output.machine_code || output.machine_code_size == 0) throw std::runtime_error("Vertex code is empty");
    psbc_free_output(&output);
    if (metadata.version != PSBC_SHADER_METADATA_VERSION || !metadata.dynamic_vertex_input || metadata.vertex_buffer_descriptor_stride != 32 || metadata.vertex_buffer_usage_mask != 3 || !metadata.vertex_buffer_per_attribute || !metadata.base_vertex_valid || !metadata.start_instance_valid) throw std::runtime_error("Unexpected dynamic vertex ABI");
    for (unsigned variant = 0; variant < 3; ++variant)
    {
        auto invalid = options;
        if (variant == 0) invalid.target = PSBC_TARGET_PS4_BASE;
        if (variant == 1) invalid.stage = PSBC_STAGE_FRAGMENT;
        if (variant == 2) invalid.vertex_attribute_count = 1;
        if (psbc_compile_shader(words.data(), words.size() * 4, &invalid, &output) == PSBC_RESULT_OK) throw std::runtime_error("Invalid dynamic vertex configuration was accepted");
        if (output.data || output.machine_code || output.size || output.machine_code_size) throw std::runtime_error("Failed compile returned shader storage");
    }
    options.stage = PSBC_STAGE_GEOMETRY;
    if (psbc_compile_shader(geometry.data(), geometry.size() * 4, &options, &output) == PSBC_RESULT_OK) throw std::runtime_error("Unpaired dynamic geometry was accepted");
    if (psbc_compile_geometry_pipeline(words.data(), words.size() * 4, geometry.data(), geometry.size() * 4, &options, &output) != PSBC_RESULT_OK) throw std::runtime_error("Dynamic geometry compilation failed");
    const auto geometryMetadata = output.metadata;
    if (!output.machine_code || !output.machine_code_size) throw std::runtime_error("Geometry code is empty");
    psbc_free_output(&output);
    if (!geometryMetadata.merged_geometry || geometryMetadata.source_stage != PSBC_STAGE_GEOMETRY || geometryMetadata.merged_es_source_stage != PSBC_STAGE_VERTEX || !geometryMetadata.dynamic_vertex_input || geometryMetadata.vertex_buffer_descriptor_stride != 32 || geometryMetadata.vertex_buffer_usage_mask != 3 || !geometryMetadata.vertex_buffer_per_attribute || !geometryMetadata.base_vertex_valid || !geometryMetadata.start_instance_valid) throw std::runtime_error("Unexpected dynamic geometry ABI");
    options.stage = PSBC_STAGE_VERTEX;
    options.dynamic_vertex_input = false;
    options.vertex_attribute_count = 2;
    options.vertex_attributes[0] = {.location = 0, .binding = 0, .format = PSBC_VERTEX_FORMAT_R32G32B32_FLOAT, .offset = 0, .stride = 12, .alignment = 4, .instance_divisor = 0};
    options.vertex_attributes[1] = {.location = 1, .binding = 1, .format = PSBC_VERTEX_FORMAT_R32G32B32A32_FLOAT, .offset = 0, .stride = 16, .alignment = 4, .instance_divisor = 0};
    if (psbc_compile_shader(words.data(), words.size() * 4, &options, &output) != PSBC_RESULT_OK) throw std::runtime_error("Static vertex compilation failed");
    const auto staticMetadata = output.metadata;
    psbc_free_output(&output);
    if (staticMetadata.dynamic_vertex_input || staticMetadata.vertex_buffer_descriptor_stride != 16 || staticMetadata.vertex_buffer_per_attribute) throw std::runtime_error("Static vertex ABI changed");
    std::cout << "Dynamic and static vertex input metadata verified\n";
}
