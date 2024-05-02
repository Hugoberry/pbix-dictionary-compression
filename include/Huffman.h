#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace Huffman {
namespace detail {
// Easily serializable
struct Node {
    std::int16_t left = -1;
    std::int16_t right = -1;
    std::uint8_t value = 0;

    [[nodiscard]] constexpr bool isLeaf() const noexcept {
        return left == -1 && right == -1;
    }
};
}  // namespace detail

// Encoded text representation
struct Encoded {
 private:
    std::vector<bool> binary_data_;
    std::vector<detail::Node> nodes_;

    void init_tree(const std::vector<std::uint8_t>& input_data);
    void init_binary_data(const std::vector<std::uint8_t>& input_data);

    [[nodiscard]] const detail::Node& root() const {
        return nodes_.back();
    }

    [[nodiscard]] std::int16_t root_index() const {
        return static_cast<std::int16_t>(nodes_.size() - 1);
    }

    Encoded() = default;

 public:
    static Encoded encode(const std::vector<std::uint8_t>& input_data);
    static Encoded encode(const void* source, std::size_t size);
    static Encoded encode(const std::vector<std::byte>& input_data);
    static Encoded encode(std::string_view input_data);

    [[nodiscard]] std::string decode() const;
};
}  // namespace Huffman

#endif  // HUFFMAN_H
