// sherpa/csrc/offline-ctc-one-best-decoder.cc
//
// Copyright (c)  2022  Xiaomi Corporation

#include "sherpa/csrc/offline-ctc-one-best-decoder.h"

#include <utility>

#include "sherpa/csrc/log.h"

namespace sherpa {

OfflineCtcOneBestDecoder::OfflineCtcOneBestDecoder(
    const OfflineCtcDecoderConfig config, torch::Device device,
    int32_t vocab_size)
    : config_(config), vocab_size_(vocab_size) {
  if (config.hlg.empty()) {
    // Use CTC topo since no HLG is provided
    SHERPA_CHECK_GT(vocab_size, 1);

    decoding_graph_ = k2::GetCtcTopo(vocab_size - 1, config.modified, device);
  } else {
    decoding_graph_ = k2::LoadFsaClass(config.hlg, device);
  }
}

std::vector<OfflineCtcDecoderResult> OfflineCtcOneBestDecoder::Decode(
    torch::Tensor log_prob, torch::Tensor log_prob_len,
    int32_t subsampling_factor /*= 1*/) {
  if (vocab_size_ > 0) {
    SHERPA_CHECK_EQ(log_prob.size(2), vocab_size_);
  }

  torch::NoGradGuard no_grad;

  auto lattice = k2::GetLattice(log_prob, log_prob_len.cpu(), decoding_graph_,
                                config_.search_beam, config_.output_beam,
                                config_.min_active_states,
                                config_.max_active_states, subsampling_factor);

  std::vector<std::vector<int32_t>> tokens = k2::BestPath(lattice);
  std::vector<OfflineCtcDecoderResult> ans(tokens.size());
  for (int32_t i = 0; i != static_cast<int32_t>(tokens.size()); ++i) {
    ans[i].tokens = std::move(tokens[i]);
    // TODO(fangjun): Set timestamps
  }

  return ans;
}

}  // namespace sherpa
