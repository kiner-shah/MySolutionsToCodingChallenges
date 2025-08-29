#include "HuffmanTree.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <stack>
#include <bitset>
#include "utils.hpp"

namespace kcompress
{
HuffmanTreeNode::HuffmanTreeNode(std::optional<char32_t> wc, std::uint64_t value, HuffmanTreeNode *left, HuffmanTreeNode *right)
    : m_utf32_codepoint{wc}, m_value{value}, m_left{left}, m_right{right}
{
}

HuffmanTreeNode::~HuffmanTreeNode()
{
    m_utf32_codepoint = std::nullopt;
    m_value = 0;
    m_left = nullptr;
    m_right = nullptr;
}

bool HuffmanTreeNode::is_leaf_node() const
{
    return m_utf32_codepoint.has_value() && m_left == nullptr && m_right == nullptr;
}

bool HuffmanTreeNode::operator>(const HuffmanTreeNode &other) const
{
    return this->m_value > other.m_value;
}

void HuffmanTree::build_tree(std::deque<HuffmanTreeNode*>& nodes)
{
    while (nodes.size() >= 2)
    {
        std::make_heap(nodes.begin(), nodes.end(), std::greater<>{});
        std::uint64_t new_value = nodes[0]->m_value + nodes[1]->m_value;
        HuffmanTreeNode* node = new HuffmanTreeNode(std::nullopt, new_value, nodes[0], nodes[1]);
        nodes.pop_front();
        nodes.pop_front();
        nodes.push_back(node);
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

void HuffmanTree::construct_bit_map(HuffmanTreeNode *root, std::string value)
{
    if (root == nullptr)
    {
        return;
    }
    if (root->is_leaf_node())
    {
        // std::cout << root->m_utf32_codepoint.value() << ' ' << value << '\n';
        m_bit_map.emplace(root->m_utf32_codepoint.value(), value);
        return;
    }
    if (root->m_left)
    {
        construct_bit_map(root->m_left, value + '0');
    }
    if (root->m_right)
    {
        construct_bit_map(root->m_right, value + '1');
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

HuffmanTree::HuffmanTree(const std::unordered_map<char32_t, std::uint64_t> &char32_frequency_map)
{
    std::deque<HuffmanTreeNode*> nodes;
    // Construct individual (leaf) nodes
    for (const auto& [utf32_codepoint, count] : char32_frequency_map)
    {
        nodes.emplace_back(new HuffmanTreeNode(utf32_codepoint, count));
    }
    build_tree(nodes);
    assert(nodes.size() == 1);
    m_root = nodes[0];

    m_tree_height = get_tree_height(m_root);

    std::string value{};
    value.reserve(m_tree_height);
    construct_bit_map(m_root, std::move(value));
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
            std::cout << ' ';
            std::array<unsigned char, 4> bytes{};
            auto no_of_bytes_for_char = convert_utf32_to_utf8_char(element->m_utf32_codepoint.value(), bytes);
            for (unsigned int index = 0; index < no_of_bytes_for_char; index++)
            {
                std::cout << bytes[index];
            }
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
    std::vector<unsigned char> buffer;
    if (!m_root)
    {
        return buffer;
    }

    unsigned char current_byte = 0;
    unsigned char remaining_bits = 8;
    unsigned char current_byte_processed_bits = 0;

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
            std::array<unsigned char, 4> bytes;
            auto no_of_bytes = kcompress::convert_utf32_to_utf8_char(node->m_utf32_codepoint.value(), bytes);

            for (unsigned int index = 0; index < no_of_bytes; )
            {
                if (remaining_bits < 8)
                {
                    current_byte = (current_byte << remaining_bits) | (bytes[index] >> (8 - remaining_bits));
                    current_byte_processed_bits = remaining_bits;
                    buffer.push_back(current_byte);

                    total_bits += remaining_bits;
                    remaining_bits = 8;
                    current_byte = 0;
                }
                else
                {
                    current_byte = ((bytes[index] << current_byte_processed_bits) & 0xff) >> current_byte_processed_bits;
                    remaining_bits = current_byte_processed_bits;
                    total_bits += (8 - current_byte_processed_bits);
                    if (remaining_bits == 0)
                    {
                        buffer.push_back(current_byte);
                        current_byte = 0;
                        remaining_bits = 8;
                    }
                    index++;
                    current_byte_processed_bits = 0;
                }
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
    if (current_byte != 0)
    {
        current_byte <<= remaining_bits;
        buffer.push_back(current_byte);
    }
    return buffer;
}

std::vector<unsigned char> HuffmanTree::serialize_payload(const std::vector<char32_t> &codepoint_sequence, std::uint64_t& total_bits)
{
    std::vector<unsigned char> output_buffer;
    unsigned char byte = 0;
    unsigned char remaining_bits = 8;
    for (auto codepoint : codepoint_sequence)
    {
        std::string code = m_bit_map[codepoint];
        for (unsigned char c : code)
        {
            if (c == '0')
            {
                byte <<= 1;
            }
            else if (c == '1')
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
        //  If 1, create a tree leaf. Then read a byte, check if state is 0
        //    If state is 0, then set converted codepoint to tree leaf
        //    Else continue reading bytes

        unsigned int state = 0;
        char32_t codepoint;
        do
        {
            // Get 8-bits a byte of multi-byte character
            unsigned char byte = 0;
            if (remaining_bits < 8)
            {
                if (index + 1 >= serialized_tree.size())
                {
                    state = 8;
                    break;
                }

                unsigned char processed_bits = 8 - remaining_bits;
                // std::bitset<8> b1(serialized_tree[index]);
                // std::bitset<8> b((serialized_tree[index] << processed_bits) & 0xff);
                // std::bitset<8> b2(serialized_tree[index + 1] >> remaining_bits);
                // std::bitset<8> b3(serialized_tree[index + 1]);
                // std::cout << static_cast<int>(processed_bits) << ' ' << static_cast<int>(remaining_bits) << '\n';
                // std::cout << b1 << " | " << b << ' ' << b2 << " | " << b3 << '\n';
                byte = ((serialized_tree[index] << processed_bits) & 0xff) | (serialized_tree[index + 1] >> remaining_bits);
            }
            else
            {
                std::bitset<8> b(serialized_tree[index]);
                // std::cout << b << '\n';
                byte = serialized_tree[index];
            }
            index++;
            total_processed_bits += 8;
            // Note that in next byte, the remaining_bits will be the same

            state = kcompress::decode_utf8_char_to_utf32(byte, state, codepoint);
            // std::cout << state << '\n';
        } while (state != 0 && state != 8 && index < serialized_tree.size());

        if (state == 8)
        {
            std::cerr << "Failure during decoding serialized tree\n";
            exit(1);
        }

        root->m_utf32_codepoint = codepoint;
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
    construct_bit_map(m_root, std::move(value));
}

std::vector<char32_t> HuffmanTree::deserialize_payload(const std::vector<unsigned char> &serialized_payload, std::uint64_t serialized_payload_bits)
{
    std::vector<char32_t> output_codepoints;

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
                return std::vector<char32_t>{};
            }
            root = root->m_left;
        }
        else if (bit == 1)
        {
            if (!root->m_right)
            {
                return std::vector<char32_t>{};
            }
            root = root->m_right;
        }
        if (root->is_leaf_node())
        {
            output_codepoints.push_back(root->m_utf32_codepoint.value());
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

    return output_codepoints;
}
} // namespace kcompress