using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Globalization;
using System.Numerics;

namespace mmixal {
    public class ArgsList : List<string> { }

    public class SymbolBindings : Dictionary<string, ExpressionValue> { }

    public interface Expression {
        ArgsList getArguments();

        ExpressionValue eval(SymbolBindings args);
    }

    public static class ExpressionBuilder {
        private static int _uniqSeed = 0;

        public static Expression makePointer(long arg) {
            return new LiteralNode(ExpressionValue.makeValue(arg));
        }

        public static Expression makeSegmentTopRef() {
            string varName = "@@" + (++_uniqSeed);
            return new CommonExpression(new ArgsList { varName }, 
                new ArgumentRefNode(ExpressionValue.makeVaribleRef(varName)));
        }

        public static Expression buildExpression(string arg) {
            var argsList = new ArgsList();
            var expr =
                buildExpression0(new Tokenizer(arg), false,
                    new LinkedList<Expression>(), new LinkedList<Token>(),
                    argsList, 0);
            return new CommonExpression(argsList, expr);
        }

        private static Expression buildExpression0(Tokenizer tokenizer,
            bool prevTokenIsOperand, LinkedList<Expression> operandDeque,
            LinkedList<Token> operatorDeque, 
            ArgsList argsList, int nestingLevel) 
        {
            var token = tokenizer.getNextToken();
            if (!tokenIsTerminator(token, nestingLevel)) {
                bool tokenIsOperand;
                if (tokenIsOperator(token)) {
                    tokenIsOperand = false;
                    if (prevTokenIsOperand) {
                        operatorDeque.AddLast(token);
                    } else {
                        operandDeque.AddLast(new FakeLeftOperandForUnaryOp());
                        operatorDeque.AddLast(makeTokenForUnaryOperator(token));
                    }
                } else {
                    tokenIsOperand = true;
                    var operand = 
                        token.Type != TokenType.BraceOpen
                            ? makeSimpleOperand(token, argsList)
                            : buildExpression0(tokenizer, false,
                                new LinkedList<Expression>(), new LinkedList<Token>(),
                                argsList, nestingLevel + 1);
                    operandDeque.AddLast(operand);
                    reduce(operandDeque, operatorDeque);
                }
                return buildExpression0(tokenizer, tokenIsOperand, 
                    operandDeque, operatorDeque, argsList, nestingLevel);
            } else {
                while (operatorDeque.Last != null
                       && operatorDeque.Last.Previous != null)
                    reduce(operandDeque, operatorDeque);
                if (operatorDeque.Last != null)
                    reduceRight(operandDeque, operatorDeque);
                return operandDeque.First.Value;
            }
        }

        private static void reduce(LinkedList<Expression> operandDeque, LinkedList<Token> operatorDeque)
        {
            var lastOperatorNode = operatorDeque.Last;
            if (lastOperatorNode != null) {
                var precedingOperatorNode = lastOperatorNode.Previous;
                if (precedingOperatorNode != null) {
                    int precedence = getOperatorPrecedence(lastOperatorNode.Value);
                    int prevPrecedence = getOperatorPrecedence(precedingOperatorNode.Value);
                    if (precedence < prevPrecedence)
                        reduceRight(operandDeque, operatorDeque);
                    else
                        reduceLeft(operandDeque, operatorDeque);
                }
            }
        }

        private static void reduceRight(LinkedList<Expression> operandDeque,
            LinkedList<Token> operatorDeque) 
        {
            var rho = operandDeque.Last;
            var lho = rho.Previous;
            var optoken = operatorDeque.Last;
            operandDeque.Remove(rho);
            operandDeque.Remove(lho);
            operatorDeque.Remove(optoken);
            operandDeque.AddLast(makeExpression(optoken.Value, lho.Value, rho.Value));
        }

        private static void reduceLeft(LinkedList<Expression> operandDeque,
            LinkedList<Token> operatorDeque) 
        {
            var optoken = operatorDeque.Last;
            var rho = operandDeque.Last;
            operatorDeque.Remove(optoken);
            operandDeque.Remove(rho);
            if (operandDeque.Last != null) {
                reduceRight(operandDeque, operatorDeque);
                operatorDeque.AddLast(optoken);
                operandDeque.AddLast(rho);
            } else {
                var lho = operandDeque.Last;
                operandDeque.Remove(lho);
                operandDeque.AddLast(makeExpression(optoken.Value, lho.Value, rho.Value));
            }
        }

