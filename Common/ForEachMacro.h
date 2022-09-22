#ifndef ForEachMacro_H_
#define ForEachMacro_H_

// Make a FOREACH macro
#define FE_0(WHAT)
#define FE_1(WHAT, X) WHAT(X) 
#define FE_2(WHAT, X, ...) WHAT(X)FE_1(WHAT, __VA_ARGS__)
#define FE_3(WHAT, X, ...) WHAT(X)FE_2(WHAT, __VA_ARGS__)
#define FE_4(WHAT, X, ...) WHAT(X)FE_3(WHAT, __VA_ARGS__)
#define FE_5(WHAT, X, ...) WHAT(X)FE_4(WHAT, __VA_ARGS__)
#define FE_6(WHAT, X, ...) WHAT(X)FE_5(WHAT, __VA_ARGS__)
#define FE_7(WHAT, X, ...) WHAT(X)FE_6(WHAT, __VA_ARGS__)
#define FE_8(WHAT, X, ...) WHAT(X)FE_7(WHAT, __VA_ARGS__)
#define FE_9(WHAT, X, ...) WHAT(X)FE_8(WHAT, __VA_ARGS__)
#define FE_10(WHAT, X, ...) WHAT(X)FE_9(WHAT, __VA_ARGS__)
#define FE_11(WHAT, X, ...) WHAT(X)FE_10(WHAT, __VA_ARGS__)
#define FE_12(WHAT, X, ...) WHAT(X)FE_11(WHAT, __VA_ARGS__)
#define FE_13(WHAT, X, ...) WHAT(X)FE_12(WHAT, __VA_ARGS__)
#define FE_14(WHAT, X, ...) WHAT(X)FE_13(WHAT, __VA_ARGS__)
#define FE_15(WHAT, X, ...) WHAT(X)FE_14(WHAT, __VA_ARGS__)
#define FE_16(WHAT, X, ...) WHAT(X)FE_15(WHAT, __VA_ARGS__)
#define FE_17(WHAT, X, ...) WHAT(X)FE_16(WHAT, __VA_ARGS__)
#define FE_18(WHAT, X, ...) WHAT(X)FE_17(WHAT, __VA_ARGS__)
#define FE_19(WHAT, X, ...) WHAT(X)FE_18(WHAT, __VA_ARGS__)
#define FE_20(WHAT, X, ...) WHAT(X)FE_19(WHAT, __VA_ARGS__)
#define FE_21(WHAT, X, ...) WHAT(X)FE_20(WHAT, __VA_ARGS__)
#define FE_22(WHAT, X, ...) WHAT(X)FE_21(WHAT, __VA_ARGS__)
#define FE_23(WHAT, X, ...) WHAT(X)FE_22(WHAT, __VA_ARGS__)
#define FE_24(WHAT, X, ...) WHAT(X)FE_23(WHAT, __VA_ARGS__)
#define FE_25(WHAT, X, ...) WHAT(X)FE_24(WHAT, __VA_ARGS__)
#define FE_26(WHAT, X, ...) WHAT(X)FE_25(WHAT, __VA_ARGS__)
#define FE_27(WHAT, X, ...) WHAT(X)FE_26(WHAT, __VA_ARGS__)
#define FE_28(WHAT, X, ...) WHAT(X)FE_27(WHAT, __VA_ARGS__)
#define FE_29(WHAT, X, ...) WHAT(X)FE_28(WHAT, __VA_ARGS__)
#define FE_30(WHAT, X, ...) WHAT(X)FE_29(WHAT, __VA_ARGS__)
#define FE_31(WHAT, X, ...) WHAT(X)FE_30(WHAT, __VA_ARGS__)
#define FE_32(WHAT, X, ...) WHAT(X)FE_31(WHAT, __VA_ARGS__)
#define FE_33(WHAT, X, ...) WHAT(X)FE_32(WHAT, __VA_ARGS__)
#define FE_34(WHAT, X, ...) WHAT(X)FE_33(WHAT, __VA_ARGS__)
#define FE_35(WHAT, X, ...) WHAT(X)FE_34(WHAT, __VA_ARGS__)
#define FE_36(WHAT, X, ...) WHAT(X)FE_35(WHAT, __VA_ARGS__)
#define FE_37(WHAT, X, ...) WHAT(X)FE_36(WHAT, __VA_ARGS__)
#define FE_38(WHAT, X, ...) WHAT(X)FE_37(WHAT, __VA_ARGS__)
#define FE_39(WHAT, X, ...) WHAT(X)FE_38(WHAT, __VA_ARGS__)
#define FE_40(WHAT, X, ...) WHAT(X)FE_39(WHAT, __VA_ARGS__)

//... repeat as needed

#define GET_MACRO(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,NAME,...) NAME 
#define FOR_EACH(action,...) \
  GET_MACRO(_0,__VA_ARGS__,FE_40,FE_39,FE_38,FE_37,FE_36,FE_35,FE_34,FE_33,FE_32,FE_31,FE_30,FE_29,FE_28,FE_27,FE_26,FE_25,FE_24,FE_23,FE_22,FE_21,FE_20,FE_19,FE_18,FE_17,FE_16,FE_15,FE_14,FE_13,FE_12,FE_11,FE_10,FE_9,FE_8,FE_7,FE_6,FE_5,FE_4,FE_3,FE_2,FE_1,FE_0)(action,__VA_ARGS__)
  
#endif
