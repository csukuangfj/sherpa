// sherpa/csrc/online-lstm-transducer-model.h
//
// Copyright (c)  2022  Xiaomi Corporation
#ifndef SHERPA_CSRC_ONLINE_LSTM_TRANSDUCER_MODEL_H_
#define SHERPA_CSRC_ONLINE_LSTM_TRANSDUCER_MODEL_H_
#include "sherpa/csrc/online-transducer-model.h"

namespace sherpa {
/** This class implements models from lstm_transducer_stateless{,2,3}
 * from icefall.
 *
 * See
 * https://github.com/k2-fsa/icefall/blob/master/egs/librispeech/ASR/lstm_transducer_stateless/lstm.py
 * for an instance.
 *
 * You can find the interface and implementation details of the
 * encoder, decoder, and joiner network in the above Python code.
 */
class OnlineLstmTransducerModel : public OnlineTransducerModel {
 public:
  /** Constructor.
   *
   * @param filename Path to the torchscript model. See
   *                 https://github.com/k2-fsa/icefall/blob/master/egs/librispeech/ASR/lstm_transducer_stateless/export.py
   *                 for how to export a model.
   * @param device  Move the model to this device on loading.
   */
  explicit OnlineLstmTransducerModel(const std::string &encoder_filename,
                                     const std::string &decoder_filename,
                                     const std::string &joiner_filename,
                                     torch::Device device = torch::kCPU);

  torch::IValue StackStates(
      const std::vector<torch::IValue> &states) const override;

  std::vector<torch::IValue> UnStackStates(torch::IValue states) const override;

  torch::IValue GetEncoderInitStates(int32_t batch_size = 1) override;

  std::tuple<torch::Tensor, torch::Tensor, torch::IValue> RunEncoder(
      const torch::Tensor &features, const torch::Tensor &features_length,
      const torch::Tensor &num_processed_frames, torch::IValue states) override;

  torch::Tensor RunDecoder(const torch::Tensor &decoder_input) override;

  torch::Tensor RunJoiner(const torch::Tensor &encoder_out,
                          const torch::Tensor &decoder_out) override;

  torch::Device Device() const override { return device_; }

  int32_t ContextSize() const override { return context_size_; }

  int32_t ChunkSize() const override { return chunk_size_; }

  int32_t ChunkShift() const override { return chunk_shift_; }

  // Non virtual methods that used by Python bindings.

  // See
  // https://github.com/k2-fsa/icefall/blob/master/egs/librispeech/ASR/lstm_transducer_stateless/lstm.py#L257
  // for what state contains for details.
  //
  // State is a tuple containing:
  //  - hx: (num_layers, batch_size, proj_size)
  //  - cx: (num_layers, batch_size, hidden_size)
  //  See icefall/egs/librispeech/ASR/lstm_transducer_stateless/lstm.py
  //  for details
  using State = std::pair<torch::Tensor, torch::Tensor>;

  torch::IValue StateToIValue(const State &s) const;
  State StateFromIValue(torch::IValue ivalue) const;

 private:
  torch::jit::Module model_;

  // The following modules are just aliases to modules in model_
  torch::jit::Module encoder_;
  torch::jit::Module decoder_;
  torch::jit::Module joiner_;

  torch::Device device_{"cpu"};

  int32_t context_size_;
  int32_t chunk_size_;
  int32_t chunk_shift_;
};

}  // namespace sherpa

#endif  // SHERPA_CSRC_ONLINE_LSTM_TRANSDUCER_MODEL_H_
