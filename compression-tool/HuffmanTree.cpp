#include "HuffmanTree.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <stack>
#include <bitset>
#include "utils.hpp"

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

void HuffmanTree::construct_bit_map(HuffmanTreeNode *root, std::string& value)
{
    if (root == nullptr)
    {
        return;
    }
    if (root->is_leaf_node())
    {
        // std::cout << root->m_char.value() << ' ' << value << '\n';
        m_bit_map.emplace(root->m_char.value(), value);
        return;
    }
    if (root->m_left)
    {
        value.push_back('0');
        construct_bit_map(root->m_left, value);
        value.pop_back();
    }
    if (root->m_right)
    {
        value.push_back('1');
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
    for (const auto& [char_value, count] : frequency_map)
    {
        nodes.emplace(new HuffmanTreeNode(char_value, count));
    }
    build_tree(nodes);
    assert(nodes.size() == 1);
    m_root = nodes.top();

    m_tree_height = get_tree_height(m_root);

    std::string value{};
    value.reserve(m_tree_height);
    construct_bit_map(m_root, value);
}

HuffmanTree::~HuffmanTree()
{
    destroy_tree(m_root);
    m_tree_height = 0;
    m_bit_map.clear();
}

HuffmanTree::BitMapType HuffmanTree::get_bit_map() const
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

            // std::array<unsigned char, 4> bytes{};
            // auto no_of_bytes_for_char = convert_utf32_to_utf8_char(element->m_utf32_codepoint.value(), bytes);
            // for (unsigned int index = 0; index < no_of_bytes_for_char; index++)
            // {
            //     std::cout << bytes[index];
            // }
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
                // std::cout << std::bitset<8>(current_byte) << " | " << std::bitset<8>(byte) << " -> ";
                current_byte = (current_byte << remaining_bits) | (byte >> (8 - remaining_bits));
                buffer.push_back(current_byte);
                // Note: remaining bits will remain same
                // std::cout << std::bitset<8>(current_byte) << " | " << std::bitset<8>(((byte << remaining_bits) & 0xff) >> remaining_bits) << ' ' << static_cast<int>(remaining_bits) << '\n';
                current_byte = ((byte << remaining_bits) & 0xff) >> remaining_bits;
                total_bits += 8;
            }
            else
            {
                // std::cout << std::bitset<8>(current_byte) << " | " << std::bitset<8>(byte) << " -> ";
                current_byte = byte;
                buffer.push_back(current_byte);
                // Note: remaining bits will remain same
                // std::cout << std::bitset<8>(current_byte) << " | " << std::bitset<8>(0) << ' ' << static_cast<int>(remaining_bits) << '\n';
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
    unsigned char byte = 0;
    unsigned char remaining_bits = 8;
    for (auto c : buffer)
    {
        auto it = m_bit_map.find(c);
        if (it == m_bit_map.end())
        {
            std::cerr << "Failure during encoding payload: character not found in bit map\n";
            exit(1);
        }
        auto& code = it->second;

        for (unsigned char code_bit : code)
        {
            if (code_bit == '0')
            {
                byte <<= 1;
            }
            else if (code_bit == '1')
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

        // std::cout << "LIndex " << index << '\n';
        root->m_left = deserialize(serialized_tree, serialized_tree_bits, index, total_processed_bits, remaining_bits, root->m_left);
        // std::cout << "RIndex " << index << '\n';
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
                // state = 8;
                std::cerr << "Failure during decoding serialized tree\n";
                exit(1);
                // break;
            }

            unsigned char processed_bits = 8 - remaining_bits;
            // std::bitset<8> b1(serialized_tree[index]);
            // std::bitset<8> b((serialized_tree[index] << processed_bits) & 0xff);
            // std::bitset<8> b2(serialized_tree[index + 1] >> remaining_bits);
            // std::bitset<8> b3(serialized_tree[index + 1]);
            // std::cout << index << ' ' << static_cast<int>(processed_bits) << ' ' << static_cast<int>(remaining_bits) << '\n';
            // std::cout << b1 << " | " << b << ' ' << b2 << " | " << b3 << '\n';
            byte = ((serialized_tree[index] << processed_bits) & 0xff) | (serialized_tree[index + 1] >> remaining_bits);
        }
        else
        {
            // std::bitset<8> b(serialized_tree[index]);
            // std::cout << index << ':' << b << '\n';
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

    std::string value{};
    value.reserve(m_tree_height);
    m_bit_map.clear();
    construct_bit_map(m_root, value);
}

std::vector<unsigned char> HuffmanTree::deserialize_payload(const std::vector<unsigned char> &serialized_payload, std::uint64_t serialized_payload_bits)
{
    std::vector<unsigned char> output;

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