#include "hessian2/basic_codec/type_ref_codec.hpp"

namespace Hessian2 {

template <>
std::unique_ptr<Object::TypeRef> Decoder::decode() {
  auto ret = reader_->peek<uint8_t>();
  if (!ret.first) {
    return nullptr;
  }
  auto code = ret.second;

  if (code == 't') {
    reader_->read<uint8_t>();
    auto ret_int = reader_->readBE<uint16_t>();
    if (!ret_int.first) {
      return nullptr;
    }

    std::string output;
    output.resize(ret_int.second);
    reader_->readNBytes(output.data(), ret_int.second);

    types_ref_.push_back(output);

    return std::make_unique<Object::TypeRef>(output);
  }

  auto ret_int = decode<int32_t>();
  if (!ret_int) {
    return nullptr;
  }

  auto ref = static_cast<uint32_t>(*ret_int);
  if (types_ref_.size() <= ref) {
    return nullptr;
  } else {
    return std::make_unique<Object::TypeRef>(types_ref_[ref]);
  }

  return nullptr;
}

template <>
bool Encoder::encode(const Object::TypeRef &value) {
  auto r = getTypeRef(value.type_);
  if (r == -1) {
    types_ref_.emplace(value.type_, types_ref_.size());
    writer_->writeByte('t');
    writer_->writeBE<uint16_t>(value.type_.length());
    writer_->rawWrite(value.type_);
  } else {
    encode<int32_t>(r);
  }

  return true;
}

}  // namespace Hessian2
