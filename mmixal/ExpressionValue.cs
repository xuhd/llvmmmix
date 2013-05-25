using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace mmixal {
    public enum ExpressionType {
        Void = 0,
        Register = 1,
        SpecialRegister = 2,
        MmixByte = 0x100 | 3,
        MmixWyde = 0x100 | 4,
        MmixTetra = 0x100 | 5,
        MmixOcta = 0x100 | 6,
        CharLiteral = 7,
        StringLiteral = 8,
        BackRef = 9,
        ForwardRef = 10,
        GlobalRef = 11,
        MmixUByte = 0x200 | 0x100 | 3,
        MmixUWyde = 0x200 | 0x100 | 4,
        MmixUTetra = 0x200 | 0x100 | 5,
        MmixUOcta = 0x200 | 0x100 | 6
    }

    public struct ExpressionValue {
        private static readonly Dictionary<string, byte> SPECIAL_REG_TABLE =
            new Tuple<string, byte>[] {
                Tuple.Create("rA", (byte)21),
                Tuple.Create("rB", (byte)0),
                Tuple.Create("rC", (byte)8),
                Tuple.Create("rD", (byte)1),
                Tuple.Create("rE", (byte)2),
                Tuple.Create("rF", (byte)22),
                Tuple.Create("rG", (byte)19),
                Tuple.Create("rH", (byte)3),
                Tuple.Create("rI", (byte)12),
                Tuple.Create("rJ", (byte)4),
                Tuple.Create("rK", (byte)15),
                Tuple.Create("rL", (byte)20),
                Tuple.Create("rM", (byte)5),
                Tuple.Create("rN", (byte)9),
                Tuple.Create("rO", (byte)10),
                Tuple.Create("rP", (byte)23),
                Tuple.Create("rQ", (byte)16),
                Tuple.Create("rR", (byte)6),
                Tuple.Create("rS", (byte)11),
                Tuple.Create("rT", (byte)13),
                Tuple.Create("rU", (byte)17),
                Tuple.Create("rV", (byte)18),
                Tuple.Create("rW", (byte)24),
                Tuple.Create("rX", (byte)25),
                Tuple.Create("rY", (byte)26),
                Tuple.Create("rZ", (byte)27),
                Tuple.Create("rBB", (byte)7),
                Tuple.Create("rTT", (byte)14),
                Tuple.Create("rWW", (byte)28),
                Tuple.Create("rXX", (byte)29),
                Tuple.Create("rYY", (byte)30),
                Tuple.Create("rZZ", (byte)31),
            }.ToDictionary(
                (Tuple<string, byte> arg) => arg.Item1,
                (Tuple<string, byte> arg) => arg.Item2);

        private readonly ExpressionType _type;

        private readonly object _value;

        private ExpressionValue(ExpressionType type, object value) {
            this._type = type;
            this._value = value;
        }

        public ExpressionValue(ExpressionValue o) 
            : this (o._type, o._value) 
        { }

        public static ExpressionValue makeValue(sbyte arg) {
            return new ExpressionValue(ExpressionType.MmixByte, arg);
        }

        public static ExpressionValue makeValue(short arg) {
            return new ExpressionValue(ExpressionType.MmixWyde, arg);
        }

        public static ExpressionValue makeValue(int arg) {
            return new ExpressionValue(ExpressionType.MmixTetra, arg);
        }

        public static ExpressionValue makeValue(long arg) {
            return new ExpressionValue(ExpressionType.MmixOcta, arg);
        }

        public static ExpressionValue makeValue(char arg) {
            return new ExpressionValue(ExpressionType.CharLiteral, arg);
        }

        public static ExpressionValue makeValue(string arg) {
            return new ExpressionValue(ExpressionType.StringLiteral, arg);
        }

        public static ExpressionValue makeSpecialRegister(string arg) {
            return new ExpressionValue(ExpressionType.SpecialRegister, arg);
        }

        public sbyte asMmixByte() {
            switch (_type) {
                case ExpressionType.MmixByte:
                    if (_value.GetType() == typeof(sbyte))
                        return (sbyte)_value;
                    else
                        throw new Exception("Bad type coercion");
                default:
                    throw new Exception("Bad type coercion");
            }
        }

        public short asMmixWyde() {
            switch (_type) {
                case ExpressionType.MmixByte:
                    return asMmixByte();
                case ExpressionType.MmixWyde:
                    if (_value.GetType() == typeof(short))
                        return (short)_value;
                    else
                        throw new Exception("Bad type coercion");
                default:
                    throw new Exception("Bad type coercion");
            }
        }

        public int asMmixTetra() {
            switch (_type) {
                case ExpressionType.MmixByte:
                    return asMmixByte();
                case ExpressionType.MmixWyde:
                    return asMmixWyde();
                case ExpressionType.MmixTetra:
                    if (_value.GetType() == typeof(int))
                        return (int)_value;
                    else
                        throw new Exception("Bad type coercion");
                default:
                    throw new Exception("Bad type coercion");
            }
        }

        public long asMmixOcta() {
            switch (_type) {
                case ExpressionType.MmixByte:
                    return asMmixByte();
                case ExpressionType.MmixWyde:
                    return asMmixWyde();
                case ExpressionType.MmixTetra:
                    return asMmixTetra();
                case ExpressionType.MmixOcta:
                    if (_value.GetType() == typeof(long))
                        return (long)_value;
                    else
                        throw new Exception("Bad type coercion");
                default:
                    throw new Exception("Bad type coercion");
            }
        }

        public char asChar() {
            if (!IsNumeric) {
                switch (_type) {
                    case ExpressionType.CharLiteral:
                        if (_value.GetType() == typeof(char))
                            return (char)_value;
                        else
                            throw new Exception("Bad type coercion");
                    default:
                        throw new Exception("Bad type coercion");
                }
            } else {
                return (char)asMmixOcta();
            }
        }

        public long asRegisterNum() {
            if (_type == ExpressionType.Register)
                if (_value.GetType() == typeof(byte))
                    return (byte)_value;
                else
                    throw new Exception("Bad type coercion");
            if (_type == ExpressionType.SpecialRegister)
                if (_value.GetType() == typeof(string))
                    return SPECIAL_REG_TABLE[(string)_value];
                else
                    throw new Exception("Bad type coercion");
            else
                throw new Exception("Bad type coercion");
        }

        public string asGlobalRef() {
            if (_type == ExpressionType.GlobalRef)
                if (_value.GetType() == typeof(string))
                    return (string)_value;
                else
                    throw new Exception("Bad type coercion");
            else
                throw new Exception("Bad type coercion");
        }

        public static ExpressionValue makeRegister(long arg) {
            if (arg >= 0 && arg <= byte.MaxValue)
                return new ExpressionValue(ExpressionType.Register, (byte)(arg & byte.MaxValue));
            else 
                throw new Exception("Register index out of range :" + arg);
        }

        public static ExpressionValue makeVaribleRef(string arg) {
            return new ExpressionValue(ExpressionType.GlobalRef, arg);
        }

        public ExpressionType Type {
            get { return _type; }
        }

        public bool IsNumeric {
            get { return ((int)_type & 0x100) != 0; }
        }

        public bool IsUnsigned {
            get { return ((int)_type & 0x200) != 0; }
        }

        public string ToStringShort() {
            return _value.ToString();
        }

        public override string ToString() {
            string strType;
            switch (_type) {
                case ExpressionType.Void:
                    strType = "Void";
                    break;
                case ExpressionType.Register:
                    strType = "Register";
                    break;
                case ExpressionType.SpecialRegister:
                    strType = "SpecialRegister";
                    break;
                case ExpressionType.MmixByte:
                    strType = "MmixByte";
                    break;
                case ExpressionType.MmixWyde:
                    strType = "MmixWyde";
                    break;
                case ExpressionType.MmixTetra:
                    strType = "MmixTetra";
                    break;
                case ExpressionType.MmixOcta:
                    strType = "MmixOcta";
                    break;
                case ExpressionType.CharLiteral:
                    strType = "CharLiteral";
                    break;
                case ExpressionType.StringLiteral:
                    strType = "StringLiteral";
                    break;
                case ExpressionType.BackRef:
                    strType = "BackRef";
                    break;
                case ExpressionType.ForwardRef:
                    strType = "ForwardRef";
                    break;
                case ExpressionType.GlobalRef:
                    strType = "GlobalRef";
                    break;
                case ExpressionType.MmixUByte:
                    strType = "MmixUByte";
                    break;
                case ExpressionType.MmixUWyde:
                    strType = "MmixUWyde";
                    break;
                case ExpressionType.MmixUTetra:
                    strType = "MmixUTetra";
                    break;
                case ExpressionType.MmixUOcta:
                    strType = "MmixUOcta";
                    break;
                default:
                    strType = "unknown";
                    break;
            }
            return string.Format("[{0} {1}]", strType, ToStringShort());
        }
    }
}
