/*
 * Copyright (c) 2022 UAVCAN Development Team.
 * Authors: Pavel Pletenev <cpp.create@gmail.com>
 * This software is distributed under the terms of the MIT License.
 *
 * Tests of serialization
 */

#include "test_helpers.hpp"
#include "uavcan/time/TimeSystem_0_1.hpp"
#include "regulated/basics/Struct__0_1.hpp"
#include "regulated/basics/Primitive_0_1.hpp"



TEST(Serialization, BasicSerialize) {
    uavcan::time::TimeSystem_0_1 a;
    uint8_t buffer[8]{};
    {
        a.value = 1;
        std::fill(std::begin(buffer), std::end(buffer), 0xAA);
        const auto result = a.serialize({{buffer}});
        ASSERT_TRUE(result);
        ASSERT_EQ(1U, *result);
        ASSERT_EQ(1U, buffer[0]);
        ASSERT_EQ(0xAA, buffer[1]);
    }
    {
        a.value = 0xFF;
        std::fill(std::begin(buffer), std::end(buffer), 0xAA);
        const auto result = a.serialize({{buffer}});
        ASSERT_TRUE(result);
        ASSERT_EQ(1U, *result);
        ASSERT_EQ(0x0FU, buffer[0]);
        ASSERT_EQ(0xAAU, buffer[1]);
    }
}

