/**
 * Copyright      2022  Xiaomi Corporation (authors: Fangjun Kuang)
 *
 * See LICENSE for clarification regarding multiple authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef SHERPA_CPP_API_ONLINE_STREAM_H_
#define SHERPA_CPP_API_ONLINE_STREAM_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "kaldifeat/csrc/feature-fbank.h"
#include "torch/script.h"

namespace sherpa {

struct OnlineRecognitionResult {
  /// Recognition results.
  /// For English, it consists of space separated words.
  /// For Chinese, it consists of Chinese words without spaces.
  std::string text;

  /// Decoded results at the token level.
  /// For instance, for BPE-based models it consists of a list of BPE tokens.
  std::vector<int32_t> tokens;

  /// timestamps.size() == tokens.size()
  /// timestamps[i] records the frame number on which tokens[i] is decoded.
  /// Frame numbers are counted after model subsampling.
  std::vector<int32_t> timestamps;  // not implemented at present

  /// ID of this segment
  int32_t segment;

  /// Starting frame of this segment.
  int32_t start_frame;

  /// True if this is the last segment.
  bool is_final;

  /** Return a json string.
   *
   * The returned string contains:
   *   {
   *     "text": "The recognition result",
   *     "tokens": [x, x, x],
   *     "timestamps": [x, x, x],
   *     "segment": x,
   *     "start_frame": x,
   *     "is_final": true|false
   *   }
   */
  std::string AsJsonString() const;
};

class Hypotheses;
struct OnlineTransducerDecoderResult;

class OnlineStream {
 public:
  explicit OnlineStream(const kaldifeat::FbankOptions &opts);
  ~OnlineStream();

  /** This would be called from the application, when you get
   * more wave data.
   *
   * @param sampling_rate Sampling rate of the input waveform. It is provided so
   *                      the code can **assert** that it matches the sampling
   *                      rate expected by the extractor.
   * @param waveform  A 1-D array containing audio samples. For
   *                  models from icefall, the samples should be in the
   *                  range [-1, 1].
   */
  void AcceptWaveform(float sampling_rate, torch::Tensor waveform);

  /** Returns the total number of frames, since the start of the utterance, that
   * are now available.  In an online-decoding context, this will likely
   * increase with time as more data becomes available.
   */
  int32_t NumFramesReady() const;

  /** Returns true if this is the last frame.
   *
   * Frame indices are zero-based, so the first frame is zero.
   */
  bool IsLastFrame(int32_t frame) const;

  /** InputFinished() tells the class you won't be providing any more waveform.
   *
   * It also affects the return value of IsLastFrame().
   */
  void InputFinished();

  /**Get a frame by its index.
   *
   * @param frame  The frame number. It starts from 0.
   *
   * @return Return a 2-D array of shape [1, feature_dim]
   */
  torch::Tensor GetFrame(int32_t frame);

  /**
   * Get the state of the encoder network corresponding to this stream.
   *
   * @return Return the state of the encoder network for this stream.
   */
  torch::IValue GetState() const;

  /**
   * Set the state of the encoder network corresponding to this stream.
   *
   * @param state The state to set.
   */
  void SetState(torch::IValue state);

  // Return a reference to the number of processed frames so far.
  // Initially, it is 0. It is always less than NumFramesReady().
  //
  // The returned reference is valid as long as this object is alive.
  int32_t &GetNumProcessedFrames();

  void SetResult(const OnlineTransducerDecoderResult &r);
  const OnlineTransducerDecoderResult &GetResult() const;

  // TODO(fangjun): Make it return a struct
  //
  // Used for greedy_search.
  //
  // Return a reference to the current recognized tokens.
  // The first context_size tokens are blanks for greedy_search.
  //
  // The returned reference is valid as long as this object is alive.
  std::vector<int32_t> &GetHyps();

  // Used for modified_beam_search.
  //
  // Get the hypotheses we have so far.
  //
  // The returned reference is valid as long as this object is alive.
  Hypotheses &GetHypotheses();

  // Return a reference to the decoder output of the last chunk.
  // Its shape is [1, decoder_dim]
  torch::Tensor &GetDecoderOut();

  // Used only for greedy search
  //
  // Get number of trailing blank frames decoded so far
  //
  // The returned reference is valid as long as this object is alive.
  int32_t &GetNumTrailingBlankFrames();

 private:
  class OnlineStreamImpl;
  std::unique_ptr<OnlineStreamImpl> impl_;
};

}  // namespace sherpa

#endif  // SHERPA_CPP_API_ONLINE_STREAM_H_
