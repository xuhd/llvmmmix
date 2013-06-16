using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;
using System.Globalization;

namespace mmixal {
    class Program {
        private const string INSTR_PTR = "@";

        private const string OPC_IS = "IS";

        private const string OPC_GREG = "GREG";

        private const string OPC_LOC = "LOC";

        private const string OPC_BYTE = "BYTE";

        private const string OPC_WYDE = "WYDE";

        private const string OPC_TETRA = "TETRA";

        private const string OPC_OCTA = "OCTA";

        private const string OPC_SET = "SET";

        private const string OPC_LDA = "LDA";

        private const long TEXT_SEG = 0x0000000000000000L;

        private const long DATA_SEG = 0x2000000000000000L;

        private const long POOL_SEG = 0x4000000000000000L;

        private const long STACK_SEG = 0x6000000000000000L;

        private const int SECTION_ALIGN = 0x20;

        private static readonly string[] SUBST_OPS =
            new string[] { OPC_SET, OPC_LDA };

        private static readonly string[] MMIX_PSEUDO_OPS =
            new string[] { OPC_IS, OPC_GREG, OPC_LOC, OPC_BYTE, OPC_WYDE, OPC_TETRA, OPC_OCTA };

        private static readonly Regex LINE_PATTERN = new Regex(@"^(?<label>\S+)?"
            + @"(?:\s+(?<opcode>\S+))"
            + @"(?<rest>$|\s+.*?)$"
            , RegexOptions.Compiled);

