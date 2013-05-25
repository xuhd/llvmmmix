using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace mmixal {
    internal enum TokenType {
        DecLiteral,
        HexLiteral,
        CharLiteral,
        StringLiteral,
        SpecialRegister,
        Registration,
        LocationPtr,
        LabelRef,
        LocalBackRef,
        LocalForwRef,
        Multiply,
        Divide,
        FractDivide,
        Remainder,
        ShiftLeft,
        ShiftRight,
        BitAnd,
        Plus,
        Minus,
        BitOr,
        BitXor,
        Complementation,
        BraceOpen,
        BraceClose,
        UnaryPlus,
        UnaryMinus,
        TokEnd
    }

    internal struct Token {
        internal TokenType Type;

        internal string Value;

        public override string ToString() {
            string strType;
            switch (Type) {
                case TokenType.DecLiteral:
                    strType = "DecLiteral";
                    break;
                case TokenType.HexLiteral:
                    strType = "HexLiteral";
                    break;
                case TokenType.CharLiteral:
                    strType = "CharLiteral";
                    break;
                case TokenType.StringLiteral:
                    strType = "StringLiteral";
                    break;
                case TokenType.LabelRef:
                    strType = "LabelRef";
                    break;
                case TokenType.LocalBackRef:
                    strType = "LocalBackRef";
                    break;
                case TokenType.LocalForwRef:
                    strType = "LocalForwRef";
                    break;
                case TokenType.SpecialRegister:
                    strType = "SpecialRegister";
                    break;
                case TokenType.Registration:
                    strType = "Registration";
                    break;
                case TokenType.LocationPtr:
                    strType = "LocationPtr";
                    break;
                case TokenType.Multiply:
                    strType = "Multiply";
                    break;
                case TokenType.Divide:
                    strType = "Divide";
                    break;
                case TokenType.FractDivide:
                    strType = "FractDivide";
                    break;
                case TokenType.Remainder:
                    strType = "Remainder";
                    break;
                case TokenType.ShiftLeft:
                    strType = "ShiftLeft";
                    break;
                case TokenType.ShiftRight:
                    strType = "ShiftRight";
                    break;
                case TokenType.BitAnd:
                    strType = "BitAnd";
                    break;
                case TokenType.Plus:
                    strType = "Plus";
                    break;
                case TokenType.Minus:
                    strType = "Minus";
                    break;
                case TokenType.BitOr:
                    strType = "BitOr";
                    break;
                case TokenType.BitXor:
                    strType = "BitXor";
                    break;
                case TokenType.Complementation:
                    strType = "Complementation";
                    break;
                case TokenType.BraceOpen:
                    strType = "BraceOpen";
                    break;
                case TokenType.BraceClose:
                    strType = "BraceClose";
                    break;
                case TokenType.TokEnd:
                    strType = "TokEnd";
                    break;
                case TokenType.UnaryPlus:
                    strType = "UnaryPlus";
                    break;
                case TokenType.UnaryMinus:
                    strType = "UnaryMinus";
                    break;
                default:
                    strType = "unknown";
                    break;
            };
            return string.Format(@"[{0} ""{1}""]", strType, Value);
        }
    }

    internal class Tokenizer {
        private const string TOK_MUL = "*";

        private const string TOK_FRACT_DIV = "//";

        private const string TOK_DIV = "/";

        private const string TOK_REMAINDER = "%";

        private const string TOK_LEFT_SHIFT = "<<";

        private const string TOK_RIGHT_SHIFT = ">>";

        private const string TOK_BITAND = "&";

        private const string TOK_PLUS = "+";

        private const string TOK_MINUS = "-";

        private const string TOK_BITOR = "|";

        private const string TOK_BITXOR = "^";

        private const string TOK_COMPL = "~";

        private const string TOK_LOCATION = "@";

        private const string TOK_REGISTRATION = "$";

        private const string TOK_BRACE_OPEN = "(";

        private const string TOK_BRACE_CLOSE = ")";

        private const string TOK_STRING = "\"";

        private static Regex CHAR_PATTERN = new Regex(@"^(?<value>(?:\'[^\']\')|(?:\'\'\'\')).*?", RegexOptions.Compiled);

        private static Regex DEC_PATTERN = new Regex(@"^(?<value>\d+).*?", RegexOptions.Compiled);

        private static Regex HEX_PATTERN = new Regex(@"^(?<value>\#[0-9A-Fa-f]+).*?", RegexOptions.Compiled);

        private static Regex SPECIAL_REGISTER_PATTERN = new Regex(@"^(?<value>r[A-Z]|rBB|rTT|rWW|rXX|rYY|rZZ)"+
            @"(?:$|[^_0-9A-Za-z\u0080-\uFFFF].*)", RegexOptions.Compiled);

        private static Regex LOCAL_LABEL_REF_PATTERN = new Regex(@"^(?<value>[0-9][BF]).*?", RegexOptions.Compiled);

        private static Regex LABEL_PATTERN = new Regex(@"^(?<value>[A-Za-z_][_0-9A-Za-z\u0080-\uFFFF]*).*?", RegexOptions.Compiled);

        private int _p0;

        private readonly string _expression;

        internal Tokenizer(string expression) {
            this._expression = expression;
            reset();
        }

        internal Token getNextToken() {
            skipSpaces();
            var t = _expression.Substring(_p0);
            if (t.Length > 0) {
                if (t.StartsWith(TOK_MUL)) {
                    return makeToken(TokenType.Multiply, TOK_MUL);
                } else if (t.StartsWith(TOK_FRACT_DIV)) {
                    return makeToken(TokenType.FractDivide, TOK_FRACT_DIV);
                } else if (t.StartsWith(TOK_DIV)) {
                    return makeToken(TokenType.Divide, TOK_DIV);
                } else if (t.StartsWith(TOK_REMAINDER)) {
                    return makeToken(TokenType.Remainder, TOK_REMAINDER);
                } else if (t.StartsWith(TOK_LEFT_SHIFT)) {
                    return makeToken(TokenType.ShiftLeft, TOK_LEFT_SHIFT);
                } else if (t.StartsWith(TOK_RIGHT_SHIFT)) {
                    return makeToken(TokenType.ShiftRight, TOK_RIGHT_SHIFT);
                } else if (t.StartsWith(TOK_BITAND)) {
                    return makeToken(TokenType.BitAnd, TOK_BITAND);
                } else if (t.StartsWith(TOK_PLUS)) {
                    return makeToken(TokenType.Plus, TOK_PLUS);
                } else if (t.StartsWith(TOK_MINUS)) {
                    return makeToken(TokenType.Minus, TOK_MINUS);
                } else if (t.StartsWith(TOK_BITOR)) {
                    return makeToken(TokenType.BitOr, TOK_BITOR);
                } else if (t.StartsWith(TOK_BITXOR)) {
                    return makeToken(TokenType.BitXor, TOK_BITXOR);
                } else if (t.StartsWith(TOK_COMPL)) {
                    return makeToken(TokenType.Complementation, TOK_COMPL);
                } else if (t.StartsWith(TOK_REGISTRATION)) {
                    return makeToken(TokenType.Registration, TOK_REGISTRATION);
                } else if (t.StartsWith(TOK_BRACE_OPEN)) {
                    return makeToken(TokenType.BraceOpen, TOK_BRACE_OPEN);
                } else if (t.StartsWith(TOK_BRACE_CLOSE)) {
                    return makeToken(TokenType.BraceClose, TOK_BRACE_CLOSE);
                } else if (t.StartsWith(TOK_LOCATION)) {
                    return makeToken(TokenType.LocationPtr, TOK_LOCATION);
                } else if (t.StartsWith(TOK_STRING)) {
                    int p1 = 1;
                    char doubleQuote = TOK_STRING[0];
                    bool stringTermFound = false;
                    bool prevCharIsDoubleQuote = false;
                    while (!stringTermFound) {
                        if (p1 == t.Length)
                            if (prevCharIsDoubleQuote)
                                stringTermFound = true;
                            else
                                throw new Exception("Unexpected end of line at pos :" + _p0);
                        if (!stringTermFound) {
                            if (t[p1] == doubleQuote) {
                                prevCharIsDoubleQuote = !prevCharIsDoubleQuote;
                            } else {
                                if (prevCharIsDoubleQuote)
                                    stringTermFound = true;
                            }
                            if (!stringTermFound)
                                ++p1;
                        }
                    }
                    return makeToken(TokenType.StringLiteral, t.Substring(0, p1));
                } else {
                    var localLabelMatcher = LOCAL_LABEL_REF_PATTERN.Match(t);
                    if (localLabelMatcher.Success) {
                        var localLabel = localLabelMatcher.Groups["value"].Value;
                        if (localLabel.EndsWith("F")) {
                            return makeToken(TokenType.LocalForwRef, localLabel);
                        } else if (localLabel.EndsWith("B")) {
                            return makeToken(TokenType.LocalBackRef, localLabel);
                        } else {
                            throw new Exception("Unexpected");
                        }
                    } else {
                        var decMatcher = DEC_PATTERN.Match(t);
                        if (decMatcher.Success) {
                            var decConstant = decMatcher.Groups["value"].Value;
                            return makeToken(TokenType.DecLiteral, decConstant);
                        } else {
                            var hexMatcher = HEX_PATTERN.Match(t);
                            if (hexMatcher.Success) {
                                var hexConstant = hexMatcher.Groups["value"].Value;
                                return makeToken(TokenType.HexLiteral, hexConstant);
                            } else {
                                var specialRegisterMatcher = SPECIAL_REGISTER_PATTERN.Match(t);
                                if (specialRegisterMatcher.Success) {
                                    var specialRegisterName = specialRegisterMatcher.Groups["value"].Value;
                                    return makeToken(TokenType.SpecialRegister, specialRegisterName);
                                } else {
                                    var labelMatcher = LABEL_PATTERN.Match(t);
                                    if (labelMatcher.Success) {
                                        var labelRef = labelMatcher.Groups["value"].Value;
                                        return makeToken(TokenType.LabelRef, labelRef);
                                    } else {
                                        var charMatcher = CHAR_PATTERN.Match(t);
                                        if (charMatcher.Success) {
                                            var charLiteral = charMatcher.Groups["value"].Value;
                                            return makeToken(TokenType.CharLiteral, charLiteral);
                                        } else {
                                            throw new Exception("Can't recognize token at position " + _p0);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                return new Token { Type = TokenType.TokEnd, Value = "" };
            }
        }

        private void skipSpaces() {
            while (_p0 < _expression.Length && Char.IsWhiteSpace(_expression[_p0]))
                ++_p0;
        }

        private Token makeToken(TokenType type, string token) {
            _p0 = _p0 + token.Length;
            return new Token { Type = type, Value = token };
        }

        internal void reset() {
            _p0 = 0;
        }
    }
}