/// This was copied from C counterpart and modified for C++
/// The reference array has been pedantically validated manually bit by bit (it did really took authors of
/// C tests about three hours).
/// The following Python script has been used to cross-check against PyUAVCAN, which has been cross-checked against
/// earlier v0 implementations beforehand:
///
///     import sys, pathlib, importlib, pyuavcan
///     sys.path.append(str(pathlib.Path.cwd()))
///     target, lookup = sys.argv[1], sys.argv[2:]
///     for lk in lookup:
///         pyuavcan.dsdl.generate_package(lk, lookup)
///     pyuavcan.dsdl.generate_package(target, lookup)
///     from regulated.basics import Struct__0_1, DelimitedFixedSize_0_1, DelimitedVariableSize_0_1, Union_0_1
///     s = Struct__0_1()
///     s.boolean = True
///     s.i10_4[0] = +0x5555                              # saturates to +511
///     s.i10_4[1] = -0x6666                              # saturates to -512
///     s.i10_4[2] = +0x0055                              # original value retained
///     s.i10_4[3] = -0x00AA                              # original value retained
///     s.f16_le2 = [
///         -65504.0,
///         +float('inf'),                                # negative infinity retained
///     ]
///     s.unaligned_bitpacked_3 = [1, 0, 1]
///     s.bytes_lt3 = [111, 222]
///     s.bytes_3[0] = -0x77
///     s.bytes_3[1] = -0x11
///     s.bytes_3[2] = +0x77
///     s.u2_le4 = [
///         0x02,                                         # retained
///         0x11,                                         # truncated => 1
///         0xFF,                                         # truncated => 3
///     ]
///     s.delimited_fix_le2 = [DelimitedFixedSize_0_1()]
///     s.u16_2[0] = 0x1234
///     s.u16_2[1] = 0x5678
///     s.aligned_bitpacked_3 = [1, 0, 0]
///     s.unaligned_bitpacked_lt3 = [1, 0]                # 0b01
///     s.delimited_var_2[0].f16 = +float('inf')
///     s.delimited_var_2[1].f64 = -1e40                  # retained
///     s.aligned_bitpacked_le3 = [1]
///     sr = b''.join(pyuavcan.dsdl.serialize(s))
///     print(len(sr), 'bytes')
///     print('\n'.join(f'0x{x:02X}U,' for x in sr))
TEST(Serialization, StructReference)
{
    regulated::basics::Struct__0_1 obj{};

    // Initialize a reference object, serialize, and compare against the reference serialized representation.
    obj.boolean = true;
    obj.i10_4[0] = +0x5555;                             // saturates to +511
    obj.i10_4[1] = -0x6666;                             // saturates to -512
    obj.i10_4[2] = +0x0055;                             // original value retained
    obj.i10_4[3] = -0x00AA;                             // original value retained
    obj.f16_le2.emplace_back(-1e9F);                    // saturated to -65504
    obj.f16_le2.emplace_back(+INFINITY);                // infinity retained
    ASSERT_EQ(2U, obj.f16_le2.size());
    //obj.unaligned_bitpacked_3[0] = 0xF5;     // 0b101, rest truncated away and ignored TODO:Fix
    obj.unaligned_bitpacked_3[0] = 1;
    obj.unaligned_bitpacked_3[1] = 0;
    obj.unaligned_bitpacked_3[2] = 1;
    //obj.sealed = 123;                           // ignored
    obj.bytes_lt3.emplace_back(111);
    obj.bytes_lt3.emplace_back(222);
    ASSERT_EQ(2U, obj.bytes_lt3.size());
    obj.bytes_3[0] = -0x77;
    obj.bytes_3[1] = -0x11;
    obj.bytes_3[2] = +0x77;
    obj.u2_le4.emplace_back(0x02);                      // retained
    obj.u2_le4.emplace_back(0x11);                      // truncated => 1
    obj.u2_le4.emplace_back(0xFF);                      // truncated => 3
    //obj.u2_le4.emplace_back(0xFF);                      // ignored because the length is 3
    ASSERT_EQ(3U, obj.u2_le4.size());
    obj.delimited_fix_le2.emplace_back();    // ignored
    ASSERT_EQ(1U, obj.delimited_fix_le2.size());
    obj.u16_2[0] = 0x1234;
    obj.u16_2[1] = 0x5678;
    obj.aligned_bitpacked_3[0] = 0xF1U;
    // obj.unaligned_bitpacked_lt3.bitpacked[0] = 0xF1U;
    obj.unaligned_bitpacked_lt3.emplace_back(1);
    obj.unaligned_bitpacked_lt3.emplace_back(0);
    ASSERT_EQ(2U, obj.unaligned_bitpacked_lt3.size());              // 0b01, rest truncated
    obj.delimited_var_2[0].set_f16(+1e9F);    // truncated to infinity
    obj.delimited_var_2[1].set_f64(-1e40);    // retained
    obj.aligned_bitpacked_le3.emplace_back(1);
    ASSERT_EQ(1U, obj.aligned_bitpacked_le3.size());                // only lsb is set, other truncated

    const uint8_t reference[] = {
        0xFEU,  // byte 0: void1, true, 6 lsb of int10 = 511
        0x07U,  // byte 1: 4 msb of int10 = 511, 4 lsb of -512 = 0b_10_0000_0000
        0x60U,  // byte 2: 6 msb of -512 (0x60 = 0b_0110_0000), 2 lsb of 0x0055 = 0b0001010101
        0x15U,  // byte 3: 8 msb of 0b_00_0101_0101,                       0x15 = 0b00010101
        0x56U,  // byte 4: ALIGNED; -0x00AA in two's complement is 0x356 = 0b_11_01010110
        0x0BU,  // byte 5: 2 msb of the above (0b11) followed by 8 bit of length prefix (2) of float16[<=2] f16_le2
        0xFCU,  // byte 6: 2 msb of the length prefix followed by 6 lsb of (float16.min = 0xfbff = 0b_11111011_11111111)
        0xEFU,  // byte 7: 0b_xx_111011_11xxxxxx (continuation of the float16)
        0x03U,  // byte 8: 2 msb of the above (0b11) and the next float16 = +inf, represented 0x7C00 = 0b_01111100_00000000
        0xF0U,  // byte 9: 0b_xx111100_00xxxxxx (continuation of the infinity)
        0x15U,  // byte 10: 2 msb of the above (0b01) followed by bool[3] unaligned_bitpacked_3 = [1, 0, 1], then PADDING
        0x02U,  // byte 11: ALIGNED; empty struct not manifested, here we have length = 2 of uint8[<3] bytes_lt3
        0x6FU,  // byte 12: bytes_lt3[0] = 111
        0xDEU,  // byte 13: bytes_lt3[1] = 222
        0x89U,  // byte 14: bytes_3[0] = -0x77 (two's complement)
        0xEFU,  // byte 15: bytes_3[1] = -0x11 (two's complement)
        0x77U,  // byte 16: bytes_3[2] = +0x77
        0x03U,  // byte 17: length = 3 of truncated uint2[<=4] u2_le4
        0x36U,  // byte 18: 0b_00_11_01_10: u2_le4[0] = 0b10, u2_le4[1] = 0b01, u2_le4[2] = 0b11, then dynamic padding
        0x01U,  // byte 19: ALIGNED; length = 1 of DelimitedFixedSize.0.1[<=2] delimited_fix_le2
        0x00U,  // byte 20: Constant DH of DelimitedFixedSize.0.1
        0x00U,  // byte 21: ditto
        0x00U,  // byte 22: ditto
        0x00U,  // byte 23: ditto
        0x34U,  // byte 24: uint16[2] u16_2; first element = 0x1234
        0x12U,  // byte 25: continuation
        0x78U,  // byte 26: second element = 0x5678
        0x56U,  // byte 27: continuation
        0x11U,  // byte 28: bool[3] aligned_bitpacked_3 = [1, 0, 0]; then 5 lsb of length = 2 of bool[<3] unaligned_bitpacked_lt3
        0x08U,  // byte 29: 3 msb of length = 2 (i.e., zeros), then values [1, 0], then 1 bit of padding before composite
        0x03U,  // byte 30: DH = 3 of the first element of DelimitedVariableSize.0.1[2] delimited_var_2
        0x00U,  // byte 31: ditto
        0x00U,  // byte 32: ditto
        0x00U,  // byte 33: ditto
        0x00U,  // byte 34: union tag = 0, f16 selected
        0x00U,  // byte 35: f16 truncated to positive infinity; see representation above
        0x7CU,  // byte 36: ditto
        0x09U,  // byte 37: DH = (8 + 1) of the second element of DelimitedVariableSize.0.1[2] delimited_var_2
        0x00U,  // byte 38: ditto
        0x00U,  // byte 39: ditto
        0x00U,  // byte 40: ditto
        0x02U,  // byte 41: union tag = 2, f64 selected (notice that union tags are always aligned by design)
        0xA5U,  // byte 42: float64 = -1e40 is 0xc83d6329f1c35ca5, this is the LSB
        0x5CU,  // byte 43: ditto
        0xC3U,  // byte 44: ditto
        0xF1U,  // byte 45: ditto
        0x29U,  // byte 46: ditto
        0x63U,  // byte 47: ditto
        0x3DU,  // byte 48: ditto
        0xC8U,  // byte 49: ditto
        0x01U,  // byte 50: length = 1 of bool[<=3] aligned_bitpacked_le3
        0x01U,  // byte 51: the one single bit of the above, then 7 bits of dynamic padding to byte// byte 51: END OF SERIALIZED REPRESENTATION
        0x55U,  // byte 52: canary  1
        0x55U,  // byte 53: canary  2
        0x55U,  // byte 54: canary  3
        0x55U,  // byte 55: canary  4
        0x55U,  // byte 56: canary  5
        0x55U,  // byte 57: canary  6
        0x55U,  // byte 58: canary  7
        0x55U,  // byte 59: canary  8
        0x55U,  // byte 60: canary  9
        0x55U,  // byte 61: canary 10
        0x55U,  // byte 62: canary 11
        0x55U,  // byte 63: canary 12
        0x55U,  // byte 64: canary 13
        0x55U,  // byte 65: canary 14
        0x55U,  // byte 66: canary 15
        0x55U,  // byte 67: canary 16
    };

    uint8_t buf[sizeof(reference)];
    (void) memset(&buf[0], 0x55U, sizeof(buf));  // fill out canaries

    auto result = obj.serialize({{buf, sizeof(buf)}});
    ASSERT_TRUE(result) << "Error is " << static_cast<int>(result.error());

    EXPECT_EQ(sizeof(reference) - 16U, result.value());

    for(size_t i=0; i< sizeof(reference); i++){
        ASSERT_EQ(reference[i], buf[i]) << "Failed at " << i;
    }

    // Check union manipulation functions.
    ASSERT_TRUE(obj.delimited_var_2[0].is_f16());
    ASSERT_FALSE(obj.delimited_var_2[0].is_f32());
    ASSERT_FALSE(obj.delimited_var_2[0].is_f64());
    obj.delimited_var_2[0].set_f32();
    ASSERT_FALSE(obj.delimited_var_2[0].is_f16());
    ASSERT_TRUE(obj.delimited_var_2[0].is_f32());
    ASSERT_FALSE(obj.delimited_var_2[0].is_f64());
    obj.delimited_var_2[0].set_f64();
    ASSERT_FALSE(obj.delimited_var_2[0].is_f16());
    ASSERT_FALSE(obj.delimited_var_2[0].is_f32());
    ASSERT_TRUE(obj.delimited_var_2[0].is_f64());

    // Test default initialization.
    obj.regulated::basics::Struct__0_1::~Struct__0_1();
    new (&obj)regulated::basics::Struct__0_1();
    ASSERT_EQ(false, obj.boolean);
    ASSERT_EQ(0, obj.i10_4[0]);
    ASSERT_EQ(0, obj.i10_4[1]);
    ASSERT_EQ(0, obj.i10_4[2]);
    ASSERT_EQ(0, obj.i10_4[3]);
    ASSERT_EQ(0U, obj.f16_le2.size());
    ASSERT_EQ(0, obj.unaligned_bitpacked_3[0]);
    ASSERT_EQ(0, obj.unaligned_bitpacked_3[1]);
    ASSERT_EQ(0, obj.unaligned_bitpacked_3[2]);
    ASSERT_EQ(0U, obj.bytes_lt3.size());
    ASSERT_EQ(0, obj.bytes_3[0]);
    ASSERT_EQ(0, obj.bytes_3[1]);
    ASSERT_EQ(0, obj.bytes_3[2]);
    ASSERT_EQ(0U, obj.u2_le4.size());
    ASSERT_EQ(0U, obj.delimited_fix_le2.size());
    ASSERT_EQ(0, obj.u16_2[0]);
    ASSERT_EQ(0, obj.u16_2[1]);
    ASSERT_EQ(0, obj.aligned_bitpacked_3[0]);
    ASSERT_EQ(0, obj.aligned_bitpacked_3[1]);
    ASSERT_EQ(0, obj.aligned_bitpacked_3[2]);
    ASSERT_EQ(0U, obj.unaligned_bitpacked_lt3.size());

    // ASSERT_EQ(0, obj.delimited_var_2[0]._tag_);
    {
        auto ptr_f16 = obj.delimited_var_2[0].get_f16_if();
        ASSERT_NE(nullptr, ptr_f16);
        ASSERT_TRUE(CompareFloatsNear(0.f, *ptr_f16, 1e-9f));
    }
    {
        auto ptr_f16 = obj.delimited_var_2[1].get_f16_if();
        ASSERT_NE(nullptr, ptr_f16);
        ASSERT_TRUE(CompareFloatsNear(0.f, *ptr_f16, 1e-9f));
    }

    ASSERT_EQ(0U, obj.aligned_bitpacked_le3.size());

    // // Deserialize the above reference representation and compare the result against the original object.
    result = obj.deserialize({{reference}, 0U});
    ASSERT_TRUE(result) << "Error was " << result.error();
    // ASSERT_EQ(0, regulated_basics_Struct__0_1_deserialize_(&obj, &reference[0], &size));
    ASSERT_EQ(sizeof(reference) - 16U, result.value());   // 16 trailing bytes implicitly truncated away

    // ASSERT_EQ(true, obj.boolean);
    // ASSERT_EQ(+511, obj.i10_4[0]);                              // saturated
    // ASSERT_EQ(-512, obj.i10_4[1]);                              // saturated
    // ASSERT_EQ(+0x55, obj.i10_4[2]);
    // ASSERT_EQ(-0xAA, obj.i10_4[3]);
    // ASSERT_TRUE(CompareFloatsNear(-65504.0f, obj.f16_le2.elements[0], 1e-3f));
    // TEST_ASSERT_FLOAT_IS_INF(obj.f16_le2.elements[1]);
    // ASSERT_EQ(2, obj.f16_le2.count);
    // ASSERT_EQ(5, obj.unaligned_bitpacked_3_bitpacked_[0]);      // unused MSB are zero-padded
    // ASSERT_EQ(111, obj.bytes_lt3.elements[0]);
    // ASSERT_EQ(222, obj.bytes_lt3.elements[1]);
    // ASSERT_EQ(2, obj.bytes_lt3.count);
    // ASSERT_EQ(-0x77, obj.bytes_3[0]);
    // ASSERT_EQ(-0x11, obj.bytes_3[1]);
    // ASSERT_EQ(+0x77, obj.bytes_3[2]);
    // ASSERT_EQ(2, obj.u2_le4.elements[0]);
    // ASSERT_EQ(1, obj.u2_le4.elements[1]);
    // ASSERT_EQ(3, obj.u2_le4.elements[2]);
    // ASSERT_EQ(3, obj.u2_le4.count);
    // ASSERT_EQ(1, obj.delimited_fix_le2.count);
    // ASSERT_EQ(0x1234, obj.u16_2[0]);
    // ASSERT_EQ(0x5678, obj.u16_2[1]);
    // ASSERT_EQ(1, obj.aligned_bitpacked_3_bitpacked_[0]);        // unused MSB are zero-padded
    // ASSERT_EQ(1, obj.unaligned_bitpacked_lt3.bitpacked[0]);     // unused MSB are zero-padded
    // ASSERT_EQ(2, obj.unaligned_bitpacked_lt3.count);
    // ASSERT_EQ(0, obj.delimited_var_2[0]._tag_);
    // TEST_ASSERT_FLOAT_IS_INF(obj.delimited_var_2[0].f16);
    // ASSERT_EQ(2, obj.delimited_var_2[1]._tag_);
    // TEST_ASSERT_DOUBLE_WITHIN(0.5, -1e+40, obj.delimited_var_2[1].f64);
    // ASSERT_EQ(1, obj.aligned_bitpacked_le3.bitpacked[0]);       // unused MSB are zero-padded
    // ASSERT_EQ(1, obj.aligned_bitpacked_le3.count);

    // // Repeat the above, but apply implicit zero extension somewhere in the middle.
    // size = 25U;
    // ASSERT_EQ(0, regulated_basics_Struct__0_1_deserialize_(&obj, &reference[0], &size));
    // ASSERT_EQ(25, size);   // the returned size shall not exceed the buffer size

    // ASSERT_EQ(true, obj.boolean);
    // ASSERT_EQ(+511, obj.i10_4[0]);                              // saturated
    // ASSERT_EQ(-512, obj.i10_4[1]);                              // saturated
    // ASSERT_EQ(+0x55, obj.i10_4[2]);
    // ASSERT_EQ(-0xAA, obj.i10_4[3]);
    // ASSERT_TRUE(CompareFloatsNear(-65504.0, obj.f16_le2.elements[0], 1e-3));
    // TEST_ASSERT_FLOAT_IS_INF(obj.f16_le2.elements[1]);
    // ASSERT_EQ(2, obj.f16_le2.count);
    // ASSERT_EQ(5, obj.unaligned_bitpacked_3_bitpacked_[0]);      // unused MSB are zero-padded
    // ASSERT_EQ(111, obj.bytes_lt3.elements[0]);
    // ASSERT_EQ(222, obj.bytes_lt3.elements[1]);
    // ASSERT_EQ(2, obj.bytes_lt3.count);
    // ASSERT_EQ(-0x77, obj.bytes_3[0]);
    // ASSERT_EQ(-0x11, obj.bytes_3[1]);
    // ASSERT_EQ(+0x77, obj.bytes_3[2]);
    // ASSERT_EQ(2, obj.u2_le4.elements[0]);
    // ASSERT_EQ(1, obj.u2_le4.elements[1]);
    // ASSERT_EQ(3, obj.u2_le4.elements[2]);
    // ASSERT_EQ(3, obj.u2_le4.count);
    // ASSERT_EQ(1, obj.delimited_fix_le2.count);
    // ASSERT_EQ(0x0034, obj.u16_2[0]);                            // <-- IMPLICIT ZERO EXTENSION STARTS HERE
    // ASSERT_EQ(0x0000, obj.u16_2[1]);                            // IT'S
    // ASSERT_EQ(0, obj.aligned_bitpacked_3_bitpacked_[0]);        //      ZEROS
    // ASSERT_EQ(0, obj.unaligned_bitpacked_lt3.count);            //          ALL
    // ASSERT_EQ(0, obj.delimited_var_2[0]._tag_);                 //              THE
    // ASSERT_TRUE(CompareFloatsNear(0, obj.delimited_var_2[0].f16, 1e-9);      //                  WA)Y
    // ASSERT_EQ(0, obj.delimited_var_2[1]._tag_);                 //                      DOWN
    // ASSERT_TRUE(CompareFloatsNear(0, obj.delimited_var_2[1].f16, 1e-9));
    // ASSERT_EQ(0, obj.aligned_bitpacked_le3.count);
}



