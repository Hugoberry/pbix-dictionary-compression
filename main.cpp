#include <iostream>
#include <vector>
#include <unordered_map>
#include <bitset>
#include <algorithm>
#include <fstream>
#include <string>
#include <queue>
#include <iomanip>
#include "kaitai/kaitaistream.h"
#include "column_data_dictionary.h"

// Huffman Tree Node definition
struct HuffmanTree {
    char c;
    HuffmanTree* left;
    HuffmanTree* right;

    HuffmanTree(uint8_t c = 0) : c(c), left(nullptr), right(nullptr) {}
    ~HuffmanTree() {
        delete left;
        delete right;
    }
};

struct HuffmanTableEntry {
    uint8_t character;
    uint8_t length;  // 0 means invalid entry
};


// Function to generate the full 256-byte Huffman array from the compact 128-byte encode_array
std::vector<uint8_t> decompress_encode_array(const std::vector<uint8_t>& compressed) {
    std::vector<uint8_t> full_array(256, 0);

    for (size_t i = 0; i < compressed.size(); i++) {
        uint8_t byte = compressed[i];
        full_array[2 * i] = byte & 0x0F;         // Lower nibble
        full_array[2 * i + 1] = (byte >> 4) & 0x0F; // Upper nibble
        // std::cout << 2*i << " Byte: " << std::bitset<8>(byte) << " Lower: " << std::bitset<4>(full_array[2 * i])<< ":"<< (int)full_array[2 * i] << " Upper: " << std::bitset<4>(full_array[2 * i + 1])<<":"<< (int)full_array[2 * i + 1] << std::endl;
    }

    return full_array;
}

// Function to generate Huffman codes based on codeword lengths
std::unordered_map<uint8_t, std::string> generate_codes(const std::vector<uint8_t>& lengths) {
    std::unordered_map<uint8_t, std::string> codes;
    std::vector<std::pair<int, uint8_t>> sorted_lengths;

    // Collect only the non-zero lengths and their associated symbols
    for (auto i = 0; i < 256; i++) {
        if (lengths[i] != 0){
// std::cout << "Processing index " << (int)i << " with length " << (int)lengths[i] << std::endl;
            sorted_lengths.emplace_back(lengths[i], i);
        }
    }

    // Sort by length first, then by character
    std::sort(sorted_lengths.begin(), sorted_lengths.end(), [](const auto& a, const auto& b) {
        return a.first != b.first ? a.first < b.first : a.second < b.second;
    });

    int code = 0;
    int last_length = 0;

    for (const auto& [length, character] : sorted_lengths) {
        if (last_length != length) {
            code <<= (length - last_length); // Shift code by difference in lengths
            last_length = length;
        }

        // Generate the code string representation
        codes[character] = std::bitset<32>(code).to_string().substr(32 - length);
        code++;
    }

    return codes;
}

// Print Huffman codes
void print_huffman_codes(const std::unordered_map<uint8_t, std::string>& codes) {
    std::cout << "Huffman Codes:\n";
    for (const auto& [character, code] : codes) {
        std::cout << (char)character << ": " << code << '\n';
    }
}

// Build Huffman tree based on generated codes
HuffmanTree* build_huffman_tree(const std::vector<uint8_t>& encode_array) {
    auto codes = generate_codes(encode_array);
// print_huffman_codes(codes);
    HuffmanTree* root = new HuffmanTree;

    for (const auto& [character, code] : codes) {
        HuffmanTree* node = root;
        for (char bit : code) {
            if (bit == '0') {
                if (!node->left) node->left = new HuffmanTree;
                node = node->left;
            } else {
                if (!node->right) node->right = new HuffmanTree;
                node = node->right;
            }
        }
        node->c = character;
    }

    return root;
}
// Decode a bitstream from start to end bit positions using the Huffman tree
std::string decode_substring(const std::string& bitstream, HuffmanTree* tree, uint32_t start_bit, uint32_t end_bit) {
    std::string result;
    const HuffmanTree* node = tree;
    uint32_t total_bits = end_bit - start_bit;

    // Adjust bit position calculation for little endian byte order
    for (uint32_t i = 0; i < total_bits; ++i) {
        uint32_t bit_pos = start_bit + i;
        uint32_t byte_pos = bit_pos / 8;
        uint32_t bit_offset = bit_pos % 8;

        // Convert byte index for little endian (pair-wise)
        byte_pos = (byte_pos & ~0x01) + (1 - (byte_pos & 0x01));

        if (!node->left && !node->right) {
            result += (char)(node->c);
            node = tree; // Reset to the root node
        }

        // Traverse the Huffman tree based on the current bit
        if (bitstream[byte_pos] & (1 << (7 - bit_offset))) {  // Adjusting bit offset to read from MSB to LSB
            node = node->right;
        } else {
            node = node->left;
        }
    }

    // Append the last character if the final node is a leaf
    if (!node->left && !node->right) {
        result += (char)(node->c);
    }

    return result;
}


