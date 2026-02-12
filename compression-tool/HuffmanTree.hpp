#pragma once

#include <queue>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <array>

namespace kcompress
{
struct HuffmanTreeNode
{
    std::optional<unsigned char> m_char;
    std::uint64_t m_value;
    HuffmanTreeNode* m_left;    // HuffmanTreeNode doesn't take ownership (doesn't free memory)
    HuffmanTreeNode* m_right;   // HuffmanTreeNode doesn't take ownership (doesn't free memory)

    explicit HuffmanTreeNode(std::optional<unsigned char> c = std::nullopt, std::uint64_t value = 0, HuffmanTreeNode* left = nullptr, HuffmanTreeNode* right = nullptr);
    ~HuffmanTreeNode();

    bool is_leaf_node() const;
    bool operator>(const HuffmanTreeNode& other) const;
};

class HuffmanTree
{
    using BitMapType = std::array<std::optional<std::vector<bool>>, 256>;
    using FrequencyMapType = std::array<std::uint64_t, 256>;
    using HeapType = std::priority_queue<HuffmanTreeNode*, std::vector<HuffmanTreeNode*>, std::greater<>>;

    HuffmanTreeNode* m_root;
    BitMapType m_bit_map;
    std::uint32_t m_tree_height;

    void build_tree(HeapType& nodes);
    void destroy_tree(HuffmanTreeNode* root);
    void construct_bit_map(HuffmanTreeNode* root, std::vector<bool>& value);
    std::uint32_t get_tree_height(HuffmanTreeNode* root) const;
    HuffmanTreeNode* deserialize(const std::vector<unsigned char>& serialized_tree, std::uint64_t serialized_tree_bits,
                    std::size_t& index, std::uint64_t& total_processed_bits, unsigned char& remaining_bits, HuffmanTreeNode* root);
public:
    HuffmanTree();
    HuffmanTree(const FrequencyMapType& frequency_map);
    ~HuffmanTree();

    const BitMapType& get_bit_map() const;
    std::uint32_t get_tree_height() const;
    void print_tree();

    std::vector<unsigned char> serialize(std::uint64_t& total_bits);
    std::vector<unsigned char> serialize_payload(const std::vector<unsigned char>& char_sequence, std::uint64_t& total_bits);
    void deserialize(const std::vector<unsigned char>& serialized_tree, std::uint64_t serialized_tree_bits);
    std::vector<unsigned char> deserialize_payload(const std::vector<unsigned char>& serialized_payload, std::uint64_t serialized_payload_bits, std::uint64_t original_file_bytes);
};
}   // namespace kcompress