        private static Token makeTokenForUnaryOperator(Token token) {
            Token newOperator;
            switch (token.Type) {
                case TokenType.Plus:
                    newOperator = new Token { Type = TokenType.UnaryPlus, Value = "+" };
                    break;
                case TokenType.Minus:
                    newOperator = new Token { Type = TokenType.UnaryMinus, Value = "-" };
                    break;
                case TokenType.Complementation:
                    newOperator = token;
                    break;
                case TokenType.Registration:
                    newOperator = token;
                    break;
                default:
                    throw new Exception("Unexpected token :" + token);
            };
            return newOperator;
        }

        private static Expression makeExpression(Token operatorToken, Expression lho, Expression rho) {
            switch (operatorToken.Type) {
                case TokenType.Multiply:
                    return new MultiplyNode(lho, rho);
                case TokenType.Divide:
                    return new DivideNode(lho, rho);
                case TokenType.FractDivide:
                    return new FractDivideNode(lho, rho);
                case TokenType.Remainder:
                    return new FractDivideNode(lho, rho);
                case TokenType.ShiftLeft:
                    return new ShiftLeftNode(lho, rho);
                case TokenType.ShiftRight:
                    return new ShiftRightNode(lho, rho);
                case TokenType.BitAnd:
                    return new BitAndNode(lho, rho);
                case TokenType.Plus:
                    return new PlusNode(lho, rho);
                case TokenType.Minus:
                    return new MinusNode(lho, rho);
                case TokenType.BitOr:
                    return new BitOrNode(lho, rho);
                case TokenType.BitXor:
                    return new BitXorNode(lho, rho);
                case TokenType.UnaryPlus:
                    return new UnaryPlusNode(rho);
                case TokenType.UnaryMinus:
                    return new UnaryMinusNode(rho);
                case TokenType.Complementation:
                    return new ComplementationNode(rho);
                case TokenType.Registration:
                    return new RegistrationNode(rho);
                default:
                    throw new Exception("Unexpected token :" + operatorToken);
            }
        }

        private static int getOperatorPrecedence(Token token) {
            switch (token.Type) {
                case TokenType.Multiply:
                    return 1;
                case TokenType.Divide:
                    return 1;
                case TokenType.FractDivide:
                    return 1;
                case TokenType.Remainder:
                    return 1;
                case TokenType.ShiftLeft:
                    return 1;
                case TokenType.ShiftRight:
                    return 1;
                case TokenType.BitAnd:
                    return 1;
                case TokenType.Plus:
                    return 2;
                case TokenType.Minus:
                    return 2;
                case TokenType.BitOr:
                    return 2;
                case TokenType.BitXor:
                    return 2;
                case TokenType.UnaryPlus:
                    return 0;
                case TokenType.UnaryMinus:
                    return 0;
                case TokenType.Complementation:
                    return 0;
                case TokenType.Registration:
                    return 0;
                default:
                    throw new Exception("Unexpected token :" + token);
            }
        }

