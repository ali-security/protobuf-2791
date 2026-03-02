#include "google/protobuf/repeated_field_proxy.h"

#include <string>

#include <gtest/gtest.h>
#include "google/protobuf/arena.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/test_protos/repeated_field_proxy_test.pb.h"

namespace google {
namespace protobuf {
namespace internal {

using ::proto2_unittest::RepeatedFieldProxyTestSimpleMessage;

// A test-only container for a repeated field that manages construction and
// destruction of the underlying repeated field, and can construct proxies.
//
// This is necessary because RepeatedFieldProxy has no public constructors,
// aside from copy assignment.
template <typename T>
class TestOnlyRepeatedFieldContainer {
  using FieldType = typename internal::RepeatedFieldTraits<T>::type;

 public:
  static TestOnlyRepeatedFieldContainer<T> New(Arena* arena) {
    return TestOnlyRepeatedFieldContainer<T>(Arena::Create<FieldType>(arena),
                                             arena);
  }

  ~TestOnlyRepeatedFieldContainer() {
    if (arena_ == nullptr) {
      delete field_;
    }
  }

  FieldType& operator*() { return *field_; }
  const FieldType& operator*() const { return *field_; }

  FieldType* operator->() { return &*field_; }
  const FieldType* operator->() const { return &*field_; }

  RepeatedFieldProxy<T> MakeProxy() {
    return RepeatedFieldProxy<T>(*field_, arena_);
  }
  RepeatedFieldProxy<const T> MakeConstProxy() const {
    return RepeatedFieldProxy<const T>(*field_);
  }

 private:
  TestOnlyRepeatedFieldContainer(FieldType* field, Arena* arena)
      : field_(field), arena_(arena) {}

  FieldType* field_;
  Arena* arena_;
};

namespace {

class RepeatedFieldProxyTest : public testing::TestWithParam<bool> {
 protected:
  bool UseArena() const { return GetParam(); }
  Arena* arena() { return UseArena() ? &arena_ : nullptr; }

  template <typename T>
  TestOnlyRepeatedFieldContainer<T> MakeRepeatedFieldContainer() {
    return TestOnlyRepeatedFieldContainer<T>::New(arena());
  }

 private:
  Arena arena_;
};

TEST_P(RepeatedFieldProxyTest, ArrayIndexing) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);

  {
    auto proxy = field.MakeProxy();
    EXPECT_EQ(proxy[0].value(), 1);
    EXPECT_EQ(proxy[1].value(), 2);
    EXPECT_EQ(proxy[2].value(), 3);
  }

  {
    auto proxy = field.MakeConstProxy();
    EXPECT_EQ(proxy[0].value(), 1);
    EXPECT_EQ(proxy[1].value(), 2);
    EXPECT_EQ(proxy[2].value(), 3);
  }
}

INSTANTIATE_TEST_SUITE_P(RepeatedFieldProxyTest, RepeatedFieldProxyTest,
                         testing::Bool(),
                         [](const testing::TestParamInfo<bool>& info) {
                           return info.param ? "WithArena" : "WithoutArena";
                         });

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