        private static readonly Regex ARGS_PATTERN = new Regex(@"(?<begin>^|,|\s)(?<arg>""(?:[^""]+|"""")*""|[^,\s]*)",
            RegexOptions.Compiled);

        private static Regex LOCAL_LABEL_POS_PATTERN = new Regex(@"^(?<value>(?<refnum>[0-9])[H])", RegexOptions.Compiled);

        private static Regex LOCAL_BACK_REF_PATTERN = new Regex(@"^(?:@(?<refid>[0-9])[B])(?:_\d+)", RegexOptions.Compiled);

        private static Regex LOCAL_FUTURE_REF_PATTERN = new Regex(@"^(?:@(?<refid>[0-9])[F])(?:_\d+)", RegexOptions.Compiled);

        private SymbolBindings _symbolBindings = new SymbolBindings();

        private UnboundSymbolDeps _unboundSymbolDeps = new UnboundSymbolDeps();

        private List<string> _unboundLocalFutureRefs = new List<string>();

        private List<string> _unboundSegmentTopRefs = new List<string>();

        private int _lineNum = 0;

        private int _linePos = 0;

        private Stream _output = null;

        private int _lastGreg = byte.MaxValue;

        private readonly ExpressionValue[] _globalRegs = new ExpressionValue[byte.MaxValue];

        static void Main(string[] argv) {
            if (argv.Length >= 1) {
                string infile = argv[0];
                string ofile;
                if (argv.Length >= 2)
                    ofile = argv[1];
                else
                    ofile = "out.mmo";
                try {
                    using (var reader = new StreamReader(new FileStream(infile, FileMode.Open, FileAccess.Read))) {
                        using (var binOutput = new FileStream(ofile, FileMode.Create, FileAccess.Write)) {
                            var prog = new Program();
                            prog.mainLoop(reader, binOutput);
                        }
                    }
                } catch (Exception e) {
                    Console.Error.WriteLine("Got exception :" + e.Message);
                }
            } else {
                Console.Error.WriteLine("Usage: mmixal <assembly file> [output file]");
            }
        }

        private void mainLoop(StreamReader reader, Stream binOutput) {
            _output = binOutput;
            makePredefinedSymbols();
            bool firstLine = true;
            while (!reader.EndOfStream) {
                var line = reader.ReadLine();
                if (!firstLine) {
                    _linePos = 0;
                    ++_lineNum;
                } else {
                    _lineNum = 0;
                    firstLine = false;
                }
                _linePos = skipWs(line, _linePos);
                if (_linePos < line.Length) {
                    if (!(char.IsLetter(line[_linePos]) || char.IsNumber(line[_linePos]))) // Wholeline comment
                        goto skipLine;
                    var chunks = line.Split(';');
                    foreach (var chunk in chunks) {
                        handleChunk(chunk);
                        _linePos += chunk.Length + 1;
                    }
                }
            skipLine: { }
            }
            bindOldSegmentTopSymbol();
            execDeferredRenderActions();
            makeGregSection();
        }

        private void makePredefinedSymbols() {
            _symbolBindings.Add("@", ExpressionValue.makeValue(0L));
            for (int i = 0; i < 10; i++)
                _symbolBindings.Add(i.ToString() + "B", ExpressionValue.makeValue(0L));
            _symbolBindings.Add("Data_Segment", ExpressionValue.makeValue(DATA_SEG));
            _symbolBindings.Add("Pool_Segment", ExpressionValue.makeValue(POOL_SEG));
            _symbolBindings.Add("Stack_Segment", ExpressionValue.makeValue(STACK_SEG));
            _symbolBindings.Add("StdIn", ExpressionValue.makeValue(0L));
            _symbolBindings.Add("StdOut", ExpressionValue.makeValue(1L));
            _symbolBindings.Add("StdErr", ExpressionValue.makeValue(2L));
            _symbolBindings.Add("Halt", ExpressionValue.makeValue(0L));
            _symbolBindings.Add("Fopen", ExpressionValue.makeValue(1L));
            _symbolBindings.Add("Fclose", ExpressionValue.makeValue(2L));
            _symbolBindings.Add("Fread", ExpressionValue.makeValue(3L));
            _symbolBindings.Add("Fgets", ExpressionValue.makeValue(4L));
            _symbolBindings.Add("Fgetws", ExpressionValue.makeValue(5L));
            _symbolBindings.Add("Fwrite", ExpressionValue.makeValue(6L));
            _symbolBindings.Add("Fputs", ExpressionValue.makeValue(7L));
            _symbolBindings.Add("Fputws", ExpressionValue.makeValue(8L));
            _symbolBindings.Add("Fseek", ExpressionValue.makeValue(9L));
            _symbolBindings.Add("Ftell", ExpressionValue.makeValue(10L));
            _symbolBindings.Add("TextRead", ExpressionValue.makeValue(0L));
            _symbolBindings.Add("TextWrite", ExpressionValue.makeValue(1L));
            _symbolBindings.Add("BinaryRead", ExpressionValue.makeValue(2L));
            _symbolBindings.Add("BinaryWrite", ExpressionValue.makeValue(3L));
            _symbolBindings.Add("BinaryReadWrite", ExpressionValue.makeValue(4L));
        }

        private List<string> collectUnboundSymbols(Expression expr) {
            var retVal = new List<string>();
            foreach (var symbol in expr.getArguments())
                if (!_symbolBindings.ContainsKey(symbol))
                    retVal.Add(symbol);
            return retVal;
        }

        private void bindSymbol(string symbolName, Expression expression) {
            _symbolBindings[symbolName] = expression.eval(_symbolBindings);
        }

        private List<string> bindLocalBackwardRefs(List<string> unboundSymbols) {
            var retVal = new List<string>();
            foreach (var us in unboundSymbols) {
                var matcher = LOCAL_BACK_REF_PATTERN.Match(us);
                if (matcher.Success)
                    _symbolBindings[us] = _symbolBindings[matcher.Groups["refid"].Value + "B"];
                else
                    retVal.Add(us);
            }
            return retVal;
        }

        private void rememberLocalForwardRefs(List<string> unboundSymbols) {
            foreach (var us in unboundSymbols) {
                var matcher = LOCAL_FUTURE_REF_PATTERN.Match(us);
                if (matcher.Success)
                    _unboundLocalFutureRefs.Add(us);
            }
        }

        private void bindPendingLocalForwardRefs(int forwardRefNum, Expression expr) {
            var retainedList = new List<string>();
            foreach (var ref0 in _unboundLocalFutureRefs) {
                var match = LOCAL_FUTURE_REF_PATTERN.Match(ref0);
                if (match.Success) {
                    int p0 = int.Parse(match.Groups["refid"].Value);
                    if (forwardRefNum == p0)
                        bindSymbol(ref0, expr);
                    else
                        retainedList.Add(ref0);
                } else {
                    throw new Exception("Should never happen");
                }
            }
            _unboundLocalFutureRefs = retainedList;
        }

        private void bindOldSegmentTopSymbol() {
            long oldPos = _symbolBindings[INSTR_PTR].asMmixOcta();
            foreach (var u in _unboundSegmentTopRefs)
                bindSymbol(u, ExpressionBuilder.makePointer(oldPos));
            _unboundSegmentTopRefs.Clear();
        }

        private void alignSection() {
            long bytesToFill = SECTION_ALIGN - (_output.Position % SECTION_ALIGN);
            if (bytesToFill == SECTION_ALIGN)
                bytesToFill = 0;
            for (int i = 0; i < bytesToFill; i++)
                _output.WriteByte(0);
        }

        private void handleChunk(string chunk) {
            var asmLine = tokenizeChunk(chunk);
            if (!MMIX_PSEUDO_OPS.Contains(asmLine.Opcode)) {
                Match localLabelMatcher;
                bool updateLocalBackReference;
                renderPrologue(asmLine, out localLabelMatcher, out updateLocalBackReference);
                renderOpcode(asmLine);
                renderEpilogue(localLabelMatcher, updateLocalBackReference, 4);
            } else {
                if (asmLine.Opcode == OPC_IS && asmLine.Label.Length != 0) {
                    handleIsPseudocode(asmLine);
                } else if (asmLine.Opcode == OPC_LOC) {
                    handleLocPseudocode(asmLine);
                } else if (asmLine.Opcode == OPC_GREG) {
                    handleGregPseudocode(asmLine);
                } else {
                    Match localLabelMatcher;
                    bool updateBackReference;
                    renderPrologue(asmLine, out localLabelMatcher, out updateBackReference);
                    long oldTop = _output.Position;
                    if (asmLine.Opcode == OPC_BYTE)
                        renderBytes(asmLine);
                    else if (asmLine.Opcode == OPC_WYDE)
                        renderWydes(asmLine);
                    else if (asmLine.Opcode == OPC_TETRA)
                        renderTetras(asmLine);
                    else if (asmLine.Opcode == OPC_OCTA)
                        renderOctas(asmLine);
                    else
                        throw new Exception("Should never happen");
                    int length = (int)(_output.Position - oldTop);
                    renderEpilogue(localLabelMatcher, updateBackReference, length);
                }
            }
        }

        private void renderPrologue(RawAsmLine asmLine, out Match localLabelMatcher, out bool updateLocalBackReference) {
            localLabelMatcher = LOCAL_LABEL_POS_PATTERN.Match(asmLine.Label);
            updateLocalBackReference = false;
            if (asmLine.Label.Length != 0) {
                var expr = ExpressionBuilder.makePointer(_symbolBindings[INSTR_PTR].asMmixOcta());
                if (!localLabelMatcher.Success) {
                    bindSymbol(asmLine.Label, expr);
                } else {
                    bindPendingLocalForwardRefs(int.Parse(localLabelMatcher.Groups["refnum"].Value), expr);
                    updateLocalBackReference = true;
                }
            }
        }

        private void renderEpilogue(Match localLabelMatcher, bool updateBackReference, int byteCount) {
            long curPtr = _symbolBindings[INSTR_PTR].asMmixOcta();
            if (updateBackReference)
                bindSymbol(localLabelMatcher.Groups["refnum"].Value + "B", ExpressionBuilder.makePointer(curPtr));
            bindSymbol(INSTR_PTR, ExpressionBuilder.makePointer(curPtr + byteCount));
        }

        private void execDeferredRenderActions() {
            long fileEnd = _output.Position;
            foreach (var dep in _unboundSymbolDeps) {
                _output.Position = dep.BinFileOffset;
                bindSymbol(INSTR_PTR, ExpressionBuilder.makePointer(dep.InstrPtr.asMmixOcta()));
                byte[] buffer = new byte[dep.Length];
                dep.Action(_symbolBindings, buffer);
                _output.Write(buffer, 0, buffer.Length);
            }
            _output.Position = fileEnd;
        }

        private void makeGregSection() {
            alignSection();
            byte[] binSegName = Encoding.ASCII.GetBytes(".greg");
            _output.Write(binSegName, 0, binSegName.Length);
            for (int i = binSegName.Length; (i % 8) != 0; i++)
                _output.WriteByte(0);
            byte[] buff = new byte[8];
            doRenderSingleOcta(ExpressionBuilder.makePointer(_lastGreg), _symbolBindings, 0, 0, buff);
            _output.Write(buff, 0, buff.Length);
            for (int i = _lastGreg; i < _globalRegs.Length; i++) {
                doRenderSingleOcta(ExpressionBuilder.makePointer(_globalRegs[i].asMmixOcta()), _symbolBindings, 0, 0, buff);
                _output.Write(buff, 0, buff.Length);
            }
            alignSection();
        }

        private void renderOpcode(RawAsmLine asmLine) {
            byte[] outputSeq = new byte[4];
            if (!SUBST_OPS.Contains(asmLine.Opcode)) {
                var opcode = OpcodeUtil.fromString(asmLine.Opcode);
                if (opcode != MmixOpcode.ILLEGAL) {
                    var opcodeCategory = OpcodeUtil.getOpcodeCategory(opcode);
                    switch (opcodeCategory) {
                        case MmixOpcodeCategory.ThreeRegs:
                            renderThreeArgsOpcode(opcode, asmLine, outputSeq);
                            break;
                        case MmixOpcodeCategory.RegAndWydeImmediate:
                            renderRegAndWydeImmediateOpcode(opcode, asmLine, outputSeq);
                            break;
                        case MmixOpcodeCategory.Branch:
                            renderBranchOpcode(opcode, asmLine, outputSeq);
                            break;
                        case MmixOpcodeCategory.Jump:
                            renderJumpOpcode(opcode, asmLine, outputSeq);
                            break;
                        case MmixOpcodeCategory.GetSpecialRegister:
                            renderGetOpcode(opcode, asmLine, outputSeq);
                            break;
                        case MmixOpcodeCategory.PutSpecialRegister:
                            renderPutOpcode(opcode, asmLine, outputSeq);
                            break;
                        default:
                            renderNonStrictOpcode(opcode, asmLine, outputSeq);
                            break;
                    }
                } else {
                    throw new Exception(string.Format("Unknown opcode mnemonic in line {0}, position {1}", _lineNum, _linePos));
                }
            } else {
                if (asmLine.Opcode == OPC_LDA) {
                    renderLdaSubstitution(asmLine, outputSeq);
                } else if (asmLine.Opcode == OPC_SET) {
                    renderSetSubstitution(asmLine, outputSeq);
                } else {
                    throw new Exception("Should never happen");
                }
            }
            _output.Write(outputSeq, 0, outputSeq.Length);
        }

        private void renderThreeArgsOpcode(MmixOpcode opcode, RawAsmLine asmLine, byte[] outputSeq) {
            string[] args;
            if (opcode == MmixOpcode.NEG && asmLine.Args.Length == 2) {
                args = new string[] { asmLine.Args[0], "$0", asmLine.Args[1] };
            } else {
                args = asmLine.Args;
            }
            if ((args.Length == 2) || (args.Length == 3)) {
                string expression = null;
                try {
                    var unboundSymbols = new List<string>();
                    expression = args[0];
                    var expr1 = ExpressionBuilder.buildExpression(expression);
                    unboundSymbols.AddRange(collectUnboundSymbols(expr1));
                    expression = args[1];
                    var expr2 = ExpressionBuilder.buildExpression(expression);
                    unboundSymbols.AddRange(collectUnboundSymbols(expr2));
                    Expression expr3;
                    if (args.Length == 3) {
                        expression = args[2];
                        expr3 = ExpressionBuilder.buildExpression(expression);
                        unboundSymbols.AddRange(collectUnboundSymbols(expr3));
                    } else {
                        expr3 = null;
                    }
                    unboundSymbols = bindLocalBackwardRefs(unboundSymbols);
                    if (unboundSymbols.Count == 0) {
                        ExpressionValue arg1;
                        ExpressionValue arg2;
                        ExpressionValue arg3;
                        try {
                            arg1 = expr1.eval(_symbolBindings);
                            if (expr3 != null) {
                                arg2 = expr2.eval(_symbolBindings);
                                arg3 = expr3.eval(_symbolBindings);
                            } else {
                                ExpressionValue globLoc = expr2.eval(_symbolBindings);
                                if (globLoc.IsNumeric) {
                                    long baseGreg, offset;
                                    if (findGreg(globLoc.asMmixOcta(), out baseGreg, out offset)) {
                                        arg2 = ExpressionValue.makeRegister(baseGreg);
                                        arg3 = ExpressionValue.makeValue(offset);
                                    } else {
                                        throw new Exception(string.Format("Cant't find an approriate global register " +
                                           " to use as a memory base the opcode {0}." +
                                           " Error found at line {1} pos {2}", asmLine.Opcode, _lineNum, _linePos));
                                    }
                                } else {
                                    throw new Exception(string.Format("Three registers,"
                                                + " two registers and immediate,"
                                                + " or absolute memory location backed by a known global register "
                                                + " are expected for the opcode {0} at line {1} pos {2}",
                                                asmLine.Opcode, _lineNum, _linePos));
                                }
                            }
                        } catch (Exception e) {
                            throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                                _lineNum, _linePos, e.Message));
                        }
                        if (arg1.Type == ExpressionType.Register
                            && arg2.Type == ExpressionType.Register) {
                            if (arg3.Type == ExpressionType.Register)
                                renderThreeRegsOpcode0(opcode,
                                    arg1.asRegisterNum(),
                                    arg2.asRegisterNum(),
                                    arg3.asRegisterNum(),
                                    outputSeq);
                            else if (arg3.IsNumeric)
                                renderTwoRegsAndImmediateOpcode0(
                                    opcode,
                                    arg1.asRegisterNum(),
                                    arg2.asRegisterNum(),
                                    arg3.asMmixOcta(),
                                    outputSeq);

                        } else {
                            throw new Exception(string.Format("First two operands for the opcode {0} should be registers. " +
                                " Error found at line {1} pos {2}",
                                asmLine.Opcode, _lineNum, _linePos));
                        }
                    } else {
                        throw new Exception(string.Format("The MMIX instruction at line {0}, position {1} "
                            + "contains references to undefined symbols : {2}. "
                            + "Future references are only allowed at JUMP-family operators and OCTA data fields.",
                             _lineNum, _linePos, string.Join(", ", unboundSymbols)));
                    }
                } catch (Exception e) {
                    throw new Exception(string.Format("Error while tried to parse expression {0} at line {1} pos {2} : {3}",
                        expression, _lineNum, _linePos, e.Message, e));
                }
            } else {
                throw new Exception(string.Format("Three registers,"
                    + " two registers and immediate,"
                    + " or absolute memory location backed by a known global register "
                    + " are expected for the opcode {0} at line {1} pos {2}",
                    asmLine.Opcode, _lineNum, _linePos));
            }
        }

        private void renderLdaSubstitution(RawAsmLine asmLine, byte[] outputSeq) {
            var args = asmLine.Args;
            if (args.Length == 2) {
                string expression = null;
                try {
                    var unboundSymbols = new List<string>();
                    expression = asmLine.Args[0];
                    var expr1 = ExpressionBuilder.buildExpression(expression);
                    unboundSymbols.AddRange(collectUnboundSymbols(expr1));
                    expression = asmLine.Args[1];
                    var expr2 = ExpressionBuilder.buildExpression(expression);
                    unboundSymbols.AddRange(collectUnboundSymbols(expr2));
                    unboundSymbols = bindLocalBackwardRefs(unboundSymbols);
                    if (unboundSymbols.Count == 0) {
                        ExpressionValue arg1;
                        ExpressionValue arg2;
                        try {
                            arg1 = expr1.eval(_symbolBindings);
                            arg2 = expr2.eval(_symbolBindings);
                        } catch (Exception e) {
                            throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                                _lineNum, _linePos, e.Message));
                        }
                        if (arg1.Type == ExpressionType.Register && arg2.IsNumeric) {
                            long baseGreg;
                            long offset;
                            if (findGreg(arg2.asMmixOcta(), out baseGreg, out offset)) {
                                renderTwoRegsAndImmediateOpcode0(MmixOpcode.ADDU,
                                    arg1.asRegisterNum(), baseGreg, offset, outputSeq);
                            } else {
                                throw new Exception(string.Format("Cant't find an approriate global register " +
                                    " to use as a memory base the opcode {0}." +
                                    " Error found at line {1} pos {2}",
                                asmLine.Opcode, _lineNum, _linePos));
                            }
                        } else {
                            throw new Exception(string.Format("First operand for the opcode {0} should be a register" +
                                " and the second one should be a wyde. " +
                                " Error found at line {1} pos {2}",
                                asmLine.Opcode, _lineNum, _linePos));
                        }
                    } else {
                        throw new Exception(string.Format("The line {0} at pos {1} contains references to undefined symbols :{2}. " +
                            "Future references are only allowed at JUMP-family operators and OCTA consttand fields.",
                             _lineNum, _linePos, string.Join(", ", unboundSymbols)));
                    }
                } catch (Exception e) {
                    throw new Exception(string.Format("Error while tried to parse expressiion {0} at line {1} pos {2} : {3}",
                        expression, _lineNum, _linePos, e.Message, e));
                }
            } else {
                throw new Exception(string.Format("Two arguments are expected for the opcode {0} at line {1} pos {2}",
                    asmLine.Opcode, _lineNum, _linePos));
            }
        }

        private bool findGreg(long absMemLocation, out long baseGreg, out long offset) {
            bool success = false;
            baseGreg = 0L;
            offset = 0L;
            for (int i = _lastGreg; i < _globalRegs.Length; ++i) {
                long regValue = _globalRegs[i].asMmixOcta();
                if (absMemLocation >= regValue && absMemLocation <= regValue + byte.MaxValue) {
                    baseGreg = i;
                    offset = absMemLocation - regValue;
                    success = true;
                    break;
                }
            }
            return success;
        }

        private void renderSetSubstitution(RawAsmLine asmLine, byte[] outputSeq) {
            var args = asmLine.Args;
            if (args.Length == 2) {
                string expression = null;
                try {
                    var unboundSymbols = new List<string>();
                    expression = asmLine.Args[0];
                    var expr1 = ExpressionBuilder.buildExpression(expression);
                    unboundSymbols.AddRange(collectUnboundSymbols(expr1));
                    expression = asmLine.Args[1];
                    var expr2 = ExpressionBuilder.buildExpression(expression);
                    unboundSymbols.AddRange(collectUnboundSymbols(expr2));
                    unboundSymbols = bindLocalBackwardRefs(unboundSymbols);
                    if (unboundSymbols.Count == 0) {
                        ExpressionValue arg1;
                        ExpressionValue arg2;
                        try {
                            arg1 = expr1.eval(_symbolBindings);
                            arg2 = expr2.eval(_symbolBindings);
                        } catch (Exception e) {
                            throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                                _lineNum, _linePos, e.Message));
                        }
                        if (arg1.Type == ExpressionType.Register && arg2.IsNumeric) {
                            renderRegAndWydeImmediateOpcode0(MmixOpcode.SETL, arg1.asRegisterNum(), arg2.asMmixOcta(), outputSeq);
                        } else if (arg1.Type == ExpressionType.Register && arg2.Type == ExpressionType.Register) {
                            renderTwoRegsAndImmediateOpcode0(MmixOpcode.OR, arg1.asRegisterNum(), arg2.asRegisterNum(), 0, outputSeq);
                        } else {
                            throw new Exception(string.Format("First operand for the opcode {0} should be a register" +
                                " and the second one should be a wyde. " +
                                " Error found at line {1} pos {2}",
                                asmLine.Opcode, _lineNum, _linePos));
                        }
                    } else {
                        throw new Exception(string.Format("The line {0} at pos {1} contains references to undefined symbols :{2}. " +
                            "Future references are only allowed at JUMP-family operators and OCTA consttand fields.",
                             _lineNum, _linePos, string.Join(", ", unboundSymbols)));
                    }
                } catch (Exception e) {
                    throw new Exception(string.Format("Error while tried to parse expressiion {0} at line {1} pos {2} : {3}",
                        expression, _lineNum, _linePos, e.Message, e));
                }
            } else {
                throw new Exception(string.Format("Two arguments are expected for the opcode {0} at line {1} pos {2}",
                    asmLine.Opcode, _lineNum, _linePos));
            }
        }

        private void renderRegAndWydeImmediateOpcode(MmixOpcode opcode, RawAsmLine asmLine, byte[] outputSeq) {
            var args = asmLine.Args;
            if (args.Length == 2) {
                string expression = null;
                try {
                    var unboundSymbols = new List<string>();
                    expression = asmLine.Args[0];
                    var expr1 = ExpressionBuilder.buildExpression(expression);
                    unboundSymbols.AddRange(collectUnboundSymbols(expr1));
                    expression = asmLine.Args[1];
                    var expr2 = ExpressionBuilder.buildExpression(expression);
                    unboundSymbols.AddRange(collectUnboundSymbols(expr2));
                    unboundSymbols = bindLocalBackwardRefs(unboundSymbols);
                    if (unboundSymbols.Count == 0) {
                        ExpressionValue arg1;
                        ExpressionValue arg2;
                        try {
                            arg1 = expr1.eval(_symbolBindings);
                            arg2 = expr2.eval(_symbolBindings);
                        } catch (Exception e) {
                            throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                                _lineNum, _linePos, e.Message));
                        }
                        if (arg1.Type == ExpressionType.Register
                            && (arg2.IsNumeric || arg2.Type == ExpressionType.CharLiteral)) {
                            long u = arg2.IsNumeric ? arg2.asMmixOcta() : Convert.ToInt64(arg2.asChar());
                            renderRegAndWydeImmediateOpcode0(opcode, arg1.asRegisterNum(), u, outputSeq);
                        } else {
                            throw new Exception(string.Format("First operand for the opcode {0} should be a register" +
                                " and the second one should be a wyde or char literal. " +
                                " Error found at line {1} pos {2}",
                                asmLine.Opcode, _lineNum, _linePos));
                        }
                    } else {
                        throw new Exception(string.Format("The line {0} at pos {1} contains references to undefined symbols :{2}. " +
                            "Future references are only allowed at JUMP-family operators and OCTA consttand fields.",
                             _lineNum, _linePos, string.Join(", ", unboundSymbols)));
                    }
                } catch (Exception e) {
                    throw new Exception(string.Format("Error while tried to parse expressiion {0} at line {1} pos {2} : {3}",
                        expression, _lineNum, _linePos, e.Message), e);
                }
            } else {
                throw new Exception(string.Format("Two arguments are expected for the opcode {0} at line {1} pos {2}",
                    asmLine.Opcode, _lineNum, _linePos));
            }
        }

        private void renderBranchOpcode(MmixOpcode opcode, RawAsmLine asmLine, byte[] outputSeq) {
            var args = asmLine.Args;
            if (args.Length == 2) {
                string expression = null;
                try {
                    var unboundSymbols = new List<string>();
                    expression = asmLine.Args[0];
                    var expr1 = ExpressionBuilder.buildExpression(expression);
                    unboundSymbols.AddRange(collectUnboundSymbols(expr1));
                    expression = asmLine.Args[1];
                    var expr2 = ExpressionBuilder.buildExpression(expression);
                    unboundSymbols.AddRange(collectUnboundSymbols(expr2));
                    unboundSymbols = bindLocalBackwardRefs(unboundSymbols);
                    if (unboundSymbols.Count == 0) {
                        doRenderBranchOpcode(opcode, asmLine, _symbolBindings, _lineNum, _linePos, outputSeq, expr1, expr2);
                    } else {
                        rememberLocalForwardRefs(unboundSymbols);
                        long outPos0 = _output.Position;
                        int lineNum0 = _lineNum;
                        int linePos0 = _linePos;
                        ExpressionValue instrPtr0 = _symbolBindings[INSTR_PTR];
                        _unboundSymbolDeps.Add(
                            new UnboundSymbolDep {
                                BinFileOffset = outPos0,
                                InstrPtr = instrPtr0,
                                Length = 4,
                                Action = (SymbolBindings args0, byte[] target) =>
                                    doRenderBranchOpcode(opcode, asmLine, args0, lineNum0, linePos0, target, expr1, expr2)
                            });
                    }
                } catch (Exception e) {
                    throw new Exception(string.Format("Error while tried to parse expressiion {0} at line {1} pos {2} : {3}",
                        expression, _lineNum, _linePos, e.Message));
                }
            } else {
                throw new Exception(string.Format("Two arguments are expected for the opcode {0} at line {1} pos {2}",
                    asmLine.Opcode, _lineNum, _linePos));
            }
        }

        private static void doRenderBranchOpcode(MmixOpcode opcode, RawAsmLine asmLine, SymbolBindings boundArgsMap,
            int lineNum, int linePos, byte[] outputSeq, Expression expr1, Expression expr2) {
            ExpressionValue arg1;
            ExpressionValue arg2;
            try {
                arg1 = expr1.eval(boundArgsMap);
                arg2 = expr2.eval(boundArgsMap);
            } catch (Exception e) {
                throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                    lineNum, linePos, e.Message));
            }
            if (arg1.Type == ExpressionType.Register && arg2.IsNumeric) {
                long offset = arg2.asMmixOcta() - boundArgsMap[INSTR_PTR].asMmixOcta();
                if (offset > 0)
                    renderBranchOpcode0(opcode,
                        arg1.asRegisterNum(),
                        offset,
                        outputSeq);
                else
                    renderBackwardBranchOpcode0(opcode,
                        arg1.asRegisterNum(),
                        -offset,
                        outputSeq);
            } else {
                throw new Exception(string.Format("First operand for the opcode {0} should be a register" +
                    " and the second one should be a wyde. " +
                    " Error found at line {1} pos {2}",
                    asmLine.Opcode, lineNum, linePos));
            }
        }

        private void renderJumpOpcode(MmixOpcode opcode, RawAsmLine asmLine, byte[] outputSeq) {
            var args = asmLine.Args;
            if (args.Length == 1) {
                string expression = null;
                try {
                    expression = asmLine.Args[0];
                    var expr1 = ExpressionBuilder.buildExpression(expression);
                    var unboundSymbols = bindLocalBackwardRefs(collectUnboundSymbols(expr1));
                    if (unboundSymbols.Count == 0) {
                        doRenderJumpOpcode(opcode, asmLine, _symbolBindings, _lineNum, _linePos, outputSeq, expr1);
                    } else {
                        rememberLocalForwardRefs(unboundSymbols);
                        long outPos0 = _output.Position;
                        int lineNum0 = _lineNum;
                        int linePos0 = _linePos;
                        ExpressionValue instrPtr0 = _symbolBindings[INSTR_PTR];
                        _unboundSymbolDeps.Add(
                           new UnboundSymbolDep {
                               BinFileOffset = outPos0,
                               InstrPtr = instrPtr0,
                               Length = 4,
                               Action = (SymbolBindings args0, byte[] target) =>
                                   doRenderJumpOpcode(opcode, asmLine, args0, lineNum0, linePos0, target, expr1)
                           });
                    }
                } catch (Exception e) {
                    throw new Exception(string.Format("Error while tried to parse expression {0} at line {1} pos {2} : {3}",
                        expression, _lineNum, _linePos, e.Message, e));
                }
            } else {
                throw new Exception(string.Format("One arguments is expected for the opcode {0} at line {1} pos {2}",
                    asmLine.Opcode, _lineNum, _linePos));
            }
        }

        private static void doRenderJumpOpcode(MmixOpcode opcode, RawAsmLine asmLine,
            SymbolBindings boundArgsMap, int lineNum, int pos, byte[] outputSeq, Expression expr1) {
            ExpressionValue arg1;
            try {
                arg1 = expr1.eval(boundArgsMap);
            } catch (Exception e) {
                throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                    lineNum, pos, e.Message));
            }
            if (arg1.IsNumeric) {
                long offset = arg1.asMmixOcta() - boundArgsMap[INSTR_PTR].asMmixOcta();
                if (offset > 0)
                    renderJumpOpcode0(opcode,
                        offset,
                        outputSeq);
                else
                    renderBackwardJumpOpcode0(opcode,
                        -offset,
                        outputSeq);
            } else {
                throw new Exception(string.Format("The operand for the opcode {0} should be an immediate numeric value." +
                    asmLine.Opcode, lineNum, pos));
            }
        }

        private void renderGetOpcode(MmixOpcode opcode, RawAsmLine asmLine, byte[] outputSeq) {
            var args = asmLine.Args;
            if (args.Length == 2) {
                string expression = null;
                try {
                    var unboundSymbols = new List<string>();
                    expression = asmLine.Args[0];
                    var expr1 = ExpressionBuilder.buildExpression(expression);
                    unboundSymbols.AddRange(collectUnboundSymbols(expr1));
                    expression = asmLine.Args[1];
                    var expr2 = ExpressionBuilder.buildExpression(expression);
                    unboundSymbols.AddRange(collectUnboundSymbols(expr2));
                    unboundSymbols = bindLocalBackwardRefs(unboundSymbols);
                    if (unboundSymbols.Count == 0) {
                        ExpressionValue arg1;
                        ExpressionValue arg2;
                        try {
                            arg1 = expr1.eval(_symbolBindings);
                            arg2 = expr2.eval(_symbolBindings);
                        } catch (Exception e) {
                            throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                                _lineNum, _linePos, e.Message));
                        }
                        if (arg1.Type == ExpressionType.Register &&
                            arg2.Type == ExpressionType.SpecialRegister) {
                            renderRegAndWydeImmediateOpcode0(opcode, arg1.asRegisterNum(), arg2.asRegisterNum(), outputSeq);
                        } else {
                            throw new Exception(string.Format("First operand for the opcode {0} should be a register" +
                                " and the second one should be a special register. " +
                                " Error found at line {1} pos {2}",
                                asmLine.Opcode, _lineNum, _linePos));
                        }
                    } else {
                        throw new Exception(string.Format("The line {0} at pos {1} contains references to undefined symbols :{2}. " +
                            "Future references are only allowed at JUMP-family operators and OCTA consttand fields.",
                             _lineNum, _linePos, string.Join(", ", unboundSymbols)));
                    }
                } catch (Exception e) {
                    throw new Exception(string.Format("Error while tried to parse expressiion {0} at line {1} pos {2} : {3}",
                        expression, _lineNum, _linePos, e.Message, e));
                }
            } else {
                throw new Exception(string.Format("Two arguments are expected for the opcode {0} at line {1} pos {2}",
                    asmLine.Opcode, _lineNum, _linePos));
            }
        }

        private void renderPutOpcode(MmixOpcode opcode, RawAsmLine asmLine, byte[] outputSeq) {
            var args = asmLine.Args;
            if (args.Length == 2) {
                string expression = null;
                try {
                    var unboundSymbols = new List<string>();
                    expression = asmLine.Args[0];
                    var expr1 = ExpressionBuilder.buildExpression(expression);
                    unboundSymbols.AddRange(collectUnboundSymbols(expr1));
                    expression = asmLine.Args[1];
                    var expr2 = ExpressionBuilder.buildExpression(expression);
                    unboundSymbols.AddRange(collectUnboundSymbols(expr2));
                    unboundSymbols = bindLocalBackwardRefs(unboundSymbols);
                    if (unboundSymbols.Count == 0) {
                        ExpressionValue arg1;
                        ExpressionValue arg2;
                        try {
                            arg1 = expr1.eval(_symbolBindings);
                            arg2 = expr2.eval(_symbolBindings);
                        } catch (Exception e) {
                            throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                                _lineNum, _linePos, e.Message));
                        }
                        if (arg1.Type == ExpressionType.SpecialRegister &&
                            (arg2.Type == ExpressionType.Register || arg2.IsNumeric)) {
                            if (arg2.Type == ExpressionType.Register) {
                                renderRegAndWydeImmediateOpcode0(opcode, arg1.asRegisterNum(), arg2.asRegisterNum(), outputSeq);
                            } else {
                                var opcode0 = OpcodeUtil.getImmediateCounterpart(opcode);
                                renderRegAndWydeImmediateOpcode0(opcode0, arg1.asRegisterNum(), arg2.asMmixOcta(), outputSeq);
                            }
                        } else {
                            throw new Exception(string.Format("First operand for the opcode {0} should be a special register" +
                                " and the second one should be a register or an immediate value. " +
                                " Error found at line {1} pos {2}",
                                asmLine.Opcode, _lineNum, _linePos));
                        }
                    } else {
                        throw new Exception(string.Format("The line {0} at pos {1} contains references to undefined symbols :{2}. " +
                            "Future references are only allowed at JUMP-family operators and OCTA consttand fields.",
                             _lineNum, _linePos, string.Join(", ", unboundSymbols)));
                    }
                } catch (Exception e) {
                    throw new Exception(string.Format("Error while tried to parse expressiion {0} at line {1} pos {2} : {3}",
                        expression, _lineNum, _linePos, e.Message, e));
                }
            } else {
                throw new Exception(string.Format("Two arguments are expected for the opcode {0} at line {1} pos {2}",
                    asmLine.Opcode, _lineNum, _linePos));
            }
        }

        private void renderNonStrictOpcode(MmixOpcode opcode, RawAsmLine asmLine, byte[] outputSeq) {
            uint arg = renderNonStrictOpcode_argAsTetra(asmLine);
            outputSeq[0] = (byte)opcode;
            outputSeq[1] = (byte)((arg >> 16) & 0xff);
            outputSeq[2] = (byte)((arg >> 8) & 0xff);
            outputSeq[3] = (byte)(arg & 0xff);
        }

        private uint renderNonStrictOpcode_argAsTetra(RawAsmLine asmLine) {
            uint arg;
            switch (asmLine.Args.Length) {
                case 0:
                    arg = 0;
                    break;
                case 1:
                    arg = renderNonStrictOpcode_argLast(asmLine.Args[0], asmLine.Opcode);
                    break;
                case 2: {
                        byte arg1 = renderNonStrictOpcode_arg1or2(asmLine.Args[0], asmLine.Opcode);
                        uint arg2 = renderNonStrictOpcode_argLast(asmLine.Args[1], asmLine.Opcode);
                        if (arg2 <= ushort.MaxValue) {
                            arg = ((uint)arg1 << 16) | arg2;
                        } else {
                            throw new Exception(string.Format("Value out of bounds for the second operand at line {0} pos {1}",
                                _lineNum, _linePos));
                        }
                    }
                    break;
                case 3: {
                        byte arg1 = renderNonStrictOpcode_arg1or2(asmLine.Args[0], asmLine.Opcode);
                        byte arg2 = renderNonStrictOpcode_arg1or2(asmLine.Args[1], asmLine.Opcode);
                        uint arg3 = renderNonStrictOpcode_argLast(asmLine.Args[2], asmLine.Opcode);
                        arg = ((uint)arg1 << 16) | ((uint)arg2 << 8) | (uint)arg3;
                    }
                    break;
                default:
                    throw new Exception(string.Format("Illegal number of arguments for the opcode {0} at line {1} pos {2}",
                        asmLine.Opcode, _lineNum, _linePos));
            }
            return arg;
        }

        private byte renderNonStrictOpcode_arg1or2(string expression, string opcode) {
            try {
                var expr1 = ExpressionBuilder.buildExpression(expression);
                var unboundSymbols = bindLocalBackwardRefs(collectUnboundSymbols(expr1));
                if (unboundSymbols.Count == 0) {
                    ExpressionValue arg1;
                    try {
                        arg1 = expr1.eval(_symbolBindings);
                    } catch (Exception e) {
                        throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                            _lineNum, _linePos, e.Message));
                    }
                    if (arg1.Type != ExpressionType.Register) {
                        if (arg1.IsNumeric) {
                            return coerceByte(arg1.asMmixOcta());
                        } else {
                            throw new Exception(string.Format("The operand for the opcode {0} should be an immediate numeric value." +
                                opcode, _lineNum, _linePos));
                        }
                    } else {
                        return (byte)arg1.asRegisterNum();
                    }
                } else {
                    throw new Exception(string.Format("The line {0} at pos {1} contains undefined symbols :{2}. " +
                        "Future references are only allowed at JUMP-family operators and OCTA consttand fields.",
                         _lineNum, _linePos, string.Join(", ", unboundSymbols)));
                }
            } catch (Exception e) {
                throw new Exception(string.Format("Error while tried to parse expression {0} at line {1} pos {2} : {3}",
                    expression, _lineNum, _linePos, e.Message, e));
            }
        }

        private uint renderNonStrictOpcode_argLast(string expression, string opcode) {
            try {
                var expr1 = ExpressionBuilder.buildExpression(expression);
                var unboundSymbols = bindLocalBackwardRefs(collectUnboundSymbols(expr1));
                if (unboundSymbols.Count == 0) {
                    ExpressionValue arg1;
                    try {
                        arg1 = expr1.eval(_symbolBindings);
                    } catch (Exception e) {
                        throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                            _lineNum, _linePos, e.Message));
                    }
                    if (arg1.Type != ExpressionType.Register) {
                        if (arg1.IsNumeric) {
                            return coerceTetra(arg1.asMmixOcta());
                        } else {
                            throw new Exception(string.Format("The operand for the opcode {0} should be an immediate numeric value." +
                                opcode, _lineNum, _linePos));
                        }
                    } else {
                        return (uint)arg1.asRegisterNum();
                    }
                } else {
                    throw new Exception(string.Format("The line {0} at pos {1} contains undefined symbols :{2}. " +
                        "Future references are only allowed at JUMP-family operators and OCTA consttand fields.",
                         _lineNum, _linePos, string.Join(", ", unboundSymbols)));
                }
            } catch (Exception e) {
                throw new Exception(string.Format("Error while tried to parse expression {0} at line {1} pos {2} : {3}",
                    expression, _lineNum, _linePos, e.Message, e));
            }
        }

        private static void renderThreeRegsOpcode0(MmixOpcode opcode, long arg1, long arg2, long arg3, byte[] outputSeq) {
            outputSeq[0] = (byte)opcode;
            outputSeq[1] = (byte)coerceByte(arg1);
            outputSeq[2] = (byte)coerceByte(arg2);
            outputSeq[3] = (byte)coerceByte(arg3);
        }

        private static void renderTwoRegsAndImmediateOpcode0(MmixOpcode opcode, long arg1, long arg2, long arg3, byte[] outputSeq) {
            var opcode0 = OpcodeUtil.getImmediateCounterpart(opcode);
            if (opcode0 != MmixOpcode.ILLEGAL) {
                renderThreeRegsOpcode0(opcode0, arg1, arg2, arg3, outputSeq);
            } else {
                throw new Exception(string.Format("Can't find the immediate counterpart for opcode {0}", opcode));
            }
        }

        private static void renderRegAndWydeImmediateOpcode0(MmixOpcode opcode, long arg1, long arg2, byte[] outputSeq) {
            outputSeq[0] = (byte)opcode;
            outputSeq[1] = (byte)coerceByte(arg1);
            ushort wyde = coerceWyde(arg2);
            outputSeq[2] = (byte)((wyde >> 8) & 0xff);
            outputSeq[3] = (byte)(wyde & 0xff);
        }

        private static void renderBranchOpcode0(MmixOpcode opcode, long arg1, long arg2, byte[] outputSeq) {
            outputSeq[0] = (byte)opcode;
            outputSeq[1] = (byte)coerceByte(arg1);
            ushort wyde = coerceWyde(arg2 >> 2);
            outputSeq[2] = (byte)((wyde >> 8) & 0xff);
            outputSeq[3] = (byte)(wyde & 0xff);
        }

        private static void renderBackwardBranchOpcode0(MmixOpcode opcode, long arg1, long arg2, byte[] outputSeq) {
            var opcode0 = OpcodeUtil.getBackwardCounterpart(opcode);
            if (opcode0 != MmixOpcode.ILLEGAL) {
                renderBranchOpcode0(opcode0, arg1, arg2, outputSeq);
            } else {
                throw new Exception(string.Format("Can't find backward counterpart for the opcode {0}", opcode));
            }
        }

        private static void renderJumpOpcode0(MmixOpcode opcode, long arg1, byte[] outputSeq) {
            outputSeq[0] = (byte)opcode;
            uint tetra = coerceTetra(arg1 >> 2);
            outputSeq[1] = (byte)((tetra >> 16) & 0xff);
            outputSeq[2] = (byte)((tetra >> 8) & 0xff);
            outputSeq[3] = (byte)(tetra & 0xff);
        }

        private static void renderBackwardJumpOpcode0(MmixOpcode opcode, long arg1, byte[] outputSeq) {
            var opcode0 = OpcodeUtil.getBackwardCounterpart(opcode);
            if (opcode0 != MmixOpcode.ILLEGAL) {
                renderJumpOpcode0(opcode0, arg1, outputSeq);
            } else {
                throw new Exception(string.Format("Can't find backward counterpart for the opcode {0}", opcode));
            }
        }

        private void renderBytes(RawAsmLine asmLine) {
            string[] args;
            if (asmLine.Args.Length != 0)
                args = asmLine.Args;
            else
                args = new string[] { "0" };
            foreach (var arg in args) {
                var expr = ExpressionBuilder.buildExpression(arg);
                var unboundSymbols = new List<string>();
                unboundSymbols.AddRange(collectUnboundSymbols(expr));
                if (unboundSymbols.Count == 0) {
                    ExpressionValue val;
                    try {
                        val = expr.eval(_symbolBindings);
                    } catch (Exception e) {
                        throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                            _lineNum, _linePos, e.Message));
                    }
                    if (val.IsNumeric) {
                        byte b = coerceByte(val.asMmixOcta());
                        _output.WriteByte(b);
                    } else {
                        switch (val.Type) {
                            case ExpressionType.CharLiteral:
                                try {
                                    val = expr.eval(_symbolBindings);
                                    char c = val.asChar();
                                    renderChar(c, _output);
                                } catch (Exception e) {
                                    throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                                        _lineNum, _linePos, e.Message));
                                }
                                break;
                            case ExpressionType.StringLiteral:
                                try {
                                    val = expr.eval(_symbolBindings);
                                    string s = val.ToStringShort();
                                    renderString(s, _output);
                                } catch (Exception e) {
                                    throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                                        _lineNum, _linePos, e.Message));
                                }
                                break;
                            default:
                                throw new Exception(string.Format("Unsupported literal type at line {0} pos {1}", _lineNum, _linePos));
                        }
                    }
                } else {
                    throw new Exception(string.Format("The line {0} at pos {1} contains undefined symbols :{2}. " +
                       "Future references are only allowed at JUMP-family operators and OCTA consttand fields.",
                        _lineNum, _linePos, string.Join(", ", unboundSymbols)));
                }
            }
            alignStream(_output, 8);
        }

        private void renderWydes(RawAsmLine asmLine) {
            string[] args;
            if (asmLine.Args.Length != 0)
                args = asmLine.Args;
            else
                args = new string[] { "0" };
            foreach (var arg in args) {
                var expr = ExpressionBuilder.buildExpression(arg);
                var unboundSymbols = new List<string>();
                unboundSymbols.AddRange(collectUnboundSymbols(expr));
                if (unboundSymbols.Count == 0) {
                    ExpressionValue val;
                    try {
                        val = expr.eval(_symbolBindings);
                    } catch (Exception e) {
                        throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                            _lineNum, _linePos, e.Message));
                    }
                    if (val.IsNumeric) {
                        ushort w = coerceWyde(val.asMmixOcta());
                        _output.WriteByte((byte)((w >> 8) & 0xff));
                        _output.WriteByte((byte)(w & 0xff));
                    } else {
                        throw new Exception(string.Format("Unsupported literal type at line {0} pos {1}", _lineNum, _linePos));
                    }
                } else {
                    throw new Exception(string.Format("The line {0} at pos {1} contains undefined symbols :{2}. " +
                      "Future references are only allowed at JUMP-family operators and OCTA consttand fields.",
                       _lineNum, _linePos, string.Join(", ", unboundSymbols)));
                }
            }
            alignStream(_output, 8);
        }

        private void renderTetras(RawAsmLine asmLine) {
            string[] args;
            if (asmLine.Args.Length != 0)
                args = asmLine.Args;
            else
                args = new string[] { "0" };
            foreach (var arg in args) {
                var expr = ExpressionBuilder.buildExpression(arg);
                ExpressionValue val;
                var unboundSymbols = new List<string>();
                unboundSymbols.AddRange(collectUnboundSymbols(expr));
                if (unboundSymbols.Count == 0) {
                    try {
                        val = expr.eval(_symbolBindings);
                    } catch (Exception e) {
                        throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                            _lineNum, _linePos, e.Message));
                    }
                    if (val.IsNumeric) {
                        uint t = coerceTetra(val.asMmixOcta());
                        _output.WriteByte((byte)((t >> 24) & 0xff));
                        _output.WriteByte((byte)((t >> 16) & 0xff));
                        _output.WriteByte((byte)((t >> 8) & 0xff));
                        _output.WriteByte((byte)(t & 0xff));
                    } else {
                        throw new Exception(string.Format("Unsupported literal type at line {0} pos {1}", _lineNum, _linePos));
                    }
                } else {
                    throw new Exception(string.Format("The line {0} at pos {1} contains undefined symbols :{2}. " +
                         "Future references are only allowed at JUMP-family operators and OCTA consttand fields.",
                          _lineNum, _linePos, string.Join(", ", unboundSymbols)));
                }
            }
            alignStream(_output, 8);
        }

        private void renderOctas(RawAsmLine asmLine) {
            string[] args;
            if (asmLine.Args.Length != 0)
                args = asmLine.Args;
            else
                args = new string[] { "0" };
            foreach (var arg in args) {
                var expr = ExpressionBuilder.buildExpression(arg);
                var unboundSymbols = new List<string>();
                unboundSymbols.AddRange(collectUnboundSymbols(expr));
                var buff = new byte[8];
                if (unboundSymbols.Count == 0) {
                    doRenderSingleOcta(expr, _symbolBindings, _lineNum, _linePos, buff);
                } else {
                    rememberLocalForwardRefs(unboundSymbols);
                    var instrPtr0 = _symbolBindings[INSTR_PTR];
                    long outPos0 = _output.Position;
                    var boundArgs0 = _symbolBindings;
                    int lineNum0 = _lineNum;
                    int linePos0 = _linePos;
                    _unboundSymbolDeps.Add(
                       new UnboundSymbolDep {
                           BinFileOffset = outPos0,
                           InstrPtr = instrPtr0,
                           Length = 8,
                           Action = (SymbolBindings args0, byte[] target) =>
                               doRenderSingleOcta(expr, boundArgs0, lineNum0, linePos0, target)
                       });
                }
                _output.Write(buff, 0, 8);
            }
        }

        private static void doRenderSingleOcta(Expression expr, SymbolBindings boundArgsMap,
            int lineNum, int linePos, byte[] target) {
            ExpressionValue val;
            try {
                val = expr.eval(boundArgsMap);
            } catch (Exception e) {
                throw new Exception(string.Format("Can't evaluate expression at line {0} pos {1} : {2}",
                    lineNum, linePos, e.Message));
            }
            if (val.IsNumeric) {
                ulong o = (ulong)val.asMmixOcta();
                target[0] = (byte)((o >> 56) & 0xff);
                target[1] = (byte)((o >> 48) & 0xff);
                target[2] = (byte)((o >> 40) & 0xff);
                target[3] = (byte)((o >> 32) & 0xff);
                target[4] = (byte)((o >> 24) & 0xff);
                target[5] = (byte)((o >> 16) & 0xff);
                target[6] = (byte)((o >> 8) & 0xff);
                target[7] = (byte)(o & 0xff);
            } else {
                throw new Exception(string.Format("Unsupported literal type at line {0} pos {1}", lineNum, linePos));
            }
        }

        private static void alignStream(Stream ms, int width) {
            int rem = width - ((int)ms.Position % width);
            if (rem == width)
                rem = 0;
            for (int i = 0; i < rem; i++)
                ms.WriteByte(0);
        }

        private void renderChar(char c, Stream binOutput) {
            using (var ms = new MemoryStream()) {
                using (var writer = new StreamWriter(ms, Encoding.UTF8)) {
                    writer.Write(c);
                    writer.Flush();
                    var buff = ms.ToArray();
                    int len = buff.Length - 3;
                    binOutput.Write(buff, 3, len);
                }
            }
        }

        private void renderString(string s, Stream binOutput) {
            var ms = new MemoryStream();
            using (var writer = new StreamWriter(ms, Encoding.UTF8)) {
                writer.Write(s);
                writer.Flush();
                var buff = ms.ToArray();
                int len = buff.Length - 3;
                binOutput.Write(buff, 3, len);
            }
        }

        private void handleIsPseudocode(RawAsmLine asmLine) {
            var localLabelMatcher = LOCAL_LABEL_POS_PATTERN.Match(asmLine.Label);
            string symbolName;
            if (!localLabelMatcher.Success)
                symbolName = asmLine.Label;
            else
                symbolName = localLabelMatcher.Groups["refnum"].Value + "B";
            if (asmLine.Args.Length == 1) {
                var expr = ExpressionBuilder.buildExpression(asmLine.Args[0]);
                var unsolvedDependencies = bindLocalBackwardRefs(collectUnboundSymbols(expr));
                if (unsolvedDependencies.Count == 0) {
                    if (localLabelMatcher.Success)
                        bindPendingLocalForwardRefs(int.Parse(localLabelMatcher.Groups["refnum"].Value), expr);
                    bindSymbol(symbolName, expr);
                } else {
                    throw new Exception(string.Format("The IS pseudocode at line {0}, pos {1} " +
                        "contains undefined symbols : {2}. " +
                        "Future references are only allowed at JUMP-family operators and OCTA data fields.",
                        _lineNum, _linePos, string.Join(", ", unsolvedDependencies)));
                }
            } else {
                throw new Exception(string.Format("Exactly one expression is expected " +
                    "as argument of the IS pseudo-op at line {0}, position {1}", _lineNum, _linePos));
            }
        }

        private void handleLocPseudocode(RawAsmLine asmLine) {
            var localLabelMatcher = LOCAL_LABEL_POS_PATTERN.Match(asmLine.Label);
            string symbolName;
            bool foundLocalLabel = false;
            if (asmLine.Label.Length != 0) {
                if (!localLabelMatcher.Success) {
                    symbolName = asmLine.Label;
                } else {
                    symbolName = localLabelMatcher.Groups["refnum"].Value + "B";
                    foundLocalLabel = true;
                }
            } else {
                symbolName = null;
            }
            if (asmLine.Args.Length == 1) {
                var expression = ExpressionBuilder.buildExpression(asmLine.Args[0]);
                alignSection();
                var undefinedSymbols = bindLocalBackwardRefs(collectUnboundSymbols(expression));
                if (undefinedSymbols.Count == 0) {
                    bindOldSegmentTopSymbol();
                    long newPos = expression.eval(_symbolBindings).asMmixOcta();
                    string segName;
                    switch (getMemSegment(newPos)) {
                        case MemSegment.Text:
                            segName = ".text";
                            break;
                        case MemSegment.Data:
                            segName = ".data";
                            break;
                        case MemSegment.Pool:
                            segName = ".pool";
                            break;
                        case MemSegment.Stack:
                            segName = ".stack";
                            break;
                        default:
                            throw new Exception("Should never happen");
                    }
                    byte[] binSegName = Encoding.ASCII.GetBytes(segName);
                    _output.Write(binSegName, 0, binSegName.Length);
                    for (int i = binSegName.Length; (i % 8) != 0; i++)
                        _output.WriteByte(0);
                    byte[] buff = new byte[8];
                    doRenderSingleOcta(ExpressionBuilder.makePointer(newPos),
                        _symbolBindings, _lineNum, _linePos, buff);
                    _output.Write(buff, 0, buff.Length);
                    for (int i = 0; i < buff.Length; i++)
                        buff[i] = 0;
                    long outPos0 = _output.Position;
                    _output.Write(buff, 0, buff.Length); // Make a hole
                    _output.Write(buff, 0, buff.Length); // 16 bytes long
                    var boundArgs0 = _symbolBindings;
                    int lineNum0 = _lineNum;
                    int linePos0 = _linePos;
                    var topRef0 = ExpressionBuilder.makeSegmentTopRef();
                    foreach (var u in topRef0.getArguments())
                        _unboundSegmentTopRefs.Add(u);
                    _unboundSymbolDeps.Add(
                       new UnboundSymbolDep {
                           BinFileOffset = outPos0,
                           InstrPtr = ExpressionValue.makeValue(newPos),
                           Length = 8,
                           Action = (SymbolBindings args0, byte[] target) =>
                               doRenderSingleOcta(topRef0, boundArgs0, lineNum0, linePos0, target)
                       });
                    _symbolBindings[INSTR_PTR] = ExpressionValue.makeValue(newPos);
                    if (symbolName != null) {
                        var expr = ExpressionBuilder.makePointer(newPos);
                        if (foundLocalLabel)
                            bindPendingLocalForwardRefs(int.Parse(localLabelMatcher.Groups["refnum"].Value), expr);
                        bindSymbol(symbolName, expr);
                    }
                } else {
                    throw new Exception(string.Format("The expression following LOC pseudocode at line {0}, pos {1} " +
                        "contains references to undefined symbols : {2}. " +
                        "Future references are only allowed at JUMP-family operators and OCTA data fields.",
                        _lineNum, _linePos, string.Join(", ", undefinedSymbols)));
                }
            } else {
                throw new Exception(string.Format("Exactly one expression is expected " +
                    " as argument of the IS pseudo-op at line {0}, position {1}", _lineNum, _linePos));
            }
        }

        private void handleGregPseudocode(RawAsmLine asmLine) {
            var localLabelMatcher = LOCAL_LABEL_POS_PATTERN.Match(asmLine.Label);
            bool foundLocalLabel = false;
            string symbolName;
            if (asmLine.Label.Length != 0) {
                if (!localLabelMatcher.Success) {
                    symbolName = asmLine.Label;
                } else {
                    symbolName = localLabelMatcher.Groups["refnum"].Value + "B";
                    foundLocalLabel = true;
                }
            } else {
                symbolName = null;
            }
            ExpressionValue regValue;
            if (asmLine.Args.Length == 1) {
                var expression = ExpressionBuilder.buildExpression(asmLine.Args[0]);
                var undefinedSymbols = bindLocalBackwardRefs(collectUnboundSymbols(expression));
                if (undefinedSymbols.Count == 0) {
                    regValue = expression.eval(_symbolBindings);
                } else {
                    throw new Exception(string.Format("The expression following GREG pseudocode at line {0}, pos {1} " +
                        "contains references to undefined symbols : {2}. " +
                        "Future references are only allowed at JUMP-family operators and OCTA data fields.",
                        _lineNum, _linePos, string.Join(", ", undefinedSymbols)));
                }
            } else {
                regValue = ExpressionValue.makeValue(0L);
            }
            int newLastGreg = --_lastGreg;
            var expr = ExpressionBuilder.buildExpression("$" + newLastGreg);
            _globalRegs[newLastGreg] = regValue;
            if (foundLocalLabel)
                bindPendingLocalForwardRefs(int.Parse(localLabelMatcher.Groups["refnum"].Value), expr);
            if (symbolName != null)
                bindSymbol(symbolName, expr);
        }

        private static byte coerceByte(long arg) {
            if (arg >= 0 && arg <= byte.MaxValue)
                return (byte)arg;
            else if (arg < 0 && arg >= sbyte.MinValue)
                return (byte)((sbyte)arg);
            else
                throw new Exception(string.Format("The value {0} is out of bounds for byte", arg));
        }

        private static ushort coerceWyde(long arg) {
            if (arg >= 0 && arg <= ushort.MaxValue)
                return (ushort)arg;
            else if (arg < 0 && arg >= short.MinValue)
                return (ushort)((short)arg);
            else
                throw new Exception(string.Format("The value {0} is out of bounds for byte", arg));
        }

        private static uint coerceTetra(long arg) {
            if (arg >= 0 && arg <= uint.MaxValue)
                return (uint)arg;
            else if (arg < 0 && arg >= int.MinValue)
                return (uint)((int)arg);
            else
                throw new Exception(string.Format("The value {0} is out of bounds for byte", arg));
        }

        private static int skipWs(string line, int pos) {
            int len = line.Length;
            int p0 = pos;
            while (p0 < len) {
                if (!char.IsWhiteSpace(line[p0]))
                    break;
                else
                    ++p0;
            }
            return p0;
        }

        private RawAsmLine tokenizeChunk(string chunk) {
            var matcher = LINE_PATTERN.Match(chunk);
            if (matcher.Success) {
                string rest = matcher.Groups["rest"].Value.TrimStart();
                var args = rest.Length != 0 ? splitArgs(rest) : new String[] { };
                return new RawAsmLine {
                    LineNum = _lineNum,
                    Position = _linePos,
                    Label = matcher.Groups["label"].Value,
                    Opcode = matcher.Groups["opcode"].Value,
                    Args = args
                };
            } else {
                throw new Exception(string.Format("Parse error at line {0} , position {1}", _lineNum, _linePos));
            }
        }

        private static string[] splitArgs(string arg) {
            var retVal = new List<string>();
            MatchCollection matches = ARGS_PATTERN.Matches(arg);
            foreach (Match m in matches)
                if (m.Groups["begin"].Value.Length == 0 || !char.IsWhiteSpace(m.Groups["begin"].Value[0]))
                    retVal.Add(m.Groups["arg"].Value);
                else
                    break;
            return retVal.ToArray();
        }

        private MemSegment getMemSegment(long ptr) {
            if ((ptr & DATA_SEG) == DATA_SEG)
                return MemSegment.Data;
            else if ((ptr & POOL_SEG) == POOL_SEG)
                return MemSegment.Stack;
            else if ((ptr & STACK_SEG) == STACK_SEG)
                return MemSegment.Stack;
            else
                return MemSegment.Text;
        }

        internal enum MemSegment {
            Text, Data, Stack, Pool
        }

        internal struct RawAsmLine {
            internal int LineNum;

            internal int Position;

            internal string Label;

            internal string Opcode;

            internal string[] Args;

            public override string ToString() {
                return string.Format("[line {0} pos {1} label {2} opcode {3} args {4}]",
                    LineNum, Position, Label, Opcode, string.Join(" ", Args));
            }
        }

        internal delegate void DeferredRenderAction(SymbolBindings boundArgs, byte[] target);

        internal struct UnboundSymbolDep {
            internal long BinFileOffset;

            internal ExpressionValue InstrPtr;

            internal int Length;

            internal DeferredRenderAction Action;
        }

        internal class UnboundSymbolDeps : List<UnboundSymbolDep> { };
    }
}