TEST(Serialization, Primitive)
{
    using namespace nunavut::testing;
    for (uint32_t i = 0U; i < 10; i++)
    {
        regulated::basics::Primitive_0_1 ref{};
        ref.a_u64  = randU64();
        ref.a_u32  = randU32();
        ref.a_u16  = randU16();
        ref.a_u8   = randU8();
        ref.a_u7   = randU8() & 127U;
        ref.n_u64  = randU64();
        ref.n_u32  = randU32();
        ref.n_u16  = randU16();
        ref.n_u8   = randU8();
        ref.n_u7   = randU8() & 127U;
        ref.a_i64  = randI64();
        ref.a_i32  = randI32();
        ref.a_i16  = randI16();
        ref.a_i8   = randI8();
        ref.a_i7   = randI8() % 64;
        ref.n_i64  = randI64();
        ref.n_i32  = randI32();
        ref.n_i16  = randI16();
        ref.n_i8   = randI8();
        ref.n_i7   = randI8() % 64;
        ref.a_f64  = randF64();
        ref.a_f32  = randF32();
        ref.a_f16  = randF16();
        ref.a_bool = randI8() % 2 == 0;
        ref.n_bool = randI8() % 2 == 0;
        ref.n_f64  = randF64();
        ref.n_f32  = randF32();
        ref.n_f16  = randF16();

        uint8_t buf[regulated::basics::Primitive_0_1::SERIALIZATION_BUFFER_SIZE_BYTES];
        std::memset(buf, 0, sizeof(buf));
        auto result = ref.serialize({{buf, sizeof(buf)}});
        ASSERT_TRUE(result) << "Error is " << result.error();
        ASSERT_EQ(
            static_cast<size_t>(regulated::basics::Primitive_0_1::SERIALIZATION_BUFFER_SIZE_BYTES), result.value());

        regulated::basics::Primitive_0_1 obj;
        result = obj.deserialize({ {buf, sizeof(buf)} });
        ASSERT_TRUE(result);
        EXPECT_EQ(
            static_cast<size_t>(regulated::basics::Primitive_0_1::SERIALIZATION_BUFFER_SIZE_BYTES), result.value());
        EXPECT_EQ(hex(ref.a_u64)   , hex(obj.a_u64) );
        EXPECT_EQ(hex(ref.a_u32)   , hex(obj.a_u32) );
        EXPECT_EQ(hex(ref.a_u16)   , hex(obj.a_u16) );
        EXPECT_EQ(hex(ref.a_u8)    , hex(obj.a_u8)  );
        EXPECT_EQ(hex(ref.a_u7)    , hex(obj.a_u7)  );
        EXPECT_EQ(hex(ref.n_u64)   , hex(obj.n_u64) );
        EXPECT_EQ(hex(ref.n_u32)   , hex(obj.n_u32) );
        EXPECT_EQ(hex(ref.n_u16)   , hex(obj.n_u16) );
        EXPECT_EQ(hex(ref.n_u8)    , hex(obj.n_u8)  );
        EXPECT_EQ(hex(ref.n_u7)    , hex(obj.n_u7)  );
        EXPECT_EQ(hex(ref.a_i64)   , hex(obj.a_i64) );
        EXPECT_EQ(hex(ref.a_i32)   , hex(obj.a_i32) );
        EXPECT_EQ(hex(ref.a_i16)   , hex(obj.a_i16) );
        EXPECT_EQ(hex(ref.a_i8)    , hex(obj.a_i8)  );
        EXPECT_EQ(hex(ref.a_i7)    , hex(obj.a_i7)  );
        EXPECT_EQ(hex(ref.n_i64)   , hex(obj.n_i64) );
        EXPECT_EQ(hex(ref.n_i32)   , hex(obj.n_i32) );
        EXPECT_EQ(hex(ref.n_i16)   , hex(obj.n_i16) );
        EXPECT_EQ(hex(ref.n_i8)    , hex(obj.n_i8)  );
        EXPECT_EQ(hex(ref.n_i7)    , hex(obj.n_i7)  );
        EXPECT_DOUBLE_EQ(ref.a_f64 , obj.a_f64 );
        EXPECT_FLOAT_EQ(ref.a_f32  , obj.a_f32 );
        EXPECT_FLOAT_EQ(ref.a_f16  , obj.a_f16 );
        EXPECT_EQ(ref.a_bool       , obj.a_bool);
        EXPECT_EQ(ref.n_bool       , obj.n_bool);
        EXPECT_DOUBLE_EQ(ref.n_f64 , obj.n_f64 );
        EXPECT_FLOAT_EQ(ref.n_f32  , obj.n_f32 );
        EXPECT_FLOAT_EQ(ref.n_f16  , obj.n_f16 );
    }
}