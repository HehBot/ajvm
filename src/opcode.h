#ifndef OPCODE_H
#define OPCODE_H

// expansion macro for enum value definition
#define ENUM_VALUE(name, assign) name assign,
// expansion macro for enum to string conversion
#define ENUM_CASE(name, assign) \
case name:                      \
    return #name;
// declare the access function and define enum values
#define DECLARE_ENUM(EnumType, ENUM_DEF) \
    enum EnumType {                      \
        ENUM_DEF(ENUM_VALUE)             \
    };
// define the access function names
#define DEFINE_ENUM_STRINGER(EnumType, ENUM_DEF)              \
    static inline char const* get_string(enum EnumType value) \
    {                                                         \
        switch (value) {                                      \
            ENUM_DEF(ENUM_CASE)                               \
        default:                                              \
            return ""; /* handle input error */               \
        }                                                     \
    }
#define OPCODE_ENUM(XX)       \
    XX(NOP, = 0x00)           \
    XX(ACONST_NULL, = 0x01)   \
    XX(ICONST_M1, = 0x02)     \
    XX(ICONST_0, )            \
    XX(ICONST_1, )            \
    XX(ICONST_2, )            \
    XX(ICONST_3, )            \
    XX(ICONST_4, )            \
    XX(ICONST_5, )            \
    XX(LCONST_0, )            \
    XX(LCONST_1, )            \
    XX(FCONST_0, )            \
    XX(FCONST_1, )            \
    XX(FCONST_2, )            \
    XX(DCONST_0, )            \
    XX(DCONST_1, )            \
    XX(BIPUSH, = 0x10)        \
    XX(SIPUSH, = 0x11)        \
    XX(LDC, = 0x12)           \
    XX(LDC_W, = 0x13)         \
    XX(LDC2_W, = 0x14)        \
    XX(ILOAD, = 0x15)         \
    XX(LLOAD, )               \
    XX(FLOAD, )               \
    XX(DLOAD, )               \
    XX(ALOAD, )               \
    XX(ILOAD_0, )             \
    XX(ILOAD_1, )             \
    XX(ILOAD_2, )             \
    XX(ILOAD_3, )             \
    XX(LLOAD_0, )             \
    XX(LLOAD_1, )             \
    XX(LLOAD_2, )             \
    XX(LLOAD_3, )             \
    XX(FLOAD_0, )             \
    XX(FLOAD_1, )             \
    XX(FLOAD_2, )             \
    XX(FLOAD_3, )             \
    XX(DLOAD_0, )             \
    XX(DLOAD_1, )             \
    XX(DLOAD_2, )             \
    XX(DLOAD_3, )             \
    XX(ALOAD_0, )             \
    XX(ALOAD_1, )             \
    XX(ALOAD_2, )             \
    XX(ALOAD_3, )             \
    XX(ISTORE, = 0x36)        \
    XX(LSTORE, )              \
    XX(FSTORE, )              \
    XX(DSTORE, )              \
    XX(ASTORE, )              \
    XX(ISTORE_0, )            \
    XX(ISTORE_1, )            \
    XX(ISTORE_2, )            \
    XX(ISTORE_3, )            \
    XX(LSTORE_0, )            \
    XX(LSTORE_1, )            \
    XX(LSTORE_2, )            \
    XX(LSTORE_3, )            \
    XX(FSTORE_0, )            \
    XX(FSTORE_1, )            \
    XX(FSTORE_2, )            \
    XX(FSTORE_3, )            \
    XX(DSTORE_0, )            \
    XX(DSTORE_1, )            \
    XX(DSTORE_2, )            \
    XX(DSTORE_3, )            \
    XX(ASTORE_0, )            \
    XX(ASTORE_1, )            \
    XX(ASTORE_2, )            \
    XX(ASTORE_3, )            \
    XX(POP, = 0x57)           \
    XX(DUP, = 0x59)           \
    XX(SWAP, = 0x5f)          \
    XX(IADD, = 0x60)          \
    XX(LADD, )                \
    XX(FADD, )                \
    XX(DADD, )                \
    XX(ISUB, )                \
    XX(LSUB, )                \
    XX(FSUB, )                \
    XX(DSUB, )                \
    XX(IMUL, )                \
    XX(LMUL, )                \
    XX(FMUL, )                \
    XX(DMUL, )                \
    XX(IDIV, )                \
    XX(LDIV, )                \
    XX(FDIV, )                \
    XX(DDIV, )                \
    XX(IREM, )                \
    XX(LREM, )                \
    XX(FREM, )                \
    XX(DREM, )                \
    XX(INEG, )                \
    XX(LNEG, )                \
    XX(FNEG, )                \
    XX(DNEG, )                \
    XX(ISHL, )                \
    XX(LSHL, )                \
    XX(ISHR, )                \
    XX(LSHR, )                \
    XX(IUSHR, )               \
    XX(LUSHR, )               \
    XX(IAND, )                \
    XX(LAND, )                \
    XX(IOR, )                 \
    XX(LOR, )                 \
    XX(IXOR, )                \
    XX(LXOR, )                \
    XX(I2L, )                 \
    XX(I2F, )                 \
    XX(I2D, )                 \
    XX(L2D, )                 \
    XX(L2I, )                 \
    XX(L2F, )                 \
    XX(F2I, )                 \
    XX(F2L, )                 \
    XX(F2D, )                 \
    XX(D2I, )                 \
    XX(D2L, )                 \
    XX(D2F, )                 \
    XX(I2B, )                 \
    XX(I2C, )                 \
    XX(I2S, )                 \
    XX(IRETURN, = 0xac)       \
    XX(LRETURN, )             \
    XX(FRETURN, )             \
    XX(DRETURN, )             \
    XX(ARETURN, )             \
    XX(RETURN, )              \
    XX(GETSTATIC, = 0xb2)     \
    XX(PUTSTATIC, )           \
    XX(GETFIELD, )            \
    XX(PUTFIELD, )            \
    XX(INVOKEVIRTUAL, = 0xb6) \
    XX(INVOKESPECIAL, )       \
    XX(INVOKESTATIC, )        \
    XX(NEW, = 0xbb)
DECLARE_ENUM(opcode, OPCODE_ENUM)
DEFINE_ENUM_STRINGER(opcode, OPCODE_ENUM)

#endif // OPCODE_H
