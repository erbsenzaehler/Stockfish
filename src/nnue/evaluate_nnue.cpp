﻿// Code for calculating NNUE evaluation function

#include <fstream>
#include <iostream>
#include <set>

#include "../evaluate.h"
#include "../position.h"
#include "../misc.h"
#include "../uci.h"

#include "evaluate_nnue.h"

namespace std 
{
#include <cstdlib>
}

ExtPieceSquare kpp_board_index[PIECE_NB] = {
 // convention: W - us, B - them
 // viewed from other side, W and B are reversed
    { PS_NONE,     PS_NONE     },
    { PS_W_PAWN,   PS_B_PAWN   },
    { PS_W_KNIGHT, PS_B_KNIGHT },
    { PS_W_BISHOP, PS_B_BISHOP },
    { PS_W_ROOK,   PS_B_ROOK   },
    { PS_W_QUEEN,  PS_B_QUEEN  },
    { PS_W_KING,   PS_B_KING   },
    { PS_NONE,     PS_NONE     },
    { PS_NONE,     PS_NONE     },
    { PS_B_PAWN,   PS_W_PAWN   },
    { PS_B_KNIGHT, PS_W_KNIGHT },
    { PS_B_BISHOP, PS_W_BISHOP },
    { PS_B_ROOK,   PS_W_ROOK   },
    { PS_B_QUEEN,  PS_W_QUEEN  },
    { PS_B_KING,   PS_W_KING   },
    { PS_NONE,     PS_NONE     }
};


namespace Eval::NNUE {

  // Input feature converter
  AlignedPtr<FeatureTransformer> feature_transformer;

  // Evaluation function
  AlignedPtr<Network> network;

  // Evaluation function file name
  std::string fileName = "nn.bin";

  // Get a string that represents the structure of the evaluation function
  std::string GetArchitectureString() {

    return "Features=" + FeatureTransformer::GetStructureString() +
        ",Network=" + Network::GetStructureString();
  }

  namespace Detail {

  // Initialize the evaluation function parameters
  template <typename T>
  void Initialize(AlignedPtr<T>& pointer) {

    pointer.reset(reinterpret_cast<T*>(std::aligned_alloc(alignof(T), sizeof(T))));
    std::memset(pointer.get(), 0, sizeof(T));
  }

  // read evaluation function parameters
  template <typename T>
  bool ReadParameters(std::istream& stream, const AlignedPtr<T>& pointer) {

    std::uint32_t header;
    stream.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!stream || header != T::GetHashValue()) return false;
    return pointer->ReadParameters(stream);
  }

  }  // namespace Detail

  // Initialize the evaluation function parameters
  void Initialize() {

    Detail::Initialize(feature_transformer);
    Detail::Initialize(network);
  }

  // read the header
  bool ReadHeader(std::istream& stream,
    std::uint32_t* hash_value, std::string* architecture) {

    std::uint32_t version, size;
    stream.read(reinterpret_cast<char*>(&version), sizeof(version));
    stream.read(reinterpret_cast<char*>(hash_value), sizeof(*hash_value));
    stream.read(reinterpret_cast<char*>(&size), sizeof(size));
    if (!stream || version != kVersion) return false;
    architecture->resize(size);
    stream.read(&(*architecture)[0], size);
    return !stream.fail();
  }

  // read evaluation function parameters
  bool ReadParameters(std::istream& stream) {

    std::uint32_t hash_value;
    std::string architecture;
    if (!ReadHeader(stream, &hash_value, &architecture)) return false;
    if (hash_value != kHashValue) return false;
    if (!Detail::ReadParameters(stream, feature_transformer)) return false;
    if (!Detail::ReadParameters(stream, network)) return false;
    return stream && stream.peek() == std::ios::traits_type::eof();
  }

  // proceed if you can calculate the difference
  static void UpdateAccumulatorIfPossible(const Position& pos) {

    feature_transformer->UpdateAccumulatorIfPossible(pos);
  }

  // Calculate the evaluation value
  static Value ComputeScore(const Position& pos, bool refresh) {

    auto& accumulator = pos.state()->accumulator;
    if (!refresh && accumulator.computed_score) {
      return accumulator.score;
    }

    alignas(kCacheLineSize) TransformedFeatureType
        transformed_features[FeatureTransformer::kBufferSize];
    feature_transformer->Transform(pos, transformed_features, refresh);
    alignas(kCacheLineSize) char buffer[Network::kBufferSize];
    const auto output = network->Propagate(transformed_features, buffer);

    auto score = static_cast<Value>(output[0] / FV_SCALE);

    accumulator.score = score;
    accumulator.computed_score = true;
    return accumulator.score;
  }

  // read the evaluation function file
  // Save and restore Options with bench command etc., so EvalFile is changed at this time,
  // This function may be called twice to flag that the evaluation function needs to be reloaded.
  void load_eval(const std::string& evalFile) {

    Initialize();
    fileName = evalFile;

    std::ifstream stream(evalFile, std::ios::binary);
    const bool result = ReadParameters(stream);

    if (!result)
        std::cout << "Error! " << fileName << " not found or wrong format" << std::endl;
    else
        std::cout << "info string NNUE " << fileName << " found & loaded" << std::endl;
  }

  // Evaluation function. Perform differential calculation.
  Value evaluate(const Position& pos) {
    return ComputeScore(pos, false);
  }

  // Evaluation function. Perform full calculation.
  Value compute_eval(const Position& pos) {
    return ComputeScore(pos, true);
  }

  // proceed if you can calculate the difference
  void update_eval(const Position& pos) {
    UpdateAccumulatorIfPossible(pos);
  }

} // namespace Eval::NNUE
