#pragma once

#include <deque>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

namespace kcompress
{
struct HuffmanTreeNode
{
    std::optional<char32_t> m_utf32_codepoint;
    std::uint64_t m_value;
    HuffmanTreeNode* m_left;    // doesn't take ownership (doesn't free memory)
    HuffmanTreeNode* m_right;   // doesn't take ownership (doesn't free memory)

    explicit HuffmanTreeNode(std::optional<char32_t> wc = std::nullopt, std::uint64_t value = 0, HuffmanTreeNode* left = nullptr, HuffmanTreeNode* right = nullptr);
    ~HuffmanTreeNode();

    bool is_leaf_node() const;
    bool operator>(const HuffmanTreeNode& other) const;
};

class HuffmanTree
{
    using BitMapType = std::unordered_map<char32_t, std::string>;

    HuffmanTreeNode* m_root;
    std::uint32_t m_tree_height;
    BitMapType m_bit_map;

    void build_tree(std::deque<HuffmanTreeNode*>& nodes);
    void destroy_tree(HuffmanTreeNode* root);
    void construct_bit_map(HuffmanTreeNode* root, std::string value);
    std::uint32_t get_tree_height(HuffmanTreeNode* root) const;
    HuffmanTreeNode* deserialize(const std::vector<unsigned char>& serialized_tree, std::uint64_t serialized_tree_bits,
                    std::size_t& index, std::uint64_t& total_processed_bits, unsigned char& remaining_bits, HuffmanTreeNode* root);
public:
    HuffmanTree();
    HuffmanTree(const std::unordered_map<char32_t, std::uint64_t>& char32_frequency_map);
    ~HuffmanTree();

    BitMapType get_bit_map() const;
    std::uint32_t get_tree_height() const;
    void print_tree();

    std::vector<unsigned char> serialize(std::uint64_t& total_bits);
    void deserialize(const std::vector<unsigned char>& serialized_tree, std::uint64_t serialized_tree_bits);
    std::vector<char32_t> deserialize_payload(const std::vector<unsigned char>& serialized_payload, std::uint64_t serialized_payload_bits);
};
}   // namespace kcompress