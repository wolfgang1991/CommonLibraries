#ifndef STATIC_ASSERT_CPP98_H_INCLUDED
#define STATIC_ASSERT_CPP98_H_INCLUDED

//! results in a compile error if Is_Condition_Met==false
template<bool Is_Condition_Met>
struct Static_assert_cpp98
{
  static void apply() {static const char junk[ Is_Condition_Met ? 1 : -1 ];}
};

template<>
struct Static_assert_cpp98<true>
{
  static void apply() {}
};

#define STATIC_ASSERT_CPP98(condition) Static_assert_cpp98<condition>::apply()

#endif