// Decode a bitstream from start to end bit positions using the Huffman tree
std::string decode_substring_with_map(const std::string& bitstream, const std::unordered_map<std::string, char>& huffmanMap, uint32_t start_bit, uint32_t end_bit) {
    std::string result;
    uint32_t total_bits = end_bit - start_bit;

    int current_code = 0;
    int code_length = 0;

    // Adjust bit position calculation for little endian byte order
    for (uint32_t i = 0; i < total_bits; ++i) {
        uint32_t bit_pos = start_bit + i;
        uint32_t byte_pos = bit_pos / 8;
        uint32_t bit_offset = bit_pos % 8;
        byte_pos = (byte_pos & ~0x01) + (1 - (byte_pos & 0x01));

        current_code = (current_code << 1) | ((bitstream[byte_pos] & (1 << (7 - bit_offset))) != 0);
        code_length++;

        // Check lookup table if code length is within limit
        if (code_length <= 15 && huffmanMap.count(std::bitset<15>(current_code).to_string().substr(15 - code_length)) > 0){
            result += huffmanMap.at(std::bitset<15>(current_code).to_string().substr(15 - code_length));
            current_code = 0;
            code_length = 0;
        }
    }

    return result;
}

// Print Huffman tree in a readable format
void print_huffman_tree(HuffmanTree* node, int indent = 0) {
    if (node == nullptr) return;

    if (node->right) print_huffman_tree(node->right, indent + 4);

    if (indent) std::cout << std::setw(indent) << ' ';
    if (!node->left && !node->right) std::cout << node->c << '\n';
    else std::cout << "⟨\n";

    if (node->left) print_huffman_tree(node->left, indent + 4);
}


int main(int argc, char* argv[]) {
    // Check for the correct number of arguments
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <dictionary_file_path>" << std::endl;
        return 1;
    }

    // Read the file name from arguments
    const char* filename = argv[1];

    // Open the file and check if it opened successfully
    std::ifstream is(filename, std::ifstream::binary);
    if (!is) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return 1;
    }
    kaitai::kstream ks(&is);

    column_data_dictionary_t dictionary(&ks);

    // Checking dictionary type and processing accordingly
    if (dictionary.dictionary_type() == column_data_dictionary_t::DICTIONARY_TYPES_XM_TYPE_STRING) {
        auto stringData = static_cast<column_data_dictionary_t::string_data_t*>(dictionary.data());
        auto pages = stringData->dictionary_pages();
        auto record_handles = stringData->dictionary_record_handles_vector_info()->vector_of_record_handle_structures();
        std::unordered_map<int, std::vector<int>> record_handles_map;
        // make record_handle a map of page_id and bit_or_byte_offset
        for (const auto& handle : *record_handles) {
            record_handles_map[handle->page_id()].push_back(handle->bit_or_byte_offset());
        }

        for(int page_id = 0; page_id < pages->size(); page_id++){
            const auto& page = pages->at(page_id);
            if(page->page_compressed()){
                auto compressed_store = static_cast<column_data_dictionary_t::compressed_strings_t*>(page->string_store());
                auto encode_array = compressed_store->encode_array();
                auto store_total_bits = compressed_store->store_total_bits();
                auto compressed_string_buffer = compressed_store->compressed_string_buffer();
                auto ui_decode_bits = compressed_store->ui_decode_bits();

                auto full_encode_array = decompress_encode_array(*encode_array);

                HuffmanTree* huffman_tree = build_huffman_tree(full_encode_array);

                auto it = record_handles_map.find(page_id);
                if (it != record_handles_map.end()) {
                    for (size_t i = 0; i < it->second.size(); i++) {
                        uint32_t start_bit = it->second[i];
                        uint32_t end_bit = (i + 1 < it->second.size()) ? it->second[i + 1] : store_total_bits; // end of the compressed buffer
                        std::string decompressed = decode_substring(compressed_string_buffer, huffman_tree, start_bit, end_bit);
// std::cout << "Decompressed string " << start_bit << "/" << end_bit << " - " << page_id << ": " << decompressed << std::endl;
                        std::cout  << decompressed << std::endl;
                    }
                }
                delete huffman_tree;
            } else {
                auto uncompressed_store = static_cast<column_data_dictionary_t::uncompressed_strings_t *>(page->string_store());
                auto uncompressed = uncompressed_store->uncompressed_character_buffer();
                // Extracting strings and filling the hashtable
                std::istringstream ss(uncompressed);
                std::string token;
                while (std::getline(ss, token, '\0'))
                { // assuming null-terminated strings in buffer
// std::cout << "Uncompressed string - " << page_id << ": " << token << std::endl;
                    std::cout << token << std::endl;
                }
            }

        }

    } else if (dictionary.dictionary_type() == column_data_dictionary_t::DICTIONARY_TYPES_XM_TYPE_LONG ||
                 dictionary.dictionary_type() == column_data_dictionary_t::DICTIONARY_TYPES_XM_TYPE_REAL)
        {
            // Handling numeric data
            auto numberData = static_cast<column_data_dictionary_t::number_data_t *>(dictionary.data());
            auto vector_info = numberData->vector_of_vectors_info();
            const auto &vals = vector_info->values();

            for (double val : *vals)
            {
                std::cout<< val << std::endl;
            }

        }

    return 0;
}
