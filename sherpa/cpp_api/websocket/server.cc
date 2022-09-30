/**
 * Copyright      2022  (authors: Pingfeng Luo)
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
#include "sherpa/cpp_api/websocket/server.h"

#include "sherpa/csrc/log.h"
#include "sherpa/csrc/online_asr.h"
#include "sherpa/csrc/parse_options.h"

static constexpr const char *kUsageMessage = R"(
Online (streaming) automatic speech recognition RPC server with sherpa.

Usage:
(1) View help information.

  ./bin/websocket-server --help

(2) Run server

  ./bin/websocket-server \
    --nn-model=/path/to/cpu_jit.pt \
    --tokens=/path/to/tokens.txt \
    --use-gpu=false \
    --server-port=6006

See
https://k2-fsa.github.io/sherpa/cpp/websocket/index.html
for more details.
)";

int main(int argc, char *argv[]) {
  // set torch
  // https://pytorch.org/docs/stable/notes/cpu_threading_torchscript_inference.html
  torch::set_num_threads(1);
  torch::set_num_interop_threads(1);
  torch::NoGradGuard no_grad;
  torch::jit::getExecutorMode() = false;
  torch::jit::getProfilingMode() = false;
  torch::jit::setGraphExecutorOptimize(false);

  // set OnlineAsr option
  sherpa::ParseOptions po(kUsageMessage);
  sherpa::OnlineAsrOptions opts;
  opts.Register(&po);
  int32_t port;
  po.Register("server-port", &port, "Server port to listen on");
  po.Read(argc, argv);
  if (argc == 1) {
    po.PrintUsage();
    exit(EXIT_FAILURE);
  }
  SHERPA_LOG(INFO) << "decoding method: " << opts.decoding_method;
  opts.Validate();
  // tips : trailing_silence for EndpointConfig is after sampling
  // rule 1 times out after 0.8 second of silence, even if we decoded nothing.
  opts.endpoint_config.rule1 = sherpa::EndpointRule(false, 0.8, 0.0);

  // rule2 times out after 0.4 second of silence after decoding something,
  opts.endpoint_config.rule2 = sherpa::EndpointRule(true, 0.4, 0.0);

  // rule3 times out after the utterance is 20 seconds long, regardless of
  // anything else.
  opts.endpoint_config.rule3 = sherpa::EndpointRule(false, 0.0, 20);

  SHERPA_LOG(INFO) << "ASR Server is listening at port " << port;
  sherpa::WebSocketServer server(port, opts);
  return 0;
}
