// Copyright 2010-2014 Google
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once


#include <cstdint>
#include <memory>
#include <fstream>
#include <string>

// This file defines some IO interfaces to compatible with Google
// IO specifications.
namespace recordio {
// This class appends a protocol buffer to a file in a binary format.
// The data written in the file follows the following format (sequentially):
// - MagicNumber (32 bits) to recognize this format.
// - Uncompressed data payload size (64 bits)
// - Compressed data payload size (64 bits), or 0 if the
//   data is not compressed.
// - Payload, possibly compressed. See RecordWriter::Compress()
//   and RecordReader::Uncompress
class RecordWriter {
 public:
  // Magic number when writing and reading protocol buffers.
  static const int kMagicNumber;

  explicit RecordWriter(std::ofstream* const file);

  template <class P>
  bool WriteProtocolMessage(const P& proto) {
    std::string uncompressed_buffer;
    proto.SerializeToString(&uncompressed_buffer);
    const uint64_t uncompressed_size = uncompressed_buffer.size();
    const std::string compressed_buffer =
        use_compression_ ? Compress(uncompressed_buffer) : "";
    const uint64_t compressed_size = compressed_buffer.size();
    file_->write(reinterpret_cast<const char*>(&kMagicNumber), sizeof(kMagicNumber));
    file_->write(reinterpret_cast<const char*>(&uncompressed_size),
                 sizeof(uncompressed_size));
    file_->write(reinterpret_cast<const char*>(&compressed_size),
                 sizeof(compressed_size));
    if (use_compression_) {
      file_->write(compressed_buffer.c_str(), compressed_size);
    } else {
      file_->write(uncompressed_buffer.c_str(), uncompressed_size);
    }
    return !file_->fail();
  }
  // Closes the underlying file.
  bool Close();

  void set_use_compression(bool use_compression);

 private:
  std::string Compress(const std::string& input) const;
  std::ofstream* const file_;
  bool use_compression_;  // default: false
};

// This class reads a protocol buffer from a file.
// The format must be the one described in RecordWriter, above.
class RecordReader {
 public:
  explicit RecordReader(std::ifstream* const file);

  template <class P>
  bool ReadProtocolMessage(P* const proto) {
    uint64_t usize = 0;
    uint64_t csize = 0;
    int magic_number = 0;
    file_->read(reinterpret_cast<char*>(&magic_number), sizeof(magic_number));
    if (magic_number != RecordWriter::kMagicNumber) {
      return false;
    }
    file_->read(reinterpret_cast<char*>(&usize), sizeof(usize));
    file_->read(reinterpret_cast<char*>(&csize), sizeof(csize));
    std::unique_ptr<char[]> buffer(new char[usize + 1]);
    if (csize != 0) {  // The data is compressed.
      std::unique_ptr<char[]> compressed_buffer(new char[csize + 1]);
      file_->read(reinterpret_cast<char*>(compressed_buffer.get()), csize);
      compressed_buffer[csize] = '\0';
      Uncompress(compressed_buffer.get(), usize, buffer.get(), usize);
    } else {
      file_->read(reinterpret_cast<char*>(buffer.get()), usize);
    }
    proto->ParseFromArray(buffer.get(), usize);
    return !file_->fail();
  }

  // Closes the underlying file.
  bool Close();

 private:
  void Uncompress(const char* const source, uint64_t source_size,
                  char* const output_buffer, uint64_t output_size) const;

  std::ifstream* const file_;
};
}  // namespace recordio