        private static Expression makeSimpleOperand(Token token, ArgsList argsList) {
            switch (token.Type) {
                case TokenType.DecLiteral:
                    return new LiteralNode(ExpressionValue.makeValue(long.Parse(token.Value)));
                case TokenType.HexLiteral:
                    return new LiteralNode(ExpressionValue.makeValue(
                        long.Parse(token.Value.Substring(1), NumberStyles.HexNumber)));
                case TokenType.CharLiteral:
                    return new LiteralNode(ExpressionValue.makeValue(
                        token.Value.Substring(1, token.Value.Length - 2)[0]));
                case TokenType.StringLiteral:
                    return new LiteralNode(ExpressionValue.makeValue(
                        token.Value.Substring(1, token.Value.Length - 2)));
                case TokenType.SpecialRegister:
                    return new LiteralNode(ExpressionValue.makeSpecialRegister(token.Value));
                case TokenType.LocationPtr:
                    if (!argsList.Contains(token.Value))
                        argsList.Add(token.Value);
                    return new ArgumentRefNode(ExpressionValue.makeVaribleRef(token.Value));
                case TokenType.LocalBackRef: {
                        var refName = "@" + token.Value + "_" + (++_uniqSeed);
                        if (!argsList.Contains(token.Value))
                            argsList.Add(refName);
                        return new ArgumentRefNode(ExpressionValue.makeVaribleRef(refName));
                    }
                case TokenType.LocalForwRef: {
                        var refName = "@" + token.Value + "_" + (++_uniqSeed);
                        if (!argsList.Contains(token.Value))
                            argsList.Add(refName);
                        return new ArgumentRefNode(ExpressionValue.makeVaribleRef(refName));
                    }
                case TokenType.LabelRef:
                    if (!argsList.Contains(token.Value)) {
                        argsList.Add(token.Value);
                    }
                    return new ArgumentRefNode(ExpressionValue.makeVaribleRef(token.Value));
                default:
                    throw new Exception("Can't make operand from token " + token);
            }
        }

        private static bool tokenIsOperator(Token token) {
            bool retVal;
            switch (token.Type) {
                case TokenType.DecLiteral:
                case TokenType.HexLiteral:
                case TokenType.CharLiteral:
                case TokenType.StringLiteral:
                case TokenType.LabelRef:
                case TokenType.LocationPtr:
                case TokenType.LocalBackRef:
                case TokenType.LocalForwRef:
                case TokenType.SpecialRegister:
                case TokenType.BraceOpen:
                    retVal = false;
                    break;
                default:
                    retVal = true;
                    break;
            }
            return retVal;
        }

        private static bool tokenIsTerminator(Token token, int nestingLevel) {
            return token.Type ==
                (nestingLevel > 0
                    ? TokenType.BraceClose
                    : TokenType.TokEnd);
        }

        internal class LiteralNode : Expression {
            private readonly ExpressionValue _value;

            internal LiteralNode(ExpressionValue labelValue) {
                this._value = labelValue;
            }

            public ArgsList getArguments() {
                return null;
            }

            public ExpressionValue eval(SymbolBindings args) {
                return _value;
            }

            public override string ToString() {
                return _value.ToStringShort();
            }
        }

        internal class ArgumentRefNode : Expression {
            private readonly ExpressionValue _argumentRef;

            internal ArgumentRefNode(ExpressionValue labelRef) {
                this._argumentRef = labelRef;
            }

            public ArgsList getArguments() {
                return null;
            }

            public ExpressionValue eval(SymbolBindings args) {
                string argRef = _argumentRef.asGlobalRef();
                if (args.ContainsKey(argRef))
                    return args[argRef];
                else
                    throw new Exception("Unresolved label reference :" + _argumentRef);
            }

            public override string ToString() {
                return _argumentRef.asGlobalRef();
            }
        }

        internal class CommonExpression : Expression {
            private readonly ArgsList _argsList;

            private readonly Expression _expression;

            internal CommonExpression(ArgsList dependencyList,
                Expression nestedExpression) {
                this._argsList = dependencyList;
                this._expression = nestedExpression;
            }

            public ArgsList getArguments() {
                return _argsList;
            }

            public ExpressionValue eval(SymbolBindings arguments) {
                return _expression.eval(arguments);
            }

            public override string ToString() {
                return string.Format("(lambda ({0}) {1})", string.Join(" ", _argsList), _expression);
            }
        }

        internal class FakeLeftOperandForUnaryOp : Expression {
            internal FakeLeftOperandForUnaryOp() { }

            public ArgsList getArguments() {
                throw new Exception("FakeLeftOperandForUnaryOp is about to be evaluated");
            }

            public ExpressionValue eval(SymbolBindings arguments) {
                throw new Exception("FakeLeftOperandForUnaryOp is about to be evaluated");
            }
        }

        internal abstract class CommonOp : Expression {
            public ArgsList getArguments() {
                return null;
            }

            public abstract ExpressionValue eval(SymbolBindings args);
        }

        internal abstract class CommonBinaryOp : CommonOp {
            private readonly Expression _lho;

            private readonly Expression _rho;

            internal CommonBinaryOp(Expression lho, Expression rho) {
                this._lho = lho;
                this._rho = rho;
            }

