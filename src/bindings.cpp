#include <emscripten/bind.h>

#include "ClpIrV1Decoder.hpp"

EMSCRIPTEN_BINDINGS(DecoderModule) {
    emscripten::class_<ClpIrV1Decoder>("ClpIrV1Decoder")
            .constructor(&ClpIrV1Decoder::create, emscripten::return_value_policy::take_ownership())
            .function("estimatedNumEvents", &ClpIrV1Decoder::get_estimated_num_events)
            .function("buildIdx", &ClpIrV1Decoder::build_idx)
            .function("decode", &ClpIrV1Decoder::decode);
}
