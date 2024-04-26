#include "hessian2/basic_codec/def_ref_codec.hpp"

namespace Hessian2 {

// class-def  ::= 'C' string int string*
// object     ::= 'O' int value*
//            ::= [x60-x6f] value*
template <>
std::unique_ptr<Object::Definition> Decoder::decode() {
  auto ret = reader_->read<uint8_t>();
  if (!ret.first) {
    return nullptr;
  }

  Object::RawDefinitionSharedPtr def =
      std::make_shared<Object::RawDefinition>();
  auto code = ret.second;
  if (code == 'O') {
    auto type_len = decode<int32_t>();
    if (!type_len) {
      return nullptr;
    }
    std::string type_name;
    type_name.resize(*type_len);
    reader_->readNBytes(type_name.data(), *type_len);

    auto field_len = decode<int32_t>();
    if (!field_len) {
      return nullptr;
    }
    def->type_ = type_name;
    def->field_names_.reserve(*field_len);

    for (int i = 0; i < *field_len; i++) {
      auto field_name = decode<std::string>();
      if (!field_name) {
        return nullptr;
      }
      def->field_names_.push_back(*field_name);
    }
    def_ref_.push_back(def);
  } else if (code == 'o') {
    auto ref_number = decode<int32_t>();
    if (!ref_number) {
      return nullptr;
    }
    if (static_cast<uint32_t>(*ref_number) >= def_ref_.size()) {
      return nullptr;
    }

    def = def_ref_[*ref_number];
  } else {
    return nullptr;
  }

  return std::make_unique<Object::Definition>(def);
}

// TODO(tianqian.zyf:) Avoid copying definitions
template <>
bool Encoder::encode(const Object::RawDefinition &value) {
  auto r = getDefRef(value);
  if (r == -1) {
    writer_->writeByte('O');
    def_ref_.push_back(std::make_shared<Object::RawDefinition>(value));
    encode<int32_t>(value.type_.length());
    writer_->rawWrite(value.type_);
    encode<int32_t>(value.field_names_.size());
    for (const auto &field_name : value.field_names_) {
      encode<std::string>(field_name);
    }
    encode<Object::RawDefinition>(value);
  } else {
    writer_->writeByte('o');
    encode<int32_t>(r);
  }

  return true;
}

// TODO(tianqian.zyf):
template <>
bool Encoder::encode(const Object::Definition &value) {
  return encode<Object::RawDefinition>(*value.data_);
}

}  // namespace Hessian2