            internal Expression Lho {
                get { return _lho;  }
            }

            internal Expression Rho {
                get { return _rho; }
            }

            internal static ExpressionType estimateResultType(ExpressionValue lht, ExpressionValue rht) {
                if (lht.IsNumeric == rht.IsNumeric)
                    if (lht.IsUnsigned == rht.IsUnsigned)
                        return lht.Type == rht.Type
                                ? lht.Type
                                : new ExpressionType[] { lht.Type, rht.Type }
                                    .OrderByDescending(typeWidthRanker)
                                    .Take(1).Single();
                    else
                        throw new Exception("Unsigned / signed mismatch");
                else
                    throw new Exception("At least one of arguments is a non-numeric value");
            }

            private static int typeWidthRanker(ExpressionType arg) {
                switch (arg) {
                    case ExpressionType.MmixByte:
                        return 0;
                    case ExpressionType.MmixWyde:
                        return 1;
                    case ExpressionType.MmixTetra:
                        return 2;
                    case ExpressionType.MmixOcta:
                        return 3;
                    case ExpressionType.MmixUByte:
                        return 0;
                    case ExpressionType.MmixUWyde:
                        return 1;
                    case ExpressionType.MmixUTetra:
                        return 2;
                    case ExpressionType.MmixUOcta:
                        return 3;
                    default:
                        throw new Exception("Binary operators is only allowed on pure numbers");
                }
            }
        }

        internal abstract class CommonUnaryOp : CommonOp {
            private readonly Expression _rho;

            internal CommonUnaryOp (Expression rho) {
                this._rho = rho;
            }

            internal Expression Rho {
                get { return _rho; }
            }
        }

        internal class MultiplyNode : CommonBinaryOp {
            public MultiplyNode(Expression lho, Expression rho) 
                : base (lho, rho)
            { }

            public override ExpressionValue eval(SymbolBindings args) {
                var lho = Lho.eval(args);
                var rho = Rho.eval(args);
                var resultType = estimateResultType(lho, rho);
                switch (resultType) {
                    case ExpressionType.MmixByte:
                        return ExpressionValue.makeValue((sbyte)(lho.asMmixByte() * rho.asMmixByte())); 
                    case ExpressionType.MmixWyde:
                        return ExpressionValue.makeValue((short)(lho.asMmixWyde() * rho.asMmixWyde()));
                    case ExpressionType.MmixTetra:
                        return ExpressionValue.makeValue(lho.asMmixTetra() * rho.asMmixTetra());
                    case ExpressionType.MmixOcta:
                        return ExpressionValue.makeValue(lho.asMmixOcta() * rho.asMmixOcta());
                    default:
                        throw new Exception(string.Format("Can't apply the operation * for operands {0} and {1}", lho, rho));
                }
            }            

            public override string ToString() {
                return string.Format("(* {0} {1})", Lho.ToString(), Rho.ToString());
            }
        }

        internal class DivideNode : CommonBinaryOp {
            public DivideNode(Expression lho, Expression rho) 
                : base (lho, rho)
            { }

            public override ExpressionValue eval(SymbolBindings args) {
                var lho = Lho.eval(args);
                var rho = Rho.eval(args);
                var resultType = estimateResultType(lho, rho);
                switch (resultType) {
                    case ExpressionType.MmixByte:
                        return ExpressionValue.makeValue((sbyte)(lho.asMmixByte() / rho.asMmixByte()));
                    case ExpressionType.MmixWyde:
                        return ExpressionValue.makeValue((short)(lho.asMmixWyde() / rho.asMmixWyde()));
                    case ExpressionType.MmixTetra:
                        return ExpressionValue.makeValue(lho.asMmixTetra() / rho.asMmixTetra());
                    case ExpressionType.MmixOcta:
                        return ExpressionValue.makeValue(lho.asMmixOcta() / rho.asMmixOcta());
                    default:
                        throw new Exception(string.Format("Can't apply the operation / for operands {0} and {1}", lho, rho));
                }
            }

            public override string ToString() {
                return string.Format("(/ {0} {1})", Lho.ToString(), Rho.ToString());
            }
        }

