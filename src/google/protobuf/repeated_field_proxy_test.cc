#include "google/protobuf/repeated_field_proxy.h"

#include <cstdint>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/test_protos/repeated_field_proxy_test.pb.h"
#include "google/protobuf/test_textproto.h"

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
    EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 1)pb"));
    EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 2)pb"));
    EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 3)pb"));
  }

  {
    auto proxy = field.MakeConstProxy();
    EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 1)pb"));
    EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 2)pb"));
    EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 3)pb"));
  }
}

TEST_P(RepeatedFieldProxyTest, MutateElementPrimitive) {
  auto field = MakeRepeatedFieldContainer<int32_t>();
  field->Add(1);
  field->Add(2);
  field->Add(3);

  {
    auto proxy = field.MakeProxy();
    proxy.set(0, 4);
    proxy.set(1, 5);

    EXPECT_EQ(proxy[0], 4);
    EXPECT_EQ(proxy[1], 5);
    EXPECT_EQ(proxy[2], 3);
  }
}

TEST_P(RepeatedFieldProxyTest, MutateElementString) {
  auto field = MakeRepeatedFieldContainer<std::string>();
  field->Add("1");
  field->Add("2");
  field->Add("3");
  field->Add("4");

  {
    auto proxy = field.MakeProxy();
    proxy[0] = "5";
    proxy[1] = std::string("6");
    const char* c_str = "7";
    proxy[2] = c_str;
    proxy[3] = absl::string_view("8");

    EXPECT_EQ(proxy[0], "5");
    EXPECT_EQ(proxy[1], "6");
    EXPECT_EQ(proxy[2], "7");
    EXPECT_EQ(proxy[3], "8");

    proxy.set(0, "9");
    proxy.set(1, std::string("10"));
    const char* c_str2 = "11";
    proxy.set(2, c_str2);
    proxy.set(3, absl::string_view("12"));

    EXPECT_EQ(proxy[0], "9");
    EXPECT_EQ(proxy[1], "10");
    EXPECT_EQ(proxy[2], "11");
    EXPECT_EQ(proxy[3], "12");
  }
}

TEST_P(RepeatedFieldProxyTest, MutateElementStringView) {
  auto field = MakeRepeatedFieldContainer<absl::string_view>();
  field->Add("1");
  field->Add("2");
  field->Add("3");
  field->Add("4");

  {
    auto proxy = field.MakeProxy();

    proxy.set(0, "5");
    proxy.set(1, std::string("6"));
    const char* c_str = "7";
    proxy.set(2, c_str);
    proxy.set(3, absl::string_view("8"));

    EXPECT_EQ(proxy[0], "5");
    EXPECT_EQ(proxy[1], "6");
    EXPECT_EQ(proxy[2], "7");
    EXPECT_EQ(proxy[3], "8");
  }
}

TEST_P(RepeatedFieldProxyTest, MutateElementCord) {
  auto field = MakeRepeatedFieldContainer<absl::Cord>();
  field->Add(absl::Cord("1"));
  field->Add(absl::Cord("2"));
  field->Add(absl::Cord("3"));
  field->Add(absl::Cord("4"));

  {
    auto proxy = field.MakeProxy();

    proxy[0] = "5";
    proxy[1] = std::string("6");
    const char* c_str = "7";
    proxy[2] = c_str;
    proxy[3] = absl::string_view("8");

    EXPECT_EQ(proxy[0], "5");
    EXPECT_EQ(proxy[1], "6");
    EXPECT_EQ(proxy[2], "7");
    EXPECT_EQ(proxy[3], "8");

    proxy.set(0, "9");
    proxy.set(1, std::string("10"));
    const char* c_str2 = "11";
    proxy.set(2, c_str2);
    proxy.set(3, absl::string_view("12"));

    EXPECT_EQ(proxy[0], "9");
    EXPECT_EQ(proxy[1], "10");
    EXPECT_EQ(proxy[2], "11");
    EXPECT_EQ(proxy[3], "12");
  }
}

TEST_P(RepeatedFieldProxyTest, MutateElementMessage) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);

  {
    auto proxy = field.MakeProxy();
    proxy[2].set_value(4);

    EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 1)pb"));
    EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 2)pb"));
    EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 4)pb"));

    RepeatedFieldProxyTestSimpleMessage msg;
    msg.set_value(5);
    proxy.set(0, msg);
    // `msg` is copied into the element, so it should be unchanged.
    EXPECT_TRUE(msg.has_value());

    RepeatedFieldProxyTestSimpleMessage msg2;
    msg2.set_value(6);
    proxy.set(1, std::move(msg2));
    // `msg2` is moved into the element, so it should be cleared.
    EXPECT_FALSE(msg2.has_value());

    EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 5)pb"));
    EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 6)pb"));
    EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 4)pb"));
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
