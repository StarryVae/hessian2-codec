#include "hessian2/basic_codec/list_codec.hpp"

namespace Hessian2 {

// # typed list/vector
// ::= x55 type value* 'Z'   # variable-length list
// ::= 'V' type int value*   # fixed-length list
// ::= [x70-77] type value*  # fixed-length typed list
template <>
std::unique_ptr<TypedListObject> Decoder::decode() {
  std::string type;
  Object::TypedList obj_list;

  auto result = std::make_unique<TypedListObject>();
  values_ref_.push_back(result.get());

  auto ret = reader_->read<uint8_t>();
  if (!ret.first) {
    return nullptr;
  }

  auto code = ret.second;

  auto ReadNumsObject = [&](int nums, bool end) -> bool {
    for (int i = 0; i < nums; i++) {
      auto o = decode<Object>();
      if (!o) {
        return false;
      }
      obj_list.values_.emplace_back(std::move(o));
    }
    if (end) {
      // Skip last 'z'
      auto ret = reader_->read<uint8_t>();
      if (!ret.first || ret.second != 'z') {
        return false;
      }
    }
    return true;
  };

  /*  auto ReadObjectUntilEnd = [&]() -> bool {
      auto ret = reader_->peek<uint8_t>();
      if (!ret.first) {
        return false;
      }

      while (ret.second != 'z') {
        auto o = decode<Object>();
        if (!o) {
          return false;
        }
        obj_list.values_.emplace_back(std::move(o));
        ret = reader_->peek<uint8_t>();
        if (!ret.first) {
          return false;
        }
      }

      // Skip last 'Z'
      reader_->read<uint8_t>();
      return true;
    };*/

  auto type_str = decode<Object::TypeRef>();
  if (!type_str) {
    return nullptr;
  }

  obj_list.type_name_ = std::move(type_str->type_);

  bool end = false;
  switch (code) {
    case 'v': {
      auto ret = decode<int32_t>();
      if (!ret) {
        return nullptr;
      }
      if (!ReadNumsObject(*ret, end)) {
        return nullptr;
      }
      break;
    }

    case 'V': {
      auto ret = reader_->read<uint8_t>();
      if (!ret.first) {
        return nullptr;
      }
      end = true;
      auto len_code = ret.second;

      switch (len_code) {
        case 'n': {
          auto ret = reader_->readBE<uint8_t>();
          if (!ret.first) {
            return nullptr;
          }
          auto len = ret.second;
          if (!ReadNumsObject(len, end)) {
            return nullptr;
          }
          break;
        }
        case 'l': {
          auto ret = reader_->readBE<uint32_t>();
          if (!ret.first) {
            return nullptr;
          }
          auto len = ret.second;
          if (!ReadNumsObject(len, end)) {
            return nullptr;
          }
          break;
        }
        default:
          return nullptr;
      }
      break;
    }
    default:
      return nullptr;
  }
  result->setTypedList(std::move(obj_list));
  return result;
}

// ::= x57 value* 'Z'        # variable-length untyped list
// ::= x58 int value*        # fixed-length untyped list
// ::= [x78-7f] value*       # fixed-length untyped list
template <>
std::unique_ptr<UntypedListObject> Decoder::decode() {
  Object::UntypedList obj_list;

  auto result = std::make_unique<UntypedListObject>();
  values_ref_.push_back(result.get());

  auto ret = reader_->read<uint8_t>();
  if (!ret.first) {
    return nullptr;
  }

  auto code = ret.second;

  auto ReadNumsObject = [&](int nums) -> bool {
    for (int i = 0; i < nums; i++) {
      auto o = decode<Object>();
      if (!o) {
        return false;
      }
      obj_list.emplace_back(std::move(o));
    }
    // Skip last 'z'
    auto ret = reader_->read<uint8_t>();
    if (!ret.first || ret.second != 'z') {
      return false;
    }
    return true;
  };

  /*  auto ReadObjectUntilEnd = [&]() -> bool {
      auto ret = reader_->peek<uint8_t>();
      if (!ret.first) {
        return false;
      }

      while (ret.second != 'z') {
        auto o = decode<Object>();
        if (!o) {
          return false;
        }
        obj_list.emplace_back(std::move(o));
        ret = reader_->peek<uint8_t>();
        if (!ret.first) {
          return false;
        }
      }
      // Skip last 'Z'
      reader_->read<uint8_t>();
      return true;
    };*/

  if (code != 'V') {
    return nullptr;
  }

  auto ret1 = reader_->read<uint8_t>();
  if (!ret1.first) {
    return nullptr;
  }
  auto len_code = ret1.second;

  switch (len_code) {
    case 'n': {
      auto ret = reader_->readBE<uint8_t>();
      if (!ret.first) {
        return nullptr;
      }
      auto len = ret.second;
      if (!ReadNumsObject(len)) {
        return nullptr;
      }
      break;
    }
    case 'l': {
      auto ret = reader_->readBE<uint32_t>();
      if (!ret.first) {
        return nullptr;
      }
      auto len = ret.second;
      if (!ReadNumsObject(len)) {
        return nullptr;
      }
      break;
    }
    default:
      return nullptr;
  }

  result->setUntypedList(std::move(obj_list));
  return result;
}

template <>
bool Encoder::encode(const TypedListObject& value) {
  values_ref_.emplace(&value, values_ref_.size());
  auto typed_list = value.toTypedList();
  ABSL_ASSERT(typed_list.has_value());
  auto& typed_list_value = typed_list.value().get();

  Object::TypeRef type_ref(typed_list_value.type_name_);
  auto len = typed_list_value.values_.size();

  auto r = getTypeRef(type_ref.type_);
  if (r == -1) {
    writer_->writeByte('V');
    encode<Object::TypeRef>(type_ref);
    if (len < 0x100) {
      writer_->writeByte('n');
      writer_->writeBE<uint8_t>(len);
    } else {
      writer_->writeByte('l');
      writer_->writeBE<uint32_t>(len);
    }
  } else {
    writer_->writeByte('v');
    encode<Object::TypeRef>(type_ref);
    encode<int32_t>(len);
  }

  for (size_t i = 0; i < len; i++) {
    encode<Object>(*typed_list_value.values_[i]);
  }

  if (r == -1) {
    writer_->writeByte('z');
  }

  return true;
}

template <>
bool Encoder::encode(const UntypedListObject& value) {
  values_ref_.emplace(&value, values_ref_.size());
  auto untyped_list = value.toUntypedList();
  ABSL_ASSERT(untyped_list.has_value());
  auto& untyped_list_value = untyped_list.value().get();

  auto len = untyped_list_value.size();

  writer_->writeByte('V');

  if (len < 0x100) {
    writer_->writeByte('n');
    writer_->writeBE<uint8_t>(len);
  } else {
    writer_->writeByte('l');
    writer_->writeBE<uint32_t>(len);
  }

  for (size_t i = 0; i < len; i++) {
    encode<Object>(*(untyped_list_value)[i]);
  }

  writer_->writeByte('z');

  return true;
}

}  // namespace Hessian2