        internal class FractDivideNode : CommonBinaryOp {
            public FractDivideNode(Expression lho, Expression rho) 
                : base (lho, rho) 
            { }

            public override ExpressionValue eval(SymbolBindings args) {
                var lho = Lho.eval(args);
                var rho = Rho.eval(args);
                var resultType = estimateResultType(lho, rho);
                BigInteger bi;
                switch (resultType) {
                    case ExpressionType.MmixByte:
                        bi = new BigInteger(lho.asMmixByte());
                        bi <<= 64;
                        bi /= rho.asMmixByte();
                        return ExpressionValue.makeValue(long.Parse(bi.ToString()));
                    case ExpressionType.MmixWyde:
                        bi = new BigInteger(lho.asMmixWyde());
                        bi <<= 64;
                        bi /= rho.asMmixWyde();
                        return ExpressionValue.makeValue(long.Parse(bi.ToString()));
                    case ExpressionType.MmixTetra:
                        bi = new BigInteger(lho.asMmixTetra());
                        bi <<= 64;
                        bi /= rho.asMmixTetra();
                        return ExpressionValue.makeValue(long.Parse(bi.ToString()));
                    case ExpressionType.MmixOcta:
                        bi = new BigInteger(lho.asMmixOcta());
                        bi <<= 64;
                        bi /= rho.asMmixOcta();
                        return ExpressionValue.makeValue(long.Parse(bi.ToString()));
                    default:
                        throw new Exception(string.Format("Can't apply the operation // for operands {0} and {1}", lho, rho));
                }
            }

            public override string ToString() {
                return string.Format("(// {0} {1})", Lho.ToString(), Rho.ToString());
            }
        }

        internal class RemainderNode : CommonBinaryOp {
            public RemainderNode(Expression lho, Expression rho) 
                : base (lho, rho)
            { }

            public override ExpressionValue eval(SymbolBindings args) {
                var lho = Lho.eval(args);
                var rho = Rho.eval(args);
                var resultType = estimateResultType(lho, rho);
                switch (resultType) {
                    case ExpressionType.MmixByte:
                        return ExpressionValue.makeValue((sbyte)(lho.asMmixByte() % rho.asMmixByte()));
                    case ExpressionType.MmixWyde:
                        return ExpressionValue.makeValue((short)(lho.asMmixWyde() % rho.asMmixWyde()));
                    case ExpressionType.MmixTetra:
                        return ExpressionValue.makeValue(lho.asMmixTetra() % rho.asMmixTetra());
                    case ExpressionType.MmixOcta:
                        return ExpressionValue.makeValue(lho.asMmixOcta() % rho.asMmixOcta());
                    default:
                        throw new Exception(string.Format("Can't apply the operation % for operands {0} and {1}", lho, rho));
                }
            }

            public override string ToString() {
                return string.Format("(% {0} {1})", Lho.ToString(), Rho.ToString());
            }
        }

        internal class ShiftLeftNode : CommonBinaryOp {
            public ShiftLeftNode(Expression lho, Expression rho) 
                : base (lho, rho) 
            { }

            public override ExpressionValue eval(SymbolBindings args) {
                var lho = Lho.eval(args);
                var rho = Rho.eval(args);
                var resultType = estimateResultType(lho, rho);
                switch (resultType) {
                    case ExpressionType.MmixByte:
                        return ExpressionValue.makeValue((sbyte) (lho.asMmixByte() << rho.asMmixByte()));
                    case ExpressionType.MmixWyde:
                        return ExpressionValue.makeValue((short)(lho.asMmixWyde() << rho.asMmixWyde()));
                    case ExpressionType.MmixTetra:
                        return ExpressionValue.makeValue(lho.asMmixTetra() << rho.asMmixTetra());
                    case ExpressionType.MmixOcta:
                        return ExpressionValue.makeValue(lho.asMmixOcta() << (int)(rho.asMmixOcta() & uint.MaxValue));
                    default:
                        throw new Exception(string.Format("Can't apply the operation << for operands {0} and {1}", lho, rho));
                }
            }


            public override string ToString() {
                return string.Format("(<< {0} {1})", Lho.ToString(), Rho.ToString());
            }
        }

