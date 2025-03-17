#pragma once

#include <deque>
#include <cstdint>
#include <optional>
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
    using BitMapType = std::unordered_map<char32_t, std::uint32_t>;

    HuffmanTreeNode* m_root;

    void build_tree(std::deque<HuffmanTreeNode*>& nodes);
    void destroy_tree(HuffmanTreeNode* root);
    void construct_bit_map(HuffmanTreeNode* root, std::uint32_t value, BitMapType& bit_map);
    std::uint32_t get_tree_height(HuffmanTreeNode* root) const;
public:
    HuffmanTree(const std::unordered_map<char32_t, std::uint64_t>& char32_frequency_map);
    ~HuffmanTree();

    BitMapType get_bit_map();
    void print_tree();
};
}   // namespace kcompress