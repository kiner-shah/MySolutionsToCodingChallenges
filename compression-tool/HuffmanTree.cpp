#include "HuffmanTree.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <stack>
#include <bitset>

namespace kcompress
{
HuffmanTreeNode::HuffmanTreeNode(std::optional<unsigned char> c, std::uint64_t value, HuffmanTreeNode *left, HuffmanTreeNode *right)
    : m_char{c}, m_value{value}, m_left{left}, m_right{right}
{
}

HuffmanTreeNode::~HuffmanTreeNode()
{
    m_char = std::nullopt;
    m_value = 0;
    m_left = nullptr;
    m_right = nullptr;
}

bool HuffmanTreeNode::is_leaf_node() const
{
    return m_char.has_value() && m_left == nullptr && m_right == nullptr;
}

bool HuffmanTreeNode::operator>(const HuffmanTreeNode &other) const
{
    return this->m_value > other.m_value;
}

void HuffmanTree::build_tree(HeapType& nodes)
{
    while (nodes.size() >= 2)
    {
        auto left = nodes.top(); nodes.pop();
        auto right = nodes.top(); nodes.pop();
        std::uint64_t new_value = left->m_value + right->m_value;
        HuffmanTreeNode* node = new HuffmanTreeNode(std::nullopt, new_value, left, right);
        nodes.push(node);
    }
}

void HuffmanTree::destroy_tree(HuffmanTreeNode *root)
{
    // Post-order traversal
    if (root == nullptr)
    {
        return;
    }
    destroy_tree(root->m_left);
    destroy_tree(root->m_right);
    delete root;
    root = nullptr;
}

void HuffmanTree::construct_bit_map(HuffmanTreeNode *root, std::vector<bool>& value)
{
    if (root == nullptr)
    {
        return;
    }
    if (root->is_leaf_node())
    {
        // std::cout << root->m_char.value() << ' ' << value << '\n';
        m_bit_map[root->m_char.value()] = value;
        return;
    }
    if (root->m_left)
    {
        value.push_back(false);
        construct_bit_map(root->m_left, value);
        value.pop_back();
    }
    if (root->m_right)
    {
        value.push_back(true);
        construct_bit_map(root->m_right, value);
        value.pop_back();
    }
}

std::uint32_t HuffmanTree::get_tree_height(HuffmanTreeNode* root) const
{
    if (root == nullptr)
    {
        return 0;
    }
    std::uint32_t left_height = get_tree_height(root->m_left);
    std::uint32_t right_height = get_tree_height(root->m_right);
    return 1 + std::max(left_height, right_height);
}

HuffmanTree::HuffmanTree()
    : m_root{nullptr}, m_tree_height{0}
{
}

HuffmanTree::HuffmanTree(const FrequencyMapType& frequency_map)
{
    HeapType nodes;
    // Construct individual (leaf) nodes
    for (std::size_t index = 0; index < frequency_map.size(); index++)
    {
        const auto& count = frequency_map[index];
        if (count > 0)
        {
            nodes.emplace(new HuffmanTreeNode(static_cast<unsigned char>(index), count));
        }
    }

    build_tree(nodes);
    assert(nodes.size() == 1);
    m_root = nodes.top();

    m_tree_height = get_tree_height(m_root);

    std::vector<bool> value{};
    value.reserve(m_tree_height);
    construct_bit_map(m_root, value);
}

HuffmanTree::~HuffmanTree()
{
    destroy_tree(m_root);
    m_tree_height = 0;
    m_bit_map.fill(std::nullopt);
}

const HuffmanTree::BitMapType& HuffmanTree::get_bit_map() const
{
    return m_bit_map;
}

std::uint32_t HuffmanTree::get_tree_height() const
{
    return m_tree_height;
}

void HuffmanTree::print_tree()
{
    if (!m_root)
    {
        return;
    }
    std::cout << "Tree height: " << m_tree_height << '\n';
    std::stack<HuffmanTreeNode*> s;
    s.push(m_root);
    while (!s.empty())
    {
        HuffmanTreeNode* element = s.top();
        s.pop();
        std::cout << '(' << element->m_value;
        if (element->is_leaf_node())
        {
            std::cout << ' ' << element->m_char.value();
        }
        std::cout << ')' << ' ';
        if (element->m_right)
        {
            s.push(element->m_right);
        }
        if (element->m_left)
        {
            s.push(element->m_left);
        }
    }
    std::cout << '\n';
}

std::vector<unsigned char> HuffmanTree::serialize(std::uint64_t& total_bits)
{
    // https://stackoverflow.com/a/759766
 
    std::vector<unsigned char> buffer;
    if (!m_root)
    {
        return buffer;
    }

    unsigned char current_byte = 0;
    unsigned char remaining_bits = 8;

    std::stack<HuffmanTreeNode*> s;
    s.push(m_root);
    while (!s.empty())
    {
        auto node = s.top();
        s.pop();

        current_byte <<= 1;
        if (node->is_leaf_node())
        {
            current_byte |= 1;
        }

        total_bits++;
        remaining_bits--;
        if (remaining_bits == 0)
        {
            buffer.push_back(current_byte);
            current_byte = 0;
            remaining_bits = 8;
        }

        if (node->is_leaf_node())
        {
            auto byte = node->m_char.value();
            if (remaining_bits < 8)
            {
                current_byte = (current_byte << remaining_bits) | (byte >> (8 - remaining_bits));
                buffer.push_back(current_byte);
                // Note: remaining bits will remain same
                current_byte = ((byte << remaining_bits) & 0xff) >> remaining_bits;
                total_bits += 8;
            }
            else
            {
                current_byte = byte;
                buffer.push_back(current_byte);
                // Note: remaining bits will remain same
                current_byte = 0;
                total_bits += 8;
            }
        }
        else
        {
            if (node->m_right)
            {
                s.push(node->m_right);
            }
            if (node->m_left)
            {
                s.push(node->m_left);
            }
        }
    }
    if (remaining_bits != 0 && remaining_bits != 8)
    {
        buffer.push_back(current_byte << remaining_bits);
    }
    return buffer;
}

std::vector<unsigned char> HuffmanTree::serialize_payload(const std::vector<unsigned char> &buffer, std::uint64_t& total_bits)
{
    std::vector<unsigned char> output_buffer;
    output_buffer.reserve(buffer.size());

    unsigned char byte = 0;
    unsigned char remaining_bits = 8;
    for (unsigned char c : buffer)
    {
        if (!m_bit_map[c].has_value())
        {
            std::cerr << "Failure during encoding payload - character "
            << "0x" << std::hex << static_cast<int>(c)
            << " not found in bit map\n";
            exit(1);
        }

        auto& code = m_bit_map[c].value();
        for (auto code_bit : code)
        {
            if (!code_bit)
            {
                byte <<= 1;
            }
            else if (code_bit)
            {
                byte = (byte << 1) | 1;
            }
            total_bits++;
            remaining_bits--;
            if (remaining_bits == 0)
            {
                remaining_bits = 8;
                output_buffer.push_back(byte);
                byte = 0;
            }
        }
    }
    if (remaining_bits != 0 && remaining_bits != 8)
    {
        output_buffer.push_back(byte << remaining_bits);
    }
    return output_buffer;
}

HuffmanTreeNode* HuffmanTree::deserialize(const std::vector<unsigned char> &serialized_tree, std::uint64_t serialized_tree_bits,
                            std::size_t& index, std::uint64_t &total_processed_bits, unsigned char &remaining_bits,
                            HuffmanTreeNode* root)
{
    // https://stackoverflow.com/a/759766

    if (index >= serialized_tree.size())
    {
        return nullptr;
    }

    if (total_processed_bits >= serialized_tree_bits)
    {
        return nullptr;
    }

    int bit = (serialized_tree[index] >> (remaining_bits - 1)) & 0x1;

    remaining_bits--;
    total_processed_bits++;
    if (remaining_bits == 0)
    {
        remaining_bits = 8;
        index++;
    }

    if (root == nullptr)
    {
        root = new HuffmanTreeNode();
    }

    if (bit == 0)
    {
        // If 0, create a non-leaf. For left and right, recursively continue for both.
        root->m_left = deserialize(serialized_tree, serialized_tree_bits, index, total_processed_bits, remaining_bits, root->m_left);
        root->m_right = deserialize(serialized_tree, serialized_tree_bits, index, total_processed_bits, remaining_bits, root->m_right);
    }
    else if (bit == 1)
    {
        //  If 1, create a tree leaf. Then read a byte and set the value of leaf with this byte
        unsigned char byte = 0;
        if (remaining_bits < 8)
        {
            if (index + 1 >= serialized_tree.size())
            {
                std::cerr << "Failure during decoding serialized tree\n";
                exit(1);
            }

            unsigned char processed_bits = 8 - remaining_bits;
            byte = ((serialized_tree[index] << processed_bits) & 0xff) | (serialized_tree[index + 1] >> remaining_bits);
        }
        else
        {
            byte = serialized_tree[index];
        }
        index++;
        total_processed_bits += 8;
        // Note that in next byte, the remaining_bits will be the same

        root->m_char = byte;
    }
    return root;
}

void HuffmanTree::deserialize(const std::vector<unsigned char> &serialized_tree, std::uint64_t serialized_tree_bits)
{
    std::uint64_t total_processed_bits = 0;
    unsigned char remaining_bits = 8;
    std::size_t index = 0;

    // Note: below block shouldn't be required in case of a binary since only one operation is done at a time - encoding or decoding
    // In case in future, someone wishes to convert this into a library, this block should be uncommented. It will give weird output
    // if encoding is done followed by decoding
    // if (m_root)
    // {
    //     destroy_tree(m_root);
    // }

    m_root = deserialize(serialized_tree, serialized_tree_bits, index, total_processed_bits, remaining_bits, m_root);

    m_tree_height = get_tree_height(m_root);

    std::vector<bool> value{};
    value.reserve(m_tree_height);
    m_bit_map.fill(std::nullopt);
    construct_bit_map(m_root, value);
}

std::vector<unsigned char> HuffmanTree::deserialize_payload(const std::vector<unsigned char> &serialized_payload, std::uint64_t serialized_payload_bits, std::uint64_t original_file_bytes)
{
    std::vector<unsigned char> output;
    output.reserve(original_file_bytes);

    HuffmanTreeNode* root = m_root;

    unsigned char remaining_bits = 8;
    std::uint64_t total_processed_bits = 0;
    for (std::size_t index = 0; index < serialized_payload.size(); )
    {
        int bit = (serialized_payload[index] >> (remaining_bits - 1)) & 0x1;

        if (bit == 0)
        {
            if (!root->m_left)
            {
                return std::vector<unsigned char>{};
            }
            root = root->m_left;
        }
        else if (bit == 1)
        {
            if (!root->m_right)
            {
                return std::vector<unsigned char>{};
            }
            root = root->m_right;
        }
        if (root->is_leaf_node())
        {
            output.push_back(root->m_char.value());
            root = m_root;
        }

        remaining_bits--;
        total_processed_bits++;
        if (total_processed_bits >= serialized_payload_bits)
        {
            break;
        }
        if (remaining_bits == 0)
        {
            remaining_bits = 8;
            index++;
        }
    }

    return output;
}
} // namespace kcompress