        internal class ShiftRightNode : CommonBinaryOp {
            public ShiftRightNode(Expression lho, Expression rho)
                : base (lho, rho) 
            { }

            public override ExpressionValue eval(SymbolBindings args) {
                var lho = Lho.eval(args);
                var rho = Rho.eval(args);
                var resultType = estimateResultType(lho, rho);
                switch (resultType) {
                    case ExpressionType.MmixByte:
                        return ExpressionValue.makeValue((sbyte) (lho.asMmixByte() >> rho.asMmixByte()));
                    case ExpressionType.MmixWyde:
                        return ExpressionValue.makeValue((short)(lho.asMmixWyde() >> rho.asMmixWyde()));
                    case ExpressionType.MmixTetra:
                        return ExpressionValue.makeValue(lho.asMmixTetra() >> rho.asMmixTetra());
                    case ExpressionType.MmixOcta:
                        return ExpressionValue.makeValue(lho.asMmixOcta() >> (int)(rho.asMmixOcta() & uint.MaxValue));
                    default:
                        throw new Exception(string.Format("Can't apply the operation >> for operands {0} and {1}", lho, rho));
                }
            }

            public override string ToString() {
                return string.Format("(>> {0} {1})", Lho.ToString(), Rho.ToString());
            }
        }

        internal class BitAndNode : CommonBinaryOp {
            public BitAndNode(Expression lho, Expression rho) 
                : base (lho, rho)
            { }

            public override ExpressionValue eval(SymbolBindings args) {
                var lho = Lho.eval(args);
                var rho = Rho.eval(args);
                var resultType = estimateResultType(lho, rho);
                switch (resultType) {
                    case ExpressionType.MmixByte:
                        return ExpressionValue.makeValue((sbyte) (lho.asMmixByte() & rho.asMmixByte()));
                    case ExpressionType.MmixWyde:
                        return ExpressionValue.makeValue((short)(lho.asMmixWyde() & rho.asMmixWyde()));
                    case ExpressionType.MmixTetra:
                        return ExpressionValue.makeValue(lho.asMmixTetra() & rho.asMmixTetra());
                    case ExpressionType.MmixOcta:
                        return ExpressionValue.makeValue(lho.asMmixOcta() & rho.asMmixOcta());
                    default:
                        throw new Exception(string.Format("Can't apply the operation & for operands {0} and {1}", lho, rho));
                }
            }

            public override string ToString() {
                return string.Format("(& {0} {1})", Lho.ToString(), Rho.ToString());
            }
        }

        internal class PlusNode : CommonBinaryOp {
            public PlusNode(Expression lho, Expression rho) 
                : base (lho, rho) 
            { }

            public override ExpressionValue eval(SymbolBindings args) {
                var lho = Lho.eval(args);
                var rho = Rho.eval(args);
                if (lho.Type != ExpressionType.Register) {
                    var resultType = estimateResultType(lho, rho);
                    switch (resultType) {
                        case ExpressionType.MmixByte:
                            return ExpressionValue.makeValue((sbyte) (lho.asMmixByte() + rho.asMmixByte()));
                        case ExpressionType.MmixWyde:
                            return ExpressionValue.makeValue((short)(lho.asMmixWyde() + rho.asMmixWyde()));
                        case ExpressionType.MmixTetra:
                            return ExpressionValue.makeValue(lho.asMmixTetra() + rho.asMmixTetra());
                        case ExpressionType.MmixOcta:
                            return ExpressionValue.makeValue(lho.asMmixOcta() + rho.asMmixOcta());
                        default:
                            throw new Exception(string.Format("Can't apply the operation + for operands {0} and {1}", lho, rho));
                    }
                } else {
                    switch (rho.Type) {
                        case ExpressionType.MmixByte:
                            return ExpressionValue.makeRegister(lho.asMmixOcta() + rho.asMmixByte());
                        case ExpressionType.MmixWyde:
                            return ExpressionValue.makeRegister(lho.asMmixByte() + rho.asMmixWyde());
                        case ExpressionType.MmixTetra:
                            return ExpressionValue.makeRegister(lho.asMmixOcta() + rho.asMmixTetra());
                        case ExpressionType.MmixOcta:
                            return ExpressionValue.makeRegister(lho.asMmixOcta() + rho.asMmixOcta());
                        default:
                            throw new Exception(string.Format("Can't apply the operation + for operands {0} and {1}", lho, rho));
                    }
                }
            }

