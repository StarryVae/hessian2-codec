#include "hessian2/basic_codec/ref_object_codec.hpp"

namespace Hessian2 {

// ref ::= x51 int
template <>
std::unique_ptr<RefObject> Decoder::decode() {
  auto ret = reader_->read<uint8_t>();
  ABSL_ASSERT(ret.first);

  uint32_t ref;
  switch (ret.second) {
    case 0x4a: {
      auto ret = reader_->read<uint8_t>();
      if (!ret.first) {
        return nullptr;
      }
      ref = ret.second;
      break;
    }
    case 0x52: {
      auto ret = reader_->readBE<uint32_t>();
      if (!ret.first) {
        return nullptr;
      }
      ref = ret.second;
      break;
    }
    default:
      return nullptr;
  }

  if (ref >= values_ref_.size()) {
    return nullptr;
  }
  return std::make_unique<RefObject>(values_ref_[ref]);
}

template <>
bool Encoder::encode(const RefObject& value) {
  auto d = value.toRefDest();
  ABSL_ASSERT(d.has_value());
  auto ref = getValueRef(d.value());
  ABSL_ASSERT(ref != -1);

  if (ref < 0x100) {
    writer_->writeByte<uint8_t>(0x4a);
    writer_->writeByte<uint8_t>(ref);
    return true;
  }

  writer_->writeByte<uint8_t>(0x52);
  writer_->writeBE<uint32_t>(ref);
  return true;
}

}  // namespace Hessian2
