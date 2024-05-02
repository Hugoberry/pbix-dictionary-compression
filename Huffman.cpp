#include "Huffman.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <queue>
#include <vector>
#include <numeric>
#include <climits>
#include <string>

using Huffman::detail::Node;
using Huffman::Encoded;

// Helper struct to keep a Node and its count together
struct NodeWithCount {
    Node node;
    std::size_t count = 0;
};

// Count the occurrences of each byte in the input data
static std::array<std::size_t, UCHAR_MAX + 1> count_bytes(const std::vector<std::uint8_t>& bytes) {
    std::array<std::size_t, UCHAR_MAX + 1> counts = {};
    for (auto ch : bytes) {
        ++counts[ch];
    }
    return counts;
}

// Create the initial leaf nodes from byte counts
static std::vector<NodeWithCount> make_leaf_nodes(const std::array<std::size_t, UCHAR_MAX + 1>& counts) {
    std::vector<NodeWithCount> nodes;
    for (std::size_t i = 0; i < counts.size(); ++i) {
        if (counts[i] > 0) {
            nodes.push_back(NodeWithCount{ Node{ -1, -1, static_cast<std::uint8_t>(i) }, counts[i] });
        }
    }
    return nodes;
}

// Create a dictionary mapping bytes to their encoded paths
using CharDictT = std::array<std::string, UCHAR_MAX + 1>;

static void init_dict(const std::vector<Node>& nodes, std::int16_t root, CharDictT& dict, std::string& path) {
    if (root < 0) return;
    assert(root < static_cast<std::int16_t>(nodes.size()));

    const auto& node = nodes[root];
    if (node.isLeaf()) {
        dict[node.value] = path;
        return;
    }

    path.push_back('0');
    init_dict(nodes, node.left, dict, path);
    path.back() = '1';
    init_dict(nodes, node.right, dict, path);
    path.pop_back();
}

static void init_dict(const std::vector<Node>& nodes, std::int16_t root, CharDictT& dict) {
    std::string path;
    init_dict(nodes, root, dict, path);
}

// Initialize the Huffman tree based on input data
void Encoded::init_tree(const std::vector<std::uint8_t>& input_data) {
    auto nodes_with_size = make_leaf_nodes(count_bytes(input_data));

    auto cmp = [&nodes_with_size](std::int16_t lhs, std::int16_t rhs) {
        return nodes_with_size[lhs].count > nodes_with_size[rhs].count;
    };

    std::priority_queue<std::int16_t, std::vector<std::int16_t>, decltype(cmp)> queue(cmp);

    for (std::int16_t i = 0; i < static_cast<std::int16_t>(nodes_with_size.size()); ++i) {
        if (nodes_with_size[i].count > 0) {
            queue.push(i);
        }
    }

    if (queue.size() == 0) {
        nodes_ = { Node{ -1, -1, 0 } };
        return;
    }

    if (queue.size() == 1) {
        nodes_ = { Node{ -1, -1, input_data.front() }, Node{ -1, 0, 0 } };
        return;
    }

    while (queue.size() > 1) {
        auto left = queue.top();
        queue.pop();
        auto right = queue.top();
        queue.pop();
        nodes_with_size.push_back(NodeWithCount{ Node{ left, right, 0 }, nodes_with_size[left].count + nodes_with_size[right].count });
        queue.push(static_cast<std::int16_t>(nodes_with_size.size() - 1));
    }

    nodes_.resize(nodes_with_size.size());
    std::transform(nodes_with_size.begin(), nodes_with_size.end(), nodes_.begin(),
        [](const NodeWithCount& nc) { return nc.node; });
}

// Initialize the binary data using the Huffman tree
void Encoded::init_binary_data(const std::vector<std::uint8_t>& input_data) {
    CharDictT dict = {};
    init_dict(nodes_, root_index(), dict);

    auto compute_size = [&dict](std::size_t sum, std::uint8_t ch) {
        return sum + dict[ch].size();
    };

    auto total_bits = std::accumulate(input_data.begin(), input_data.end(), std::size_t{0}, compute_size);
    binary_data_.resize(total_bits);

    std::size_t bit_index = 0;
    for (auto ch : input_data) {
        for (auto bit : dict[ch]) {
            binary_data_[bit_index++] = (bit == '1');
        }
    }
}

Encoded Encoded::encode(const std::vector<std::uint8_t>& input_data) {
    Encoded encoded;
    encoded.init_tree(input_data);
    encoded.init_binary_data(input_data);
    return encoded;
}

Encoded Encoded::encode(const void* source, std::size_t size) {
    auto data = static_cast<const std::uint8_t*>(source);
    return encode(std::vector<std::uint8_t>(data, data + size));
}

Encoded Encoded::encode(const std::vector<std::byte>& input_data) {
    return encode(reinterpret_cast<const std::uint8_t*>(input_data.data()), input_data.size());
}

Encoded Encoded::encode(std::string_view input_data) {
    return encode(reinterpret_cast<const std::uint8_t*>(input_data.data()), input_data.size());
}

std::string Encoded::decode() const {
    std::string decoded;
    const Node* node = &root();

    for (bool bit : binary_data_) {
        node = &nodes_[bit ? node->right : node->left];
        if (node->isLeaf()) {
            decoded += static_cast<char>(node->value);
            node = &root();
        }
    }

    return decoded;
}