            public override string ToString() {
                return string.Format("(+ {0} {1})", Lho.ToString(), Rho.ToString());
            }
        }

        internal class MinusNode : CommonBinaryOp {
            public MinusNode(Expression leftHandOperand, Expression rightHandOperand) 
                : base (leftHandOperand, rightHandOperand)
            { }

            public override ExpressionValue eval(SymbolBindings args) {
                var lho = Lho.eval(args);
                var rho = Rho.eval(args);
                if (lho.Type != ExpressionType.Register) {
                    var resultType = estimateResultType(lho, rho);
                    switch (resultType) {
                        case ExpressionType.MmixByte:
                            return ExpressionValue.makeValue((sbyte) (lho.asMmixByte() - rho.asMmixByte()));
                        case ExpressionType.MmixWyde:
                            return ExpressionValue.makeValue((short)(lho.asMmixWyde() - rho.asMmixWyde()));
                        case ExpressionType.MmixTetra:
                            return ExpressionValue.makeValue(lho.asMmixTetra() - rho.asMmixTetra());
                        case ExpressionType.MmixOcta:
                            return ExpressionValue.makeValue(lho.asMmixOcta() - rho.asMmixOcta());
                        default:
                            throw new Exception(string.Format("Can't apply the operation - for operands {0} and {1}", lho, rho));
                    }
                } else {
                    if (rho.Type == ExpressionType.Register)
                        return ExpressionValue.makeValue(rho.asRegisterNum() - rho.asRegisterNum());
                    else
                        throw new Exception(string.Format("Can't apply the operation - for operands {0} and {1}", lho, rho));
                }
            }

            public override string ToString() {
                return string.Format("(- {0} {1})", Lho.ToString(), Rho.ToString());
            }
        }

        internal class BitOrNode : CommonBinaryOp {
            public BitOrNode(Expression lho, Expression rho) 
                : base (lho, rho)
            { }

            public override ExpressionValue eval(SymbolBindings args) {
                var lho = Lho.eval(args);
                var rho = Rho.eval(args);
                var resultType = estimateResultType(lho, rho);
                switch (resultType) {
                    case ExpressionType.MmixByte:
                        return ExpressionValue.makeValue((sbyte)(lho.asMmixByte() | rho.asMmixByte()));
                    case ExpressionType.MmixWyde:
                        return ExpressionValue.makeValue((short)(lho.asMmixWyde() | rho.asMmixWyde()));
                    case ExpressionType.MmixTetra:
                        return ExpressionValue.makeValue(lho.asMmixTetra() | rho.asMmixTetra());
                    case ExpressionType.MmixOcta:
                        return ExpressionValue.makeValue(lho.asMmixOcta() | rho.asMmixOcta());
                    default:
                        throw new Exception(string.Format("Can't apply the operation | for operands {0} and {1}", lho, rho));
                }
            }

            public override string ToString() {
                return string.Format("(| {0} {1})", Lho.ToString(), Rho.ToString());
            }
        }

        internal class BitXorNode : CommonBinaryOp {
            public BitXorNode(Expression lho, Expression rho) 
                : base (lho, rho) 
            { }

            public override ExpressionValue eval(SymbolBindings args) {
                var lho = Lho.eval(args);
                var rho = Rho.eval(args);
                var resultType = estimateResultType(lho, rho);
                switch (resultType) {
                    case ExpressionType.MmixByte:
                        return ExpressionValue.makeValue((sbyte)(lho.asMmixByte() ^ rho.asMmixByte()));
                    case ExpressionType.MmixWyde:
                        return ExpressionValue.makeValue((short)(lho.asMmixWyde() ^ rho.asMmixWyde()));
                    case ExpressionType.MmixTetra:
                        return ExpressionValue.makeValue(lho.asMmixTetra() ^ rho.asMmixTetra());
                    case ExpressionType.MmixOcta:
                        return ExpressionValue.makeValue(lho.asMmixOcta() ^ rho.asMmixOcta());
                    default:
                        throw new Exception(string.Format("Can't apply the operation ^ for operands {0} and {1}", lho, rho));
                }
            }

