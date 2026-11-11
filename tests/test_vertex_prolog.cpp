#include "psbc_compile.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

int main(int argc, char** argv)
{
    if (argc != 2) throw std::invalid_argument("Expected vertex prolog SPIR-V fixture");
    std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
    const auto size = input.tellg();
    if (!input || size < 20 || size % 4) throw std::runtime_error("Invalid SPIR-V file");
    std::vector<std::uint32_t> words(static_cast<std::size_t>(size) / 4);
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(words.data()), size)) throw std::runtime_error("Cannot read SPIR-V");
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
    if (metadata.version != PSBC_SHADER_METADATA_VERSION || !metadata.vertex_prolog_valid || metadata.vertex_prolog_attribute_count != 2 || metadata.vertex_buffer_usage_mask != 3 || !metadata.vertex_buffer_per_attribute || metadata.vertex_prolog_inputs_user_data_dword + 2 > metadata.user_sgpr_count) throw std::runtime_error("Unexpected dynamic vertex ABI");
    for (unsigned variant = 0; variant < 3; ++variant)
    {
        auto invalid = options;
        if (variant == 0) invalid.target = PSBC_TARGET_PS4_BASE;
        if (variant == 1) invalid.stage = PSBC_STAGE_FRAGMENT;
        if (variant == 2) invalid.vertex_attribute_count = 1;
        if (psbc_compile_shader(words.data(), words.size() * 4, &invalid, &output) == PSBC_RESULT_OK) throw std::runtime_error("Invalid dynamic vertex configuration was accepted");
        if (output.data || output.machine_code || output.size || output.machine_code_size) throw std::runtime_error("Failed compile returned shader storage");
    }
    std::cout << "Dynamic vertex input metadata verified\n";
}