            public override string ToString() {
                return string.Format("(^ {0} {1})", Lho.ToString(), Rho.ToString());
            }
        }

        internal class ComplementationNode : CommonUnaryOp {
            public ComplementationNode(Expression rightHandOperand)
                : base (rightHandOperand) 
            { }

            public override ExpressionValue eval(SymbolBindings args) {
                var rho = Rho.eval(args);
                var resultType = rho.Type;
                switch (resultType) {
                    case ExpressionType.MmixByte:
                        return ExpressionValue.makeValue(~rho.asMmixByte());
                    case ExpressionType.MmixWyde:
                        return ExpressionValue.makeValue(~rho.asMmixWyde());
                    case ExpressionType.MmixTetra:
                        return ExpressionValue.makeValue(~rho.asMmixTetra());
                    case ExpressionType.MmixOcta:
                        return ExpressionValue.makeValue(~rho.asMmixOcta());
                    default:
                        throw new Exception(string.Format("Can't apply the operation ~ for operand {0}", rho));
                }
            }

            public override string ToString() {
                return string.Format("(~ {0})", Rho.ToString());
            }
        }

        internal class RegistrationNode : CommonUnaryOp {
            public RegistrationNode(Expression rightHandOperand) 
                : base (rightHandOperand) 
            { }

            public override ExpressionValue eval(SymbolBindings args) {
                var rho = Rho.eval(args);
                var resultType = rho.Type;
                switch (resultType) {
                    case ExpressionType.MmixByte:
                        return ExpressionValue.makeRegister(rho.asMmixByte());
                    case ExpressionType.MmixWyde:
                        return ExpressionValue.makeRegister(rho.asMmixWyde());
                    case ExpressionType.MmixTetra:
                        return ExpressionValue.makeRegister(rho.asMmixTetra());
                    case ExpressionType.MmixOcta:
                        return ExpressionValue.makeRegister(rho.asMmixOcta());
                    default:
                        throw new Exception(string.Format("Can't apply the operation ~ for operand {0}", rho));
                }
            }

            public override string ToString() {
                return string.Format("($ {0})", Rho.ToString());
            }
        }

        internal class UnaryPlusNode : CommonUnaryOp {
            public UnaryPlusNode(Expression rightHandOperand)
                : base (rightHandOperand) 
            { }

            public override ExpressionValue eval(SymbolBindings args) {
                var rho = Rho.eval(args);
                var resultType = rho.Type;
                switch (resultType) {
                    case ExpressionType.MmixByte:
                        return ExpressionValue.makeValue(rho.asMmixByte());
                    case ExpressionType.MmixWyde:
                        return ExpressionValue.makeValue(rho.asMmixWyde());
                    case ExpressionType.MmixTetra:
                        return ExpressionValue.makeValue(rho.asMmixTetra());
                    case ExpressionType.MmixOcta:
                        return ExpressionValue.makeValue(rho.asMmixOcta());
                    default:
                        throw new Exception(string.Format("Can't apply the operation ~ for operand {0}", rho));
                }
            }

            public override string ToString() {
                return string.Format("(+ {0})", Rho.ToString());
            }
        }

        internal class UnaryMinusNode : CommonUnaryOp {
            public UnaryMinusNode(Expression rightHandOperand) 
                : base (rightHandOperand)
            {}

            public override ExpressionValue eval(SymbolBindings args) {
                var rho = Rho.eval(args);
                var resultType = rho.Type;
                switch (resultType) {
                    case ExpressionType.MmixByte:
                        return ExpressionValue.makeValue(-rho.asMmixByte());
                    case ExpressionType.MmixWyde:
                        return ExpressionValue.makeValue(-rho.asMmixWyde());
                    case ExpressionType.MmixTetra:
                        return ExpressionValue.makeValue(-rho.asMmixTetra());
                    case ExpressionType.MmixOcta:
                        return ExpressionValue.makeValue(-rho.asMmixOcta());
                    default:
                        throw new Exception(string.Format("Can't apply the operation ~ for operand {0}", rho));
                }
            }

            public override string ToString() {
                return string.Format("(- {0})", Rho.ToString());
            }
        }
    }
}
